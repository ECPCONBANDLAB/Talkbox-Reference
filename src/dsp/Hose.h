// TalkBox Reference — Hose.h
// The talkbox hose: a lossy digital waveguide of adjustable physical length
// (1.5–2.0 m by default), with fractional delay (4-point Lagrange), wall
// damping and a guarded feedback loop.
#pragma once

#include "Common.h"

namespace tb {

class Hose {
public:
    static constexpr double kMinLengthM = 0.5;
    static constexpr double kMaxLengthM = 3.0;

    void prepare(double sr, double maxLengthM = kMaxLengthM) {
        sr_ = sr > 1000.0 ? sr : 48000.0;
        const double span = clampd(maxLengthM, kMinLengthM, kMaxLengthM);
        maxDelay_ = static_cast<int>(std::ceil(span / kSpeedOfSound * sr_)) + 8;
        buffer_.assign(static_cast<size_t>(maxDelay_), 0.0);
        write_ = 0;
        damping_.setCutoff(sr_, dampHz_);
        reset();
    }

    void setLength(double metres) {
        lengthM_ = clampd(metres, kMinLengthM, kMaxLengthM);
        delaySamples_ = lengthM_ / kSpeedOfSound * sr_;
        if (delaySamples_ > static_cast<double>(maxDelay_ - 6))
            delaySamples_ = static_cast<double>(maxDelay_ - 6);
    }
    double length() const { return lengthM_; }
    double delaySamples() const { return delaySamples_; }
    double delayMs() const { return delaySamples_ / sr_ * 1000.0; }

    void setFeedback(double g) { feedback_ = clampd(g, 0.0, 0.995); }
    double feedback() const { return feedback_; }
    void setDampingHz(double hz) {
        dampHz_ = clampd(hz, 200.0, sr_ * 0.45);
        damping_.setCutoff(sr_, dampHz_);
    }
    double dampingHz() const { return dampHz_; }
    void setWallLoss(double dbPerMetre) { wallLoss_ = clampd(dbPerMetre, 0.0, 12.0); }

    double process(double x) {
        if (!std::isfinite(x)) x = 0.0;
        const double delayed = readFractional();
        const double damped = damping_.process(delayed);
        // Wall loss is applied per pass, proportional to the physical length.
        const double loss = dbToGain(-wallLoss_ * lengthM_);
        double fb = feedback_ * loss;
        // Guard: the loop must stay contractive even with hostile coefficients.
        if (!(std::abs(fb) < 0.9995)) fb = 0.9995 * (fb < 0 ? -1.0 : 1.0);
        const double w = x + fb * damped;
        buffer_[static_cast<size_t>(write_)] = std::isfinite(w) ? w : 0.0;
        write_ = (write_ + 1) % maxDelay_;
        // Mouth-end tap: the air column itself is lossy, so the damped signal is
        // what actually leaves the hose.
        const double y = damped;
        return std::isfinite(y) ? y : 0.0;
    }

    void reset() {
        std::fill(buffer_.begin(), buffer_.end(), 0.0);
        damping_.reset();
        write_ = 0;
    }

private:
    double readFractional() const {
        const double d = delaySamples_;
        if (d < 3.0) return buffer_[static_cast<size_t>(write_)];
        const int iDelay = static_cast<int>(d);
        const double mu = d - iDelay;
        auto at = [this](int back) {
            int idx = write_ - back;
            while (idx < 0) idx += maxDelay_;
            return buffer_[static_cast<size_t>(idx)];
        };
        const double y0 = at(iDelay);
        const double y1 = at(iDelay + 1);
        const double y2 = at(iDelay + 2);
        const double y3 = at(iDelay + 3);
        // Third-order Lagrange through (0,y0)(1,y1)(2,y2)(3,y3), evaluated at mu.
        const double l0 = -((mu - 1.0) * (mu - 2.0) * (mu - 3.0)) / 6.0;
        const double l1 = (mu * (mu - 2.0) * (mu - 3.0)) / 2.0;
        const double l2 = -(mu * (mu - 1.0) * (mu - 3.0)) / 2.0;
        const double l3 = (mu * (mu - 1.0) * (mu - 2.0)) / 6.0;
        return y0 * l0 + y1 * l1 + y2 * l2 + y3 * l3;
    }

    double sr_ = 48000.0;
    double lengthM_ = 1.75, delaySamples_ = 0.0;
    double feedback_ = 0.9, wallLoss_ = 0.35, dampHz_ = 5200.0;
    int maxDelay_ = 1, write_ = 0;
    std::vector<double> buffer_;
    OnePoleLP damping_;
};

}  // namespace tb
