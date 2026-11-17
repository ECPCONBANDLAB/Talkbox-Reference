// TalkBox Reference - Editor implementation.
#include "PluginEditor.h"

using namespace tbui;

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------
juce::AudioParameterChoice* TalkBoxReferenceEditor::choice(const char* id)
{
    return dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(id));
}
juce::AudioParameterFloat* TalkBoxReferenceEditor::flt(const char* id)
{
    return dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter(id));
}
juce::AudioParameterBool* TalkBoxReferenceEditor::bll(const char* id)
{
    return dynamic_cast<juce::AudioParameterBool*>(proc.apvts.getParameter(id));
}

tbui::Knob& TalkBoxReferenceEditor::addKnob(const char* paramId, const juce::String& caption,
                                            const juce::String& suffix, int decimals,
                                            bool bipolar, int tab)
{
    auto* p = flt(paramId);
    jassert(p != nullptr);           // every knob must bind to a real parameter
    knobs.push_back({ std::make_unique<Knob>(caption, *p, suffix, decimals, bipolar),
                      0, 0, 0, 0 });
    auto& k = *knobs.back().knob;
    addAndMakeVisible(k);
    tabbed.push_back({ &k, tab });
    return k;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
TalkBoxReferenceEditor::TalkBoxReferenceEditor(TalkBoxReferenceProcessor& p)
    : AudioProcessorEditor(p), proc(p), pad(p)
{
    setSize(980, 700);
    setResizable(true, true);
    setResizeLimits(900, 640, 1600, 1100);

    addAndMakeVisible(pad);
    buildControls();

    bypassBtn.setClickingTogglesState(true);
    bypassBtn.setColour(juce::TextButton::buttonOnColourId, colour::warn);
    bypassBtn.setColour(juce::TextButton::buttonColourId, colour::panelRaised);
    bypassBtn.setColour(juce::TextButton::textColourOffId, colour::textDim);
    bypassBtn.setColour(juce::TextButton::textColourOnId, colour::text);
    bypassBtn.onClick = [this] {
        if (auto* b = bll("bypass"))
            b->setValueNotifyingHost(bypassBtn.getToggleState() ? 1.0f : 0.0f);
    };
    addAndMakeVisible(bypassBtn);

    advBtn.setClickingTogglesState(true);
    advBtn.setColour(juce::TextButton::buttonOnColourId, colour::accentDim);
    advBtn.setColour(juce::TextButton::buttonColourId, colour::panelRaised);
    advBtn.setColour(juce::TextButton::textColourOffId, colour::textDim);
    advBtn.setColour(juce::TextButton::textColourOnId, colour::text);
    advBtn.onClick = [this] {
        visibleTabCount = advBtn.getToggleState() ? 5 : 2;
        if (currentTab >= visibleTabCount) switchTab(0);
        repaint();
    };
    addAndMakeVisible(advBtn);

    switchTab(0);
    startTimerHz(20);   // pad animation only; everything else repaints on demand
}

TalkBoxReferenceEditor::~TalkBoxReferenceEditor() = default;

void TalkBoxReferenceEditor::buildControls()
{
    // ---- Tab 0: SIMPLE -- the macros a player touches first ----------------
    addKnob("dryWet",        "Mix",        " %",  0, false, 0);
    addKnob("outputGain",    "Output",     " dB", 1, false, 0);
    addKnob("carrierDrive",  "Drive",      " %",  0, false, 0);
    addKnob("tubeCharacter", "Tube",       " %",  0, false, 0);
    addKnob("mouthSize",     "Mouth",      " x",  2, false, 0);
    addKnob("articSmooth",   "Artic",      " %",  0, false, 0);
    addKnob("consonantCrisp","Crisp",      " %",  0, false, 0);
    addKnob("micProximity",  "Proximity",  " %",  0, false, 0);

    // ---- Tab 1: VOCAL TRACT ------------------------------------------------
    addKnob("padX",         "Tongue Fwd", " %", 0, false, 1);
    addKnob("padY",         "Tongue Up",  " %", 0, false, 1);
    addKnob("jaw",          "Jaw",        " %", 0, false, 1);
    addKnob("tongueX",      "Body Fwd",   " %", 0, false, 1);
    addKnob("tongueHeight", "Body Up",    " %", 0, false, 1);
    addKnob("lipRounding",  "Rounding",   " %",  0, false, 1);
    addKnob("mouthOpening", "Aperture",   " %",  0, false, 1);
    addKnob("nasal",        "Nasal",      " %",  0, false, 1);
    addKnob("vtl",          "VTL",        " x",  2, false, 1);
    addKnob("glide",        "Glide",      " %",  0, false, 1);

    // ---- Tab 2: MIC & CARRIER ---------------------------------------------
    addKnob("micPreGain",     "Mic Gain",  " dB", 1, false, 2);
    addKnob("gateThresh",     "Gate",      " dB", 0, false, 2);
    addKnob("whisperBoost",   "Whisper",   " x",  2, false, 2);
    addKnob("sibilDeess",     "De-ess",    " %",  0, false, 2);
    addKnob("voiceBleed",     "Bleed",     " %",  0, false, 2);
    addKnob("micSmooth",      "Smooth",    " %",  0, false, 2);
    addKnob("inputGain",      "Carrier",   " dB", 1, false, 2);
    addKnob("synthOctave",    "Octave",    "",    0, true,  2);
    addKnob("synthGlide",     "Porta",     " %",  0, false, 2);
    addKnob("synthVibrato",   "Vibrato",   " st", 1, false, 2);

    // ---- Tab 3: HARDWARE CHAIN --------------------------------------------
    addKnob("driverComp",    "Driver",    " %",  0, false, 3);
    addKnob("diaphragmAsym", "Asym",      " %",  0, true,  3);
    addKnob("hornCut",       "Horn Cut",  " Hz", 0, false, 3);
    addKnob("hoseLength",    "Hose",      " m",  2, false, 3);
    addKnob("hoseDamping",   "Damping",   " %",  0, false, 3);
    addKnob("micDistance",   "Mic Dist",  " cm", 0, false, 3);
    addKnob("proxBoost",     "Prox",      " %",  0, false, 3);
    addKnob("ambience",      "Ambience",  " %",  0, false, 3);

    // ---- Tab 4: SYSTEM ----------------------------------------------------
    addKnob("padX",  "Pad X", " %", 0, false, 4);   // duplicate binding is fine:
    addKnob("padY",  "Pad Y", " %", 0, false, 4);   // knobs are stateless views

    // Choice parameters -> segmented controls
    auto addSeg = [&](const char* id, int tab, int cxStep) {
        if (auto* c = choice(id)) {
            auto s = std::make_unique<Segmented>(*c);
            auto* raw = s.get();
            segments.push_back(std::move(s));
            addAndMakeVisible(*raw);
            tabbed.push_back({ raw, tab });
            (void)cxStep;
        }
    };
    addSeg("sourceMode", 0, 1);
    addSeg("carrierMode", 0, 1);
    addSeg("quality", 0, 1);
    addSeg("gateBehavior", 2, 1);
    addSeg("micRouting", 2, 1);
    addSeg("synthWave", 2, 1);
    addSeg("micModel", 3, 1);

    // Boolean parameters -> toggles
    auto addTog = [&](const char* id, const juce::String& cap, int tab) {
        if (auto* b = bll(id)) {
            auto t = std::make_unique<Toggle>(cap, *b);
            auto* raw = t.get();
            toggles.push_back(std::move(t));
            addAndMakeVisible(*raw);
            tabbed.push_back({ raw, tab });
        }
    };
    addTog("bypass", "Bypass", 4);

    // Dropdowns for long choice lists
    for (const char* id : { "micModel" }) { (void)id; }
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
void TalkBoxReferenceEditor::switchTab(int newTab)
{
    currentTab = juce::jlimit(0, 4, newTab);
    for (auto& k : knobs)  k.knob->setVisible(k.knob != nullptr);
    for (auto& [comp, tab] : tabbed)
        comp->setVisible(tab == currentTab);
    resized();
    repaint();
}

void TalkBoxReferenceEditor::resized()
{
    auto area = getLocalBounds();

    headerArea = area.removeFromTop(58);
    tabBarArea = area.removeFromTop(34);

    auto content = area.reduced(12, 8);

    // Simple tab keeps the pad large and puts the macros in two rows; the
    // advanced tabs drop the pad's share so the sections get room.
    const bool simple = (currentTab == 0);
    const int padW = simple ? juce::jmax(280, content.getWidth() * 34 / 100)
                            : juce::jmax(220, content.getWidth() * 26 / 100);

    auto left = content.removeFromLeft(padW);
    padArea = left.removeFromTop(simple ? left.getHeight() - 8 : 240);
    pad.setBounds(padArea);
    content.removeFromLeft(12);
    bodyArea = content;

    // Column-flow layout: each tab declares how many knobs per row.
    const int perRow = simple ? 4 : 5;
    const int rowH = simple ? 116 : 104;
    const int knobW = juce::jmax(64, bodyArea.getWidth() / perRow - 10);

    int i = 0;
    for (auto& k : knobs) {
        const bool onTab = (i < (int)tabbed.size()) ? false : false;
        (void)onTab;
        ++i;
    }

    // Lay out only the knobs that belong to the current tab.
    int slot = 0;
    for (size_t n = 0; n < knobs.size(); ++n) {
        auto& k = knobs[n];
        // find its tab
        int tab = -1;
        for (auto& [comp, t] : tabbed) if (comp == k.knob.get()) { tab = t; break; }
        if (tab != currentTab) continue;
        const int col = slot % perRow, row = slot / perRow;
        const int x = bodyArea.getX() + col * (bodyArea.getWidth() / perRow);
        const int y = bodyArea.getY() + row * rowH;
        k.knob->setBounds(x + 4, y, knobW, rowH - 8);
        ++slot;
    }

    // Choices and toggles flow underneath the knobs, full width.
    int extraY = bodyArea.getY() + ((slot + perRow - 1) / perRow) * rowH + 6;
    int segIdx = 0, togIdx = 0;
    for (auto& [comp, tab] : tabbed) {
        if (comp == &pad) continue;
        if (tab != currentTab) continue;
        const bool isSeg = dynamic_cast<Segmented*>(comp) != nullptr;
        const bool isTog = dynamic_cast<Toggle*>(comp) != nullptr;
        if (!isSeg && !isTog) continue;
        const int w = isSeg ? juce::jmin(200, bodyArea.getWidth() / 3) : 90;
        const int col = isSeg ? segIdx++ : (togIdx++ + 3);
        comp->setBounds(bodyArea.getX() + col * (w + 10), extraY, w, 26);
    }

    bypassBtn.setBounds(headerArea.getRight() - 96, headerArea.getY() + 16, 84, 26);
    advBtn.setBounds(headerArea.getRight() - 196, headerArea.getY() + 16, 92, 26);
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------
void TalkBoxReferenceEditor::paint(juce::Graphics& g)
{
    g.fillAll(colour::ground);
    drawHeader(g, headerArea);
    drawTabBar(g, tabBarArea);

    // Section frame around the control body
    drawSection(g, bodyArea, {});

    // Pad frame
    drawSection(g, padArea.expanded(2), {});

    // Which controls are live, spelled out: this is the bit that makes the
    // panel feel like an instrument rather than a wall of knobs.
    auto footer = getLocalBounds().removeFromBottom(18).reduced(14, 0);
    g.setColour(colour::textFaint);
    g.setFont(labelFont(9.0f));
    const bool mic = proc.engine.p.source == tb::SourceMode::Mic;
    juce::String status;
    status << (mic ? "MIC MODE - vowel tracked live from input"
                   : "PAD MODE - drag the pad to articulate")
           << "   |   shift-drag a knob for fine control, double-click to reset";
    g.drawText(status, footer, juce::Justification::centredLeft, false);

    // Signal present indicator
    const float rms = proc.visRms.load();
    auto led = footer.removeFromRight(90).toFloat();
    g.setColour(colour::textFaint);
    g.drawText("SIGNAL", led.removeFromLeft(42).toNearestInt(),
               juce::Justification::centredLeft, false);
    auto bar = led.reduced(2.0f, 5.0f);
    g.setColour(colour::panelRaised);
    g.fillRoundedRectangle(bar, 2.0f);
    if (rms > 0.0005f) {
        const float lvl = juce::jlimit(0.0f, 1.0f, std::sqrt(rms) * 2.2f);
        g.setColour(colour::meter);
        g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * lvl), 2.0f);
    }
}

void TalkBoxReferenceEditor::drawHeader(juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour(colour::panel);
    g.fillRect(r);
    g.setColour(colour::border);
    g.drawHorizontalLine(r.getBottom() - 1, (float)r.getX(), (float)r.getRight());

    auto t = r.reduced(16, 0);
    // Wordmark: "TalkBox" in cream, "Reference" in amber, like the reference.
    const juce::Font titleF = titleFont(23.0f);
    g.setFont(titleF);
    const juce::String a("TalkBox "), b("Reference");
    const int wa = juce::jmax(60, titleF.getStringWidth(a));
    g.setColour(colour::text);
    g.drawText(a, t.removeFromLeft(wa + 4), juce::Justification::centredLeft, false);
    g.setColour(colour::accent);
    g.drawText(b, t.removeFromLeft(150), juce::Justification::centredLeft, false);

    g.setColour(colour::textFaint);
    g.setFont(labelFont(9.0f));
    g.drawText("v1.2.0  -  GPLv3  -  formant-accurate vocal tract",
               r.reduced(16, 0).removeFromRight(260).removeFromBottom(20),
               juce::Justification::centredRight, false);
}

void TalkBoxReferenceEditor::drawTabBar(juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour(colour::ground);
    g.fillRect(r);
    static const char* names[5] = { "SIMPLE", "VOCAL TRACT", "MIC & CARRIER",
                                    "HARDWARE CHAIN", "SYSTEM" };
    auto t = r.reduced(16, 0);
    for (int i = 0; i < visibleTabCount; ++i) {
        const int w = 118;
        auto seg = t.removeFromLeft(w);
        const bool on = (i == currentTab);
        if (on) {
            g.setColour(colour::accent);
            g.fillRect(seg.removeFromBottom(2).toFloat());
        }
        g.setColour(on ? colour::text : colour::textFaint);
        g.setFont(labelFont(10.0f));
        g.drawText(names[i], seg.removeFromBottom(30), juce::Justification::centredLeft, false);
    }
}

void TalkBoxReferenceEditor::timerCallback()
{
    if (currentTab == 0 || currentTab == 1) pad.repaint();
    // Signal meter lives in the footer, so it needs a repaint while audio runs.
    repaint(getLocalBounds().removeFromBottom(20));
}

// ---------------------------------------------------------------------------
// Straightforward mouse handling for the tab bar
// ---------------------------------------------------------------------------
namespace {
struct TabCatcher : juce::Component {};
}

void TalkBoxReferenceEditor::mouseDown(const juce::MouseEvent& e)
{
    if (!tabBarArea.contains(e.getPosition())) return;
    const int idx = juce::jlimit(0, visibleTabCount - 1, (e.getPosition().x - 16) / 118);
    switchTab(idx);
}

void TalkBoxReferenceEditor::mouseWheelMove(const juce::MouseEvent&,
                                            const juce::MouseWheelDetails&) {}
