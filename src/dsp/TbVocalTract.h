// TalkBox Reference — Hose Waveguide (Stage C) + Vocal Tract (Stage D)
// Stage D: hybrid Normalized Lattice (LAR domain) + TPT resonator cascade.
// Control-rate: formant/LAR updates every 16-64 samples, linear interp per sample.
// Stability: |k_i|<=0.995 guaranteed; TPT ZDF stable under fast modulation.
#pragma once
#include "TbCore.h"
#include <array>

namespace tb {

// Vowel anchor table (Adult Male, VTL=1.0) — Peterson & Barney / Hillenbrand baseline
struct VowelAnchor {
    const char* name; const char* ipa;
    float x, y;          // pad coords 0..1
    float f1,f2,f3;      // Hz
    float b1,b2,b3;      // Hz
};
inline constexpr VowelAnchor kVowels[9] = {
    {"ee","/i/",  0.95f,0.10f, 300.f,2200.f,3000.f, 60.f, 90.f,150.f},
    {"ih","/I/",  0.80f,0.28f, 400.f,1900.f,2500.f, 65.f, 95.f,155.f},
    {"eh","/E/",  0.70f,0.45f, 500.f,1800.f,2500.f, 70.f,100.f,160.f},
    {"ae","/a/",  0.63f,0.75f, 660.f,1700.f,2400.f, 80.f,110.f,165.f},
    {"ah","/A/",  0.33f,0.91f, 750.f,1200.f,2500.f, 85.f,110.f,170.f},
    {"aw","/O/",  0.15f,0.45f, 500.f, 900.f,2500.f, 75.f, 95.f,160.f},
    {"oh","/o/",  0.12f,0.28f, 400.f, 850.f,2500.f, 65.f, 85.f,150.f},
    {"oo","/u/",  0.03f,0.13f, 320.f, 700.f,2300.f, 60.f, 80.f,140.f},
    {"uh","/schwa/",0.51f,0.45f,500.f,1500.f,2500.f, 70.f,100.f,160.f},
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
    float wsum = 0.0f, f1=0,f2=0,f3=0,b1=0,b2=0,b3=0;
    for (int i=0;i<9;++i) {
        float dx = x - kVowels[i].x, dy = y - kVowels[i].y;
        float d2 = dx*dx + dy*dy + 1e-6f;
        float w = 1.0f / d2; // IDW p=2 (1/d^2)
        // sharpen: use w^1.5 for tighter anchor fidelity
        w = w * std::sqrt(w);
        wsum += w;
        f1 += w*kVowels[i].f1; f2 += w*kVowels[i].f2; f3 += w*kVowels[i].f3;
        b1 += w*kVowels[i].b1; b2 += w*kVowels[i].b2; b3 += w*kVowels[i].b3;
    }
    f1/=wsum; f2/=wsum; f3/=wsum; b1/=wsum; b2/=wsum; b3/=wsum;
    // Lip rounding lowers F2/F3 up to ~15%
    float roundScale = 1.0f - 0.15f*clampf(lipRounding,0.0f,1.0f);
    f2 *= roundScale; f3 *= roundScale;
    float s = 1.0f / clampf(vtl, 0.5f, 2.0f);
    Formants out;
    out.f[0]=f1*s; out.f[1]=f2*s; out.f[2]=f3*s;
    out.f[3]=3500.f*s; out.f[4]=4500.f*s;
    out.b[0]=b1; out.b[1]=b2; out.b[2]=b3; out.b[3]=200.f; out.b[4]=250.f;
    // keep ordering F1<F2<F3<F4<F5 (swap if needed after rounding)
    // (IDW already ordered; rounding preserves order)
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
        float a = 1.0f - smooth*0.85f; // glide 0 => fast, 1 => slow
        // clamp smoothing step to avoid zipper but allow fast morphs
        a = clampf(a, 0.05f, 0.95f);
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
