// One hosted plugin (VST3, or AU on macOS) used as Mouthpiece's carrier.
// An instrument gets the incoming MIDI; an effect gets the external input (sidechain) as its audio.
// Loading, removing and state: message thread. render(): audio thread (never blocks; outputs silence while swapping).
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class HostedCarrier : public juce::ChangeBroadcaster
{
public:
    HostedCarrier();
    ~HostedCarrier() override;

    void prepare (double sampleRate, int blockSize);
    void release();

    // Writes the plugin's output (stereo) into `out`. `in` (mono, may be null) feeds an effect's input.
    // Returns false (and leaves `out` cleared) when no plugin is loaded.
    bool render (const float* in, juce::AudioBuffer<float>& out, juce::MidiBuffer& midi, int numSamples);

    bool load (const juce::PluginDescription&, juce::String& error);
    void unload();
    juce::AudioPluginInstance* getPlugin() const { return instance.get(); }
    juce::String getName() const { return instance != nullptr ? instance->getName() : juce::String(); }
    int getLatencySamples() const { return instance != nullptr ? instance->getLatencySamples() : 0; }

    std::unique_ptr<juce::XmlElement> toXml() const;
    void fromXml (const juce::XmlElement&);

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
    juce::File getPluginListFile() const;
    juce::String lastError;   // why the last session restore could not load the plugin
    juce::File getDeadMansPedalFile() const;
    void saveKnownPlugins() const;

private:
    std::unique_ptr<juce::AudioPluginInstance> instance;
    juce::PluginDescription loadedDesc;
    juce::CriticalSection swapLock;
    juce::AudioBuffer<float> scratch;
    double rate = 48000.0;
    int block = 512;
    bool prepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HostedCarrier)
};
