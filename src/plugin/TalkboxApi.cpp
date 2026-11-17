// TalkBox Reference — TalkboxApi.cpp
#include "TalkboxApi.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace tb {

namespace {
const ParamInfo kInfo[] = {
    {"vowel", "Vowel", 0, 8, 5, true, ""},
    {"vtl", "Vocal tract length", 0.70, 1.45, 1.00, false, "x"},
    {"formantShift", "Formant shift", -12, 12, 0, false, "st"},
    {"nasal", "Nasal coupling", 0, 1, 0.12, false, ""},
    {"lipAperture", "Lip aperture", 0, 1, 1, false, ""},
    {"hoseLength", "Hose length", 1.5, 2.0, 1.75, false, "m"},
    {"hoseFeedback", "Hose feedback", 0, 0.995, 0.88, false, ""},
    {"drive", "Driver", -24, 36, 6, false, "dB"},
    {"oversample", "Oversampling", 1, 4, 2, true, "x"},
    {"carrierMode", "Carrier", 0, 4, 1, true, ""},
    {"carrierFreq", "Carrier pitch", 40, 880, 110, false, "Hz"},
    {"carrierNoise", "Carrier noise", 0, 1, 0, false, ""},
    {"micModel", "Microphone", 0, 3, 0, true, ""},
    {"micDistance", "Mic distance", 0.02, 1.5, 0.15, false, "m"},
    {"saturation", "Saturation", 0, 1, 0.18, false, ""},
    {"output", "Output", -24, 12, -3, false, "dB"},
    {"ceiling", "Limiter ceiling", -12, 0, -0.3, false, "dB"},
};
static_assert(sizeof(kInfo) / sizeof(kInfo[0]) == static_cast<size_t>(ParamId::Count),
              "param table out of sync with ParamId");
}  // namespace

const ParamInfo& paramInfo(ParamId id) { return kInfo[static_cast<size_t>(id)]; }
const char* paramName(ParamId id) { return kInfo[static_cast<size_t>(id)].id; }

void TalkboxApi::prepare(double sampleRate, int maxBlock) {
    (void)maxBlock;
    sampleRate_ = sampleRate > 1000.0 ? sampleRate : 48000.0;
    engine_.prepare(sampleRate_, kMaxBlock);
    loadPreset(currentPreset_);
    reset();
    prepared_ = true;
}

void TalkboxApi::reset() {
    engine_.reset();
}

void TalkboxApi::pushToEngine() {
    EngineParams p;
    p.vowel = static_cast<int>(values_[static_cast<size_t>(ParamId::Vowel)]);
    p.vtl = values_[static_cast<size_t>(ParamId::Vtl)];
    p.formantShift = values_[static_cast<size_t>(ParamId::FormantShift)];
    p.nasal = values_[static_cast<size_t>(ParamId::Nasal)];
    p.lipAperture = values_[static_cast<size_t>(ParamId::LipAperture)];
    p.hoseLength = values_[static_cast<size_t>(ParamId::HoseLength)];
    p.hoseFeedback = values_[static_cast<size_t>(ParamId::HoseFeedback)];
    p.driveDb = values_[static_cast<size_t>(ParamId::Drive)];
    p.oversample = static_cast<int>(values_[static_cast<size_t>(ParamId::Oversample)]);
    p.carrierMode = static_cast<CarrierMode>(static_cast<int>(values_[static_cast<size_t>(ParamId::CarrierMode)]));
    p.carrierFreq = values_[static_cast<size_t>(ParamId::CarrierFreq)];
    p.carrierNoiseMix = values_[static_cast<size_t>(ParamId::CarrierNoise)];
    p.mic = static_cast<MicModel>(static_cast<int>(values_[static_cast<size_t>(ParamId::MicModel)]));
    p.micDistance = values_[static_cast<size_t>(ParamId::MicDistance)];
    p.saturation = values_[static_cast<size_t>(ParamId::Saturation)];
    p.outputDb = values_[static_cast<size_t>(ParamId::Output)];
    p.ceilingDb = values_[static_cast<size_t>(ParamId::Ceiling)];
    engine_.setParams(p);
}

void TalkboxApi::setParam(ParamId id, double value) {
    const auto& info = paramInfo(id);
    double v = clampd(value, info.min, info.max);
    if (info.isDiscrete) v = std::round(v);
    values_[static_cast<size_t>(id)] = v;
    currentPreset_ = -1;
    pushToEngine();
}

double TalkboxApi::getParamNormalised(ParamId id) const {
    const auto& info = paramInfo(id);
    return clampd((getParam(id) - info.min) / (info.max - info.min), 0.0, 1.0);
}

void TalkboxApi::setParamNormalised(ParamId id, double norm) {
    const auto& info = paramInfo(id);
    setParam(id, info.min + clampd(norm, 0.0, 1.0) * (info.max - info.min));
}

std::string TalkboxApi::toDisplay(ParamId id, double value) {
    const auto& info = paramInfo(id);
    char buf[64];
    if (id == ParamId::Vowel) {
        static const char* names[] = {"i", "I", "e", "E", "ae", "a", "open o", "o", "u"};
        const int v = std::max(0, std::min(8, static_cast<int>(value)));
        return std::string("/") + names[v] + "/";
    }
    if (id == ParamId::Oversample) {
        std::snprintf(buf, sizeof(buf), "%dx", static_cast<int>(value));
        return buf;
    }
    if (id == ParamId::CarrierMode) {
        static const char* names[] = {"External", "Saw", "Square", "Pulse", "Noise"};
        return names[std::max(0, std::min(4, static_cast<int>(value)))];
    }
    if (id == ParamId::MicModel) {
        static const char* names[] = {"Dynamic", "Small diaphragm", "Condenser", "Ribbon"};
        return names[std::max(0, std::min(3, static_cast<int>(value)))];
    }
    if (info.unit[0] == '\0') {
        std::snprintf(buf, sizeof(buf), "%.2f", value);
        return buf;
    }
    std::snprintf(buf, sizeof(buf), "%.2f %s", value, info.unit);
    return buf;
}

std::string TalkboxApi::presetName(int index) const {
    if (index < 0 || index >= numPresets()) return {};
    return factoryPresets()[static_cast<size_t>(index)].name;
}

void TalkboxApi::loadPreset(int index) {
    if (index < 0 || index >= numPresets()) return;
    const EngineParams& p = factoryPresets()[static_cast<size_t>(index)].params;
    values_[static_cast<size_t>(ParamId::Vowel)] = p.vowel;
    values_[static_cast<size_t>(ParamId::Vtl)] = p.vtl;
    values_[static_cast<size_t>(ParamId::FormantShift)] = p.formantShift;
    values_[static_cast<size_t>(ParamId::Nasal)] = p.nasal;
    values_[static_cast<size_t>(ParamId::LipAperture)] = p.lipAperture;
    values_[static_cast<size_t>(ParamId::HoseLength)] = p.hoseLength;
    values_[static_cast<size_t>(ParamId::HoseFeedback)] = p.hoseFeedback;
    values_[static_cast<size_t>(ParamId::Drive)] = p.driveDb;
    values_[static_cast<size_t>(ParamId::Oversample)] = p.oversample;
    values_[static_cast<size_t>(ParamId::CarrierMode)] = static_cast<int>(p.carrierMode);
    values_[static_cast<size_t>(ParamId::CarrierFreq)] = p.carrierFreq;
    values_[static_cast<size_t>(ParamId::CarrierNoise)] = p.carrierNoiseMix;
    values_[static_cast<size_t>(ParamId::MicModel)] = static_cast<int>(p.mic);
    values_[static_cast<size_t>(ParamId::MicDistance)] = p.micDistance;
    values_[static_cast<size_t>(ParamId::Saturation)] = p.saturation;
    values_[static_cast<size_t>(ParamId::Output)] = p.outputDb;
    values_[static_cast<size_t>(ParamId::Ceiling)] = p.ceilingDb;
    currentPreset_ = index;
    pushToEngine();
}

void TalkboxApi::trigger(Consonant c, double level) { engine_.triggerConsonant(c, level); }

int TalkboxApi::process(const float* carrierIn, float* outL, float* outR, int numSamples) {
    if (!prepared_ || !outL || !outR) return 0;
    if (numSamples > kMaxBlock) numSamples = kMaxBlock;
    engine_.processBlockStereo(outL, outR, carrierIn, numSamples);
    return numSamples;
}

std::string TalkboxApi::saveState() const {
    std::ostringstream os;
    os.precision(6);
    for (int i = 0; i < static_cast<int>(ParamId::Count); ++i) {
        if (i) os << ';';
        os << kInfo[i].id << '=' << values_[static_cast<size_t>(i)];
    }
    return os.str();
}

bool TalkboxApi::restoreState(const std::string& state) {
    if (state.empty()) return false;
    std::istringstream is(state);
    std::string token;
    bool any = false;
    while (std::getline(is, token, ';')) {
        const auto eq = token.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = token.substr(0, eq);
        const double value = std::strtod(token.substr(eq + 1).c_str(), nullptr);
        for (int i = 0; i < static_cast<int>(ParamId::Count); ++i) {
            if (key == kInfo[i].id) {
                setParam(static_cast<ParamId>(i), value);
                any = true;
                break;
            }
        }
    }
    return any;
}

}  // namespace tb
