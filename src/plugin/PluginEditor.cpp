// TalkBox Reference - Editor implementation.
#include "PluginEditor.h"

using namespace tbui;

// ---------------------------------------------------------------------------
// Parameter lookup helpers
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
    jassert(p != nullptr);
    auto k = std::make_unique<Knob>(caption, *p, suffix, decimals, bipolar);
    auto& ref = *k;
    knobs.push_back(std::move(k));
    knobTabs.push_back(tab);
    addAndMakeVisible(ref);
    return ref;
}

tbui::MacroKnob& TalkBoxReferenceEditor::addMacro(const juce::String& caption,
                                                  const juce::String& suffix, int decimals,
                                                  std::vector<MacroKnob::Target> targets, int tab)
{
    auto m = std::make_unique<MacroKnob>(caption, suffix, decimals, std::move(targets));
    auto& ref = *m;
    macros.push_back(std::move(m));
    macroTabs.push_back(tab);
    addAndMakeVisible(ref);
    return ref;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
TalkBoxReferenceEditor::TalkBoxReferenceEditor(TalkBoxReferenceProcessor& p)
    : AudioProcessorEditor(p), proc(p), pad(p)
{
    setSize(1000, 720);
    setResizable(true, true);
    setResizeLimits(920, 640, 1600, 1100);

    addAndMakeVisible(pad);
    buildControls();

    // ---- Header buttons ---------------------------------------------------
    auto styleTextButton = [](juce::TextButton& b, juce::Colour on) {
        b.setColour(juce::TextButton::buttonOnColourId, on);
        b.setColour(juce::TextButton::buttonColourId, colour::panelRaised);
        b.setColour(juce::TextButton::textColourOffId, colour::textDim);
        b.setColour(juce::TextButton::textColourOnId, colour::text);
    };

    styleTextButton(bypassBtn, colour::warn);
    bypassBtn.setClickingTogglesState(true);
    bypassBtn.onClick = [this] {
        if (auto* b = bll("bypass"))
            b->setValueNotifyingHost(bypassBtn.getToggleState() ? 1.0f : 0.0f);
    };
    addAndMakeVisible(bypassBtn);

    styleTextButton(advBtn, colour::accentDim);
    advBtn.setClickingTogglesState(true);
    advBtn.onClick = [this] {
        visibleTabCount = advBtn.getToggleState() ? 5 : 2;
        if (currentTab >= visibleTabCount) switchTab(0);
        repaint();
    };
    addAndMakeVisible(advBtn);

    // ---- Dice: generates a whole new patch --------------------------------
    styleTextButton(diceBtn, colour::accent);
    diceBtn.onClick = [this] {
        tb::Dice::roll(proc.apvts);
        ++diceRoll;
        presetName.setText("random " + juce::String(diceRoll), juce::dontSendNotification);
    };
    addAndMakeVisible(diceBtn);

    // ---- Preset stepper: one default, then the dice ----------------------
    presetName.setText("init", juce::dontSendNotification);
    presetName.setJustificationType(juce::Justification::centred);
    presetName.setColour(juce::Label::textColourId, colour::text);
    presetName.setFont(labelFont(12.0f));
    addAndMakeVisible(presetName);

    for (auto* b : { &presetPrev, &presetNext }) {
        styleTextButton(*b, colour::accentDim);
        addAndMakeVisible(*b);
    }
    presetPrev.onClick = [this] {
        // Step back to the neutral init patch rather than pretending the old
        // static presets existed; they never loaded and all sounded the same.
        tb::Dice::roll(proc.apvts);
        presetName.setText("random " + juce::String(++diceRoll), juce::dontSendNotification);
    };
    presetNext.onClick = [this] {
        tb::Dice::roll(proc.apvts);
        presetName.setText("random " + juce::String(++diceRoll), juce::dontSendNotification);
    };

    switchTab(0);
    startTimerHz(20);
}

TalkBoxReferenceEditor::~TalkBoxReferenceEditor() = default;

// ---------------------------------------------------------------------------
// Controls
// ---------------------------------------------------------------------------
void TalkBoxReferenceEditor::buildControls()
{
    // =======================================================================
    // The six macros. Each replaces a group of single-parameter knobs.
    // =======================================================================
    auto T = [this](const char* id, float w, bool bip = false) {
        MacroKnob::Target t;
        t.param = proc.apvts.getParameter(id);
        t.weight = w; t.bipolar = bip;
        return t;
    };

    // AIR - the breathy/nasal quality and how much top end the horn lets out.
    // The vowel itself lives on the pad; a macro that swept X and Y together
    // would duplicate it without ever selecting a vowel.
    addMacro("Air", "", 0, { T("nasal", 1.0f), T("hornCut", 0.8f),
                             T("proxBoost", 0.5f) }, 0);

    // OPEN - how far the mouth is open: aperture, jaw follow it.
    addMacro("Open", "", 0, { T("mouthOpening", 1.0f), T("jaw", 0.6f) }, 0);

    // BITE - brightness and focus: rounding down while size rises.
    addMacro("Bite", "", 0, { T("lipRounding", 1.0f), T("mouthSize", 0.5f) }, 0);

    // DRIVE - carrier into the driver, with the tube colour riding along.
    addMacro("Drive", "", 0, { T("carrierDrive", 1.0f), T("tubeCharacter", 0.7f),
                               T("driverComp", 0.5f) }, 0);

    // MOTION - articulation inertia and vowel glide together.
    addMacro("Motion", "", 0, { T("glide", 1.0f), T("articSmooth", 1.0f) }, 0);

    // SPACE - the tube: hose, ambience and horn damping.
    addMacro("Space", "", 0, { T("hoseLength", 1.0f), T("ambience", 1.0f),
                               T("hoseDamping", 0.6f) }, 0);

    // Global, always visible, not part of the macro block.
    addKnob("outputGain", "Output", " dB", 1, false, 0);

    // =======================================================================
    // ADVANCED tabs: every underlying parameter, individually.
    // =======================================================================
    // Vocal tract
    addKnob("padX", "Tongue Fwd", " %", 0, false, 1);
    addKnob("padY", "Tongue Up",  " %", 0, false, 1);
    addKnob("jaw", "Jaw", " %", 0, false, 1);
    addKnob("tongueX", "Body Fwd", " %", 0, false, 1);
    addKnob("tongueHeight", "Body Up", " %", 0, false, 1);
    addKnob("lipRounding", "Rounding", " %", 0, false, 1);
    addKnob("mouthOpening", "Aperture", " %", 0, false, 1);
    addKnob("nasal", "Nasal", " %", 0, false, 1);
    addKnob("vtl", "VTL", " x", 2, false, 1);
    addKnob("glide", "Glide", " %", 0, false, 1);
    addKnob("articSmooth", "Artic", " %", 0, false, 1);

    // Mic & carrier
    addKnob("micPreGain", "Mic Gain", " dB", 1, false, 2);
    addKnob("gateThresh", "Gate", " dB", 0, false, 2);
    addKnob("whisperBoost", "Whisper", " x", 2, false, 2);
    addKnob("sibilDeess", "De-ess", " %", 0, false, 2);
    addKnob("voiceBleed", "Bleed", " %", 0, false, 2);
    addKnob("micSmooth", "Smooth", " %", 0, false, 2);
    addKnob("inputGain", "Carrier", " dB", 1, false, 2);
    addKnob("arpRate", "Arp", " /s", 1, false, 2);
    addKnob("synthOctave", "Octave", "", 0, true, 2);
    addKnob("synthGlide", "Porta", " %", 0, false, 2);
    addKnob("synthVibrato", "Vibrato", " st", 1, false, 2);

    // Hardware chain
    addKnob("driverComp", "Driver", " %", 0, false, 3);
    addKnob("diaphragmAsym", "Asym", " %", 0, true, 3);
    addKnob("hornCut", "Horn Cut", " Hz", 0, false, 3);
    addKnob("hoseLength", "Hose", " m", 2, false, 3);
    addKnob("hoseDamping", "Damping", " %", 0, false, 3);
    addKnob("micDistance", "Mic Dist", " cm", 0, false, 3);
    addKnob("proxBoost", "Prox", " %", 0, false, 3);
    addKnob("ambience", "Ambience", " %", 0, false, 3);
    addKnob("mouthSize", "Mouth", " x", 2, false, 3);
    addKnob("tubeCharacter", "Tube", " %", 0, false, 3);
    addKnob("carrierDrive", "Drive", " %", 0, false, 3);

    // System
    addKnob("dryWet", "Mix", " %", 0, false, 4);
    addKnob("micProximity", "Proximity", " %", 0, false, 4);
    addKnob("consonantCrisp", "Crisp", " %", 0, false, 4);

    // Choice parameters
    auto addSeg = [&](const char* id, int tab) {
        if (auto* c = choice(id)) {
            auto s = std::make_unique<Segmented>(*c);
            auto* raw = s.get();
            segments.push_back(std::move(s));
            addAndMakeVisible(*raw);
            tabbed.push_back({ raw, tab });
        }
    };
    addSeg("sourceMode", 0);
    addSeg("carrierMode", 0);
    addSeg("quality", 0);
    addSeg("gateBehavior", 2);
    addSeg("micRouting", 2);
    addSeg("synthWave", 2);
    addSeg("micModel", 3);

    if (auto* b = bll("bypass")) {
        auto t = std::make_unique<Toggle>("Bypass", *b);
        auto* raw = t.get();
        toggles.push_back(std::move(t));
        addAndMakeVisible(*raw);
        tabbed.push_back({ raw, 4 });
    }
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
void TalkBoxReferenceEditor::switchTab(int newTab)
{
    currentTab = juce::jlimit(0, 4, newTab);
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i]->setVisible(knobTabs[i] == currentTab && (currentTab != 0 || i == 0));
    for (size_t i = 0; i < macros.size(); ++i)
        macros[i]->setVisible(macroTabs[i] == currentTab && currentTab == 0);
    for (auto& [comp, tab] : tabbed)
        comp->setVisible(tab == currentTab);
    resized();
    repaint();
}

void TalkBoxReferenceEditor::resized()
{
    auto area = getLocalBounds();
    headerArea = area.removeFromTop(66);
    tabBarArea = area.removeFromTop(32);
    auto content = area.reduced(16, 10);

    const bool simple = (currentTab == 0);

    if (simple) {
        // Simple: pad on the left, macro block on the right. Deliberately
        // asymmetric so it does not read like a spreadsheet.
        const int padW = juce::jmax(300, content.getWidth() * 38 / 100);
        auto left = content.removeFromLeft(padW);
        padArea = left;
        pad.setBounds(padArea);
        content.removeFromLeft(16);
        macroArea = content;

        // 6 macros in two rows of three, with the output knob beside them.
        // Reserve a strip at the bottom for the three choice selectors, or the
        // second macro row pushes them off the panel.
        const int selectorStrip = 34;
        const int cols = 3;
        const int cellW = macroArea.getWidth() / (cols + 1);
        // Cap the cell height: at large window sizes an uncapped cell stretches
        // the dials into vast empty circles, which looks broken rather than
        // generous. 210 px is the point where a macro ring still reads as a
        // control and the block stays balanced.
        const int cellH = juce::jlimit(96, 210, (macroArea.getHeight() - selectorStrip) / 2 - 6);
        for (int i = 0; i < (int)macros.size(); ++i) {
            const int c = i % cols, r = i / cols;
            macros[(size_t)i]->setBounds(macroArea.getX() + c * cellW + 6,
                                         macroArea.getY() + r * cellH + 6,
                                         cellW - 12, cellH - 12);
        }
        if (!knobs.empty())
            knobs[0]->setBounds(macroArea.getRight() - cellW + 10,
                                macroArea.getY() + 6, cellW - 20, cellH - 12);

        // Choices along the bottom of the macro block, clamped inside the panel.
        // Push the selector strip to the bottom of the panel so a tall window
        // does not leave a gap under the macros.
        const int y = juce::jmin(juce::jmax(macroArea.getY() + 2 * cellH + 4,
                                            macroArea.getBottom() - 30),
                                 macroArea.getBottom() - 28);
        int x = macroArea.getX();
        for (auto& [comp, tab] : tabbed) {
            if (tab != 0) continue;
            const int w = juce::jmax(120, juce::jmin(220, macroArea.getWidth() / 3 - 10));
            comp->setBounds(x, y, w, 26);
            x += w + 6;
        }
    } else {
        // Advanced: a narrow pad for reference, then a flowing control area.
        auto left = content.removeFromLeft(juce::jmax(200, content.getWidth() * 24 / 100));
        padArea = left.removeFromTop(juce::jmin(left.getHeight(), 260));
        pad.setBounds(padArea);
        content.removeFromLeft(14);
        macroArea = content;

        int knobsOnTab = 0, extrasOnTab = 0;
        for (size_t i = 0; i < knobs.size(); ++i) if (knobTabs[i] == currentTab) ++knobsOnTab;
        for (auto& [comp, tab] : tabbed)
            if (tab == currentTab &&
                (dynamic_cast<Segmented*>(comp) || dynamic_cast<Toggle*>(comp))) ++extrasOnTab;

        const int extraH = extrasOnTab > 0 ? 34 : 0;
        const int usableH = juce::jmax(80, macroArea.getHeight() - extraH);
        const int colsByWidth = juce::jmax(1, macroArea.getWidth() / 96);
        int perRow = juce::jmin(colsByWidth, juce::jmax(1, knobsOnTab));
        int rowsNeeded = (knobsOnTab + perRow - 1) / juce::jmax(1, perRow);
        while (rowsNeeded * 96 > usableH && perRow < colsByWidth) {
            ++perRow;
            rowsNeeded = (knobsOnTab + perRow - 1) / juce::jmax(1, perRow);
        }
        const int rowH = juce::jmax(84, juce::jmin(124, usableH / juce::jmax(1, rowsNeeded)));
        const int knobW = juce::jmax(60, macroArea.getWidth() / juce::jmax(1, perRow) - 8);

        // Centre the grid vertically in whatever space is left, rather than
        // pinning it to the top and leaving a hole at the bottom on a tall
        // window.
        const int gridRows = juce::jmax(1, (knobsOnTab + perRow - 1) / perRow);
        const int gridTop = macroArea.getY() + juce::jmax(0, (usableH - gridRows * rowH) / 2);
        int slot = 0;
        for (size_t i = 0; i < knobs.size(); ++i) {
            if (knobTabs[i] != currentTab) continue;
            const int c = slot % perRow, r = slot / perRow;
            knobs[i]->setBounds(macroArea.getX() + c * (macroArea.getWidth() / perRow) + 4,
                                gridTop + r * rowH, knobW, rowH - 8);
            ++slot;
        }

        const int usedRows = gridRows;
        const int extraY = juce::jmin(macroArea.getY() + usedRows * rowH + 4,
                                      macroArea.getBottom() - 30);
        int segIdx = 0;
        for (auto& [comp, tab] : tabbed) {
            if (tab != currentTab) continue;
            const bool isSeg = dynamic_cast<Segmented*>(comp) != nullptr;
            if (!isSeg) continue;
            const int w = juce::jmin(200, macroArea.getWidth() / 3);
            comp->setBounds(macroArea.getX() + segIdx * (w + 10), extraY, w, 26);
            ++segIdx;
        }
    }

    // ---- Header ----------------------------------------------------------
    auto h = headerArea.reduced(16, 0);
    bypassBtn.setBounds(h.removeFromRight(86).withSizeKeepingCentre(86, 26));
    h.removeFromRight(8);
    advBtn.setBounds(h.removeFromRight(96).withSizeKeepingCentre(96, 26));
    h.removeFromRight(8);
    diceBtn.setBounds(h.removeFromRight(64).withSizeKeepingCentre(64, 26));
    h.removeFromRight(10);
    presetNext.setBounds(h.removeFromRight(24).withSizeKeepingCentre(24, 26));
    presetName.setBounds(h.removeFromRight(120).withSizeKeepingCentre(120, 26));
    presetPrev.setBounds(h.removeFromRight(24).withSizeKeepingCentre(24, 26));
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------
void TalkBoxReferenceEditor::paint(juce::Graphics& g)
{
    g.fillAll(colour::ground);
    drawHeader(g, headerArea);
    drawTabBar(g, tabBarArea);

    // Panels are drawn as sections, not as a uniform grid: the pad and the
    // macro block get their own frames with breathing room between them.
    if (currentTab == 0) {
        drawSection(g, padArea.expanded(3), "vowel space");
        drawSection(g, macroArea.expanded(3), "shape");
    } else {
        if (padArea.getHeight() > 40) drawSection(g, padArea.expanded(3), "vowel");
        drawSection(g, macroArea.expanded(3), {});
    }

    // If the host is not feeding audio and no MIDI has ever arrived, say so.
    // This is the single most common "the plugin is broken" report: an effect
    // insert with Internal Synth selected and nothing playing the notes.
    const bool internalSynth = proc.engine.p.carrier == tb::CarrierMode::Internal;
    const bool noMidiYet = !proc.visGotMidi.load();
    const bool silent = proc.visRms.load() < 0.0002f;
    if (internalSynth && noMidiYet && silent) {
        auto banner = getLocalBounds().removeFromBottom(46).reduced(16, 4);
        g.setColour(colour::warn.withAlpha(0.14f));
        g.fillRoundedRectangle(banner.toFloat(), 5.0f);
        g.setColour(colour::warn);
        g.drawRoundedRectangle(banner.toFloat().reduced(0.5f), 5.0f, 1.0f);
        g.setFont(labelFont(10.5f));
        g.drawText("INTERNAL SYNTH IS SELECTED BUT NO MIDI IS ARRIVING. "
                   "Switch the carrier to EXTERNAL AUDIO to use the audio on this track, "
                   "or route a MIDI clip with a held chord into this plugin.",
                   banner.reduced(10, 0), juce::Justification::centredLeft, true);
    }

    auto footer = getLocalBounds().removeFromBottom(18).reduced(16, 0);
    g.setColour(colour::textFaint);
    g.setFont(labelFont(9.0f));
    const bool mic = proc.engine.p.source == tb::SourceMode::Mic;
    juce::String status;
    status << (mic ? "MIC MODE - vowel tracked from the mic input"
                   : "PAD MODE - drag the vowel pad, or hold a chord")
           << "   |   shift-drag for fine control, double-click resets, DICE for a new patch";
    g.drawText(status, footer, juce::Justification::centredLeft, false);

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
    g.drawText("v1.3.0  -  GPLv3  -  formant-accurate vocal tract",
               r.reduced(16, 0).removeFromRight(280).removeFromBottom(18),
               juce::Justification::centredRight, false);
}

void TalkBoxReferenceEditor::drawTabBar(juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour(colour::ground);
    g.fillRect(r);
    static const char* names[5] = { "PLAY", "VOCAL TRACT", "MIC & CARRIER",
                                    "HARDWARE CHAIN", "SYSTEM" };
    auto t = r.reduced(16, 0);
    for (int i = 0; i < visibleTabCount; ++i) {
        const int w = 122;
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
    if (auto* b = bll("bypass")) {
        const bool on = b->get();   // AudioParameterBool::get() is the public accessor
        if (bypassBtn.getToggleState() != on) bypassBtn.setToggleState(on, juce::dontSendNotification);
    }
    if (currentTab == 0 || currentTab == 1) pad.repaint();
    repaint(getLocalBounds().removeFromBottom(20));
}

void TalkBoxReferenceEditor::mouseDown(const juce::MouseEvent& e)
{
    if (!tabBarArea.contains(e.getPosition())) return;
    const int idx = juce::jlimit(0, visibleTabCount - 1, (e.getPosition().x - 16) / 122);
    switchTab(idx);
}

void TalkBoxReferenceEditor::mouseWheelMove(const juce::MouseEvent&,
                                            const juce::MouseWheelDetails&) {}
