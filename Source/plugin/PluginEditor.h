#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <map>
#include "PluginProcessor.h"

struct Theme
{
    const char* name;
    juce::Colour bg, bg2, panel, panelEdge, text, textDim, accent, accent2, knob, knobEdge, display;
    int knobStyle;      // 0 = arc ring, 1 = hardware pointer knob, 2 = glowing arc, 3 = flat ink
    int barStyle;       // 0 = smooth bars, 1 = LED segments, 2 = glow bars, 3 = outline bars
};

class MouthpieceLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void setTheme (const Theme& t);
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (13.0f)); }
    juce::Font getPopupMenuFont() override { return juce::Font (juce::FontOptions (14.0f)); }
    Theme theme {};
};

class MouthpieceEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MouthpieceEditor (MouthpieceProcessor&);
    ~MouthpieceEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Content : public juce::Component
    {
    public:
        explicit Content (MouthpieceEditor& e) : ed (e) {}
        void paint (juce::Graphics& g) override { ed.paintContent (g); }
        MouthpieceEditor& ed;
    };

    struct Knob
    {
        juce::Slider slider;
        juce::String label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    struct Choice
    {
        juce::ComboBox box;
        juce::String label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
    };
    struct Panel { juce::String title; juce::Rectangle<int> area; };

    void timerCallback() override;
    void paintContent (juce::Graphics&);
    void drawDisplay (juce::Graphics&, juce::Rectangle<float>);
    void drawMeter (juce::Graphics&, juce::Rectangle<float>, float level, const juce::String& label);
    Knob& knob (int param, const juce::String& label);
    Choice& choice (int param, const juce::String& label);
    void placeKnobs (juce::Rectangle<int> area, std::initializer_list<Knob*> ks, int cols);
    void applyTheme (int index);
    void refreshPresetBoxes();
    void stepPreset (int delta);
    juce::String statusText() const;

    MouthpieceProcessor& proc;
    MouthpieceLookAndFeel lnf;
    Content content { *this };
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Choice>> choices;
    std::vector<Panel> panels;
    std::map<int, Knob*> knobFor;
    std::map<int, Choice*> choiceFor;

    juce::ToggleButton freeze { "FREEZE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttach;
    juce::ComboBox categoryBox, presetBox, themeBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::MidiKeyboardComponent keyboard;

    std::array<float, mp::kDisplayBands> bars {};
    std::array<float, mp::kDisplayBands> peaks {};
    float modMeter = 0, carMeter = 0, outMeter = 0;
    int lastProgramStamp = -1;
    juce::Rectangle<int> displayArea, headerArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MouthpieceEditor)
};
