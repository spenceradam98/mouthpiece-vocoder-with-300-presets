#include "HostedCarrier.h"

HostedCarrier::HostedCarrier()
{
    formatManager.addDefaultFormats();
    if (auto xml = juce::XmlDocument::parse (getPluginListFile()))
        knownPlugins.recreateFromXml (*xml);
}

HostedCarrier::~HostedCarrier()
{
    const juce::ScopedLock sl (swapLock);
    if (instance != nullptr) instance->releaseResources();
    instance.reset();
}

juce::File HostedCarrier::getPluginListFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("MouthpieceAudio").getChildFile ("Mouthpiece").getChildFile ("KnownPlugins.xml");
}

juce::File HostedCarrier::getDeadMansPedalFile() const { return getPluginListFile().getSiblingFile ("ScanInProgress.txt"); }

void HostedCarrier::saveKnownPlugins() const
{
    if (auto xml = knownPlugins.createXml())
    {
        getPluginListFile().getParentDirectory().createDirectory();
        xml->writeTo (getPluginListFile());
    }
}

void HostedCarrier::prepare (double sampleRate, int blockSize)
{
    const juce::ScopedLock sl (swapLock);
    rate = sampleRate;
    block = juce::jmax (1, blockSize);
    scratch.setSize (2, block * 2, false, true, true);
    if (instance != nullptr)
    {
        instance->setRateAndBufferSizeDetails (rate, block);
        instance->prepareToPlay (rate, block);
    }
    prepared = true;
}

void HostedCarrier::release()
{
    const juce::ScopedLock sl (swapLock);
    if (instance != nullptr) instance->releaseResources();
    prepared = false;
}

bool HostedCarrier::render (const float* in, juce::AudioBuffer<float>& out, juce::MidiBuffer& midi, int n)
{
    out.clear (0, n);
    const juce::ScopedTryLock sl (swapLock);
    if (! sl.isLocked() || instance == nullptr || ! prepared) return false;

    const int chans = juce::jmax (2, juce::jmax (instance->getTotalNumInputChannels(), instance->getTotalNumOutputChannels()));
    if (scratch.getNumChannels() < chans || scratch.getNumSamples() < n) return false;   // resized only off the audio thread
    juce::AudioBuffer<float> io (scratch.getArrayOfWritePointers(), chans, n);
    io.clear();
    const int ins = instance->getMainBusNumInputChannels();
    if (in != nullptr)
        for (int c = 0; c < juce::jmin (ins, chans); ++c) io.copyFrom (c, 0, in, n);

    instance->processBlock (io, midi);

    const int outs = juce::jmax (1, instance->getMainBusNumOutputChannels());
    for (int c = 0; c < out.getNumChannels(); ++c) out.copyFrom (c, 0, io, juce::jmin (c, outs - 1), 0, n);
    return true;
}

bool HostedCarrier::load (const juce::PluginDescription& desc, juce::String& error)
{
    auto p = formatManager.createPluginInstance (desc, rate, block, error);
    if (p == nullptr)
    {
        if (error.isEmpty()) error = "Could not load " + desc.name;
        return false;
    }
    // stereo main buses where the plugin allows it
    auto layout = p->getBusesLayout();
    if (! layout.inputBuses.isEmpty()) layout.inputBuses.getReference (0) = juce::AudioChannelSet::stereo();
    if (! layout.outputBuses.isEmpty()) layout.outputBuses.getReference (0) = juce::AudioChannelSet::stereo();
    if (p->checkBusesLayoutSupported (layout)) p->setBusesLayout (layout);
    if (p->getMainBusNumOutputChannels() == 0)
    {
        error = desc.name + " has no audio output, so it can't be a carrier";
        return false;
    }
    p->setRateAndBufferSizeDetails (rate, block);
    p->prepareToPlay (rate, block);

    const int chans = juce::jmax (2, juce::jmax (p->getTotalNumInputChannels(), p->getTotalNumOutputChannels()));
    std::unique_ptr<juce::AudioPluginInstance> old;
    {
        const juce::ScopedLock sl (swapLock);
        if (scratch.getNumChannels() < chans) scratch.setSize (chans, block * 2, false, true, true);
        old = std::move (instance);
        instance = std::move (p);
        loadedDesc = desc;
    }
    if (old != nullptr) old->releaseResources();
    old.reset();
    sendChangeMessage();
    return true;
}

void HostedCarrier::unload()
{
    std::unique_ptr<juce::AudioPluginInstance> old;
    {
        const juce::ScopedLock sl (swapLock);
        old = std::move (instance);
    }
    if (old != nullptr) old->releaseResources();
    old.reset();
    sendChangeMessage();
}

std::unique_ptr<juce::XmlElement> HostedCarrier::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement> ("HOSTED");
    if (instance == nullptr) return xml;
    // the description we loaded from, not instance->getPluginDescription(): on some platforms the latter names
    // the binary inside the bundle, which the format can't reopen
    xml->addChildElement (loadedDesc.createXml().release());
    juce::MemoryBlock state;
    instance->getStateInformation (state);
    xml->createNewChildElement ("STATE")->addTextElement (state.toBase64Encoding());
    return xml;
}

void HostedCarrier::fromXml (const juce::XmlElement& xml)
{
    juce::PluginDescription desc;
    auto* d = xml.getChildByName ("PLUGIN");
    if (d == nullptr || ! desc.loadFromXml (*d)) { if (instance != nullptr) unload(); return; }
    juce::String error;
    if (! load (desc, error)) { DBG ("Hosted carrier restore: " << error); lastError = error; return; }
    if (auto* st = xml.getChildByName ("STATE"))
    {
        juce::MemoryBlock state;
        if (state.fromBase64Encoding (st->getAllSubText()))
            instance->setStateInformation (state.getData(), (int) state.getSize());
    }
}
