// TalkBox Reference — PluginEditor.cpp
#include "PluginEditor.h"

namespace {
const juce::Colour kBg{0xff10131a};
const juce::Colour kPanel{0xff181d27};
const juce::Colour kAccent{0xff41d1b0};
const juce::Colour kAccent2{0xffffb454};
const juce::Colour kText{0xffdfe6f0};
const juce::Colour kDim{0xff7c8798};

juce::Font labelFont() { return juce::Font(juce::FontOptions(11.0f)); }
juce::Font titleFont() { return juce::Font(juce::FontOptions(15.0f, juce::Font::bold)); }
}  // namespace

TalkboxAudioProcessorEditor::TalkboxAudioProcessorEditor(TalkboxAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processor_(p) {
    setSize(720, 460);
    setResizable(true, false);
    setResizeLimits(720, 460, 1080, 690);
    buildControls();
    startTimerHz(30);
}

void TalkboxAudioProcessorEditor::styleSlider(juce::Slider& s) {
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 16);
    s.setColour(juce::Slider::rotarySliderFillColourId, kAccent);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a3140));
    s.setColour(juce::Slider::thumbColourId, kText);
    s.setColour(juce::Slider::textBoxTextColourId, kText);
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void TalkboxAudioProcessorEditor::addContinuous(const juce::String& paramId, const juce::String& caption,
                                                juce::Rectangle<int> area) {
    auto c = std::make_unique<ContinuousControl>();
    styleSlider(c->slider);
    c->label.setText(caption, juce::dontSendNotification);
    c->label.setFont(labelFont());
    c->label.setColour(juce::Label::textColourId, kDim);
    c->label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(c->slider);
    addAndMakeVisible(c->label);
    c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor_.state(), paramId, c->slider);
    c->slider.setBounds(area.withTrimmedTop(14));
    c->label.setBounds(area.removeFromTop(14));
    continuous_.push_back(std::move(c));
}

void TalkboxAudioProcessorEditor::addChoice(const juce::String& paramId, const juce::String& caption,
                                            juce::Rectangle<int> area) {
    auto c = std::make_unique<ChoiceControl>();
    c->label.setText(caption, juce::dontSendNotification);
    c->label.setFont(labelFont());
    c->label.setColour(juce::Label::textColourId, kDim);
    addAndMakeVisible(c->box);
    addAndMakeVisible(c->label);
    c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor_.state(), paramId, c->box);
    c->box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff222a36));
    c->box.setColour(juce::ComboBox::textColourId, kText);
    c->box.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff2d3644));
    c->label.setBounds(area.removeFromTop(14));
    c->box.setBounds(area);
    choices_.push_back(std::move(c));
}

void TalkboxAudioProcessorEditor::buildControls() {
    presetLabel_.setText("Preset", juce::dontSendNotification);
    presetLabel_.setFont(labelFont());
    presetLabel_.setColour(juce::Label::textColourId, kDim);
    addAndMakeVisible(presetLabel_);

    for (int i = 0; i < processor_.api().numPresets(); ++i)
        presetBox_.addItem(processor_.getProgramName(i), i + 1);
    presetBox_.setSelectedId(3, juce::dontSendNotification);
    presetBox_.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff222a36));
    presetBox_.setColour(juce::ComboBox::textColourId, kText);
    presetBox_.onChange = [this] {
        processor_.setCurrentProgram(presetBox_.getSelectedId() - 1);
    };
    addAndMakeVisible(presetBox_);

    // Column 1 — source + hose
    addChoice("vowel", "VOWEL", {24, 92, 200, 26});
    addChoice("carrierMode", "CARRIER", {24, 152, 200, 26});
    addContinuous("carrierFreq", "PITCH", {24, 196, 96, 92});
    addContinuous("carrierNoise", "NOISE", {128, 196, 96, 92});
    addContinuous("hoseLength", "HOSE LENGTH", {24, 300, 96, 92});
    addContinuous("hoseFeedback", "FEEDBACK", {128, 300, 96, 92});

    // Column 2 — tract
    addContinuous("vtl", "VTL", {264, 92, 92, 92});
    addContinuous("formantShift", "FORMANT", {364, 92, 92, 92});
    addContinuous("nasal", "NASAL", {264, 196, 92, 92});
    addContinuous("lipAperture", "LIPS", {364, 196, 92, 92});
    addContinuous("drive", "DRIVE", {264, 300, 92, 92});
    addChoice("oversample", "OVERSAMPLING", {364, 310, 92, 24});

    // Column 3 — microphone + master
    addChoice("micModel", "MICROPHONE", {492, 92, 204, 26});
    addContinuous("micDistance", "DISTANCE", {492, 130, 96, 92});
    addContinuous("saturation", "SATURATION", {596, 130, 96, 92});
    addContinuous("output", "OUTPUT", {492, 240, 96, 92});
    addContinuous("ceiling", "CEILING", {596, 240, 96, 92});

    for (auto* b : {&plosiveButton_, &fricativeButton_, &nasalButton_}) {
        b->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222a36));
        b->setColour(juce::TextButton::textColourOnId, kBg);
        b->setColour(juce::TextButton::textColourOffId, kText);
        addAndMakeVisible(b);
    }
    plosiveButton_.onClick = [this] { processor_.triggerConsonant(tb::Consonant::Plosive); };
    fricativeButton_.onClick = [this] { processor_.triggerConsonant(tb::Consonant::S); };
    nasalButton_.onClick = [this] { processor_.triggerConsonant(tb::Consonant::Nasal); };
}

void TalkboxAudioProcessorEditor::resized() {
    auto header = getLocalBounds().reduced(24, 0).withHeight(64);
    presetLabel_.setBounds(header.removeFromLeft(48).withTrimmedTop(30).withHeight(16));
    presetBox_.setBounds(header.removeFromLeft(200).withTrimmedTop(24).withHeight(28));

    auto footer = getLocalBounds().reduced(24, 18).removeFromBottom(34);
    plosiveButton_.setBounds(footer.removeFromLeft(56).withSizeKeepingCentre(48, 26));
    footer.removeFromLeft(8);
    fricativeButton_.setBounds(footer.removeFromLeft(56).withSizeKeepingCentre(48, 26));
    footer.removeFromLeft(8);
    nasalButton_.setBounds(footer.removeFromLeft(56).withSizeKeepingCentre(48, 26));
}

void TalkboxAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(kBg);

    auto header = getLocalBounds().removeFromTop(64).reduced(24, 0);
    g.setColour(kText);
    g.setFont(titleFont());
    g.drawText("TALKBOX REFERENCE", header.removeFromTop(34), juce::Justification::centredLeft);
    g.setColour(kDim);
    g.setFont(labelFont());
    g.drawText("source-filter vocal tract / hose / microphone", header, juce::Justification::centredLeft);

    // Panels
    auto drawPanel = [&](juce::Rectangle<int> r, const juce::String& title, juce::Colour accent) {
        g.setColour(kPanel);
        g.fillRoundedRectangle(r.toFloat(), 8.0f);
        g.setColour(accent.withAlpha(0.55f));
        g.fillRoundedRectangle(r.toFloat().withHeight(2.0f), 1.0f);
        g.setColour(kDim);
        g.setFont(labelFont());
        g.drawText(title, r.reduced(12, 8).removeFromTop(12), juce::Justification::centredLeft);
    };
    drawPanel({16, 74, 220, 330}, "SOURCE", kAccent2);
    drawPanel({244, 74, 232, 330}, "VOCAL TRACT", kAccent);
    drawPanel({484, 74, 220, 330}, "MICROPHONE", juce::Colour(0xff8ab4ff));

    // Meter
    auto meter = juce::Rectangle<int>(24, 418, 680, 10);
    g.setColour(juce::Colour(0xff222a36));
    g.fillRoundedRectangle(meter.toFloat(), 4.0f);
    const float level = juce::jlimit(0.0f, 1.0f, meter_);
    g.setColour(level > 0.9f ? juce::Colour(0xffff6b6b) : kAccent);
    g.fillRoundedRectangle(meter.toFloat().withWidth(meter.getWidth() * level), 4.0f);

    g.setColour(kDim);
    g.setFont(labelFont());
    g.drawText(juce::String("latency ") + juce::String(processor_.currentLatencyMs(), 2) + " ms   hose " +
                   juce::String(processor_.currentHoseMs(), 2) + " ms",
               juce::Rectangle<int>(424, 424, 280, 14), juce::Justification::centredRight);
}

void TalkboxAudioProcessorEditor::timerCallback() {
    const float target = juce::jlimit(0.0f, 1.0f, processor_.outputPeak());
    meter_ = juce::jmax(target, meter_ * 0.86f);
    repaint();
}
