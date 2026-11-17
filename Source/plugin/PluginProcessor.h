#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Engine.h"
#include "Presets.h"
#include "HostedCarrier.h"

class MouthpieceProcessor : public juce::AudioProcessor
{
public:
    MouthpieceProcessor();
    ~MouthpieceProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override { hosted.release(); }
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return mp::numPresets(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    const mp::Engine& getEngine() const { return engine; }
    bool sidechainActive() const { return sidechainOn.load(); }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;   // on-screen keyboard (handy in the standalone app)
    std::atomic<int> theme { 0 };
    std::atomic<int> programChanged { 0 };   // bumped when a program loads, so the editor can refresh

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // carrier plugin slot (Carrier = Plugin)
    HostedCarrier hosted;

    // user presets: one .mppreset XML file per preset in Documents/Mouthpiece/Presets
    static juce::File getUserPresetFolder();
    juce::Array<juce::File> getUserPresets() const;
    bool saveUserPreset (const juce::String& name, juce::String& error);
    bool loadUserPreset (const juce::File&);
    juce::String getUserPresetName() const { return userPresetName; }

private:
    mp::Params readParams() const;

    mp::Engine engine;
    std::vector<float> mono, side, outL, outR;
    std::vector<mp::NoteEvent> events;
    juce::AudioBuffer<float> hostOut;
    std::vector<float> hostMono;
    juce::String userPresetName;   // non-empty while a user preset is the current sound
    std::array<std::atomic<float>*, mp::kNumParams> raw {};
    std::atomic<bool> sidechainOn { false };
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MouthpieceProcessor)
};
