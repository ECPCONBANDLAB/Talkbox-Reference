// TalkBox Reference — Common.h
// Shared primitives: constants, one-poles, biquads, FFT and spectrum helpers.
// C++20, header-only, no dynamic allocation outside construction/setup.
#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace tb {

inline constexpr double kPi = 3.14159265358979323846;
inline constexpr double kTwoPi = 6.28318530717958647692;
inline constexpr double kSpeedOfSound = 343.0;   // m/s at ~20 C
inline constexpr double kEps = 1e-12;

inline double clampd(double v, double lo, double hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
inline double dbToGain(double db) { return std::pow(10.0, db / 20.0); }
inline double gainToDb(double g) { return 20.0 * std::log10(std::max(std::abs(g), 1e-12)); }
inline double lerp(double a, double b, double t) { return a + (b - a) * t; }
inline bool isFinite(double v) { return std::isfinite(v); }
inline double softLimit(double v, double ceiling) {
    return ceiling * std::tanh(v / std::max(ceiling, 1e-9));
}

// ---------------------------------------------------------------------------
// Deterministic RNG (xorshift32) — audio threads never touch std::rand.
// ---------------------------------------------------------------------------
struct Rng {
    uint32_t s = 0x9e3779b9u;
    explicit Rng(uint32_t seed = 0x9e3779b9u) : s(seed ? seed : 1u) {}
    uint32_t next() {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return s;
    }
    double uniform() { return (next() >> 8) * (1.0 / 16777216.0); }  // [0,1)
    double bipolar() { return 2.0 * uniform() - 1.0; }
    void seed(uint32_t v) { s = v ? v : 1u; }
};

// ---------------------------------------------------------------------------
// One-pole building blocks
// ---------------------------------------------------------------------------
struct OnePoleLP {
    double a = 0.0, z = 0.0;
    void setCutoff(double sr, double fc) {
        fc = clampd(fc, 0.1, sr * 0.49);
        a = std::exp(-kTwoPi * fc / sr);
    }
    double process(double x) {
        z = (1.0 - a) * x + a * z;
        if (!std::isfinite(z)) z = 0.0;
        return z;
    }
    void reset() { z = 0.0; }
};

struct OnePoleHP {
    OnePoleLP lp;
    void setCutoff(double sr, double fc) { lp.setCutoff(sr, fc); }
    double process(double x) { return x - lp.process(x); }
    void reset() { lp.reset(); }
};

// DC blocker, first order, 18 Hz by default.
struct DcBlocker {
    double R = 0.9976, x1 = 0.0, y1 = 0.0;
    void setCutoff(double sr, double fc) {
        R = clampd(1.0 - kTwoPi * fc / sr, 0.90, 0.99999);
    }
    double process(double x) {
        const double y = x - x1 + R * y1;
        x1 = x;
        y1 = std::isfinite(y) ? y : 0.0;
        return y1;
    }
    void reset() { x1 = y1 = 0.0; }
};

// ---------------------------------------------------------------------------
// RBJ biquad (transposed direct form II)
// ---------------------------------------------------------------------------
struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void setBypass() { b0 = 1; b1 = b2 = a1 = a2 = 0; z1 = z2 = 0; }

    static double clampFreq(double sr, double fc) { return clampd(fc, 5.0, sr * 0.49); }

    void setLowpass(double sr, double fc, double q = 0.7071067811865476) {
        const double w0 = kTwoPi * clampFreq(sr, fc) / sr;
        const double cw = std::cos(w0), sw = std::sin(w0), alpha = sw / (2.0 * q);
        const double a0 = 1.0 + alpha;
        b0 = (1.0 - cw) * 0.5 / a0;
        b1 = (1.0 - cw) / a0;
        b2 = b0;
        a1 = -2.0 * cw / a0;
        a2 = (1.0 - alpha) / a0;
    }

    void setHighpass(double sr, double fc, double q = 0.7071067811865476) {
        const double w0 = kTwoPi * clampFreq(sr, fc) / sr;
        const double cw = std::cos(w0), sw = std::sin(w0), alpha = sw / (2.0 * q);
        const double a0 = 1.0 + alpha;
        b0 = (1.0 + cw) * 0.5 / a0;
        b1 = -(1.0 + cw) / a0;
        b2 = b0;
        a1 = -2.0 * cw / a0;
        a2 = (1.0 - alpha) / a0;
    }

    void setPeaking(double sr, double fc, double q, double gainDb) {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w0 = kTwoPi * clampFreq(sr, fc) / sr;
        const double cw = std::cos(w0), sw = std::sin(w0), alpha = sw / (2.0 * q);
        const double a0 = 1.0 + alpha / A;
        b0 = (1.0 + alpha * A) / a0;
        b1 = (-2.0 * cw) / a0;
        b2 = (1.0 - alpha * A) / a0;
        a1 = (-2.0 * cw) / a0;
        a2 = (1.0 - alpha / A) / a0;
    }

    void setLowShelf(double sr, double fc, double q, double gainDb) {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w0 = kTwoPi * clampFreq(sr, fc) / sr;
        const double cw = std::cos(w0), sw = std::sin(w0);
        const double beta = std::sqrt(std::max(A, 1e-6)) / q;
        const double a0 = (A + 1.0) + (A - 1.0) * cw + beta * sw;
        b0 = A * ((A + 1.0) - (A - 1.0) * cw + beta * sw) / a0;
        b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cw) / a0;
        b2 = A * ((A + 1.0) - (A - 1.0) * cw - beta * sw) / a0;
        a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cw) / a0;
        a2 = ((A + 1.0) + (A - 1.0) * cw - beta * sw) / a0;
    }

    void setHighShelf(double sr, double fc, double q, double gainDb) {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w0 = kTwoPi * clampFreq(sr, fc) / sr;
        const double cw = std::cos(w0), sw = std::sin(w0);
        const double beta = std::sqrt(std::max(A, 1e-6)) / q;
        const double a0 = (A + 1.0) - (A - 1.0) * cw + beta * sw;
        b0 = A * ((A + 1.0) + (A - 1.0) * cw + beta * sw) / a0;
        b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cw) / a0;
        b2 = A * ((A + 1.0) + (A - 1.0) * cw - beta * sw) / a0;
        a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cw) / a0;
        a2 = ((A + 1.0) - (A - 1.0) * cw - beta * sw) / a0;
    }

    double process(double x) {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        if (!std::isfinite(z1) || !std::isfinite(z2)) { z1 = z2 = 0.0; return 0.0; }
        return y;
    }
    void reset() { z1 = z2 = 0.0; }
};

// ---------------------------------------------------------------------------
// TPT state-variable bandpass (Cytomic topology) — zero-delay feedback.
// ---------------------------------------------------------------------------
struct TptBandpass {
    double g = 0.1, k = 1.0, a1 = 0.9, a2 = 0.01, a3 = 0.001;
    double s1 = 0.0, s2 = 0.0;
    double peakGain = 1.0;

    void set(double sr, double fc, double q, double gain = 1.0) {
        fc = clampd(fc, 20.0, sr * 0.45);
        q = clampd(q, 0.4, 60.0);
        g = std::tan(kPi * fc / sr);
        k = 1.0 / q;
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        // Bandpass (v1) peak gain is Q; normalise so the formant peak is `gain`.
        peakGain = gain / q;
    }

    double process(double x) {
        const double v3 = x - s2;
        const double v1 = a1 * s1 + a2 * v3;
        const double v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.0 * v1 - s1;
        s2 = 2.0 * v2 - s2;
        if (!std::isfinite(s1) || !std::isfinite(s2)) { s1 = s2 = 0.0; return 0.0; }
        return v1 * peakGain;
    }
    void reset() { s1 = s2 = 0.0; }
};

// ---------------------------------------------------------------------------
// FIR lowpass (windowed sinc, Blackman) used by the localised oversampler.
// ---------------------------------------------------------------------------
class FirLowpass {
public:
    void design(int taps, double cutoffNorm, double gain) {
        taps = std::max(3, taps | 1);
        h_.assign(static_cast<size_t>(taps), 0.0);
        z_.assign(static_cast<size_t>(taps), 0.0);
        pos_ = 0;
        const int m = (taps - 1) / 2;
        double sum = 0.0;
        for (int i = 0; i < taps; ++i) {
            const double n = static_cast<double>(i - m);
            const double sinc = (std::abs(n) < 1e-9) ? 2.0 * cutoffNorm
                                                      : std::sin(kTwoPi * cutoffNorm * n) / (kPi * n);
            const double w = 0.42 - 0.5 * std::cos(kTwoPi * i / (taps - 1)) +
                             0.08 * std::cos(2.0 * kTwoPi * i / (taps - 1));
            h_[static_cast<size_t>(i)] = sinc * w;
            sum += h_[static_cast<size_t>(i)];
        }
        if (std::abs(sum) > 1e-12)
            for (auto& c : h_) c = c / sum * gain;
    }
    int taps() const { return static_cast<int>(h_.size()); }
    double process(double x) {
        z_[static_cast<size_t>(pos_)] = x;
        double acc = 0.0;
        int idx = pos_;
        for (size_t i = 0; i < h_.size(); ++i) {
            acc += h_[i] * z_[static_cast<size_t>(idx)];
            idx = (idx == 0) ? static_cast<int>(h_.size()) - 1 : idx - 1;
        }
        pos_ = (pos_ + 1) % static_cast<int>(h_.size());
        return acc;
    }
    void reset() { std::fill(z_.begin(), z_.end(), 0.0); pos_ = 0; }

private:
    std::vector<double> h_, z_;
    int pos_ = 0;
};

// ---------------------------------------------------------------------------
// FFT + spectrum analysis helpers (tests, meters, UI analyser)
// ---------------------------------------------------------------------------
class Fft {
public:
    explicit Fft(int n = 2048) { prepare(n); }

    void prepare(int n) {
        n_ = std::max(8, n);
        rev_.assign(static_cast<size_t>(n_), 0);
        int bits = 0;
        while ((1 << bits) < n_) ++bits;
        for (int i = 0; i < n_; ++i) {
            int r = 0;
            for (int b = 0; b < bits; ++b)
                if (i & (1 << b)) r |= 1 << (bits - 1 - b);
            rev_[static_cast<size_t>(i)] = r;
        }
        tw_.resize(static_cast<size_t>(n_ / 2));
        for (int i = 0; i < n_ / 2; ++i)
            tw_[static_cast<size_t>(i)] =
                std::complex<double>(std::cos(-kTwoPi * i / n_), std::sin(-kTwoPi * i / n_));
    }

    int size() const { return n_; }

    void forward(std::vector<double>& re, std::vector<double>& im) const {
        re.resize(static_cast<size_t>(n_));
        im.resize(static_cast<size_t>(n_), 0.0);
        for (int i = 0; i < n_; ++i) {
            const int r = rev_[static_cast<size_t>(i)];
            if (r > i) {
                std::swap(re[static_cast<size_t>(i)], re[static_cast<size_t>(r)]);
                std::swap(im[static_cast<size_t>(i)], im[static_cast<size_t>(r)]);
            }
        }
        for (int len = 2; len <= n_; len <<= 1) {
            const int half = len / 2;
            const int step = n_ / len;
            for (int i = 0; i < n_; i += len) {
                for (int j = 0; j < half; ++j) {
                    const std::complex<double> w = tw_[static_cast<size_t>(j * step)];
                    const int a = i + j, b = i + j + half;
                    const std::complex<double> u(re[static_cast<size_t>(a)], im[static_cast<size_t>(a)]);
                    const std::complex<double> v =
                        std::complex<double>(re[static_cast<size_t>(b)], im[static_cast<size_t>(b)]) * w;
                    re[static_cast<size_t>(a)] = u.real() + v.real();
                    im[static_cast<size_t>(a)] = u.imag() + v.imag();
                    re[static_cast<size_t>(b)] = u.real() - v.real();
                    im[static_cast<size_t>(b)] = u.imag() - v.imag();
                }
            }
        }
    }

    // Magnitude spectrum with Hann window, averaged across segments.
    std::vector<double> magnitudeAvg(const std::vector<double>& sig, double sr) const {
        const int bins = n_ / 2 + 1;
        std::vector<double> acc(static_cast<size_t>(bins), 0.0);
        int segments = 0;
        for (size_t off = 0; off + static_cast<size_t>(n_) <= sig.size(); off += static_cast<size_t>(n_ / 2)) {
            std::vector<double> re(static_cast<size_t>(n_));
            for (int i = 0; i < n_; ++i) {
                const double w = 0.5 - 0.5 * std::cos(kTwoPi * i / (n_ - 1));
                re[static_cast<size_t>(i)] = sig[off + static_cast<size_t>(i)] * w;
            }
            std::vector<double> im;
            forward(re, im);
            for (int b = 0; b < bins; ++b) {
                acc[static_cast<size_t>(b)] +=
                    std::hypot(re[static_cast<size_t>(b)], im[static_cast<size_t>(b)]);
            }
            ++segments;
        }
        if (segments == 0) {
            std::vector<double> re(static_cast<size_t>(n_));
            for (int i = 0; i < n_; ++i) {
                const double w = 0.5 - 0.5 * std::cos(kTwoPi * i / (n_ - 1));
                re[static_cast<size_t>(i)] = (i < static_cast<int>(sig.size())) ? sig[static_cast<size_t>(i)] * w : 0.0;
            }
            std::vector<double> im;
            forward(re, im);
            for (int b = 0; b < bins; ++b)
                acc[static_cast<size_t>(b)] = std::hypot(re[static_cast<size_t>(b)], im[static_cast<size_t>(b)]);
            segments = 1;
        }
        for (auto& v : acc) v /= static_cast<double>(segments);
        (void)sr;
        return acc;
    }

private:
    int n_ = 2048;
    std::vector<int> rev_;
    std::vector<std::complex<double>> tw_;
};

// Bin frequency for index b.
inline double binHz(int b, int fftSize, double sr) { return b * sr / fftSize; }

// Local maximum around target with parabolic interpolation.
inline double peakHz(const std::vector<double>& mag, double sr, int fftSize, double targetHz,
                     double tolHz) {
    if (mag.size() < 4) return 0.0;
    const double df = sr / fftSize;
    int lo = static_cast<int>((targetHz - tolHz) / df);
    int hi = static_cast<int>((targetHz + tolHz) / df);
    lo = std::max(1, std::min(lo, static_cast<int>(mag.size()) - 2));
    hi = std::max(lo, std::min(hi, static_cast<int>(mag.size()) - 2));
    int best = lo;
    for (int b = lo; b <= hi; ++b)
        if (mag[static_cast<size_t>(b)] > mag[static_cast<size_t>(best)]) best = b;
    const double a = mag[static_cast<size_t>(best - 1)];
    const double b0 = mag[static_cast<size_t>(best)];
    const double c = mag[static_cast<size_t>(best + 1)];
    const double denom = (a - 2.0 * b0 + c);
    double delta = 0.0;
    if (std::abs(denom) > 1e-15) delta = 0.5 * (a - c) / denom;
    delta = clampd(delta, -0.5, 0.5);
    return (best + delta) * df;
}

// Local maximum nearest to a target frequency inside a half-window, with
// parabolic interpolation. Falls back to the window maximum. This is the
// standard "formant peak in an expected band" estimator.
inline double formantPeakHz(const std::vector<double>& mag, double sr, int fftSize,
                           double targetHz, double halfWindowHz) {
    if (mag.size() < 4) return 0.0;
    const double df = sr / fftSize;
    int lo = static_cast<int>((targetHz - halfWindowHz) / df);
    int hi = static_cast<int>((targetHz + halfWindowHz) / df);
    lo = std::max(1, std::min(lo, static_cast<int>(mag.size()) - 2));
    hi = std::max(lo, std::min(hi, static_cast<int>(mag.size()) - 2));

    auto refine = [&](int b) {
        const double a = mag[static_cast<size_t>(b - 1)];
        const double c0 = mag[static_cast<size_t>(b)];
        const double c1 = mag[static_cast<size_t>(b + 1)];
        const double denom = a - 2.0 * c0 + c1;
        double d = 0.0;
        if (std::abs(denom) > 1e-15) d = 0.5 * (a - c1) / denom;
        return (b + clampd(d, -0.5, 0.5)) * df;
    };

    int bestLocal = -1;
    double bestDist = 1e30;
    for (int b = lo; b <= hi; ++b) {
        if (mag[static_cast<size_t>(b)] > mag[static_cast<size_t>(b - 1)] &&
            mag[static_cast<size_t>(b)] >= mag[static_cast<size_t>(b + 1)]) {
            const double f = refine(b);
            const double d = std::abs(f - targetHz);
            if (d < bestDist) { bestDist = d; bestLocal = b; }
        }
    }
    if (bestLocal >= 0) return refine(bestLocal);

    int best = lo;
    for (int b = lo; b <= hi; ++b)
        if (mag[static_cast<size_t>(b)] > mag[static_cast<size_t>(best)]) best = b;
    return refine(best);
}

// Energy in a band, in dB (relative to a full-scale sine = 0 dB).
inline double bandEnergyDb(const std::vector<double>& mag, double sr, int fftSize, double f1,
                           double f2) {
    const double df = sr / fftSize;
    int lo = std::max(1, static_cast<int>(f1 / df));
    int hi = std::min(static_cast<int>(mag.size()) - 1, static_cast<int>(f2 / df));
    double acc = 0.0;
    for (int b = lo; b <= hi; ++b) {
        const double m = mag[static_cast<size_t>(b)];
        acc += m * m;
    }
    return 10.0 * std::log10(std::max(acc, 1e-30));
}

}  // namespace tb
