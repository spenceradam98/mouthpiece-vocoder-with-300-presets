// Checks the carrier plugin slot and user presets: loads the built TestSaw.vst3 into Mouthpiece's slot,
// vocodes a vowel with it, saves/restores the session, and round-trips a user preset.
// Usage: mouthpiece_host_test <path to TestSaw.vst3>
#include "../Source/plugin/PluginProcessor.h"

static int failures = 0;
#define CHECK(c, msg) do { if (! (c)) { ++failures; std::printf ("FAIL: %s\n", msg); } else std::printf ("ok: %s\n", msg); } while (0)

static double renderRms (MouthpieceProcessor& proc, bool sendNotes)
{
    juce::AudioBuffer<float> buf (4, 512);   // main stereo in/out + (disabled) sidechain
    juce::MidiBuffer midi;
    mp::Biquad f; f.setBandpass (48000.0, 800.0, 5.0);
    double ph = 0, sum = 0; long count = 0;
    for (int block = 0; block < 100; ++block)
    {
        midi.clear();
        if (sendNotes && block == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 0);
        buf.clear();
        for (int i = 0; i < 512; ++i)
        {
            ph += 120.0 / 48000.0; if (ph >= 1) ph -= 1;
            const float v = f.process ((float) (2 * ph - 1)) * 1.5f;
            buf.setSample (0, i, v); buf.setSample (1, i, v);
        }
        proc.processBlock (buf, midi);
        if (block > 50) for (int i = 0; i < 512; ++i) { sum += buf.getSample (0, i) * buf.getSample (0, i); ++count; }
    }
    return std::sqrt (sum / (double) count);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 2) { std::printf ("usage: %s TestSaw.vst3\n", argv[0]); return 1; }

    MouthpieceProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    auto setParam = [&] (int id, float v)
    {
        auto* p = proc.apvts.getParameter (mp::paramDefs()[id].id);
        p->setValueNotifyingHost (p->convertTo0to1 (v));
    };

    // carrier = Plugin with nothing loaded: silent
    setParam (mp::kCarrier, (float) mp::CarPlugin);
    setParam (mp::kSibilance, 0.0f);
    setParam (mp::kUnvoiced, 0.0f);
    CHECK (renderRms (proc, true) < 1e-4, "empty plugin slot is silent");

    juce::OwnedArray<juce::PluginDescription> found;
    juce::VST3PluginFormat vst3;
    vst3.findAllTypesForFile (found, argv[1]);
    CHECK (found.size() == 1, "test instrument found");
    if (found.isEmpty()) return 1;
    juce::String error;
    CHECK (proc.hosted.load (*found[0], error), "test instrument loads into the slot");
    const double withPlugin = renderRms (proc, true);
    std::printf ("   output rms with hosted carrier: %.4f\n", withPlugin);
    CHECK (withPlugin > 0.01, "hosted instrument is vocoded by the voice");

    // session round trip restores the hosted plugin
    juce::MemoryBlock state;
    proc.getStateInformation (state);
    MouthpieceProcessor proc2;
    proc2.prepareToPlay (48000.0, 512);
    proc2.setStateInformation (state.getData(), (int) state.getSize());
    CHECK (proc2.hosted.getName() == "TestSaw", "session restore reloads the hosted plugin");
    CHECK (renderRms (proc2, true) > 0.01, "restored session still sounds");

    // user preset round trip
    setParam (mp::kFormant, 5.0f);
    setParam (mp::kBands, 33.0f);
    CHECK (proc.saveUserPreset ("Host Test Preset", error), "save user preset");
    setParam (mp::kFormant, 0.0f);
    setParam (mp::kBands, 8.0f);
    auto file = MouthpieceProcessor::getUserPresetFolder().getChildFile ("Host Test Preset.mppreset");
    CHECK (proc.loadUserPreset (file), "load user preset");
    CHECK (std::abs (proc.apvts.getRawParameterValue ("formant")->load() - 5.0f) < 0.01f
           && std::abs (proc.apvts.getRawParameterValue ("bands")->load() - 33.0f) < 0.01f, "user preset restores values");
    CHECK (proc.getUserPresetName() == "Host Test Preset", "user preset becomes current");
    file.deleteFile();

    proc.hosted.unload();
    CHECK (renderRms (proc, true) < 1e-4, "unloading the plugin silences the carrier");

    std::printf (failures == 0 ? "all host tests passed\n" : "%d host test failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
