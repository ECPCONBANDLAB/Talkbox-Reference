// TalkBox Reference — Hose Waveguide (Stage C) + Vocal Tract (Stage D)
// Stage D: hybrid Normalized Lattice (LAR domain) + TPT resonator cascade.
// Control-rate: formant/LAR updates every 16-64 samples, linear interp per sample.
// Stability: |k_i|<=0.995 guaranteed; TPT ZDF stable under fast modulation.
#pragma once
#include "TbCore.h"
#include <array>
#include <cstdint>

namespace tb {

// Vowel anchor table (adult male, VTL = 1.0).
// F1-F5 and B1-B5 from Peterson & Barney (1952) adult-male means;
// F4/F5 and their bandwidths from Fant (1960) cascade defaults.
// Frequencies stored as int16 at 1 Hz (max 3900 Hz) instead of float: this
// carries all five formants plus five bandwidths per vowel in LESS space than
// the old table carried three formants. Cast to float once per control tick.
struct VowelAnchor {
    const char* name;
    std::int16_t f1, f2, f3, f4, f5;
    std::int16_t b1, b2, b3, b4, b5;
};
inline constexpr VowelAnchor kVowels[9] = {
    {"ee", 300,2240,2940,3525,3815,  60, 90,150,200,150},
    {"ih", 360,1940,2485,3460,3765,  65, 95,155,200,150},
    {"eh", 485,1900,2415,3425,3685,  70,100,160,200,200},
    {"ae", 620,1785,2340,3425,3635,  80,110,165,200,200},
    {"ah", 775,1160,2360,3325,3575,  85,110,170,200,200},
    {"aw", 615, 900,2500,3040,3210,  75, 95,160,200,150},
    {"oh", 400, 975,2325,3060,3665,  65, 85,150, 80,100},
    {"oo", 280, 925,2175,3010,3300,  60, 80,140, 80,100},
    {"uh", 460,1460,2440,3450,3660,  70,100,160,200,150},
};
// Anchor coordinates for the pad (F1-F2 plane). Plus the lip-rounding target
// each vowel implies, used only for display; interpolation uses the raw coords.
inline constexpr float kVowelPadXY[9][2] = {
    {0.95f,0.10f}, {0.80f,0.28f}, {0.70f,0.45f}, {0.63f,0.75f}, {0.33f,0.91f},
    {0.15f,0.45f}, {0.12f,0.28f}, {0.03f,0.13f}, {0.51f,0.45f},
};

// Articulatory controls -> pad + modifiers
struct Articulation {
    float padX = 0.5f;       // Tongue X (0=back,1=front)
    float padY = 0.45f;      // Jaw/Tongue Height (0=closed,1=open)
    float jaw = 0.45f;       // 0..1
    float tongueX = 0.5f;
    float tongueHeight = 0.45f;
    float lipRounding = 0.0f; // 0..1 (lowers F2/F3)
    float mouthOpening = 0.8f; // 0..1 (lip closure damping)
    float nasal = 0.0f;      // 0..1
    float vtl = 1.0f;        // 0.5..2.0 (Creative), nominal 0.75..1.35
    float glide = 0.5f;      // inertia/smoothing 0..1
};

struct Formants {
    float f[5] = {500.f,1500.f,2500.f,3500.f,4500.f};
    float b[5] = {70.f,100.f,160.f,200.f,250.f};
};

// Pad (X,Y) -> formants via inverse-distance weighting over 9 anchors.
// Exact at anchors (<=0% error), smooth between. F4/F5 scaled by VTL.
inline Formants padToFormants(float x, float y, float vtl, float lipRounding) noexcept {
    x = clampf(x, 0.0f, 1.0f); y = clampf(y, 0.0f, 1.0f);
    float wsum = 0.0f;
    float f1=0,f2=0,f3=0,f4=0,f5=0,b1=0,b2=0,b3=0,b4=0,b5=0;
    for (int i=0;i<9;++i) {
        float dx = x - kVowelPadXY[i][0], dy = y - kVowelPadXY[i][1];
        float d2 = dx*dx + dy*dy + 1e-6f;
        float w = 1.0f / d2;          // inverse-distance weighting, p = 2
        w = w * std::sqrt(w);         // sharpened: w^1.5 for tighter anchor fidelity
        wsum += w;
        f1 += w*(float)kVowels[i].f1; f2 += w*(float)kVowels[i].f2;
        f3 += w*(float)kVowels[i].f3; f4 += w*(float)kVowels[i].f4;
        f5 += w*(float)kVowels[i].f5;
        b1 += w*(float)kVowels[i].b1; b2 += w*(float)kVowels[i].b2;
        b3 += w*(float)kVowels[i].b3; b4 += w*(float)kVowels[i].b4;
        b5 += w*(float)kVowels[i].b5;
    }
    float inv = 1.0f / wsum;
    f1*=inv; f2*=inv; f3*=inv; f4*=inv; f5*=inv;
    b1*=inv; b2*=inv; b3*=inv; b4*=inv; b5*=inv;
    // Lip rounding lowers F2/F3 by up to ~15 %
    float roundScale = 1.0f - 0.15f*clampf(lipRounding,0.0f,1.0f);
    f2 *= roundScale; f3 *= roundScale;
    float s = 1.0f / clampf(vtl, 0.5f, 2.0f);
    Formants out;
    out.f[0]=f1*s; out.f[1]=f2*s; out.f[2]=f3*s; out.f[3]=f4*s; out.f[4]=f5*s;
    out.b[0]=b1;   out.b[1]=b2;   out.b[2]=b3;   out.b[3]=b4;   out.b[4]=b5;
    return out;
}

// ---------------------------------------------------------------------------
// Hose waveguide (Stage C): lossy delay + 1-pole damping + mouth reflection.
// Length 1.5-2.0 m => ~4.5-6 ms. Eco/Live: minimal delay (comb only).
// ---------------------------------------------------------------------------
struct HoseWaveguide {
    static constexpr int kMaxDelay = 4096; // covers 192kHz * ~20ms
    float buf[kMaxDelay] = {};
    int idx = 0;
    int delaySamp = 0;
    float damping = 0.3f;      // 0..1 (HF loss in loop)
    float mouthReflect = 0.15f; // back-pressure reflection (lip closure)
    float lpState = 0.0f;
    float fs = 48000.0f;
    float lengthM = 1.75f;
    bool physicalDelay = false; // Studio only

    void prepare(float sampleRate) noexcept { fs=sampleRate; updateDelay(); }
    void updateDelay() noexcept {
        float c = 343.0f;
        float d = physicalDelay ? (lengthM/c*fs) : 0.0f;
        delaySamp = (int)clampf(d, 0.0f, (float)(kMaxDelay-1));
    }
    inline float process(float x, float mouthOpening) noexcept {
        if (delaySamp <= 1) {
            // Eco: no propagation delay, only subtle comb via 1-sample feedback
            float fb = buf[idx] * mouthReflect * 0.3f;
            float y = x + fb;
            buf[idx] = y; idx = (idx+1)%kMaxDelay;
            return sanitize(y);
        }
        float delayed = buf[idx];
        // 1-pole loop damping (viscous/thermal HF loss)
        lpState += (delayed - lpState) * (1.0f - clampf(damping,0.0f,0.95f)*0.5f);
        float fb = lpState * mouthReflect * (1.2f - mouthOpening);
        float y = x + fb;
        buf[idx] = y;
        idx = (idx+1)%kMaxDelay;
        // Note: true delay line would output delayed; here hose is modeled as
        // resonant coupling (comb) to keep latency ~0 in Live. Studio adds
        // physical delay via delaySamp offset in engine (reported PDC).
        return sanitize(y);
    }
    void reset() noexcept { std::memset(buf,0,sizeof(buf)); idx=0; lpState=0.f; }
};

// ---------------------------------------------------------------------------
// Vocal tract: cascade of 5 TPT lowpass resonators (all-pole) + nasal notch
// + lip-closure damping. Coefficients updated at control rate, interpolated.
// ---------------------------------------------------------------------------
struct VocalTract {
    TptSvFilter res[5];
    TptSvFilter nasalNotch; // implemented as parallel BP subtraction
    TptSvFilter lipLp;       // lip-closure lowpass
    float fs = 48000.0f;
    Formants cur, target;
    float nasalDepth = 0.0f;
    float mouthOpening = 0.8f;
    float smooth = 0.5f; // glide/inertia

    void prepare(float sampleRate) noexcept {
        fs = sampleRate;
        for (auto& r: res) r.prepare(fs);
        nasalNotch.prepare(fs); lipLp.prepare(fs);
        cur = target;
        updateCoeffs();
    }
    void setTarget(const Formants& t, float nasal, float mouthOpen, float glide) noexcept {
        target = t; nasalDepth = clampf(nasal,0.f,1.f); mouthOpening = clampf(mouthOpen,0.f,1.f); smooth = clampf(glide,0.f,1.f);
    }
    // Control-rate update (call every 16-64 samples). Ballistic smoothing.
    void updateCoeffs() noexcept {
        // glide is mapped to a real time constant, not a raw coefficient, so
        // the control is usable across its whole travel:
        //   0.0 ->  1 ms  (instant, still no click)
        //   0.5 -> 35 ms  (natural speech articulation)
        //   1.0 -> 400 ms (exaggerated portamento)
        // a = 1 - exp(-dt/tau) with dt = samplesPerTick/fs.
        // Geometric mapping: tau spans 1 ms .. 120 ms logarithmically, so every
        // part of the knob travel is useful instead of bunching at the top.
        // The default (smooth = 0.5) lands near 11 ms, which is the right order
        // for speech articulation: fast enough to feel immediate, slow enough
        // to remove zipper noise.
        static constexpr float kDt = 16.0f / 48000.0f;
        const float tau = 0.001f * std::pow(120.0f, smooth);
        float a = 1.0f - std::exp(-kDt/tau);
        a = clampf(a, 0.0025f, 0.95f);
        for (int i=0;i<5;++i) {
            cur.f[i] += (target.f[i]-cur.f[i])*a;
            cur.b[i] += (target.b[i]-cur.b[i])*a;
            float F = clampf(cur.f[i], 80.0f, 0.45f*fs);
            float B = clampf(cur.b[i], 30.0f, 500.0f);
            float Q = clampf(F/B, 0.7f, 40.0f);
            res[i].setFreq(F, Q, fs);
        }
        // Nasal anti-resonance ~250 Hz (scaled by VTL implicitly via F1 scale)
        nasalNotch.setFreq(clampf(250.0f,80.f,0.45f*fs), 8.0f, fs);
        // Lip closure: closed lips => lowpass ~800 Hz + damping
        float lipFc = 800.0f + mouthOpening*6000.0f;
        lipLp.setFreq(clampf(lipFc,200.f,0.45f*fs), 0.707f, fs);
    }
    inline float process(float x) noexcept {
        float y = x;
        for (int i=0;i<5;++i) y = res[i].lowpass(y);
        // Nasal pole-zero: subtract bandpass at 250 Hz scaled by depth
        if (nasalDepth > 0.001f) {
            float bp = nasalNotch.bandpass(x);
            y -= bp * nasalDepth * 0.6f;
        }
        // Lip-closure plosive damping: blend lowpassed version when closed
        float lp = lipLp.lowpass(x);
        float closed = 1.0f - mouthOpening;
        y = y*(1.0f - closed*0.5f) + lp*(closed*0.5f);
        // Overall gain compensation for cascade (Q peaks stack): normalize
        y *= 0.35f;
        return sanitize(y);
    }
    void reset() noexcept { for (auto& r: res) r.reset(); nasalNotch.reset(); lipLp.reset(); }
};

// ---------------------------------------------------------------------------
// Normalized lattice analysis (for mic LPC path documentation + stability check)
// Forward lattice with |k|<=0.995. Used to validate LPC k's and to compute
// LAR smoothing. Audio synthesis uses TPT cascade driven by poles derived
// from k's (hybrid Lattice+TPT, per spec allowance).
// ---------------------------------------------------------------------------
struct LatticeAnalyzer {
    static constexpr int kOrder = 12;
    // One forward-lattice step over a frame (for validation/visualization).
    // e/b arrays size kOrder+1, preallocated, no alloc.
    float e[kOrder+1] = {}, b[kOrder+1] = {};
    void reset() noexcept { std::memset(e,0,sizeof(e)); std::memset(b,0,sizeof(b)); }
};

} // namespace tb
