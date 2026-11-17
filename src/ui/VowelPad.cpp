// TalkBox Reference - vowel-space pad implementation.
// The pad is a real F1/F2 plot: x is F2 (inverted, as in phonetics), y is F1
// (inverted, so /i/ sits top-left and /a/ bottom-right like every vowel chart).
#include "VowelPad.h"
#include "../plugin/PluginProcessor.h"
#include "../dsp/TbVocalTract.h"

namespace {
    // Log frequency mapping: vowel space is perceived logarithmically.
    constexpr float kF1Lo = 200.0f, kF1Hi = 950.0f;
    constexpr float kF2Lo = 650.0f, kF2Hi = 2500.0f;
    float normLog(float v, float lo, float hi)
    {
        return juce::jlimit(0.0f, 1.0f,
            (std::log(juce::jmax(v, 1.0f)) - std::log(lo)) / (std::log(hi) - std::log(lo)));
    }
}

juce::Rectangle<float> VowelPad::plotArea() const
{
    return getLocalBounds().toFloat().reduced(10.0f);
}

juce::Point<float> VowelPad::toScreen(float f1, float f2) const
{
    auto a = plotArea();
    const float nx = 1.0f - normLog(f2, kF2Lo, kF2Hi);   // F2 inverted
    const float ny = 1.0f - normLog(f1, kF1Lo, kF1Hi);   // F1 inverted
    return { a.getX() + nx * a.getWidth(), a.getY() + ny * a.getHeight() };
}

void VowelPad::paint(juce::Graphics& g)
{
    auto a = plotArea();

    // Panel + border
    g.setColour(tbui::colour::panelRaised.darker(0.55f));
    g.fillRoundedRectangle(a, 6.0f);

    // Iso-frequency grid at the published formant decades
    g.setColour(tbui::colour::border.withAlpha(0.55f));
    for (float f2 : {700.0f, 1000.0f, 1400.0f, 2000.0f}) {
        auto p = toScreen(kF1Lo, f2);
        g.drawVerticalLine((int)p.x, a.getY(), a.getBottom());
    }
    for (float f1 : {300.0f, 450.0f, 650.0f}) {
        auto p = toScreen(f1, kF2Lo);
        g.drawHorizontalLine((int)p.y, a.getX(), a.getRight());
    }

    // Axis captions
    g.setColour(tbui::colour::textFaint);
    g.setFont(tbui::labelFont(8.5f));
    g.drawText("F2 2500 Hz", a.reduced(4, 2).removeFromTop(10).toNearestInt(),
               juce::Justification::centredLeft, false);
    g.drawText("650 Hz", a.reduced(4, 2).removeFromTop(10).toNearestInt(),
               juce::Justification::centredRight, false);
    g.drawText("F1 200 Hz", a.reduced(4, 0).removeFromBottom(10).toNearestInt(),
               juce::Justification::centredLeft, false);
    g.drawText("950 Hz", a.reduced(4, 0).removeFromBottom(10).toNearestInt(),
               juce::Justification::centredRight, false);

    // Anchors are drawn from the SAME table the audio uses (tb::kVowels).
    // v1.2.0 drew them from a hand-copied second table with different names and
    // different F1/F2 values ("i" at 270/2290 when the model actually produces
    // 300/2240), so every label sat somewhere the sound was not. One table,
    // one truth.
    for (const auto& v : tb::kVowels) {
        auto p = toScreen(v.f1, v.f2);
        g.setColour(tbui::colour::textFaint.withAlpha(0.85f));
        g.fillEllipse(p.x - 1.5f, p.y - 1.5f, 3.0f, 3.0f);
        g.setFont(tbui::labelFont(9.0f));
        // Draw the label clear of the dot, offset away from the nearest edge so
        // it never runs off the plot or sits on top of the marker.
        const auto a = plotArea();
        const float cx = p.x < a.getCentreX() ? p.x + 4.0f : p.x - 28.0f;
        const float cy = p.y < a.getCentreY() ? p.y - 1.0f : p.y - 10.0f;
        g.drawText(v.name, juce::Rectangle<float>(cx, cy, 24.0f, 11.0f),
                   juce::Justification::centredLeft, false);
    }

    // Current articulation. In Mic mode it tracks the live vowel.
    float f1 = proc.visF[0].load(), f2 = proc.visF[1].load();
    if (!(f1 > 0.0f) || !(f2 > 0.0f)) { f1 = 500.0f; f2 = 1500.0f; }
    auto cur = toScreen(f1, f2);

    const bool mic = proc.engine.p.source == tb::SourceMode::Mic;
    const float gate = proc.visGate.load();

    // Trajectory halo: shows recent movement so the pad reads as an instrument
    // being played, not a static marker.
    if (mic && gate > 0.02f) {
        g.setColour(tbui::colour::accent.withAlpha(0.20f * gate));
        g.fillEllipse(cur.x - 18.0f, cur.y - 18.0f, 36.0f, 36.0f);
    }

    // Mouth aperture as the marker's ring radius
    const float mouth = juce::jlimit(0.0f, 1.0f, proc.visMouth.load());
    const float r = 7.0f + mouth * 4.0f;

    g.setColour(tbui::colour::accent.withAlpha(0.18f));
    g.fillEllipse(cur.x - r * 2.2f, cur.y - r * 2.2f, r * 4.4f, r * 4.4f);
    g.setColour(tbui::colour::accentHot);
    g.fillEllipse(cur.x - r, cur.y - r, r * 2.0f, r * 2.0f);
    g.setColour(tbui::colour::ground);
    g.fillEllipse(cur.x - r * 0.42f, cur.y - r * 0.42f, r * 0.84f, r * 0.84f);
    g.setColour(tbui::colour::accent);
    g.drawEllipse(cur.x - r, cur.y - r, r * 2.0f, r * 2.0f, 1.6f);

    // Live formant readout under the pad
    if (showFormants) {
        auto strip = getLocalBounds().toFloat().removeFromBottom(14.0f);
        g.setColour(tbui::colour::textDim);
        g.setFont(tbui::readoutFont(9.5f));
        juce::String txt;
        txt << "F1 " << juce::String((int)f1) << "   F2 " << juce::String((int)f2)
            << "   F3 " << juce::String((int)proc.visF[2].load());
        g.drawText(txt, strip.toNearestInt(), juce::Justification::centred, false);
    }

    g.setColour(tbui::colour::border);
    g.drawRoundedRectangle(a.reduced(0.5f), 6.0f, 1.0f);
}

void VowelPad::mouseDrag(const juce::MouseEvent& e)
{
    auto a = plotArea();
    if (a.getWidth() <= 0.0f || a.getHeight() <= 0.0f) return;

    // Convert the click through the same log mapping the paints uses, then
    // invert the vowel-table IDW numerically so the drag lands on the vowel
    // the user is pointing at (the pad coords are not linear in F1/F2).
    const float nx = juce::jlimit(0.0f, 1.0f, ((float)e.getPosition().x - a.getX()) / a.getWidth());
    const float ny = juce::jlimit(0.0f, 1.0f, ((float)e.getPosition().y - a.getY()) / a.getHeight());
    const float targetF2 = std::exp(std::log(kF2Lo) + (1.0f - nx) * (std::log(kF2Hi) - std::log(kF2Lo)));
    const float targetF1 = std::exp(std::log(kF1Lo) + (1.0f - ny) * (std::log(kF1Hi) - std::log(kF1Lo)));

    // Search the pad space for the closest match. 16x16 is plenty (0.06 % of
    // the pad per step) and costs nothing at drag rate.
    float bestX = 0.5f, bestY = 0.45f, bestD = 1e18f;
    for (int iy = 0; iy <= 16; ++iy) {
        for (int ix = 0; ix <= 16; ++ix) {
            const float px = (float)ix / 16.0f, py = (float)iy / 16.0f;
            const auto f = tb::padToFormants(px, py, 1.0f, 0.0f);
            const float d = std::pow(std::log(juce::jmax(f.f[1], 1.0f) / targetF2), 2.0f)
                          + std::pow(std::log(juce::jmax(f.f[0], 1.0f) / targetF1), 2.0f);
            if (d < bestD) { bestD = d; bestX = px; bestY = py; }
        }
    }

    if (auto* p = proc.apvts.getParameter("padX"))
        p->setValueNotifyingHost(p->convertTo0to1(bestX));
    if (auto* p = proc.apvts.getParameter("padY"))
        p->setValueNotifyingHost(p->convertTo0to1(bestY));

    repaint();
}
