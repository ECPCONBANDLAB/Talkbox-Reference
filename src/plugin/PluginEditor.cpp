// TalkBox Reference — Editor implementation (vector GUI, no images)
#include "PluginEditor.h"

// ---------------------------------------------------------------------------
// VowelPad
// ---------------------------------------------------------------------------
void VowelPad::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();

    g.setColour(juce::Colours::black);
    g.fillRect(b);

    // Vowel space grid (F1/F2 axes aid)
    g.setColour(juce::Colours::darkgrey);
    for (int i = 1; i < 4; ++i) {
        const float x = b.getX() + b.getWidth()  * (float)i / 4.0f;
        const float y = b.getY() + b.getHeight() * (float)i / 4.0f;
        g.drawLine(x, b.getY(), x, b.getBottom(), 0.5f);
        g.drawLine(b.getX(), y, b.getRight(), y, 0.5f);
    }

    // Vowel anchors (labels)
    g.setColour(juce::Colours::grey);
    g.setFont(10.0f);
    for (int i = 0; i < 9; ++i) {
        const float x = b.getX() + tb::kVowelPadXY[i][0] * b.getWidth();
        const float y = b.getY() + (1.0f - tb::kVowelPadXY[i][1]) * b.getHeight();
        g.drawText(juce::String(tb::kVowels[i].name), juce::Rectangle<float>(x - 10.0f, y - 6.0f, 20.0f, 12.0f),
                   juce::Justification::centred);
    }

    // Current position: pad automation, or tracked formants in mic mode
    float px = 0.5f, py = 0.45f;
    if (auto* v = proc.apvts.getRawParameterValue("padX")) px = v->load();
    if (auto* v = proc.apvts.getRawParameterValue("padY")) py = v->load();

    if (proc.engine.p.source == tb::SourceMode::Mic) {
        // Map tracked F1/F2 back onto approximate pad coordinates.
        const float f1 = proc.visF[0].load(), f2 = proc.visF[1].load();
        px = juce::jlimit(0.0f, 1.0f, juce::jmap(f2, 650.0f, 2300.0f, 0.0f, 1.0f));
        py = juce::jlimit(0.0f, 1.0f, juce::jmap(f1, 250.0f, 800.0f, 0.0f, 1.0f));
    }

    const float cx = b.getX() + px * b.getWidth();
    const float cy = b.getY() + (1.0f - py) * b.getHeight();

    g.setColour(juce::Colours::orange);
    g.fillEllipse(cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
    g.setColour(juce::Colours::white);
    g.drawEllipse(cx - 8.0f, cy - 8.0f, 16.0f, 16.0f, 2.0f);

    // Mouth openness ring + arc (Graphics has no drawArc: use a Path).
    const float mouth = proc.visMouth.load();
    g.setColour(juce::Colours::cyan);
    g.drawEllipse(cx - 14.0f, cy - 14.0f, 28.0f, 28.0f, 1.0f);

    juce::Path arc;
    arc.addArc(cx - 14.0f, cy - 14.0f, 28.0f, 28.0f,
               juce::MathConstants<float>::pi * 1.5f,
               juce::MathConstants<float>::pi * 1.5f + mouth * juce::MathConstants<float>::pi,
               true);
    g.strokePath(arc, juce::PathStrokeType(2.0f));
}

void VowelPad::mouseDrag(const juce::MouseEvent& e) {
    const auto b = getLocalBounds();
    if (b.getWidth() <= 0 || b.getHeight() <= 0) return;

    const float x = juce::jlimit(0.0f, 1.0f, (float)e.getPosition().x / (float)b.getWidth());
    const float y = juce::jlimit(0.0f, 1.0f, 1.0f - (float)e.getPosition().y / (float)b.getHeight());

    // Drive the real parameters so automation and the audio thread both see it.
    if (auto* p = proc.apvts.getParameter("padX"))
        p->setValueNotifyingHost(p->convertTo0to1(x));
    if (auto* p = proc.apvts.getParameter("padY"))
        p->setValueNotifyingHost(p->convertTo0to1(y));

    repaint();
}

// ---------------------------------------------------------------------------
// Editor
// ---------------------------------------------------------------------------
TalkBoxReferenceEditor::TalkBoxReferenceEditor(TalkBoxReferenceProcessor& p)
    : AudioProcessorEditor(p), proc(p), pad(p) {
    setSize(900, 640);

    addAndMakeVisible(pad);

    for (auto* box : { &sourceBox, &carrierBox, &qualityBox, &presetBox })
        addAndMakeVisible(*box);
    for (auto* btn : { &bypassBtn, &simpleAdvBtn, &micSetupBtn })
        addAndMakeVisible(*btn);

    sourceBox.addItem("Pad/MIDI", 1);
    sourceBox.addItem("Live Mic Control", 2);
    carrierBox.addItem("External Audio", 1);
    carrierBox.addItem("Internal Talkbox Synth", 2);
    qualityBox.addItem("Eco", 1);
    qualityBox.addItem("Balanced", 2);
    qualityBox.addItem("Studio", 3);
    qualityBox.addItem("Creative", 4);
    presetBox.addItem("Mic-Ready Funk Lead", 1);
    presetBox.addItem("Whisper Talkbox", 2);
    presetBox.addItem("Classic Guitar Hose", 3);
    presetBox.addItem("Pad Vowel Morphs", 4);

    bypassBtn.setButtonText("Bypass");
    bypassBtn.setClickingTogglesState(true);
    simpleAdvBtn.setButtonText("Advanced");
    simpleAdvBtn.setClickingTogglesState(true);
    micSetupBtn.setButtonText("Mic Quick-Setup & Headphone Safety");

    addAndMakeVisible(tabs);
    tabs.addTab("Simple",        juce::Colours::darkgrey, &simplePage, false);
    tabs.addTab("Vocal Tract",   juce::Colours::darkgrey, &tractPage,  false);
    tabs.addTab("Mic & Carrier", juce::Colours::darkgrey, &micPage,    false);
    tabs.addTab("Hardware Chain",juce::Colours::darkgrey, &hwPage,     false);
    tabs.addTab("System",        juce::Colours::darkgrey, &sysPage,    false);

    startTimerHz(30); // ~33 Hz visualizer (near-zero CPU)
}

void TalkBoxReferenceEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkslategrey);
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("TalkBox Reference - mic-playable hardware talkbox",
               getLocalBounds().removeFromTop(24).reduced(10, 0),
               juce::Justification::centredLeft);
}

void TalkBoxReferenceEditor::resized() {
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(20); // title strip

    pad.setBounds(area.removeFromLeft(400).removeFromTop(400));

    auto right = area.withTrimmedLeft(10).removeFromTop(180);
    const juce::Component* const controls[] = { &sourceBox, &carrierBox, &qualityBox,
                                                &presetBox, &bypassBtn, &simpleAdvBtn };
    for (auto* c : controls)
        const_cast<juce::Component*>(c)->setBounds(
            right.removeFromTop(26).removeFromLeft(180).reduced(0, 1));

    micSetupBtn.setBounds(getLocalBounds().reduced(10).removeFromBottom(190).removeFromTop(30));
    tabs.setBounds(getLocalBounds().reduced(10).removeFromBottom(150));
}

void TalkBoxReferenceEditor::timerCallback() { pad.repaint(); }
