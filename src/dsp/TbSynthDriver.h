// TalkBox Reference — Carrier Synth (PolyBLEP) + Compression Driver (ADAA)
// Stage A (internal synth) + Stage B (driver/horn nonlinearity)
#pragma once
#include "TbCore.h"

namespace tb {

// ---------------------------------------------------------------------------
// PolyBLEP oscillator: saw / pulse. Alias-suppressed, zero alloc.
// ---------------------------------------------------------------------------
struct PolyBlePOsc {
    float phase = 0.0f;      // 0..1
    float freq = 220.0f;
    float fs = 48000.0f;
    float pulseWidth = 0.5f; // 0.05..0.95
    int waveform = 0;        // 0=saw, 1=pulse

    void prepare(float sampleRate) noexcept { fs = sampleRate; }
    void setFreq(float f) noexcept { freq = clampf(f, 20.0f, 8000.0f); }
    void reset() noexcept { phase = 0.0f; }

    static inline float polyBlep(float t, float dt) noexcept {
        if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }
    inline float process() noexcept {
        float dt = freq / fs;
        float y = 0.0f;
        if (waveform == 0) { // saw: 2*phase-1
            y = 2.0f * phase - 1.0f;
            y -= polyBlep(phase, dt);
        } else { // pulse
            y = (phase < pulseWidth) ? 1.0f : -1.0f;
            y -= polyBlep(phase, dt);
            float pw = pulseWidth;
            float t2 = phase < pw ? phase + 1.0f - pw : phase - pw;
            y += polyBlep(t2, dt);
            y *= 0.5f;
        }
        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;
        return y * 0.7f;
    }
};

// Talkbox carrier synth: mono/poly-lite (4 voices), glide, vibrato, sub.
// Modeled after bright Minimoog/DX-style harmonic carriers (Stage A).
struct TalkboxSynth {
    static constexpr int kMaxVoices = 4;
    struct Voice { PolyBlePOsc osc; float targetFreq=220.f, curFreq=220.f; float level=0.f; bool active=false; };
    Voice voices[kMaxVoices];
    float fs = 48000.0f;
    float glideTime = 0.06f;   // seconds (portamento)
    float vibratoRate = 5.5f, vibratoDepth = 0.0f; // semitones
    float vibratoPhase = 0.0f;
    float subLevel = 0.25f;    // -1 octave sine sub
    float subPhase = 0.0f;
    int waveform = 0;
    float pulseWidth = 0.5f;
    float pitchBend = 0.0f;    // semitones

    void prepare(float sampleRate) noexcept {
        fs = sampleRate;
        for (auto& v : voices) { v.osc.prepare(fs); v.osc.waveform = waveform; }
    }
    void setWaveform(int w) noexcept { waveform = w; for (auto& v: voices) v.osc.waveform=w; }
    void noteOn(float midiNote, float vel=1.0f) noexcept {
        float f = 440.0f * std::pow(2.0f, (midiNote - 69.0f)/12.0f);
        // steal quietest voice
        int idx = 0; float best = 1e9f;
        for (int i=0;i<kMaxVoices;++i) if (!voices[i].active) { idx=i; break; }
        else if (voices[i].level < best) { best=voices[i].level; idx=i; }
        voices[idx].targetFreq = f;
        voices[idx].curFreq = voices[idx].active ? voices[idx].curFreq : f;
        voices[idx].level = clampf(vel, 0.0f, 1.0f);
        voices[idx].active = true;
        voices[idx].osc.setFreq(f);
    }
    void noteOff(float midiNote) noexcept {
        float f = 440.0f * std::pow(2.0f, (midiNote - 69.0f)/12.0f);
        for (auto& v : voices) if (std::fabs(v.targetFreq - f) < 1.0f) v.active=false;
    }
    void allNotesOff() noexcept { for (auto& v: voices) v.active=false; }
    inline float process() noexcept {
        float glideCoeff = (glideTime > 0.001f) ? std::exp(-1.0f/(glideTime*fs)) : 0.0f;
        vibratoPhase += vibratoRate/fs; if (vibratoPhase>=1.0f) vibratoPhase-=1.0f;
        float vib = std::sin(kTwoPi*vibratoPhase) * vibratoDepth; // semitones
        float out = 0.0f; int nActive=0;
        for (auto& v : voices) {
            if (!v.active && v.level < 0.001f) continue;
            float target = v.targetFreq * std::pow(2.0f,(pitchBend+vib)/12.0f);
            v.curFreq = glideCoeff*v.curFreq + (1.0f-glideCoeff)*target;
            v.osc.setFreq(v.curFreq);
            float s = v.osc.process();
            // simple release: fade level when inactive
            if (!v.active) { v.level *= 0.999f; if (v.level<0.001f) continue; }
            out += s * v.level;
            ++nActive;
        }
        if (nActive>0) out /= (float)nActive;
        // sub oscillator (sine, -1 octave of first active voice)
        if (subLevel > 0.001f && nActive>0) {
            float f0 = voices[0].curFreq * 0.5f;
            subPhase += f0/fs; if (subPhase>=1.0f) subPhase-=1.0f;
            out += std::sin(kTwoPi*subPhase) * subLevel * 0.5f;
        }
        return sanitize(out);
    }
    void reset() noexcept { for (auto& v: voices){v.osc.reset(); v.active=false; v.level=0.f;} subPhase=0.f; }
};

// ---------------------------------------------------------------------------
// ADAA driver nonlinearity (Stage B)
// f(x) = cubic soft-clip with asymmetry:
//   f(x) = x + a*x^2 - x^3/3   for |x|<1,  sign(x)*(2/3 + a) outside (clamped)
// Antiderivative F1(x) = x^2/2 + a*x^3/3 - x^4/12  inside; |x|-1/2 + a*sign... outside
// First-order ADAA: y[n] = (F1(x[n])-F1(x[n-1]))/(x[n]-x[n-1]), fallback f(mid).
// Zero-latency (Eco/Live). Balanced/Studio add localized 2x/4x oversampling
// around this block only (see TalkboxEngine).
// ---------------------------------------------------------------------------
struct AdaaDriver {
    float xPrev = 0.0f, f1Prev = 0.0f;
    float drive = 1.0f;      // input gain into clipper
    float asymmetry = 0.0f;  // -0.5..0.5 (diaphragm asymmetry)
    float comp = 0.0f;       // dynamic excursion compression 0..1
    float env = 0.0f;        // compressor envelope

    static inline float f0(float x, float a) noexcept {
        if (x > 1.0f) return 2.0f/3.0f + a;
        if (x < -1.0f) return -2.0f/3.0f + a;
        return x + a*x*x - x*x*x/3.0f;
    }
    static inline float f1(float x, float a) noexcept {
        if (x > 1.0f) return (x - 0.5f) + a*(2.0f/3.0f); // integral of const outside (approx, continuous at 1)
        if (x < -1.0f) return (-x - 0.5f)*(-1.0f) + a*(2.0f/3.0f); // keep continuous-ish; ADAA only needs differences
        // Inside: x^2/2 + a x^3/3 - x^4/12 ; at x=1: 0.5 + a/3 - 1/12 = 5/12 + a/3
        return x*x*0.5f + a*x*x*x/3.0f - x*x*x*x/12.0f;
    }
    // Fix continuity of F1 outside: use F1(1)=5/12+a/3, slope = f(1)=2/3+a
    static inline float f1c(float x, float a) noexcept {
        float f1_1 = 5.0f/12.0f + a/3.0f;
        float f1_m1 = 5.0f/12.0f - a/3.0f; // x^2/2 - a|x|^3/3 ... at -1: 0.5 - a/3 -1/12
        if (x > 1.0f) return f1_1 + (2.0f/3.0f + a)*(x-1.0f);
        if (x < -1.0f) return f1_m1 + (-2.0f/3.0f + a)*(x+1.0f);
        return x*x*0.5f + a*x*x*x/3.0f - x*x*x*x/12.0f;
    }
    inline float process(float x) noexcept {
        // dynamic compression: envelope follower reduces drive on loud signals
        float ax = std::fabs(x);
        env += (ax - env) * (ax > env ? 0.02f : 0.002f);
        float g = drive / (1.0f + comp * env * 2.0f);
        float xd = x * g;
        float y;
        float dx = xd - xPrev;
        if (std::fabs(dx) < 1e-6f) {
            y = f0(0.5f*(xd+xPrev), asymmetry);
        } else {
            y = (f1c(xd, asymmetry) - f1Prev) / dx;
        }
        xPrev = xd; f1Prev = f1c(xd, asymmetry);
        return sanitize(y);
    }
    void reset() noexcept { xPrev=f1Prev=env=0.0f; }
};

// Driver bandpass: HP ~90 Hz + LP ~6 kHz (TPT one-poles). Horn roll-off.
struct DriverBandpass {
    TptSvFilter::OnePole hp, lp;
    float fs = 48000.0f;
    float hornCut = 6500.0f; // 5.5-7.5 kHz
    void prepare(float sampleRate) noexcept { fs=sampleRate; setCoeffs(); }
    void setCoeffs() noexcept { hp.setCutoff(90.0f, fs); lp.setCutoff(hornCut, fs); }
    inline float process(float x) noexcept {
        float y = hp.highpass(x);
        y = lp.lowpass(y);
        return sanitize(y);
    }
    void reset() noexcept { hp.reset(); lp.reset(); }
};

} // namespace tb
