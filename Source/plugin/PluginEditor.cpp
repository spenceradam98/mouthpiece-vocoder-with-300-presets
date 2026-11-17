#include "PluginEditor.h"

using namespace juce;

namespace
{
constexpr int W = 1240, H = 650;
constexpr int kKnobW = 64, kKnobH = 92;

const Theme kThemes[] = {
    { "Studio Night", Colour (0xff121419), Colour (0xff1a1d24), Colour (0xff1f232b), Colour (0xff2c313c), Colour (0xffe6e8ec),
      Colour (0xff8a919e), Colour (0xff3fd0c9), Colour (0xff7c8cff), Colour (0xff2a2f39), Colour (0xff3a404c), Colour (0xff0c0e12), 0, 0 },
    { "Rack '79", Colour (0xff8f8b82), Colour (0xffb7b3a8), Colour (0xffd9d5c9), Colour (0xff8a867c), Colour (0xff1b1b1b),
      Colour (0xff5a574f), Colour (0xffe8641e), Colour (0xffd8b043), Colour (0xff1c1c1c), Colour (0xff9a968c), Colour (0xff141210), 1, 1 },
    { "Neon", Colour (0xff0e0818), Colour (0xff1a0d2e), Colour (0xff1a1030), Colour (0xff3a2266), Colour (0xfff4eaff),
      Colour (0xffa58fc9), Colour (0xffff3fb4), Colour (0xff3fe0ff), Colour (0xff22163a), Colour (0xff5a3a90), Colour (0xff07040d), 2, 2 },
    { "Paper", Colour (0xffece6d8), Colour (0xfff3efe6), Colour (0xfffffdf8), Colour (0xffcfc6b2), Colour (0xff23211d),
      Colour (0xff77705f), Colour (0xff2747d6), Colour (0xffd2452f), Colour (0xfff3efe6), Colour (0xff23211d), Colour (0xfff7f3ea), 3, 3 },
};
constexpr int kNumThemes = (int) (sizeof (kThemes) / sizeof (kThemes[0]));
}

// ============================================================================ LookAndFeel
void MouthpieceLookAndFeel::setTheme (const Theme& t)
{
    theme = t;
    setColour (ComboBox::backgroundColourId, t.panel.darker (t.knobStyle == 3 ? 0.03f : 0.25f));
    setColour (ComboBox::textColourId, t.text);
    setColour (ComboBox::outlineColourId, t.panelEdge);
    setColour (ComboBox::arrowColourId, t.accent);
    setColour (PopupMenu::backgroundColourId, t.panel);
    setColour (PopupMenu::textColourId, t.text);
    setColour (PopupMenu::headerTextColourId, t.accent);
    setColour (PopupMenu::highlightedBackgroundColourId, t.accent.withAlpha (0.35f));
    setColour (PopupMenu::highlightedTextColourId, t.text);
    setColour (TextButton::textColourOffId, t.text);
    setColour (TextButton::textColourOnId, t.text);
    setColour (Slider::textBoxTextColourId, t.textDim);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId, t.accent.withAlpha (0.4f));
    setColour (TextEditor::textColourId, t.text);
    setColour (TextEditor::backgroundColourId, t.panel);
    setColour (Label::textColourId, t.textDim);
    setColour (ToggleButton::textColourId, t.text);
    setColour (MidiKeyboardComponent::keyDownOverlayColourId, t.accent.withAlpha (0.6f));
    setColour (MidiKeyboardComponent::mouseOverKeyOverlayColourId, t.accent.withAlpha (0.25f));
}

void MouthpieceLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider& s)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float d = jmin (b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const float angle = start + pos * (end - start);
    const float rad = d * 0.5f;
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float zeroAngle = bipolar ? start + (float) s.valueToProportionOfLength (0.0) * (end - start) : start;
    const auto& t = theme;

    auto arc = [&] (float r, float from, float to, Colour col, float thick)
    {
        Path p;
        p.addCentredArc (c.x, c.y, r, r, 0.0f, from, to, true);
        g.setColour (col);
        g.strokePath (p, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));
    };

    switch (t.knobStyle)
    {
        case 1:   // hardware: black skirted knob with a white pointer and scale ticks
        {
            g.setColour (t.text.withAlpha (0.55f));
            for (int i = 0; i <= 10; ++i)
            {
                const float a = start + (end - start) * (float) i / 10.0f;
                g.drawLine (Line<float> (c.getPointOnCircumference (rad, a), c.getPointOnCircumference (rad - 3.5f, a)), 1.0f);
            }
            auto body = Rectangle<float> (d * 0.78f, d * 0.78f).withCentre (c);
            g.setColour (Colours::black.withAlpha (0.35f));
            g.fillEllipse (body.translated (1.5f, 2.5f));
            g.setGradientFill (ColourGradient (Colour (0xff4a4a4a), body.getX(), body.getY(), Colour (0xff0a0a0a), body.getRight(), body.getBottom(), false));
            g.fillEllipse (body);
            auto cap = body.reduced (d * 0.09f);
            g.setGradientFill (ColourGradient (Colour (0xff353535), cap.getX(), cap.getY(), Colour (0xff151515), cap.getRight(), cap.getBottom(), false));
            g.fillEllipse (cap);
            g.setColour (Colours::white.withAlpha (0.92f));
            g.drawLine (Line<float> (c.getPointOnCircumference (cap.getWidth() * 0.1f, angle), c.getPointOnCircumference (body.getWidth() * 0.5f, angle)), 2.4f);
            break;
        }
        case 2:   // neon: glowing arc
        {
            arc (rad - 4.0f, start, end, t.knobEdge.withAlpha (0.6f), 4.0f);
            for (int i = 3; i >= 1; --i)
                arc (rad - 4.0f, zeroAngle, angle, t.accent.withAlpha (0.12f * (float) (4 - i)), 4.0f + (float) i * 3.0f);
            arc (rad - 4.0f, zeroAngle, angle, t.accent, 3.0f);
            auto body = Rectangle<float> (d * 0.56f, d * 0.56f).withCentre (c);
            g.setColour (t.knob);
            g.fillEllipse (body);
            g.setColour (t.accent2);
            g.drawLine (Line<float> (c.getPointOnCircumference (body.getWidth() * 0.15f, angle), c.getPointOnCircumference (body.getWidth() * 0.5f, angle)), 2.0f);
            break;
        }
        case 3:   // paper: flat ink circle and arc
        {
            arc (rad - 3.0f, start, end, t.panelEdge, 1.5f);
            arc (rad - 3.0f, zeroAngle, angle, t.accent, 3.0f);
            auto body = Rectangle<float> (d * 0.58f, d * 0.58f).withCentre (c);
            g.setColour (t.panel);
            g.fillEllipse (body);
            g.setColour (t.text);
            g.drawEllipse (body, 1.4f);
            g.drawLine (Line<float> (c, c.getPointOnCircumference (body.getWidth() * 0.5f, angle)), 1.8f);
            break;
        }
        default:  // studio: ring arc around a soft knob
        {
            arc (rad - 3.0f, start, end, t.knobEdge, 3.5f);
            arc (rad - 3.0f, zeroAngle, angle, t.accent, 3.5f);
            auto body = Rectangle<float> (d * 0.62f, d * 0.62f).withCentre (c);
            g.setGradientFill (ColourGradient (t.knob.brighter (0.25f), body.getX(), body.getY(), t.knob.darker (0.4f), body.getRight(), body.getBottom(), false));
            g.fillEllipse (body);
            g.setColour (t.text);
            g.drawLine (Line<float> (c.getPointOnCircumference (body.getWidth() * 0.18f, angle), c.getPointOnCircumference (body.getWidth() * 0.46f, angle)), 2.0f);
            break;
        }
    }
}

void MouthpieceLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (on ? theme.accent : theme.panel.darker (0.2f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (highlighted ? theme.accent : theme.panelEdge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    g.setColour (on ? (theme.knobStyle == 3 ? Colours::white : Colours::black) : theme.text);
    g.setFont (FontOptions (12.0f, Font::bold));
    g.drawText (b.getButtonText(), r, Justification::centred);
}

void MouthpieceLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (down ? theme.accent.withAlpha (0.5f) : theme.panel.darker (0.25f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (highlighted ? theme.accent : theme.panelEdge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void MouthpieceLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.hasKeyboardFocus (true) ? theme.accent : theme.panelEdge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (theme.accent);
    g.fillPath (arrow);
}

// ============================================================================ hosted plugin windows
class MouthpieceEditor::PluginWindow : public DocumentWindow
{
public:
    PluginWindow (AudioProcessor& p, std::function<void()> onClose)
        : DocumentWindow (p.getName(), Colours::darkgrey, DocumentWindow::closeButton), processor (p), closed (std::move (onClose))
    {
        setUsingNativeTitleBar (true);
        if (auto* ed = p.createEditorIfNeeded()) setContentNonOwned (ed, true);
        else setContentOwned (new GenericAudioProcessorEditor (p), true);
        auto* ed = dynamic_cast<AudioProcessorEditor*> (getContentComponent());
        setResizable (ed != nullptr && ed->isResizable(), false);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
        setAlwaysOnTop (true);
    }
    ~PluginWindow() override
    {
        if (auto* ed = dynamic_cast<AudioProcessorEditor*> (getContentComponent()))
        {
            if (dynamic_cast<GenericAudioProcessorEditor*> (ed) == nullptr)
            {
                processor.editorBeingDeleted (ed);
                clearContentComponent();
                delete ed;
            }
        }
    }
    void closeButtonPressed() override { MessageManager::callAsync (closed); }
    AudioProcessor& processor;

private:
    std::function<void()> closed;
};

class MouthpieceEditor::ScannerWindow : public DocumentWindow
{
public:
    ScannerWindow (HostedCarrier& h, std::function<void()> onClose)
        : DocumentWindow ("Plugins", Colours::darkgrey, DocumentWindow::closeButton), hosted (h), closed (std::move (onClose))
    {
        setUsingNativeTitleBar (true);
        auto* list = new PluginListComponent (hosted.formatManager, hosted.knownPlugins, hosted.getDeadMansPedalFile(), nullptr, false);
        list->setSize (640, 480);
        setContentOwned (list, true);
        setResizable (true, false);
        centreWithSize (640, 480);
        setVisible (true);
        setAlwaysOnTop (true);
    }
    void closeButtonPressed() override { hosted.saveKnownPlugins(); MessageManager::callAsync (closed); }

private:
    HostedCarrier& hosted;
    std::function<void()> closed;
};

// ============================================================================ Editor
MouthpieceEditor::MouthpieceEditor (MouthpieceProcessor& p)
    : AudioProcessorEditor (&p), proc (p), keyboard (p.keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    using namespace mp;
    // header: preset browser and theme
    int nc = 0;
    const auto* cats = presetCategories (nc);
    for (int i = 0; i < nc; ++i) categoryBox.addItem (cats[i].name, i + 1);
    categoryBox.addSeparator();
    categoryBox.addItem ("User", kUserCategoryId);
    categoryBox.onChange = [this]
    {
        if (categoryBox.getSelectedId() == kUserCategoryId)
        {
            // list the user's presets; load the first if the current sound isn't one of them
            userFiles = proc.getUserPresets();
            if (proc.getUserPresetName().isEmpty() && ! userFiles.isEmpty()) proc.loadUserPreset (userFiles.getFirst());
            else refreshPresetBoxes();
            return;
        }
        // picking a category loads its first preset
        int n = 0;
        const auto* defs = presetDefs (n);
        const int cat = categoryBox.getSelectedId() - 1;
        int current = proc.getCurrentProgram();
        if (isPositiveAndBelow (current, n) && defs[current].category == cat) return;
        for (int i = 0; i < n; ++i)
            if (defs[i].category == cat) { proc.setCurrentProgram (i); break; }
    };
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= kUserPresetIdBase)
        {
            const int i = id - kUserPresetIdBase;
            if (isPositiveAndBelow (i, userFiles.size()) && userFiles[i].getFileNameWithoutExtension() != proc.getUserPresetName())
                proc.loadUserPreset (userFiles[i]);
            return;
        }
        if (id > 0 && (id - 1 != proc.getCurrentProgram() || proc.getUserPresetName().isNotEmpty())) proc.setCurrentProgram (id - 1);
    };
    for (int i = 0; i < kNumThemes; ++i) themeBox.addItem (kThemes[i].name, i + 1);
    themeBox.onChange = [this] { applyTheme (themeBox.getSelectedId() - 1); };
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    saveButton.onClick = [this] { saveUserPreset(); };
    saveButton.setTooltip ("Save the current sound as a user preset");
    pluginButton.onClick = [this] { showPluginMenu(); };
    pluginButton.setTooltip ("Load a VST3/AU instrument or effect as the carrier");
    for (auto* c : { (Component*) &categoryBox, (Component*) &presetBox, (Component*) &themeBox, (Component*) &prevButton, (Component*) &nextButton,
                     (Component*) &saveButton, (Component*) &pluginButton })
        content.addAndMakeVisible (c);

    // input
    choice (kModSource, "MODULATOR");
    knob (kModGain, "GAIN"); knob (kGate, "GATE"); knob (kEmphasis, "EMPHASIS");
    // vocoder
    choice (kType, "TYPE");
    knob (kBands, "BANDS"); knob (kLowHz, "LOW"); knob (kHighHz, "HIGH"); knob (kFormant, "FORMANT"); knob (kBandwidth, "WIDTH");
    knob (kAttack, "ATTACK"); knob (kRelease, "RELEASE"); knob (kSibilance, "SIBILANCE"); knob (kUnvoiced, "UNVOICED"); knob (kRobot, "ROBOT");
    freezeAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, paramDefs()[kFreeze].id, freeze);
    content.addAndMakeVisible (freeze);
    // carrier
    choice (kCarrier, "CARRIER"); choice (kWave, "WAVE"); choice (kKey, "KEY"); choice (kScale, "SCALE"); choice (kChord, "CHORD");
    knob (kOctave, "OCTAVE"); knob (kUnison, "UNISON"); knob (kDetune, "DETUNE"); knob (kPulseWidth, "PW"); knob (kOsc2Mix, "OSC 2");
    knob (kOsc2Semi, "INTERVAL"); knob (kSub, "SUB");
    knob (kNoise, "NOISE"); knob (kTone, "TONE"); knob (kGlide, "GLIDE"); knob (kAmpAttack, "AMP ATK"); knob (kAmpRelease, "AMP REL");
    knob (kVibrato, "VIBRATO");
    // output
    knob (kSpread, "SPREAD"); knob (kEnsemble, "ENSEMBLE"); knob (kSpace, "SPACE"); knob (kDrive, "DRIVE"); knob (kMix, "MIX"); knob (kOutput, "OUTPUT");

    keyboard.setAvailableRange (36, 96);   // C2..C7: 36 white keys fill the width
    keyboard.setWantsKeyboardFocus (false);
    content.addAndMakeVisible (keyboard);

    applyTheme (jlimit (0, kNumThemes - 1, proc.theme.load()));
    refreshPresetBoxes();

    setResizable (true, true);
    setResizeLimits (W / 2, H / 2, W * 2, H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) W / H);
    setSize (W, H);
    startTimerHz (30);
}

MouthpieceEditor::~MouthpieceEditor()
{
    pluginWindow.reset();
    if (scannerWindow != nullptr) { proc.hosted.saveKnownPlugins(); scannerWindow.reset(); }
    saveDialog.reset();
    setLookAndFeel (nullptr);
}

MouthpieceEditor::Knob& MouthpieceEditor::knob (int param, const String& label)
{
    auto k = std::make_unique<Knob>();
    k->label = label;
    k->slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (Slider::TextBoxBelow, false, kKnobW, 16);
    k->slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    k->slider.setTooltip (mp::paramDefs()[param].name);
    k->attach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, mp::paramDefs()[param].id, k->slider);
    content.addAndMakeVisible (k->slider);
    knobFor[param] = k.get();
    knobs.push_back (std::move (k));
    return *knobs.back();
}

MouthpieceEditor::Choice& MouthpieceEditor::choice (int param, const String& label)
{
    auto c = std::make_unique<Choice>();
    c->label = label;
    StringArray items;
    items.addTokens (mp::paramDefs()[param].choices, "|", "");
    c->box.addItemList (items, 1);
    c->attach = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, mp::paramDefs()[param].id, c->box);
    content.addAndMakeVisible (c->box);
    choiceFor[param] = c.get();
    choices.push_back (std::move (c));
    return *choices.back();
}

void MouthpieceEditor::applyTheme (int index)
{
    index = jlimit (0, kNumThemes - 1, index);
    proc.theme.store (index);
    lnf.setTheme (kThemes[index]);
    themeBox.setSelectedId (index + 1, dontSendNotification);
    keyboard.setColour (MidiKeyboardComponent::whiteNoteColourId, index == 3 ? Colours::white : Colour (0xffeeeeee));
    keyboard.setColour (MidiKeyboardComponent::blackNoteColourId, Colour (0xff1a1a1a));
    keyboard.setColour (MidiKeyboardComponent::keySeparatorLineColourId, Colours::black.withAlpha (0.4f));
    keyboard.setColour (MidiKeyboardComponent::shadowColourId, Colours::transparentBlack);
    sendLookAndFeelChange();
    content.repaint();
}

void MouthpieceEditor::refreshPresetBoxes()
{
    if (proc.getUserPresetName().isNotEmpty() || categoryBox.getSelectedId() == kUserCategoryId)
    {
        userFiles = proc.getUserPresets();
        categoryBox.setSelectedId (kUserCategoryId, dontSendNotification);
        presetBox.clear (dontSendNotification);
        if (userFiles.isEmpty()) presetBox.addItem ("(no user presets yet: press SAVE)", -1);
        for (int i = 0; i < userFiles.size(); ++i)
        {
            presetBox.addItem (userFiles[i].getFileNameWithoutExtension(), kUserPresetIdBase + i);
            if (userFiles[i].getFileNameWithoutExtension() == proc.getUserPresetName())
                presetBox.setSelectedId (kUserPresetIdBase + i, dontSendNotification);
        }
        return;
    }
    int n = 0;
    const auto* defs = mp::presetDefs (n);
    const int cur = jlimit (0, n - 1, proc.getCurrentProgram());
    const int cat = defs[cur].category;
    categoryBox.setSelectedId (cat + 1, dontSendNotification);
    presetBox.clear (dontSendNotification);
    for (int i = 0; i < n; ++i)
        if (defs[i].category == cat) presetBox.addItem (defs[i].name, i + 1);
    presetBox.setSelectedId (cur + 1, dontSendNotification);
}

void MouthpieceEditor::stepPreset (int delta)
{
    if (proc.getUserPresetName().isNotEmpty())
    {
        userFiles = proc.getUserPresets();
        int cur = 0;
        for (int i = 0; i < userFiles.size(); ++i)
            if (userFiles[i].getFileNameWithoutExtension() == proc.getUserPresetName()) cur = i;
        if (! userFiles.isEmpty()) proc.loadUserPreset (userFiles[((cur + delta) % userFiles.size() + userFiles.size()) % userFiles.size()]);
        return;
    }
    const int n = mp::numPresets();
    proc.setCurrentProgram (((proc.getCurrentProgram() + delta) % n + n) % n);
}

void MouthpieceEditor::placeKnobs (Rectangle<int> area, std::initializer_list<Knob*> ks, int cols)
{
    int i = 0;
    for (auto* k : ks)
    {
        const int col = i % cols, row = i / cols;
        k->slider.setBounds (area.getX() + col * kKnobW, area.getY() + row * kKnobH + 16, kKnobW, kKnobH - 18);
        ++i;
    }
}

void MouthpieceEditor::resized()
{
    const float s = (float) getWidth() / (float) W;
    content.setBounds (0, 0, W, H);
    content.setTransform (AffineTransform::scale (s));

    using namespace mp;
    headerArea = { 0, 0, W, 54 };
    prevButton.setBounds (276, 13, 28, 28);
    categoryBox.setBounds (308, 13, 176, 28);
    presetBox.setBounds (488, 13, 236, 28);
    nextButton.setBounds (728, 13, 28, 28);
    saveButton.setBounds (762, 13, 52, 28);
    pluginButton.setBounds (874, 13, 182, 28);
    themeBox.setBounds (W - 176, 13, 160, 28);

    displayArea = { 16, 62, W - 32, 150 };

    const int top = 224, ph = 2 * kKnobH + 78;
    const int gap = 10;
    int x = 16;
    panels.clear();
    auto panel = [&] (const String& title, int cols)
    {
        const int w = cols * kKnobW + 20;
        panels.push_back ({ title, { x, top, w, ph } });
        x += w + gap;
        return panels.back().area.reduced (10, 0).withTrimmedTop (26);
    };

    auto in = panel ("INPUT", 2);
    choiceFor[kModSource]->box.setBounds (in.getX(), in.getY() + 14, in.getWidth(), 24);
    placeKnobs (in.withTrimmedTop (40), { knobFor[kModGain], knobFor[kGate] }, 2);
    placeKnobs (in.withTrimmedTop (40 + kKnobH), { knobFor[kEmphasis] }, 2);

    auto voc = panel ("VOCODER", 5);
    choiceFor[kType]->box.setBounds (voc.getX(), voc.getY() + 14, 150, 24);
    freeze.setBounds (voc.getRight() - 80, voc.getY() + 14, 80, 24);
    placeKnobs (voc.withTrimmedTop (40), { knobFor[kBands], knobFor[kFormant], knobFor[kBandwidth], knobFor[kLowHz], knobFor[kHighHz],
                                           knobFor[kAttack], knobFor[kRelease], knobFor[kSibilance], knobFor[kUnvoiced], knobFor[kRobot] }, 5);

    auto car = panel ("CARRIER", 7);
    {
        const int cw = (car.getWidth() - 4 * 6) / 5;
        int cx = car.getX();
        for (int p : { (int) kCarrier, (int) kWave, (int) kKey, (int) kScale, (int) kChord })
        {
            const int w = p == kCarrier ? cw + 26 : (p == kKey ? cw - 30 : (p == kWave ? cw - 4 : cw + 4));
            choiceFor[p]->box.setBounds (cx, car.getY() + 14, w, 24);
            cx += w + 6;
        }
    }
    placeKnobs (car.withTrimmedTop (40), { knobFor[kOctave], knobFor[kUnison], knobFor[kDetune], knobFor[kPulseWidth], knobFor[kOsc2Mix],
                                           knobFor[kOsc2Semi], knobFor[kSub], knobFor[kNoise], knobFor[kTone],
                                           knobFor[kGlide], knobFor[kAmpAttack], knobFor[kAmpRelease], knobFor[kVibrato] }, 7);

    auto out = panel ("OUTPUT", 3);
    placeKnobs (out.withTrimmedTop (40), { knobFor[kSpread], knobFor[kEnsemble], knobFor[kSpace], knobFor[kDrive], knobFor[kMix], knobFor[kOutput] }, 3);

    keyboard.setBounds (16, top + ph + 12, W - 32, H - (top + ph + 12) - 14);
    keyboard.setKeyWidth ((float) (W - 32) / 36.0f);
}

void MouthpieceEditor::paint (Graphics& g)
{
    g.fillAll (lnf.theme.bg);
}

void MouthpieceEditor::paintContent (Graphics& g)
{
    const auto& t = lnf.theme;
    g.setGradientFill (ColourGradient (t.bg2, 0, 0, t.bg, 0, (float) H, false));
    g.fillAll();

    if (t.knobStyle == 1)   // rack: brushed metal streaks and wooden end cheeks
    {
        Random r (7);
        for (int y = 0; y < H; y += 2)
        {
            g.setColour (Colours::white.withAlpha (0.03f + 0.04f * r.nextFloat()));
            g.drawHorizontalLine (y, 0.0f, (float) W);
        }
    }
    else if (t.knobStyle == 2)   // neon: perspective grid in the background
    {
        g.setColour (t.accent.withAlpha (0.07f));
        for (int i = -20; i <= 20; ++i) g.drawLine ((float) W * 0.5f, (float) H * 0.3f, (float) W * 0.5f + (float) i * 120.0f, (float) H, 1.0f);
        for (int i = 0; i < 12; ++i)
        {
            const float y = (float) H * 0.3f + std::pow ((float) i / 12.0f, 2.0f) * (float) H * 0.7f;
            g.drawHorizontalLine ((int) y, 0.0f, (float) W);
        }
    }

    // header
    g.setColour (t.text);
    g.setFont (FontOptions (26.0f, Font::bold));
    g.drawText ("MOUTHPIECE", 18, 10, 200, 32, Justification::centredLeft);
    g.setColour (t.accent);
    g.setFont (FontOptions (11.0f, Font::bold));
    g.drawText ("VOCODER", 200, 22, 90, 14, Justification::centredLeft);
    g.setColour (t.textDim);
    g.setFont (FontOptions (11.0f));
    g.drawText (proc.getUserPresetName().isNotEmpty() ? String ("user") : String (proc.getCurrentProgram() + 1) + " / " + String (mp::numPresets()),
                818, 13, 56, 28, Justification::centredLeft);

    drawDisplay (g, displayArea.toFloat());

    for (const auto& p : panels)
    {
        auto r = p.area.toFloat();
        g.setColour (t.panel.withAlpha (t.knobStyle == 2 ? 0.85f : 1.0f));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (t.knobStyle == 2 ? t.accent.withAlpha (0.45f) : t.panelEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
        if (t.knobStyle == 1)   // panel screws
        {
            g.setColour (t.panelEdge);
            for (auto pt : { r.getTopLeft().translated (8, 8), r.getTopRight().translated (-8, 8), r.getBottomLeft().translated (8, -8), r.getBottomRight().translated (-8, -8) })
                g.fillEllipse (Rectangle<float> (6, 6).withCentre (pt));
        }
        g.setColour (t.accent);
        g.setFont (FontOptions (12.0f, Font::bold));
        g.drawText (p.title, r.withHeight (26).reduced (t.knobStyle == 1 ? 18 : 12, 0), Justification::centredLeft);
    }

    // labels over controls
    g.setFont (FontOptions (10.5f, Font::bold));
    g.setColour (t.textDim);
    for (auto& k : knobs)
    {
        auto b = k->slider.getBounds();
        g.drawText (k->label, b.getX() - 4, b.getY() - 15, b.getWidth() + 8, 14, Justification::centred);
    }
    for (auto& c : choices)
    {
        auto b = c->box.getBounds();
        g.drawText (c->label, b.getX(), b.getY() - 15, b.getWidth(), 14, Justification::centredLeft);
    }
}

void MouthpieceEditor::drawMeter (Graphics& g, Rectangle<float> r, float level, const String& label)
{
    const auto& t = lnf.theme;
    const float db = Decibels::gainToDecibels (level, -60.0f);
    const float frac = jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    auto bar = r.withTrimmedBottom (14.0f);
    g.setColour (t.panelEdge.withAlpha (0.5f));
    g.fillRoundedRectangle (bar, 2.0f);
    g.setColour (db > -3.0f ? t.accent2 : t.accent);
    g.fillRoundedRectangle (bar.withTrimmedTop (bar.getHeight() * (1.0f - frac)), 2.0f);
    g.setColour (t.textDim);
    g.setFont (FontOptions (9.5f, Font::bold));
    g.drawText (label, r.withTop (r.getBottom() - 13.0f).expanded (6.0f, 0.0f), Justification::centred);
}

void MouthpieceEditor::drawDisplay (Graphics& g, Rectangle<float> r)
{
    const auto& t = lnf.theme;
    g.setColour (t.display);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (t.knobStyle == 2 ? t.accent.withAlpha (0.5f) : t.panelEdge);
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

    auto inner = r.reduced (14.0f, 12.0f);
    auto meters = inner.removeFromRight (96.0f);
    const float mw = 22.0f;
    drawMeter (g, meters.removeFromLeft (mw), modMeter, "IN");
    meters.removeFromLeft (10.0f);
    drawMeter (g, meters.removeFromLeft (mw), carMeter, "CAR");
    meters.removeFromLeft (10.0f);
    drawMeter (g, meters.removeFromLeft (mw), outMeter, "OUT");
    inner.removeFromRight (14.0f);

    auto text = inner.removeFromTop (18.0f);
    const bool lightDisplay = t.knobStyle == 3;
    g.setColour (lightDisplay ? t.text : t.accent);
    g.setFont (FontOptions (12.0f, Font::bold));
    const int type = (int) proc.apvts.getRawParameterValue ("type")->load();
    String heading = String (mp::choiceLabel (mp::kType, type)).toUpperCase();
    if (type == mp::TypeTalkbox) heading << "  /  LPC FORMANT FILTER";
    else heading << "  /  " << (int) proc.apvts.getRawParameterValue ("bands")->load() << (type == mp::TypeSpectral ? " BAND RESOLUTION" : " BANDS");
    g.drawText (heading, text, Justification::centredLeft);
    g.setColour (lightDisplay ? t.textDim : (t.knobStyle == 1 ? Colour (0xffe9e2cf) : t.text.withAlpha (0.8f)));
    g.setFont (FontOptions (12.0f));
    g.drawText (statusText(), text, Justification::centredRight);

    inner.removeFromTop (6.0f);
    const int nb = mp::kDisplayBands;
    const float bw = inner.getWidth() / (float) nb;
    for (int b = 0; b < nb; ++b)
    {
        auto col = inner.withX (inner.getX() + bw * (float) b).withWidth (bw).reduced (2.0f, 0.0f);
        const float h = col.getHeight() * bars[(size_t) b];
        const auto c = t.accent.interpolatedWith (t.accent2, (float) b / (float) nb);
        switch (t.barStyle)
        {
            case 1:   // LED segments
            {
                const int segs = 14;
                const float sh = col.getHeight() / (float) segs;
                const int lit = (int) std::round (bars[(size_t) b] * segs);
                for (int s = 0; s < segs; ++s)
                {
                    auto seg = Rectangle<float> (col.getX(), col.getBottom() - sh * (float) (s + 1), col.getWidth(), sh).reduced (0.0f, 1.2f);
                    const Colour on = s > 11 ? Colour (0xffff4a2a) : (s > 8 ? t.accent2 : t.accent);
                    g.setColour (s < lit ? on : on.withAlpha (0.12f));
                    g.fillRect (seg);
                }
                break;
            }
            case 2:   // glow
            {
                auto bar = col.withTop (col.getBottom() - h);
                g.setColour (c.withAlpha (0.18f));
                g.fillRoundedRectangle (bar.expanded (3.0f, 3.0f), 4.0f);
                g.setGradientFill (ColourGradient (c, bar.getX(), bar.getBottom(), Colours::white.withAlpha (0.9f), bar.getX(), bar.getY(), false));
                g.fillRoundedRectangle (bar, 2.0f);
                break;
            }
            case 3:   // outline ink
            {
                g.setColour (t.panelEdge.withAlpha (0.5f));
                g.drawRect (col, 0.6f);
                g.setColour (b % 2 == 0 ? t.accent : t.accent2);
                g.fillRect (col.withTop (col.getBottom() - h));
                break;
            }
            default:
                g.setColour (c.withAlpha (0.9f));
                g.fillRoundedRectangle (col.withTop (col.getBottom() - h), 2.0f);
                break;
        }
        // peak hold tick
        g.setColour (lightDisplay ? t.text : t.text.withAlpha (0.7f));
        const float py = col.getBottom() - col.getHeight() * peaks[(size_t) b];
        if (peaks[(size_t) b] > 0.02f) g.drawHorizontalLine ((int) py, col.getX(), col.getRight());
    }
}

String MouthpieceEditor::statusText() const
{
    using namespace mp;
    auto v = [this] (int id) { return (int) proc.apvts.getRawParameterValue (paramDefs()[id].id)->load(); };
    const auto& eng = proc.getEngine();
    const int carrier = v (kCarrier);
    if (v (kModSource) == ModSidechain && ! proc.sidechainActive()) return "Modulator is set to Sidechain: route a track to the sidechain input";
    switch (carrier)
    {
        case CarSynth:
            return eng.heldNotes.load() > 0 ? "Carrier: synth, " + String (eng.heldNotes.load()) + " voice(s)"
                                            : "Carrier: synth. Play MIDI notes or the keyboard below";
        case CarAutoPitch:
        {
            const float hz = eng.detectedHz.load();
            if (hz <= 0) return "Auto Pitch: listening (sing or play into the input)";
            const int note = (int) std::lround (69.0 + 12.0 * std::log2 (hz / 440.0));
            return "Auto Pitch: " + MidiMessage::getMidiNoteName (note, true, true, 3) + "  " + String (hz, 1) + " Hz";
        }
        case CarDrone:
            return "Drone: " + String (choiceLabel (kKey, v (kKey))) + " " + String (choiceLabel (kChord, v (kChord)));
        case CarNoise: return "Carrier: noise (whisper)";
        case CarExternal:
            if (v (kModSource) == ModSidechain) return "Carrier: main input, voice on sidechain";
            return proc.sidechainActive() ? "Carrier: sidechain input" : "Carrier is External: route a synth or track to the sidechain";
        case CarPlugin:
            if (proc.hosted.getPlugin() == nullptr) return "Carrier is Plugin: load one with the plugin button above";
            return "Carrier: " + proc.hosted.getName() + (proc.hosted.getPlugin()->acceptsMidi() ? " (play MIDI)" : " (processing the sidechain)");
        default: return {};
    }
}

void MouthpieceEditor::timerCallback()
{
    const auto& eng = proc.getEngine();
    for (int b = 0; b < mp::kDisplayBands; ++b)
    {
        const float lv = eng.display[(size_t) b].load (std::memory_order_relaxed);
        const float db = Decibels::gainToDecibels (lv, -60.0f);
        const float target = jlimit (0.0f, 1.0f, (db + 54.0f) / 54.0f);
        auto& bar = bars[(size_t) b];
        bar = target > bar ? target : bar * 0.86f + target * 0.14f;
        auto& pk = peaks[(size_t) b];
        pk = bar > pk ? bar : jmax (0.0f, pk - 0.008f);
    }
    modMeter = eng.modLevel.load();
    carMeter = eng.carLevel.load();
    outMeter = eng.outLevel.load();

    const int stamp = proc.programChanged.load();
    if (stamp != lastProgramStamp)
    {
        lastProgramStamp = stamp;
        refreshPresetBoxes();
        if (themeBox.getSelectedId() != proc.theme.load() + 1) applyTheme (proc.theme.load());
    }

    pluginButton.setButtonText (pluginButtonText());

    // dim controls that the current mode ignores
    using namespace mp;
    auto v = [this] (int id) { return (int) proc.apvts.getRawParameterValue (paramDefs()[id].id)->load(); };
    const int type = v (kType), carrier = v (kCarrier);
    const bool synthCarrier = carrier <= CarDrone;
    const bool bank = type != TypeTalkbox;
    for (int p : { (int) kBands, (int) kLowHz, (int) kHighHz }) knobFor[p]->slider.setAlpha (bank ? 1.0f : 0.35f);
    for (int p : { (int) kAttack, (int) kRelease }) knobFor[p]->slider.setAlpha (bank ? 1.0f : 0.35f);
    for (int p : { (int) kOctave, (int) kUnison, (int) kDetune, (int) kOsc2Mix, (int) kOsc2Semi, (int) kSub, (int) kNoise, (int) kGlide,
                   (int) kAmpAttack, (int) kAmpRelease, (int) kVibrato })
        knobFor[p]->slider.setAlpha (synthCarrier ? 1.0f : 0.35f);
    knobFor[kPulseWidth]->slider.setAlpha (synthCarrier && v (kWave) == WPulse ? 1.0f : 0.35f);
    choiceFor[kWave]->box.setAlpha (synthCarrier ? 1.0f : 0.35f);
    for (int p : { (int) kKey, (int) kScale }) choiceFor[p]->box.setAlpha (carrier == CarAutoPitch || carrier == CarDrone || v (kChord) >= 14 ? 1.0f : 0.35f);
    choiceFor[kChord]->box.setAlpha (synthCarrier ? 1.0f : 0.35f);

    content.repaint (displayArea);
    content.repaint (headerArea);
}

// ============================================================================ plugin slot and user presets
String MouthpieceEditor::pluginButtonText() const
{
    const auto name = proc.hosted.getName();
    return name.isEmpty() ? String ("Load carrier plugin...") : "Plugin: " + name;
}

void MouthpieceEditor::showPluginMenu()
{
    auto types = proc.hosted.knownPlugins.getTypes();
    PopupMenu m;
    const bool loaded = proc.hosted.getPlugin() != nullptr;
    if (loaded)
    {
        m.addSectionHeader (proc.hosted.getName());
        m.addItem ("Open plugin window", [this] { openPluginWindow(); });
        m.addItem ("Remove plugin", [this]
        {
            closePluginWindow();
            proc.hosted.unload();
        });
        m.addSeparator();
    }
    if (types.isEmpty())
        m.addItem ("No plugins found yet. Scan for plugins...", [this] { openScanner(); });
    else
    {
        PopupMenu list;
        KnownPluginList::addToMenu (list, types, KnownPluginList::sortByCategory);
        m.addSubMenu (loaded ? "Replace with" : "Load", list);
        m.addSeparator();
        m.addItem ("Scan for plugins...", [this] { openScanner(); });
    }
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&pluginButton), [this, types] (int result)
    {
        const int idx = KnownPluginList::getIndexChosenByMenu (types, result);
        if (! isPositiveAndBelow (idx, types.size())) return;
        closePluginWindow();
        String error;
        if (! proc.hosted.load (types.getReference (idx), error))
        {
            AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "Could not load plugin", error);
            return;
        }
        // switch the carrier to the plugin and show its window
        if (auto* prm = proc.apvts.getParameter (mp::paramDefs()[mp::kCarrier].id))
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) mp::CarPlugin));
        openPluginWindow();
    });
}

void MouthpieceEditor::openPluginWindow()
{
    auto* p = proc.hosted.getPlugin();
    if (p == nullptr) return;
    if (pluginWindow != nullptr) { pluginWindow->toFront (true); return; }
    pluginWindow = std::make_unique<PluginWindow> (*p, [this] { closePluginWindow(); });
}

void MouthpieceEditor::closePluginWindow() { pluginWindow.reset(); }

void MouthpieceEditor::openScanner()
{
    if (scannerWindow != nullptr) { scannerWindow->toFront (true); return; }
    scannerWindow = std::make_unique<ScannerWindow> (proc.hosted, [this] { scannerWindow.reset(); });
}

void MouthpieceEditor::saveUserPreset()
{
    saveDialog = std::make_unique<AlertWindow> ("Save preset", "Name for this preset (saved in Documents/Mouthpiece/Presets):", MessageBoxIconType::NoIcon);
    const String suggestion = proc.getUserPresetName().isNotEmpty() ? proc.getUserPresetName() : presetBox.getText() + " (mine)";
    saveDialog->addTextEditor ("name", suggestion);
    saveDialog->addButton ("Save", 1, KeyPress (KeyPress::returnKey));
    saveDialog->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
    saveDialog->enterModalState (true, ModalCallbackFunction::create ([this] (int result)
    {
        if (result == 1 && saveDialog != nullptr)
        {
            String error;
            const auto name = saveDialog->getTextEditorContents ("name");
            if (! proc.saveUserPreset (name, error))
                AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "Could not save preset", error);
        }
        MessageManager::callAsync ([sp = Component::SafePointer<MouthpieceEditor> (this)] { if (sp != nullptr) sp->saveDialog.reset(); });
    }), false);
}
