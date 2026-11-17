// TalkBox Reference — Editor implementation (vector GUI, no images)
#include "PluginEditor.h"

void VowelPad::paint(juce::Graphics& g) {
    auto b = getLocalBounds();
    g.setColour(juce::Colours::black);
    g.fillRect(b);
    // Vowel space grid + F1/F2 axes
    g.setColour(juce::Colours::darkgrey);
    for (int i=1;i<4;++i) {
        float x = b.getX() + b.getWidth()*i/4.f;
        float y = b.getY() + b.getHeight()*i/4.f;
        g.drawLine(x, b.getY(), x, b.getY()+b.getHeight(), 0.5f);
        g.drawLine(b.getX(), y, b.getX()+b.getWidth(), y, 0.5f);
    }
    // Vowel anchors (labels)
    g.setColour(juce::Colours::grey);
    g.setFont(10.f);
    for (auto& v : tb::kVowels) {
        float x = b.getX() + v.x*b.getWidth();
        float y = b.getY() + (1.f-v.y)*b.getHeight();
        g.drawText(v.name, (int)x-10, (int)y-6, 20, 12, juce::Justification::centred);
    }
    // Current position (pad or tracked)
    float px = proc.apvts.getRawParameterValue("padX")->load();
    float py = proc.apvts.getRawParameterValue("padY")->load();
    if (proc.engine.p.source == tb::SourceMode::Mic) {
        // Animate to tracked vowel: map F1/F2 back to pad approx
        float f1 = proc.visF[0].load(), f2 = proc.visF[1].load();
        px = juce::jmap(f2, 650.f, 2300.f, 0.f, 1.f);
        py = juce::jmap(f1, 250.f, 800.f, 0.f, 1.f);
        px = juce::jlimit(0.f,1.f,px); py = juce::jlimit(0.f,1.f,py);
    }
    float cx = b.getX() + px*b.getWidth();
    float cy = b.getY() + (1.f-py)*b.getHeight();
    g.setColour(juce::Colours::orange);
    g.fillEllipse(cx-8, cy-8, 16, 16);
    g.setColour(juce::Colours::white);
    g.drawEllipse(cx-8, cy-8, 16, 16, 2.f);
    // Mouth openness arc
    float mouth = proc.visMouth.load();
    g.setColour(juce::Colours::cyan);
    g.drawEllipse(cx-14, cy-14, 28, 28, 1.f);
    g.drawArc(cx-14, cy-14, 28, 28, -3.14f/2, -3.14f/2 + mouth*3.14f, 1.f);
}
void VowelPad::mouseDown(const juce::MouseInput& mi) { mouseDrag(mi); }
void VowelPad::mouseDrag(const juce::MouseInput& mi) {
    auto b = getLocalBounds();
    float x = juce::jlimit(0.f,1.f, (mi.getPosition().x)/ (float)b.getWidth());
    float y = juce::jlimit(0.f,1.f, 1.f - (mi.getPosition().y)/(float)b.getHeight());
    proc.apvts.getState().setProperty("padX", x, nullptr);
    proc.apvts.getState().setProperty("padY", y, nullptr);
    // Direct param set (for responsiveness)
    if (auto* p = proc.apvts.getParameter("padX")) p->setValueNotifyingHost(x);
    if (auto* p = proc.apvts.getParameter("padY")) p->setValueNotifyingHost(y);
}

TalkBoxReferenceEditor::TalkBoxReferenceEditor(TalkBoxReferenceProcessor& p)
    : AudioProcessorEditor(p), proc(p), pad(p) {
    setSize(900, 640);
    addAndMakeVisible(pad);
    addAndMakeVisible(sourceBox); addAndMakeVisible(carrierBox);
    addAndMakeVisible(qualityBox); addAndMakeVisible(presetBox);
    addAndMakeVisible(bypassBtn); addAndMakeVisible(simpleAdvBtn); addAndMakeVisible(micSetupBtn);
    sourceBox.addItem("Pad/MIDI", 1); sourceBox.addItem("Live Mic Control", 2);
    carrierBox.addItem("External Audio", 1); carrierBox.addItem("Internal Talkbox Synth", 2);
    qualityBox.addItem("Eco", 1); qualityBox.addItem("Balanced", 2); qualityBox.addItem("Studio", 3); qualityBox.addItem("Creative", 4);
    presetBox.addItem("Mic-Ready Funk Lead", 1); presetBox.addItem("Whisper Talkbox", 2);
    presetBox.addItem("Classic Guitar Hose", 3); presetBox.addItem("Pad Vowel Morphs", 4);
    bypassBtn.setButtonText("Bypass"); bypassBtn.setClickingTogglesState(true);
    simpleAdvBtn.setButtonText("Advanced"); simpleAdvBtn.setClickingTogglesState(true);
    micSetupBtn.setButtonText("Mic Quick-Setup & Headphone Safety");
    addAndMakeVisible(tabs);
    tabs.addTab("Simple", 1, &simplePage, false);
    tabs.addTab("Vocal Tract", 2, &tractPage, false);
    tabs.addTab("Mic & Carrier", 3, &micPage, false);
    tabs.addTab("Hardware Chain", 4, &hwPage, false);
    tabs.addTab("System", 5, &sysPage, false);
    startTimer(30); // ~33 Hz visualizer (near-zero CPU)
}
void TalkBoxReferenceEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkslategrey);
    g.setColour(juce::Colours::white); g.setFont(14.f);
    g.drawText("TalkBox Reference — mic-playable hardware talkbox", 10, 4, 600, 20, juce::Justification::topLeft);
}
void TalkBoxReferenceEditor::resized() {
    pad.setBounds(10, 30, 400, 400);
    sourceBox.setBounds(420, 30, 160, 24); carrierBox.setBounds(420, 60, 160, 24);
    qualityBox.setBounds(420, 90, 160, 24); presetBox.setBounds(420, 120, 160, 24);
    bypassBtn.setBounds(420, 150, 160, 24); simpleAdvBtn.setBounds(420, 180, 160, 24);
    micSetupBtn.setBounds(10, 440, 880, 30);
    tabs.setBounds(10, 480, 880, 150);
}
void TalkBoxReferenceEditor::timerCallback() { pad.repaint(); }
