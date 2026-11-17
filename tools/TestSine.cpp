// Minimal MIDI instrument used to verify the carrier plugin slot: one saw voice per held note.
#include <juce_audio_utils/juce_audio_utils.h>

class TestSaw : public juce::AudioProcessor
{
public:
    TestSaw() : AudioProcessor (BusesProperties().withOutput ("Out", juce::AudioChannelSet::stereo())) {}
    void prepareToPlay (double sr, int) override { rate = sr; }
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi) override
    {
        for (const auto m : midi)
        {
            const auto& msg = m.getMessage();
            if (msg.isNoteOn()) notes.addIfNotAlreadyThere (msg.getNoteNumber());
            else if (msg.isNoteOff()) notes.removeFirstMatchingValue (msg.getNoteNumber());
        }
        b.clear();
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            float s = 0;
            for (int k = 0; k < notes.size(); ++k)
            {
                auto& ph = phases[(size_t) (notes[k] & 127)];
                ph += juce::MidiMessage::getMidiNoteInHertz (notes[k]) / rate;
                if (ph >= 1) ph -= 1;
                s += (float) (2 * ph - 1) * 0.2f;
            }
            for (int c = 0; c < b.getNumChannels(); ++c) b.setSample (c, i, s);
        }
    }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    const juce::String getName() const override { return "TestSaw"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& d) override { d.append ("saw", 3); }
    void setStateInformation (const void*, int) override {}

private:
    double rate = 48000;
    juce::Array<int> notes;
    double phases[128] {};
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TestSaw(); }
