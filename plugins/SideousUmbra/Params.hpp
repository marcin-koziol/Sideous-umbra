/*
 * Sideous Umbra - shared parameter definitions used by both the DSP plugin
 * side and the UI, so ranges/units/labels can't drift between the two.
 * Deliberately small (17 params) compared to sideous-noise's 26-parameter
 * matrix - this is the family's small/joke plugin, not a re-run of that
 * complexity. No enum/dropdown parameters at all: every control here is a
 * continuous knob.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace sideous {

enum Params : uint32_t {
    kParamVowel = 0,
    kParamVowelDrift,
    kParamFormantResonance,
    kParamFormantDrive,
    kParamGrowl,

    kParamSwoopAmount,
    kParamSwoopTime,
    kParamVibratoRate,
    kParamVibratoDepth,

    kParamAmpAttack,
    kParamAmpDecay,
    kParamAmpSustain,
    kParamAmpRelease,
    kParamAmpCurve,
    kParamVelocitySensitivity,

    kParamMasterVolume,
    kParamMasterDrive,

    // output-only meter (see paintUmbraEyes() in ui/UIPainter.hpp): not a
    // user control, no knob in buildLayout(). Drives the cat mascot's eye
    // brightness from the live output level. Must stay last before
    // kParamCount so real/automatable params keep their existing indices.
    kParamOutLevel,

    kParamCount
};

enum class ParamShape { Linear, Logarithmic };

struct ParamInfo
{
    const char* name;
    const char* symbol;
    const char* unit;
    float min;
    float max;
    float def;
    ParamShape shape;
};

inline const ParamInfo& getParamInfo(uint32_t index) noexcept
{
    static constexpr ParamInfo table[kParamCount] = {
        { "Vowel",             "vowel",              "",   0.0f,     4.0f,     1.0f,  ParamShape::Linear },
        { "Vowel Drift",       "vowel_drift",        "",  -4.0f,     4.0f,     2.5f,  ParamShape::Linear },
        { "Formant Resonance", "formant_resonance",  "",   0.0f,     1.0f,     0.42f, ParamShape::Linear },
        { "Formant Drive",     "formant_drive",      "",   0.0f,     1.0f,     0.1f,  ParamShape::Linear },
        { "Growl",             "growl",              "",   0.0f,     1.0f,     0.18f, ParamShape::Linear },

        { "Swoop Amount",      "swoop_amount",       "st", -12.0f,   12.0f,     7.0f,  ParamShape::Linear },
        { "Swoop Time",        "swoop_time",         "s",  0.01f,    1.0f,     0.16f,  ParamShape::Logarithmic },
        { "Vibrato Rate",      "vibrato_rate",       "Hz", 0.5f,    12.0f,     5.0f,   ParamShape::Logarithmic },
        { "Vibrato Depth",     "vibrato_depth",      "st", 0.0f,     2.0f,     0.3f,   ParamShape::Linear },

        { "Amp Attack",        "amp_attack",         "s",  0.001f,   2.0f,     0.02f,  ParamShape::Logarithmic },
        { "Amp Decay",         "amp_decay",          "s",  0.001f,   2.0f,     0.22f,  ParamShape::Logarithmic },
        { "Amp Sustain",       "amp_sustain",        "",   0.0f,     1.0f,     0.4f,   ParamShape::Linear },
        { "Amp Release",       "amp_release",        "s",  0.001f,   3.0f,     0.35f,  ParamShape::Logarithmic },
        { "Amp Curve",         "amp_curve",          "",   0.0f,     1.0f,     0.35f,  ParamShape::Linear },
        { "Velocity Sens",     "velocity_sens",      "",   0.0f,     1.0f,     0.8f,   ParamShape::Linear },

        { "Master Volume",     "master_volume",      "",   0.0f,     1.0f,     0.8f,   ParamShape::Linear },
        { "Master Drive",      "master_drive",       "",   0.0f,     1.0f,     0.05f,  ParamShape::Linear },

        { "Eye Level",         "eye_level",          "",   0.0f,     1.0f,     0.0f,   ParamShape::Linear },
    };
    return table[index];
}

// Hosts automate a parameter's range linearly (VST3 normalisation, CLAP
// plain values; DPF passes the logarithmic hint on to LV2 only), which crams
// most of a 20 Hz..20 kHz or 1 ms..5 s range into the first few percent of an
// automation lane. So every Logarithmic parameter is exposed to the host as
// a 0..100 % position along its log curve (for a cutoff, ~10 % per octave).
// The DSP, the UI and preset files keep working in real units: the plugin
// and the UI convert only where values cross to and from the host.
inline bool isHostPosition(uint32_t index) noexcept
{
    return index < kParamCount && getParamInfo(index).shape == ParamShape::Logarithmic;
}

inline float paramToHost(uint32_t index, float value) noexcept
{
    if (!isHostPosition(index))
        return value;
    const ParamInfo& info = getParamInfo(index);
    const float lo = std::max(info.min, 1e-6f);
    const float v = std::clamp(value, lo, info.max);
    return 100.0f * std::log(v / lo) / std::log(info.max / lo);
}

inline float paramFromHost(uint32_t index, float value) noexcept
{
    if (!isHostPosition(index))
        return value;
    const ParamInfo& info = getParamInfo(index);
    const float lo = std::max(info.min, 1e-6f);
    return lo * std::pow(info.max / lo, std::clamp(value, 0.0f, 100.0f) * 0.01f);
}

} // namespace sideous
