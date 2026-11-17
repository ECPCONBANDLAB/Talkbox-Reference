// TalkBox Reference - UI knob.
// Large circular knob with an arc indicator and the value printed underneath,
// following the Valhalla reference: the number is always visible, so the user
// never has to hover to know where a control sits. Bipolar controls draw their
// arc from the centre. All vector, DPI independent.
#pragma once
#include "TbLookAndFeel.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace tbui {

class Knob : public juce::Component, private juce::Timer {
public:
    // suffix examples: " %", " ms", " Hz", " dB", "" ; decimals for the readout
    Knob(const juce::String& caption, juce::RangedAudioParameter& param,
         const juce::String& suffix = {}, int decimals = 0, bool bipolar = false)
        : cap(caption), p(param), suf(suffix), dec(decimals), isBipolar(bipolar)
    {
        setInterceptsMouseClicks(true, false);
        // Pull the displayed value straight from the parameter at ~20 Hz so
        // automation and preset changes animate without any audio-thread work.
        startTimerHz(20);
        setRepaintsOnMouseActivity(true);
    }

    void setCaption(const juce::String& s) { cap = s; repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float readH = 13.0f;
        const float capH  = 11.0f;
        auto dialArea = b.withTrimmedBottom(readH + capH + 1.0f);
        const float d = juce::jmin(dialArea.getWidth(), dialArea.getHeight());
        auto dial = juce::Rectangle<float>(d, d).withCentre(dialArea.getCentre());

        const float v01 = norm();
        const float start = juce::MathConstants<float>::pi * 1.25f;
        const float end   = juce::MathConstants<float>::pi * 2.75f;
        const float ang   = start + v01 * (end - start);

        const float radius = d * 0.5f - 3.0f;
        const float thick  = juce::jmax(3.0f, d * 0.085f);
        auto ring = dial.reduced(thick * 0.5f + 2.0f);

        // Track
        juce::Path track;
        track.addCentredArc(ring.getCentreX(), ring.getCentreY(), ring.getWidth() * 0.5f,
                            ring.getHeight() * 0.5f, 0.0f, start, end, true);
        g.setColour(colour::panelRaised);
        g.strokePath(track, juce::PathStrokeType(thick, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        // Value arc. Bipolar arcs grow outward from 12 o'clock.
        const float arcFrom = isBipolar ? (start + end) * 0.5f : start;
        if (std::abs(ang - arcFrom) > 0.02f) {
            juce::Path arc;
            arc.addCentredArc(ring.getCentreX(), ring.getCentreY(), ring.getWidth() * 0.5f,
                              ring.getHeight() * 0.5f, 0.0f,
                              juce::jmin(arcFrom, ang), juce::jmax(arcFrom, ang), true);
            g.setColour(isMouseOverOrDragging() ? colour::accentHot : colour::accent);
            g.strokePath(arc, juce::PathStrokeType(thick, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        }

        // Knob body
        auto body = dial.reduced(d * 0.20f);
        g.setGradientFill(juce::ColourGradient(colour::panelRaised.brighter(0.10f),
                                               body.getTopLeft(),
                                               colour::panel.darker(0.25f),
                                               body.getBottomRight(), false));
        g.fillEllipse(body);
        g.setColour(colour::border);
        g.drawEllipse(body, 1.0f);

        // Pointer
        const float pr = body.getWidth() * 0.5f;
        juce::Path ptr;
        const float px = body.getCentreX() + std::sin(ang) * pr * 0.72f;
        const float py = body.getCentreY() - std::cos(ang) * pr * 0.72f;
        const float px2 = body.getCentreX() + std::sin(ang) * pr * 0.30f;
        const float py2 = body.getCentreY() - std::cos(ang) * pr * 0.30f;
        ptr.startNewSubPath(px2, py2);
        ptr.lineTo(px, py);
        g.setColour(colour::text);
        g.strokePath(ptr, juce::PathStrokeType(juce::jmax(1.6f, d * 0.035f),
                                               juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

        // Readout + caption
        auto rb = b.removeFromBottom(readH);
        drawValue(g, rb.toNearestInt(), text(v01), isMouseOverOrDragging() ? colour::text : colour::text);
        drawCaption(g, b.removeFromBottom(capH).toNearestInt(), cap);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        dragStart = norm();
        dragY = e.getPosition().y;
        dragMod = e.mods.isShiftDown();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        // Vertical drag, shift for fine. 180 px of travel spans the range.
        const float sensitivity = (dragMod ? 0.0016f : 0.0060f);
        const float delta = (float)(dragY - e.getPosition().y) * sensitivity;
        setNorm(juce::jlimit(0.0f, 1.0f, dragStart + delta));
    }

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        // Reset to the parameter's default, which is what users expect.
        // AudioParameterFloat::getDefaultValue() is private in JUCE 7, so go
        // through the public AudioProcessorParameter base and normalise by
        // hand using the parameter's own range.
        if (auto* withRange = dynamic_cast<juce::AudioParameterFloat*>(&p)) {
            const float defRaw = p.getDefaultValue();
            const auto& r = withRange->getNormalisableRange();
            setNorm(juce::jlimit(0.0f, 1.0f, r.convertTo0to1(defRaw)));
        }
    }

    void mouseWheelMove(const juce::MouseEvent&,
                        const juce::MouseWheelDetails& w) override
    {
        setNorm(juce::jlimit(0.0f, 1.0f, norm() + w.deltaY * 0.5f));
    }

private:
    void timerCallback() override
    {
        const float v = norm();
        if (std::abs(v - lastShown) > 0.0f) { lastShown = v; repaint(); }
    }

    float norm() const { return p.getValue(); }
    void setNorm(float v)
    {
        p.setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, v));
        lastShown = -1.0f;
        repaint();
    }

    juce::String text(float v01) const
    {
        juce::String s;
        if (auto* f = dynamic_cast<juce::AudioParameterFloat*>(&p))
            s = juce::String(f->getNormalisableRange().convertFrom0to1(v01), dec);
        else if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(&p))
            return c->getCurrentChoiceName().substring(0, 7);
        else if (auto* bpar = dynamic_cast<juce::AudioParameterBool*>(&p))
            return bpar->get() ? "ON" : "OFF";
        else
            s = juce::String(v01, dec);
        return s + suf;
    }

    juce::String cap, suf;
    juce::RangedAudioParameter& p;
    int dec;
    bool isBipolar;
    float dragStart = 0.0f, lastShown = -1.0f;
    int dragY = 0;
    bool dragMod = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)
};

} // namespace tbui
