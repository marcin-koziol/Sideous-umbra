/*
 * Sideous Umbra - a small, silly, genuinely playable cat-meow synth: classic
 * formant/vocal synthesis (5-vowel morph across three parallel bandpass
 * filters) driving a PolyBLEP-sawtooth "vocal fold" blended with growl
 * noise, topped with a two-stage pitch-swoop envelope so every note actually
 * sounds like a "mrreow" rather than a flat drone. Structure mirrors
 * sideous-noise's SideousNoisePlugin.cpp (same initParameter/getParameterValue/
 * setParameterValue/run() shape, same voice-stealing priority), but much
 * smaller: 8 voices instead of 16, no LFO destination routing, no pitch
 * bend/mod wheel performance controls - the small/joke plugin of the family.
 */

#include "DistrhoPlugin.hpp"
#include "Params.hpp"
#include "dsp/MeowVoice.hpp"

#include <array>
#include <cmath>

START_NAMESPACE_DISTRHO

using namespace sideous;

// -----------------------------------------------------------------------------------------------------------

static constexpr const uint32_t kNumVoices = 8;

class SideousUmbraPlugin : public Plugin
{
public:
    SideousUmbraPlugin()
        : Plugin(kParamCount, 0, 0)
    {
        // distinct per-voice seed so a chord's simultaneous notes' growl
        // noise decorrelates instead of sounding like one signal summed louder
        for (uint32_t i = 0; i < kNumVoices; ++i)
            fVoices[i].setSeed(0x9e3779b9u * (i + 1));

        sampleRateChanged(getSampleRate());

        // fParams/fMasterVolume/fMasterDrive's own member-initializer defaults
        // are just fallbacks for early construction - Params.hpp's table is
        // the single source of truth for actual defaults (shared with the
        // UI), so push it through the real setParameterValue() path here to
        // guarantee the two can never silently drift apart.
        for (uint32_t i = 0; i < kParamCount; ++i)
            if (i != kParamOutLevel)
                setParameterValue(i, getParamInfo(i).def);
    }

protected:
    // ---------------------------------------------------------------------
    // Information

    const char* getLabel() const override { return "Sideous Umbra"; }
    const char* getDescription() const override
    {
        return "A small, silly, genuinely playable cat-meow synth. Classic formant/vocal "
               "synthesis - a PolyBLEP sawtooth \"vocal fold\" blended with growl noise, "
               "pushed through three parallel bandpass filters morphed across a 5-vowel "
               "table - topped with a two-stage pitch-swoop envelope, and the vowel itself "
               "drifts across that same glide, so every note actually sounds like a \"mrreow\" "
               "rather than a flat drone.";
    }
    const char* getMaker() const override { return "Sideous"; }
    const char* getHomePage() const override { return DISTRHO_PLUGIN_URI; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(0, 1, 0); }

    // ---------------------------------------------------------------------
    // Init

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        const ParamInfo& info = getParamInfo(index);

        if (index == kParamOutLevel)
        {
            // live output-level meter driving the cat mascot's eye
            // brightness in the UI - an output, not a user control, so it
            // must NOT be automatable (hosts don't write to output params;
            // DPF's per-format bridges treat kParameterIsOutput as
            // read-only regardless, but keep the hint clean/correct here).
            parameter.hints = kParameterIsOutput;
        }
        else
        {
            parameter.hints = kParameterIsAutomatable;
            if (info.shape == ParamShape::Logarithmic)
                parameter.hints |= kParameterIsLogarithmic;
        }

        parameter.name = info.name;
        parameter.symbol = info.symbol;
        parameter.unit = info.unit;
        parameter.ranges.min = info.min;
        parameter.ranges.max = info.max;
        parameter.ranges.def = info.def;
    }

    // ---------------------------------------------------------------------
    // Internal data

    float getParameterValue(uint32_t index) const override
    {
        switch (index)
        {
        case kParamVowel:               return fParams.vowel;
        case kParamVowelDrift:          return fParams.vowelDrift;
        case kParamFormantResonance:    return fParams.formantResonance;
        case kParamFormantDrive:        return fParams.formantDrive;
        case kParamGrowl:               return fParams.growl;
        case kParamSwoopAmount:         return fParams.swoopAmount;
        case kParamSwoopTime:           return fParams.swoopTime;
        case kParamVibratoRate:         return fParams.vibratoRateHz;
        case kParamVibratoDepth:        return fParams.vibratoDepth;
        case kParamAmpAttack:           return fParams.ampAttack;
        case kParamAmpDecay:            return fParams.ampDecay;
        case kParamAmpSustain:          return fParams.ampSustain;
        case kParamAmpRelease:          return fParams.ampRelease;
        case kParamAmpCurve:            return fParams.ampCurve;
        case kParamVelocitySensitivity: return fParams.velocitySensitivity;
        case kParamMasterVolume:        return fMasterVolume;
        case kParamMasterDrive:         return fMasterDrive;
        case kParamOutLevel:            return fOutLevel;
        default:                        return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index)
        {
        // kParamOutLevel is an output - hosts shouldn't write to it, but be
        // defensive and just no-op rather than fall through to the voices'
        // applyParams() below.
        case kParamOutLevel: return;
        case kParamVowel:               fParams.vowel = value; break;
        case kParamVowelDrift:          fParams.vowelDrift = value; break;
        case kParamFormantResonance:    fParams.formantResonance = value; break;
        case kParamFormantDrive:        fParams.formantDrive = value; break;
        case kParamGrowl:               fParams.growl = value; break;
        case kParamSwoopAmount:         fParams.swoopAmount = value; break;
        case kParamSwoopTime:           fParams.swoopTime = value; break;
        case kParamVibratoRate:         fParams.vibratoRateHz = value; break;
        case kParamVibratoDepth:        fParams.vibratoDepth = value; break;
        case kParamAmpAttack:           fParams.ampAttack = value; break;
        case kParamAmpDecay:            fParams.ampDecay = value; break;
        case kParamAmpSustain:          fParams.ampSustain = value; break;
        case kParamAmpRelease:          fParams.ampRelease = value; break;
        case kParamAmpCurve:            fParams.ampCurve = value; break;
        case kParamVelocitySensitivity: fParams.velocitySensitivity = value; break;
        case kParamMasterVolume:        fMasterVolume = value; break;
        case kParamMasterDrive:         fMasterDrive = value; break;
        default: return;
        }

        for (sideous::MeowVoice& voice : fVoices)
            voice.applyParams(fParams);
    }

    // ---------------------------------------------------------------------
    // Audio/MIDI Processing

    void activate() override
    {
        sampleRateChanged(getSampleRate());
    }

    void sampleRateChanged(double newSampleRate) override
    {
        fSampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        for (sideous::MeowVoice& voice : fVoices)
        {
            voice.setSampleRate(newSampleRate);
            voice.applyParams(fParams);
        }

        // ~8ms attack / ~250ms release one-pole coefficients, recomputed
        // whenever the sample rate changes so the eye-level meter reads the
        // same regardless of host sample rate - fast enough to feel
        // responsive to note-ons, slow enough on the way down to read like a
        // VU/peak meter rather than flickering every sample.
        fAttackCoeff = 1.0f - std::exp(-1.0f / (0.008f * (float)fSampleRate));
        fReleaseCoeff = 1.0f - std::exp(-1.0f / (0.25f * (float)fSampleRate));
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];

        uint32_t nextEvent = 0;

        for (uint32_t frame = 0; frame < frames; ++frame)
        {
            while (nextEvent < midiEventCount && midiEvents[nextEvent].frame == frame)
                handleMidiEvent(midiEvents[nextEvent++]);

            float mix = 0.0f;
            for (sideous::MeowVoice& voice : fVoices)
                mix += voice.process();

            mix *= fMasterVolume * kVoiceHeadroom;

            // Drive: character/vibe saturation, not just a safety limiter -
            // same tanh-crossfade trick as sideous-noise's run()
            if (fMasterDrive > 0.0f)
            {
                const float driveGain = 1.0f + fMasterDrive * 7.0f;
                const float saturated = std::tanh(mix * driveGain);
                mix = mix + fMasterDrive * (saturated - mix);
            }

            // gentle safety saturation: several simultaneous resonant
            // formant voices can otherwise blow well past 0dBFS
            mix = std::tanh(mix);

            // exponential one-pole envelope follower on the final output,
            // read by the UI (as kParamOutLevel) to brighten the cat
            // mascot's eyes with the live "meow" loudness. kVoiceHeadroom
            // keeps the *audio* signal conservative on purpose (room for
            // several resonant formant voices to sum without clipping), but
            // that also means a single typical note only reaches ~0.05-0.1
            // raw mix amplitude - nowhere near enough to visibly move the
            // eye color. kMeterGain boosts just the meter reading (never the
            // audio itself) so normal playing actually lights the eyes up;
            // clamped to 1 since loud chords can otherwise push it well past.
            const float meterInput = std::min(std::fabs(mix) * kMeterGain, 1.0f);
            const float coeff = meterInput > fOutLevel ? fAttackCoeff : fReleaseCoeff;
            fOutLevel += (meterInput - fOutLevel) * coeff;

            outL[frame] = mix;
            outR[frame] = mix;
        }

        while (nextEvent < midiEventCount)
            handleMidiEvent(midiEvents[nextEvent++]);
    }

private:
    // 8 voices is deliberately more headroom-friendly than sideous-noise's
    // 16-voice / 0.25 pairing (fewer simultaneous formant peaks to sum)
    static constexpr const float kVoiceHeadroom = 0.4f;
    // see the eye-level meter comment in run() - purely a metering multiplier
    static constexpr const float kMeterGain = 12.0f;

    void handleMidiEvent(const MidiEvent& event) noexcept
    {
        if (event.size < 2 || event.size > 3)
            return;

        const uint8_t status = event.data[0] & 0xF0;
        const uint8_t note = event.data[1];
        const uint8_t velocity = event.size > 2 ? event.data[2] : 0;

        if (status == 0x90 && velocity > 0)
        {
            triggerVoiceNoteOn(note, velocity);
        }
        else if (status == 0x80 || (status == 0x90 && velocity == 0))
        {
            triggerVoiceNoteOff(note);
        }
    }

    void triggerVoiceNoteOn(int note, int velocity) noexcept
    {
        sideous::MeowVoice* target = nullptr;

        for (sideous::MeowVoice& voice : fVoices)
        {
            if (!voice.isActive())
            {
                target = &voice;
                break;
            }
        }

        if (target == nullptr)
        {
            for (sideous::MeowVoice& voice : fVoices)
            {
                if (voice.isReleasing())
                {
                    target = &voice;
                    break;
                }
            }
        }

        if (target == nullptr)
            target = &fVoices[fStealCursor++ % kNumVoices];

        target->applyParams(fParams);
        target->noteOn(note, (float)velocity / 127.0f);
    }

    void triggerVoiceNoteOff(int note) noexcept
    {
        for (sideous::MeowVoice& voice : fVoices)
        {
            if (voice.isActive() && voice.getNote() == note && !voice.isReleasing())
                voice.noteOff();
        }
    }

    std::array<sideous::MeowVoice, kNumVoices> fVoices;
    sideous::MeowVoiceParams fParams;
    float fMasterVolume = 0.8f;
    float fMasterDrive = 0.05f;
    uint32_t fStealCursor = 0;
    double fSampleRate = 44100.0;

    // live output-level meter (see kParamOutLevel) and its one-pole
    // attack/release coefficients, recomputed in sampleRateChanged()
    float fOutLevel = 0.0f;
    float fAttackCoeff = 1.0f;
    float fReleaseCoeff = 1.0f;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SideousUmbraPlugin)
};

// -----------------------------------------------------------------------------------------------------------

Plugin* createPlugin()
{
    return new SideousUmbraPlugin();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
