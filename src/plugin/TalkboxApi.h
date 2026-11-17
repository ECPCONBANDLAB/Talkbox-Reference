// TalkBox Reference — TalkboxApi.h
// Framework-agnostic plugin core: parameter addressing, preset loading, state
// serialisation and block processing. Used by the JUCE shell and by hosts that
// prefer a plain C++ interface (no JUCE, no allocations on the audio thread).
#pragma once

#include <array>
#include <string>
#include <vector>

#include "../dsp/Engine.h"

namespace tb {

enum class ParamId : int {
    Vowel = 0,
    Vtl,
    FormantShift,
    Nasal,
    LipAperture,
    HoseLength,
    HoseFeedback,
    Drive,
    Oversample,
    CarrierMode,
    CarrierFreq,
    CarrierNoise,
    MicModel,
    MicDistance,
    Saturation,
    Output,
    Ceiling,
    Count
};

struct ParamInfo {
    const char* id;
    const char* label;
    double min;
    double max;
    double def;
    bool isDiscrete;
    const char* unit;
};

const ParamInfo& paramInfo(ParamId id);
const char* paramName(ParamId id);

class TalkboxApi {
public:
    static constexpr int kMaxBlock = 2048;

    void prepare(double sampleRate, int maxBlock = kMaxBlock);
    void reset();

    double sampleRate() const { return sampleRate_; }
    int latencySamples() const { return engine_.latencySamples(); }
    double latencyMs() const { return engine_.latencyMs(); }
    double hoseDelayMs() const { return engine_.hoseDelayMs(); }

    // Parameter access by id (engine units, not normalised).
    void setParam(ParamId id, double value);
    double getParam(ParamId id) const { return values_[static_cast<size_t>(id)]; }
    void setParamNormalised(ParamId id, double norm);
    double getParamNormalised(ParamId id) const;
    static std::string toDisplay(ParamId id, double value);

    // Presets.
    int numPresets() const { return static_cast<int>(factoryPresets().size()); }
    std::string presetName(int index) const;
    void loadPreset(int index);
    int currentPreset() const { return currentPreset_; }

    // Articulation trigger (consonant bridge).
    void trigger(Consonant c, double level = 1.0);

    // Audio: stereo out, mono carrier/sidechain in (may be null for the internal
    // oscillator). Returns the number of samples written.
    int process(const float* carrierIn, float* outL, float* outR, int numSamples);

    // Very small state blob: "k=v;k=v;..." — enough for a host to restore a session.
    std::string saveState() const;
    bool restoreState(const std::string& state);

    TalkboxEngine& engine() { return engine_; }

private:
    void pushToEngine();

    TalkboxEngine engine_;
    double sampleRate_ = 48000.0;
    int currentPreset_ = 2;  // Balanced
    std::array<double, static_cast<size_t>(ParamId::Count)> values_{};
    bool prepared_ = false;
};

}  // namespace tb
