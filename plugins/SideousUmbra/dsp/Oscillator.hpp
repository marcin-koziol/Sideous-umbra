/*
 * Sideous Umbra - PolyBLEP band-limited sawtooth oscillator, trimmed from
 * sideous's Oscillator.hpp down to just the Saw case (the "vocal fold" buzz
 * that drives the formant filter bank in MeowVoice.hpp - Pulse/Triangle are
 * out of scope for a meow voice, so they're dropped rather than carried
 * along unused).
 */

#pragma once

#include <cmath>

namespace sideous {

class Oscillator
{
public:
    void setSampleRate(double sampleRate) noexcept
    {
        fSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    }

    void setFrequency(float hz) noexcept
    {
        fIncrement = (double)hz / fSampleRate;
    }

    void resetPhase(float phase = 0.0f) noexcept { fPhase = phase; }

    // band-limited saw, -1..1
    float process() noexcept
    {
        float out = 2.0f * (float)fPhase - 1.0f;
        out -= polyBlep(fPhase, fIncrement);

        fPhase += fIncrement;
        if (fPhase >= 1.0)
            fPhase -= 1.0;
        else if (fPhase < 0.0)
            fPhase += 1.0;

        return out;
    }

private:
    static float polyBlep(double t, double dt) noexcept
    {
        if (dt <= 0.0)
            return 0.0f;

        if (t < dt)
        {
            const double x = t / dt;
            return (float)(x + x - x * x - 1.0);
        }
        else if (t > 1.0 - dt)
        {
            const double x = (t - 1.0) / dt;
            return (float)(x * x + x + x + 1.0);
        }
        return 0.0f;
    }

    double fSampleRate = 44100.0;
    double fPhase = 0.0;
    double fIncrement = 0.0;
};

} // namespace sideous
