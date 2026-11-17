// TalkBox Reference — tests/LpcTools.h
// Minimal LPC analysis used by the validation suite (Levinson-Durbin +
// envelope peak picking). Analysis only: not part of the real-time DSP path.
#pragma once

#include <cmath>
#include <vector>

#include "Common.h"

namespace tb {
namespace analysis {

// Levinson-Durbin. a[0] = 1, a[1..order] are the predictor coefficients of
// A(z) = 1 + sum a_i z^-i. Returns the prediction error power.
inline double levinson(const std::vector<double>& r, int order, std::vector<double>& a) {
    a.assign(static_cast<size_t>(order) + 1, 0.0);
    a[0] = 1.0;
    std::vector<double> tmp(static_cast<size_t>(order) + 1, 0.0);
    double err = r[0];
    for (int m = 1; m <= order; ++m) {
        double acc = r[static_cast<size_t>(m)];
        for (int i = 1; i < m; ++i) acc += a[static_cast<size_t>(i)] * r[static_cast<size_t>(m - i)];
        const double k = (std::abs(err) > 1e-18) ? -acc / err : 0.0;
        tmp = a;
        for (int i = 1; i < m; ++i)
            a[static_cast<size_t>(i)] = tmp[static_cast<size_t>(i)] + k * tmp[static_cast<size_t>(m - i)];
        a[static_cast<size_t>(m)] = k;
        err *= (1.0 - k * k);
        if (err <= 1e-18) break;
    }
    return err;
}

inline bool lpc(const std::vector<double>& x, int order, std::vector<double>& a) {
    const int n = static_cast<int>(x.size());
    if (n < order * 4) return false;
    std::vector<double> w(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const double win = 0.54 - 0.46 * std::cos(kTwoPi * i / (n - 1));
        w[static_cast<size_t>(i)] = x[static_cast<size_t>(i)] * win;
    }
    std::vector<double> r(static_cast<size_t>(order) + 1, 0.0);
    for (int lag = 0; lag <= order; ++lag) {
        double acc = 0.0;
        for (int i = lag; i < n; ++i) acc += w[static_cast<size_t>(i)] * w[static_cast<size_t>(i - lag)];
        r[static_cast<size_t>(lag)] = acc / n;
    }
    if (r[0] <= 1e-20) return false;
    levinson(r, order, a);
    return true;
}

// Frequency of the LPC envelope maximum nearest to `targetHz`.
inline double lpcEnvelopePeakHz(const std::vector<double>& a, double sr, double targetHz,
                                double halfWindowHz, int grid = 8192) {
    const int order = static_cast<int>(a.size()) - 1;
    const double fmax = sr * 0.5;
    int best = -1;
    double bestDist = 1e9;
    double prev = 0.0, cur = 0.0, next = 0.0;
    auto env = [&](int i) {
        const double w = kTwoPi * (i * fmax / grid) / sr;
        double re = 0.0, im = 0.0;
        for (int k = 0; k <= order; ++k) {
            re += a[static_cast<size_t>(k)] * std::cos(-w * k);
            im += a[static_cast<size_t>(k)] * std::sin(-w * k);
        }
        return 1.0 / std::max(1e-9, std::hypot(re, im));
    };
    for (int i = 1; i < grid - 1; ++i) {
        const double f = i * fmax / grid;
        cur = env(i);
        prev = env(i - 1);
        next = env(i + 1);
        if (cur > prev && cur >= next && std::abs(f - targetHz) <= halfWindowHz) {
            const double d = std::abs(f - targetHz);
            if (d < bestDist) { bestDist = d; best = i; }
        }
    }
    if (best < 0) return 0.0;
    // Parabolic refinement in log magnitude.
    const double a0 = std::log(env(best - 1)), a1 = std::log(env(best)), a2 = std::log(env(best + 1));
    const double den = a0 - 2.0 * a1 + a2;
    double d = 0.0;
    if (std::abs(den) > 1e-15) d = 0.5 * (a0 - a2) / den;
    d = clampd(d, -0.5, 0.5);
    return (best + d) * fmax / grid;
}

}  // namespace analysis
}  // namespace tb
