// Renders the editor in every theme to PNG files, with the display fed by real engine output.
// Build with -DMOUTHPIECE_BUILD_SCREENSHOT=ON; run: mouthpiece_screenshot <outdir> (needs a display, e.g. Xvfb on Linux).
#include "../Source/plugin/PluginProcessor.h"
#include "../Source/plugin/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]) : juce::File::getCurrentWorkingDirectory());
    outDir.createDirectory();

    struct Shot { int theme; int preset; const char* file; };
    const Shot shots[] = { { 0, 0, "theme-studio-night.png" }, { 1, 24, "theme-rack-79.png" }, { 2, 48, "theme-neon.png" }, { 3, 72, "theme-paper.png" } };
    for (const auto& s : shots)
    {
        MouthpieceProcessor proc;
        proc.setCurrentProgram (s.preset);
        proc.theme.store (s.theme);
        proc.prepareToPlay (48000.0, 512);

        // a vowel-ish voice on the input and a held chord so the display and meters show activity
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        double ph = 0;
        mp::Biquad f1, f2;
        f1.setBandpass (48000.0, 650.0, 5.0);
        f2.setBandpass (48000.0, 2300.0, 6.0);
        for (int block = 0; block < 60; ++block)
        {
            midi.clear();
            if (block == 0)
                for (int n : { 48, 55, 60, 64 }) midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
            for (int i = 0; i < 512; ++i)
            {
                ph += 130.0 / 48000.0; if (ph >= 1) ph -= 1;
                const float x = (float) (2 * ph - 1);
                const float v = f1.process (x) * 0.9f + f2.process (x) * 0.5f;
                buf.setSample (0, i, v); buf.setSample (1, i, v);
            }
            proc.processBlock (buf, midi);
        }
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        ed->setSize (1240, 650);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat png;
        auto file = outDir.getChildFile (s.file);
        file.deleteFile();
        juce::FileOutputStream os (file);
        png.writeImageToStream (img, os);
        std::printf ("wrote %s\n", file.getFullPathName().toRawUTF8());
    }
    return 0;
}
