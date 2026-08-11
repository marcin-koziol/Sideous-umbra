/*
 * Sideous Umbra - a single polyphonic voice: PolyBLEP sawtooth "vocal fold"
 * blended with white noise (Growl) -> three parallel bandpass filters tuned
 * to a 5-vowel formant table (F1/F2/F3, continuously morphed) -> amp ADSR.
 * Pitch is the note frequency plus two always-on modulators: a one-shot
 * "swoop" envelope (up-then-back-down pitch glide fired on every note-on,
 * regardless of legato, so every note actually sounds like a "mrreow") and a
 * continuous sine vibrato. The vowel itself also drifts during that same
 * swoop gesture (SwoopEnvelope::progress01() drives it) rather than sitting
 * static for the whole note - real meows change mouth shape as they glide,
 * e.g. an open "eh" closing toward "ow"/"oo", and that vowel movement turns
 * out to matter at least as much as the pitch glide for reading as a cat
 * rather than a synth blip.
 *
 * Structural template is sideous's Voice.hpp / sideous-noise's NoiseVoice.hpp
 * (same noteOn/noteOff/kill/isActive/isReleasing bookkeeping, applyParams()
 * shape, per-voice seeding), but this is a much smaller voice - no filter
 * envelope, no LFO destination routing, no sub-oscillator.
 */

#pragma once

#include "Oscillator.hpp"
#include "Noise.hpp"
#include "ADSR.hpp"
#include "Filter.hpp"
#include "LFO.hpp"

#include <cmath>
#include <algorithm>

namespace sideous {

// -----------------------------------------------------------------------------------------------------------
// Swoop envelope - a dedicated up-then-back-down pitch-offset generator, not
// a generic ADSR (whose Attack->Decay->Sustain->Release shape doesn't fit a
// "rise to a peak, fall back to zero, then hold at zero" contour). Always
// fires on note-on regardless of retrigger/legato state, since the whole
// point is that every note gets its own little "mrreow" glide.
class SwoopEnvelope
{
public:
    void setSampleRate(double sampleRate) noexcept
    {
        fSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    }

    // amountSemitones may be negative (a downward swoop/whine instead of the
    // classic upward "mrreow"). riseSeconds is the Swoop Time parameter; the
    // fall stage takes 35% longer than the rise for a slightly slower
    // settle, which reads more like a real vocalization than a symmetric
    // glide.
    void trigger(float amountSemitones, float riseSeconds) noexcept
    {
        fAmount = amountSemitones;
        fRiseLength = std::max(1u, (uint32_t)(riseSeconds * (float)fSampleRate));
        fFallLength = std::max(1u, (uint32_t)(riseSeconds * 1.35f * (float)fSampleRate));
        fStage = Stage::Rise;
        fStageSample = 0;
    }

    // current pitch offset in semitones
    float process() noexcept
    {
        switch (fStage)
        {
        case Stage::Rise:
        {
            const float t = (float)fStageSample / (float)fRiseLength;
            const float shaped = std::pow(std::clamp(t, 0.0f, 1.0f), kExponent);
            fLevel = fAmount * shaped;
            if (++fStageSample >= fRiseLength)
            {
                fStage = Stage::Fall;
                fStageSample = 0;
            }
            break;
        }
        case Stage::Fall:
        {
            const float t = (float)fStageSample / (float)fFallLength;
            const float shaped = std::pow(std::clamp(t, 0.0f, 1.0f), kExponent);
            fLevel = fAmount * (1.0f - shaped);
            if (++fStageSample >= fFallLength)
            {
                fStage = Stage::Hold;
                fLevel = 0.0f;
            }
            break;
        }
        case Stage::Hold:
        default:
            fLevel = 0.0f;
            break;
        }
        return fLevel;
    }

    // 0 at the start of the rise, 1 once the fall has completed (and stays at
    // 1 through Hold) - a single normalized "how far through the gesture are
    // we" signal, used to drive the vowel drift in lockstep with the pitch
    // glide so a note's pitch contour and its vowel color change together,
    // the way a real "mee-YOW" pulls both at once instead of separately.
    float progress01() const noexcept
    {
        switch (fStage)
        {
        case Stage::Rise:
            return ((float)fStageSample / (float)fRiseLength) *
                   ((float)fRiseLength / (float)(fRiseLength + fFallLength));
        case Stage::Fall:
            return ((float)(fRiseLength + fStageSample)) / (float)(fRiseLength + fFallLength);
        case Stage::Hold:
        default:
            return 1.0f;
        }
    }

private:
    enum class Stage { Rise, Fall, Hold };

    // < 1: fast initial move, slow settle into the peak/back to zero - same
    // t^exponent trick as ADSR's curve shaping, fixed rather than a knob
    // since this is a small/focused plugin, not a full envelope editor.
    static constexpr float kExponent = 0.55f;

    double fSampleRate = 44100.0;
    float fAmount = 0.0f;
    uint32_t fRiseLength = 1;
    uint32_t fFallLength = 1;
    uint32_t fStageSample = 0;
    float fLevel = 0.0f;
    Stage fStage = Stage::Hold;
};

// -----------------------------------------------------------------------------------------------------------

struct MeowVoiceParams
{
    float vowel = 1.0f;              // 0..4, starting position, continuous morph across the 5-point vowel table below
    float vowelDrift = 2.5f;         // -4..4, added to vowel over the swoop's rise+fall, scaled by SwoopEnvelope::progress01() -
                                      // ties the vowel color change to the same gesture as the pitch glide, e.g. an open
                                      // "eh" sliding toward a closed "oh/oo" by the time the glide settles, like a real "mee-YOW"
    float formantResonance = 0.42f;  // 0..1, shared by F1/F2/F3
    float formantDrive = 0.1f;       // 0..1, shared by F1/F2/F3
    float growl = 0.18f;             // 0..1, saw/noise crossfade (0 = pure tone, 1 = mostly rasp)

    float swoopAmount = 7.0f;        // semitones, may be negative for a downward whine
    float swoopTime = 0.16f;         // seconds, rise stage length (fall runs ~35% longer)

    float vibratoRateHz = 5.0f;
    float vibratoDepth = 0.3f;       // semitones

    float ampAttack = 0.02f, ampDecay = 0.22f, ampSustain = 0.4f, ampRelease = 0.35f;
    float ampCurve = 0.35f;
    float velocitySensitivity = 0.8f;
};

class MeowVoice
{
public:
    // 5-point vowel formant table (Hz): index 0 "ah" .. index 4 "oo". A
    // continuous Vowel parameter linearly interpolates between the
    // floor/ceil entries by its fractional part.
    static constexpr float kVowelTable[5][3] = {
        { 700.0f, 1220.0f, 2600.0f }, // ah
        { 530.0f, 1840.0f, 2480.0f }, // eh
        { 270.0f, 2290.0f, 3010.0f }, // ee
        { 570.0f,  840.0f, 2410.0f }, // oh
        { 300.0f,  870.0f, 2240.0f }, // oo
    };

    // distinct per voice so simultaneous notes' noise/rasp components
    // decorrelate instead of sounding like the same signal summed louder
    void setSeed(uint32_t seed) noexcept { fNoise.setSeed(seed); }

    void setSampleRate(double sampleRate) noexcept
    {
        fSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        fOsc.setSampleRate(sampleRate);
        fAmpEnv.setSampleRate(sampleRate);
        fSwoop.setSampleRate(sampleRate);
        fVibrato.setSampleRate(sampleRate);
        fF1.setSampleRate(sampleRate);
        fF2.setSampleRate(sampleRate);
        fF3.setSampleRate(sampleRate);
    }

    void noteOn(int note, float velocity) noexcept
    {
        fNote = note;
        fVelocity = velocity;
        fBaseFreq = noteToHz(note);

        fAmpEnv.noteOn();
        // always fires, regardless of legato/retrigger - every note gets its
        // own swoop, that's the whole point of this instrument
        fSwoop.trigger(fSwoopAmount, fSwoopTime);
        fVibrato.reset();

        fActive = true;
    }

    void noteOff() noexcept { fAmpEnv.noteOff(); }

    // hard cut, used for voice stealing
    void kill() noexcept { fActive = false; }

    bool isActive() const noexcept { return fActive && !fAmpEnv.isIdle(); }
    bool isReleasing() const noexcept { return fAmpEnv.getStage() == ADSR::Stage::Release; }
    int getNote() const noexcept { return fNote; }

    void applyParams(const MeowVoiceParams& p) noexcept
    {
        fSwoopAmount = p.swoopAmount;
        fSwoopTime = p.swoopTime;

        fVibrato.setFrequency(p.vibratoRateHz);
        fVibratoDepth = p.vibratoDepth;

        fGrowl = std::clamp(p.growl, 0.0f, 1.0f);

        fVowelBase = p.vowel;
        fVowelDrift = p.vowelDrift;

        fF1.setType(FilterType::Bandpass);
        fF2.setType(FilterType::Bandpass);
        fF3.setType(FilterType::Bandpass);
        fF1.setResonance(p.formantResonance);
        fF2.setResonance(p.formantResonance);
        fF3.setResonance(p.formantResonance);
        fF1.setDrive(p.formantDrive);
        fF2.setDrive(p.formantDrive);
        fF3.setDrive(p.formantDrive);

        fAmpEnv.setAttack(p.ampAttack);
        fAmpEnv.setDecay(p.ampDecay);
        fAmpEnv.setSustain(p.ampSustain);
        fAmpEnv.setRelease(p.ampRelease);
        fAmpEnv.setCurve(p.ampCurve);
        fVelocitySensitivity = p.velocitySensitivity;
    }

    float process() noexcept
    {
        if (!fActive)
            return 0.0f;

        // pitch: base note frequency + one-shot swoop (semitones) +
        // continuous vibrato (semitones), combined in log-frequency space
        const float swoopSemitones = fSwoop.process();
        const float vibratoSemitones = fVibrato.process() * fVibratoDepth;
        const float pitchRatio = std::exp2((swoopSemitones + vibratoSemitones) / 12.0f);
        fOsc.setFrequency(fBaseFreq * pitchRatio);

        const float saw = fOsc.process();
        const float noise = fNoise.process();
        const float source = saw * (1.0f - fGrowl) + noise * fGrowl;

        const float ampLevel = fAmpEnv.process();
        if (fAmpEnv.isIdle())
        {
            fActive = false;
            return 0.0f;
        }

        // vowel color drifts across the note in lockstep with the swoop's
        // pitch glide (see MeowVoiceParams::vowelDrift) - recomputed every
        // sample, same as the pitch ratio above, since it's continuously
        // moving for the first part of the note rather than fixed per-note
        const float vowel = std::clamp(fVowelBase + fVowelDrift * fSwoop.progress01(), 0.0f, 4.0f);
        computeFormants(vowel, fF1Hz, fF2Hz, fF3Hz);

        // three parallel formant bandpasses, summed with decreasing weight
        // (F1 strongest, F3 weakest) then normalized for headroom
        const float f1 = fF1.process(source, fF1Hz);
        const float f2 = fF2.process(source, fF2Hz);
        const float f3 = fF3.process(source, fF3Hz);
        const float formants = (f1 * 1.0f + f2 * 0.6f + f3 * 0.35f) * kFormantNormalize;

        const float velocityGain = 1.0f - fVelocitySensitivity * (1.0f - fVelocity);
        return formants * ampLevel * velocityGain;
    }

private:
    static float noteToHz(int note) noexcept
    {
        return 440.0f * std::exp2(((float)note - 69.0f) / 12.0f);
    }

    static void computeFormants(float vowel, float& f1, float& f2, float& f3) noexcept
    {
        vowel = std::clamp(vowel, 0.0f, 4.0f);
        const int idx0 = std::min((int)vowel, 4);
        const int idx1 = std::min(idx0 + 1, 4);
        const float frac = vowel - (float)idx0;

        f1 = kVowelTable[idx0][0] + (kVowelTable[idx1][0] - kVowelTable[idx0][0]) * frac;
        f2 = kVowelTable[idx0][1] + (kVowelTable[idx1][1] - kVowelTable[idx0][1]) * frac;
        f3 = kVowelTable[idx0][2] + (kVowelTable[idx1][2] - kVowelTable[idx0][2]) * frac;
    }

    // 1 / (1.0 + 0.6 + 0.35) weighting sum, so a fully in-phase peak across
    // all three bands still lands near unity rather than clipping headroom
    static constexpr float kFormantNormalize = 1.0f / 1.95f;

    Oscillator fOsc;
    Noise fNoise;
    ADSR fAmpEnv;
    Filter fF1, fF2, fF3;
    SwoopEnvelope fSwoop;
    LFO fVibrato;

    double fSampleRate = 44100.0;
    float fBaseFreq = 440.0f;

    float fF1Hz = 700.0f, fF2Hz = 1220.0f, fF3Hz = 2600.0f;
    float fVowelBase = 1.0f;
    float fVowelDrift = 2.5f;
    float fSwoopAmount = 5.0f;
    float fSwoopTime = 0.12f;
    float fVibratoDepth = 0.3f;
    float fGrowl = 0.15f;
    float fVelocitySensitivity = 0.8f;

    int fNote = -1;
    float fVelocity = 0.0f;
    bool fActive = false;
};

} // namespace sideous
