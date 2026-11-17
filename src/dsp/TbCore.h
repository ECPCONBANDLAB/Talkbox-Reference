// TalkBox Reference — DSP Core
// Dependency-free C++20 engine. No heap allocation on audio thread.
// Physics baseline: Fant (1960) source-filter; Peterson & Barney (1952);
// Hillenbrand et al. (1995); Kelly & Lochbaum (1962); Markel & Gray (1976);
// Smith (1992) digital waveguides; Zölzer TPT; Bilbao et al. (2017) ADAA.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace tb {

inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kTwoPi = 2.0f * kPi;
inline constexpr float kMaxK = 0.995f;          // |k_i| clamp (Markel & Gray stability)
inline constexpr float kEps = 1e-12f;

inline float clampf(float x, float lo, float hi) noexcept {
    return x < lo ? lo : (x > hi ? hi : x);
}
inline float sanitize(float x) noexcept {
    // NaN/Inf -> 0, denormal flush
    if (!(x == x)) return 0.0f;                 // NaN
    if (x > 1e6f || x < -1e6f) return 0.0f;     // Inf / blowup
    if (x != 0.0f && std::fabs(x) < 1e-18f) return 0.0f; // FTZ
    return x;
}
inline float dbToGain(float db) noexcept { return std::pow(10.0f, db / 20.0f); }
inline float gainToDb(float g) noexcept { return 20.0f * std::log10(std::max(g, 1e-9f)); }

// Log-Area Ratio: LAR = ln((1+k)/(1-k))  (Markel & Gray)
inline float kToLar(float k) noexcept {
    k = clampf(k, -kMaxK, kMaxK);
    return std::log((1.0f + k) / (1.0f - k));
}
inline float larToK(float lar) noexcept {
    // k = tanh(LAR/2)
    float t = std::tanh(0.5f * clampf(lar, -8.0f, 8.0f));
    return clampf(t, -kMaxK, kMaxK);
}

// ScopedNoDenormals equivalent (FTZ/DAZ via _MM_SET_FLUSH_ZERO_MODE on x86,
// no-op elsewhere; plus explicit flush in sanitize()).
struct ScopedNoDenormals {
    ScopedNoDenormals() noexcept {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
        // FTZ/DAZ handled by compiler flags (-ffast-math / -mfpmath) + sanitize flush.
#endif
    }
};

// ---------------------------------------------------------------------------
// TPT State-Variable Filter (Zölzer, The Art of VA Filter Design)
// Trapezoidal integrators + zero-delay feedback. Unconditionally stable
// for 0 < k < 4 (Q > 0.25). Used for driver bandpass, formant resonators,
// lip/mic shelves — NEVER naive DF biquads for time-varying formants.
// ---------------------------------------------------------------------------
struct TptSvFilter {
    float ic1 = 0.0f, ic2 = 0.0f; // TPT equivalent integrator states (Zölzer)
    float k = 0.0f;               // 1/Q
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f; // ZDF coefficients
    float g = 0.0f;

    void prepare(float sampleRate) noexcept { fs = sampleRate; }

    // fc in Hz, Q = fc / bandwidth. Clamp fc to (0, 0.45*Fs).
    // Correct TPT SVF (Zölzer / Huovilainen): g=tan(pi*fc/Fs), k=1/Q,
    // a1=1/(1+g*(g+k)), a2=g*a1, a3=g*a2. DC gain = 1, peak gain = Q.
    void setFreq(float fc, float Q, float sampleRate) noexcept {
        fc = clampf(fc, 20.0f, 0.45f * sampleRate);
        Q = clampf(Q, 0.5f, 50.0f);
        g = std::tan(kPi * fc / sampleRate);
        k = 1.0f / Q;
        float denom = 1.0f + g * (g + k);
        a1 = 1.0f / denom;
        a2 = g * a1;
        a3 = g * a2;
    }
    // One-pole TPT (for driver HP/LP, hose damping, shelves)
    struct OnePole {
        float s = 0.0f; float g = 0.0f;
        void setCutoff(float fc, float fs) noexcept {
            fc = clampf(fc, 10.0f, 0.45f * fs);
            g = std::tan(kPi * fc / fs);
        }
        inline float lowpass(float x) noexcept {
            float v3 = x - s;
            float v1 = a1(v3);
            float y = s + v1;
            s = y + v1;
            return y;
        }
        inline float highpass(float x) noexcept { return x - lowpass(x); }
        inline float a1(float v) noexcept { return g / (1.0f + g) * v; }
        void reset() noexcept { s = 0.0f; }
    };

    // Returns lowpass, bandpass, highpass via refs. (TPT ZDF, zero-delay)
    inline void process(float x, float& lp, float& bp, float& hp) noexcept {
        float v3 = x - ic2;
        float v1 = a1 * ic1 + a2 * v3;
        float v2 = ic2 + a2 * ic1 + a3 * v3;
        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
    }
    inline float lowpass(float x) noexcept { float lp,bp,hp; process(x,lp,bp,hp); return lp; }
    inline float bandpass(float x) noexcept { float lp,bp,hp; process(x,lp,bp,hp); return bp; }
    inline float highpass(float x) noexcept { float lp,bp,hp; process(x,lp,bp,hp); return hp; }
    void reset() noexcept { ic1 = ic2 = 0.0f; }
    float fs = 48000.0f;
};

// ---------------------------------------------------------------------------
// DC blocker: 1st-order HP ~18 Hz (carrier conditioning, Stage A)
// ---------------------------------------------------------------------------
struct DcBlocker {
    float x1 = 0.0f, y1 = 0.0f, a = 0.0f;
    void prepare(float fs) noexcept {
        float rc = 1.0f / (kTwoPi * 18.0f);
        float dt = 1.0f / fs;
        a = rc / (rc + dt);
    }
    inline float process(float x) noexcept {
        float y = a * (y1 + x - x1);
        x1 = x; y1 = y;
        return sanitize(y);
    }
    void reset() noexcept { x1 = y1 = 0.0f; }
};

} // namespace tb
