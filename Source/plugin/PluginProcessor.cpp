#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout MouthpieceProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    for (int i = 0; i < mp::kNumParams; ++i)
    {
        const auto& d = mp::paramDefs()[i];
        const ParameterID id { d.id, 1 };
        switch (d.kind)
        {
            case mp::Kind::Choice:
            {
                StringArray labels;
                labels.addTokens (d.choices, "|", "");
                p.push_back (std::make_unique<AudioParameterChoice> (id, d.name, labels, (int) d.def));
                break;
            }
            case mp::Kind::Bool:
                p.push_back (std::make_unique<AudioParameterBool> (id, d.name, d.def > 0.5f));
                break;
            case mp::Kind::Int:
                p.push_back (std::make_unique<AudioParameterInt> (id, d.name, (int) d.min, (int) d.max, (int) d.def,
                                                                  AudioParameterIntAttributes().withLabel (d.unit)));
                break;
            case mp::Kind::Float:
            default:
            {
                NormalisableRange<float> range (d.min, d.max, 0.0f);
                if (d.skewCentre > 0.0f) range.setSkewForCentre (d.skewCentre);
                const String unit = d.unit;
                const int decimals = (d.max - d.min) > 50.0f ? 0 : ((d.max - d.min) > 5.0f ? 1 : 2);
                p.push_back (std::make_unique<AudioParameterFloat> (
                    id, d.name, range, d.def,
                    AudioParameterFloatAttributes().withLabel (unit).withStringFromValueFunction (
                        [decimals, unit] (float v, int) { return String (v, decimals) + (unit.isEmpty() ? "" : " " + unit); })));
                break;
            }
        }
    }
    return { p.begin(), p.end() };
}

MouthpieceProcessor::MouthpieceProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "Mouthpiece", createLayout())
{
    for (int i = 0; i < mp::kNumParams; ++i) raw[(size_t) i] = apvts.getRawParameterValue (mp::paramDefs()[i].id);
}

bool MouthpieceProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    const auto in = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo()) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}

void MouthpieceProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto n = (size_t) juce::jmax (1, samplesPerBlock);
    mono.assign (n, 0.0f); side.assign (n, 0.0f); outL.assign (n, 0.0f); outR.assign (n, 0.0f); hostMono.assign (n, 0.0f);
    hostOut.setSize (2, (int) n);
    hosted.prepare (sampleRate, samplesPerBlock);
    events.reserve (512);
    engine.setParams (readParams());
    engine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (engine.latencySamples());
    keyboardState.reset();
}

mp::Params MouthpieceProcessor::readParams() const
{
    mp::Params p;
    for (int i = 0; i < mp::kNumParams; ++i) p[(size_t) i] = raw[(size_t) i]->load();
    return p;
}

void MouthpieceProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if ((int) mono.size() < n)   // hosts may exceed the announced block size
    {
        mono.resize ((size_t) n); side.resize ((size_t) n); outL.resize ((size_t) n); outR.resize ((size_t) n);
        hostMono.resize ((size_t) n);
        hostOut.setSize (2, n, false, false, true);
    }

    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    auto mixDown = [n] (const juce::AudioBuffer<float>& b, std::vector<float>& dst)
    {
        const int ch = b.getNumChannels();
        for (int i = 0; i < n; ++i)
        {
            float s = 0;
            for (int c = 0; c < ch; ++c) s += b.getReadPointer (c)[i];
            dst[(size_t) i] = ch > 0 ? s / (float) ch : 0.0f;
        }
    };

    auto mainIn = getBusBuffer (buffer, true, 0);
    mixDown (mainIn, mono);
    bool scOn = false;
    if (getBusCount (true) > 1)
        if (auto* bus = getBus (true, 1); bus != nullptr && bus->isEnabled())
        {
            auto sc = getBusBuffer (buffer, true, 1);
            if (sc.getNumChannels() > 0) { mixDown (sc, side); scOn = true; }
        }
    if (! scOn) std::fill (side.begin(), side.begin() + n, 0.0f);
    sidechainOn.store (scOn);

    events.clear();
    for (const auto meta : midi)
    {
        const auto& m = meta.getMessage();
        mp::NoteEvent e;
        e.offset = juce::jlimit (0, n - 1, meta.samplePosition);
        if (m.isNoteOn()) { e.type = mp::NoteEvent::NoteOn; e.note = m.getNoteNumber(); e.value = m.getFloatVelocity(); }
        else if (m.isNoteOff()) { e.type = mp::NoteEvent::NoteOff; e.note = m.getNoteNumber(); }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) e.type = mp::NoteEvent::AllOff;
        else if (m.isPitchWheel()) { e.type = mp::NoteEvent::PitchBend; e.value = (float) (m.getPitchWheelValue() - 8192) / 8192.0f; }
        else if (m.isControllerOfType (1)) { e.type = mp::NoteEvent::ModWheel; e.value = (float) m.getControllerValue() / 127.0f; }
        else continue;
        if (events.size() < events.capacity()) events.push_back (e);
    }

    const auto p = readParams();
    engine.setParams (p);
    if (engine.latencySamples() != getLatencySamples()) setLatencySamples (engine.latencySamples());

    const bool modFromSide = (int) p[mp::kModSource] == mp::ModSidechain;
    const float* mod = modFromSide ? side.data() : mono.data();
    const float* ext = modFromSide ? mono.data() : side.data();
    if ((int) std::lround (p[mp::kCarrier]) == mp::CarPlugin)
    {
        // the hosted plugin plays the MIDI (instrument) or processes the external input (effect); its output is the carrier
        juce::AudioBuffer<float> hb (hostOut.getArrayOfWritePointers(), 2, n);
        if (hosted.render (ext, hb, midi, n))
            for (int i = 0; i < n; ++i) hostMono[(size_t) i] = 0.5f * (hb.getSample (0, i) + hb.getSample (1, i));
        else
            std::fill (hostMono.begin(), hostMono.begin() + n, 0.0f);
        ext = hostMono.data();
    }
    engine.process (mod, ext, outL.data(), outR.data(), n, events.data(), (int) events.size());

    const int outCh = getMainBusNumOutputChannels();
    for (int c = getTotalNumOutputChannels(); --c >= 0;)
        if (c >= outCh) buffer.clear (c, 0, n);
    if (outCh == 1)
        for (int i = 0; i < n; ++i) buffer.getWritePointer (0)[i] = 0.5f * (outL[(size_t) i] + outR[(size_t) i]);
    else if (outCh >= 2)
    {
        buffer.copyFrom (0, 0, outL.data(), n);
        buffer.copyFrom (1, 0, outR.data(), n);
    }
}

const juce::String MouthpieceProcessor::getProgramName (int index)
{
    int n = 0, nc = 0;
    const auto* defs = mp::presetDefs (n);
    const auto* cats = mp::presetCategories (nc);
    if (! juce::isPositiveAndBelow (index, n)) return {};
    return juce::String (cats[defs[index].category].name) + " / " + defs[index].name;
}

void MouthpieceProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, mp::numPresets())) return;
    currentProgram = index;
    userPresetName.clear();
    const auto p = mp::presetParams (index);
    for (int i = 0; i < mp::kNumParams; ++i)
        if (auto* prm = apvts.getParameter (mp::paramDefs()[i].id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (p[(size_t) i]));
    programChanged.fetch_add (1);
}

void MouthpieceProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("theme", theme.load(), nullptr);
    state.setProperty ("userPreset", userPresetName, nullptr);
    if (auto xml = state.createXml())
    {
        xml->addChildElement (hosted.toXml().release());
        copyXmlToBinary (*xml, destData);
    }
}

void MouthpieceProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            if (auto* h = xml->getChildByName ("HOSTED"))
            {
                hosted.fromXml (*h);
                xml->removeChildElement (h, true);
            }
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = tree.getProperty ("program", 0);
            theme.store ((int) tree.getProperty ("theme", 0));
            userPresetName = tree.getProperty ("userPreset", "").toString();
            apvts.replaceState (tree);
            programChanged.fetch_add (1);
        }
}

juce::File MouthpieceProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Mouthpiece").getChildFile ("Presets");
}

juce::Array<juce::File> MouthpieceProcessor::getUserPresets() const
{
    auto files = getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.mppreset");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileNameWithoutExtension().compareIgnoreCase (b.getFileNameWithoutExtension()) < 0; });
    return files;
}

bool MouthpieceProcessor::saveUserPreset (const juce::String& rawName, juce::String& error)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty()) { error = "Please enter a name"; return false; }
    juce::XmlElement xml ("MouthpiecePreset");
    xml.setAttribute ("name", rawName.trim());
    xml.setAttribute ("version", 1);
    for (int i = 0; i < mp::kNumParams; ++i)
        xml.setAttribute (mp::paramDefs()[i].id, (double) raw[(size_t) i]->load());
    auto folder = getUserPresetFolder();
    if (! folder.createDirectory()) { error = "Could not create " + folder.getFullPathName(); return false; }
    auto file = folder.getChildFile (name + ".mppreset");
    if (! xml.writeTo (file)) { error = "Could not write " + file.getFullPathName(); return false; }
    userPresetName = file.getFileNameWithoutExtension();
    programChanged.fetch_add (1);
    return true;
}

bool MouthpieceProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("MouthpiecePreset")) return false;
    auto p = mp::defaultParams();   // keys missing from older files fall back to defaults
    for (int i = 0; i < mp::kNumParams; ++i)
    {
        const auto& d = mp::paramDefs()[i];
        p[(size_t) i] = juce::jlimit (d.min, d.max, (float) xml->getDoubleAttribute (d.id, d.def));
    }
    for (int i = 0; i < mp::kNumParams; ++i)
        if (auto* prm = apvts.getParameter (mp::paramDefs()[i].id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (p[(size_t) i]));
    userPresetName = file.getFileNameWithoutExtension();
    programChanged.fetch_add (1);
    return true;
}

juce::AudioProcessorEditor* MouthpieceProcessor::createEditor() { return new MouthpieceEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MouthpieceProcessor(); }
