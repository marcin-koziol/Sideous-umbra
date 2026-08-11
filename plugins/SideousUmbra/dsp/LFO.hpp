/*
 * Sideous Umbra - free-running sine LFO, trimmed from sideous-noise's
 * multi-waveform LFO.hpp down to just the Sine case, used for pitch
 * vibrato in MeowVoice.hpp. No Amplitude/Cutoff/RingMod destination
 * plumbing here (unlike the sideous-noise family) - vibrato is this
 * plugin's only LFO destination, so it's applied directly rather than
 * through a generic destination switch.
 */

#pragma once

#include <cmath>

namespace sideous {

class LFO
{
public:
    void setSampleRate(double sampleRate) noexcept
    {
        fSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    }

    void setFrequency(float hz) noexcept { fIncrement = (double)hz / fSampleRate; }

    // retrigger to a consistent starting phase, called on note-on
    void reset() noexcept { fPhase = 0.0; }

    // returns -1..1
    float process() noexcept
    {
        const float out = std::sin(2.0f * (float)M_PI * (float)fPhase);

        fPhase += fIncrement;
        if (fPhase >= 1.0)
            fPhase -= 1.0;

        return out;
    }

private:
    double fSampleRate = 44100.0;
    double fPhase = 0.0;
    double fIncrement = 0.0;
};

} // namespace sideous
