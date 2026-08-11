/*
 * Sideous Umbra - a handful of factory example presets, seeded into the
 * user's preset directory (see ui/PresetStore.hpp) the first time the UI
 * ever runs with an empty preset library. Compiled in (rather than shipped
 * as separate data files) so they exist identically across every build/
 * install of the plugin regardless of platform or plugin format - no
 * bundle-path discovery needed, they just get written out as ordinary,
 * user-editable ".supreset" files on first run and behave exactly like any
 * preset the user saves themselves from that point on.
 *
 * Spread across this instrument's whole range on purpose. The two gestures
 * that make Sideous Umbra sound like a cat rather than a synth blip are the
 * SwoopEnvelope (a one-shot pitch rise-then-fall fired on every note-on, see
 * dsp/MeowVoice.hpp) and the vowel drift that rides along with it (an open
 * mouth shape sliding toward a closed one, or vice versa, over the same
 * gesture) - so most of these presets are built by picking a point on the
 * swoop-amount/swoop-time/vowel-drift axes first, then dialing growl/
 * formant drive/envelope around that to match the character.
 */

#pragma once

#include "PresetStore.hpp"

#include <cstring>
#include <utility>
#include <vector>

namespace sideous {
namespace ui {

struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> overrides; // symbol -> value; everything else stays at that param's Params.hpp default
};

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets = {
        // Essentially the plugin's compiled-in defaults, spelled out
        // explicitly here as the reference point every other preset below is
        // a deliberate departure from: an "eh"-ish vowel with a moderate
        // drift, a 7-semitone upward swoop over 160ms, light growl, and a
        // fairly ordinary amp envelope
        { "Classic Meow", {
            { "vowel", 1.0f }, { "vowel_drift", 2.5f },
            { "formant_resonance", 0.42f }, { "formant_drive", 0.1f }, { "growl", 0.18f },
            { "swoop_amount", 7.0f }, { "swoop_time", 0.16f },
            { "vibrato_rate", 5.0f }, { "vibrato_depth", 0.3f },
            { "amp_attack", 0.02f }, { "amp_decay", 0.22f }, { "amp_sustain", 0.4f }, { "amp_release", 0.35f },
        }},
        // Short swoop time (50ms) and a snappy decay/release read as a quick
        // upward "chirp" rather than a drawn-out "mrreow"; vowel parked near
        // "ee" (index 2, brightest/most closed-front formants of the table)
        // for that small, high-pitched-reading timbre, growl kept very low
        // so it stays clean rather than raspy
        { "Kitten Chirp", {
            { "vowel", 2.1f }, { "vowel_drift", 0.8f },
            { "formant_resonance", 0.45f }, { "formant_drive", 0.05f }, { "growl", 0.06f },
            { "swoop_amount", 10.0f }, { "swoop_time", 0.05f },
            { "vibrato_rate", 7.0f }, { "vibrato_depth", 0.15f },
            { "amp_attack", 0.004f }, { "amp_decay", 0.08f }, { "amp_sustain", 0.15f }, { "amp_release", 0.12f },
            { "master_volume", 0.7f },
        }},
        // Swoop amount maxed out and slowed down (350ms rise) for a big,
        // deliberate glide; heavy growl plus high formant resonance/drive
        // pushes the tone toward rasp/hiss; vowel starts open ("ah") and
        // drifts hard negative (closing down) rather than the default's
        // positive drift, and the release is stretched out for a note that
        // trails off menacingly instead of cutting clean
        { "Angry Yowl", {
            { "vowel", 0.0f }, { "vowel_drift", -3.0f },
            { "formant_resonance", 0.75f }, { "formant_drive", 0.4f }, { "growl", 0.55f },
            { "swoop_amount", 12.0f }, { "swoop_time", 0.35f },
            { "vibrato_rate", 4.0f }, { "vibrato_depth", 0.4f },
            { "amp_attack", 0.01f }, { "amp_decay", 0.3f }, { "amp_sustain", 0.6f }, { "amp_release", 0.9f },
            { "master_drive", 0.2f },
        }},
        // Swoop amount pulled down near zero so the pitch barely moves at
        // all (a purr has no glide), vowel drift likewise near zero so the
        // mouth shape holds still; slow vibrato is the only real movement, a
        // long attack/sustain/release reads as a sustained rumble rather
        // than a discrete note, and a bit of growl stands in for purr texture
        { "Purr-ish Drone", {
            { "vowel", 3.0f }, { "vowel_drift", 0.2f },
            { "formant_resonance", 0.3f }, { "formant_drive", 0.08f }, { "growl", 0.35f },
            { "swoop_amount", 0.5f }, { "swoop_time", 0.5f },
            { "vibrato_rate", 1.0f }, { "vibrato_depth", 0.15f },
            { "amp_attack", 0.3f }, { "amp_decay", 0.4f }, { "amp_sustain", 0.9f }, { "amp_release", 1.5f },
        }},
        // Vowel drift maxed at +4 - the biggest mouth-shape swing the
        // instrument can do, "ah" sliding all the way toward "oo" over the
        // swoop gesture - paired with a slow-ish swoop and a long release so
        // there's time to actually hear the vowel move; vibrato depth is
        // pushed up for a pronounced, quavering wail
        { "Wailing Siamese", {
            { "vowel", 0.5f }, { "vowel_drift", 4.0f },
            { "formant_resonance", 0.5f }, { "formant_drive", 0.15f }, { "growl", 0.25f },
            { "swoop_amount", 9.0f }, { "swoop_time", 0.4f },
            { "vibrato_rate", 6.0f }, { "vibrato_depth", 1.2f },
            { "amp_attack", 0.05f }, { "amp_decay", 0.3f }, { "amp_sustain", 0.7f }, { "amp_release", 1.8f },
        }},
        // Formant resonance and drive both pushed high for a ringing,
        // synthetic edge on the vocal formants; fast vibrato plus a snappy,
        // near-instant swoop and envelope keep it feeling mechanical rather
        // than organic, and master drive adds an extra layer of digital grit
        { "Robo-Cat", {
            { "vowel", 2.5f }, { "vowel_drift", 1.0f },
            { "formant_resonance", 0.9f }, { "formant_drive", 0.8f }, { "growl", 0.1f },
            { "swoop_amount", 5.0f }, { "swoop_time", 0.08f },
            { "vibrato_rate", 11.0f }, { "vibrato_depth", 0.6f },
            { "amp_attack", 0.001f }, { "amp_decay", 0.1f }, { "amp_sustain", 0.5f }, { "amp_release", 0.2f },
            { "master_drive", 0.5f },
        }},
        // Swoop amount negative - the corner of the range the default
        // positive-only feel skips over - so pitch glides *down* instead of
        // the classic "mrreow" rise, reading as a descending whine/complaint
        // rather than a greeting; vowel drift also negative so the mouth
        // closes in step with the falling pitch
        { "Downward Whine", {
            { "vowel", 1.5f }, { "vowel_drift", -2.0f },
            { "formant_resonance", 0.4f }, { "formant_drive", 0.12f }, { "growl", 0.2f },
            { "swoop_amount", -10.0f }, { "swoop_time", 0.25f },
            { "vibrato_rate", 4.5f }, { "vibrato_depth", 0.35f },
            { "amp_attack", 0.02f }, { "amp_decay", 0.25f }, { "amp_sustain", 0.3f }, { "amp_release", 0.6f },
        }},
        // Velocity sensitivity dropped to near zero (soft and loud key hits
        // sound almost the same, unlike every other preset here) and the amp
        // curve pushed toward the slow-settling end - combined with a closed
        // "oo" vowel, minimal growl/drive, and a gentle swoop, this is the
        // softest, least aggressive corner of the parameter space
        { "Velvet Coo", {
            { "vowel", 4.0f }, { "vowel_drift", -1.0f },
            { "formant_resonance", 0.25f }, { "formant_drive", 0.02f }, { "growl", 0.05f },
            { "swoop_amount", 4.0f }, { "swoop_time", 0.3f },
            { "vibrato_rate", 3.0f }, { "vibrato_depth", 0.2f },
            { "amp_attack", 0.15f }, { "amp_decay", 0.3f }, { "amp_sustain", 0.6f }, { "amp_release", 0.8f },
            { "amp_curve", 0.7f }, { "velocity_sens", 0.1f },
            { "master_volume", 0.65f },
        }},
        // The opposite extreme from Velvet Coo: growl and both formant
        // knobs maxed near 1 for maximum rasp/ring, swoop amount at the
        // negative limit with an almost-instant rise for a harsh downward
        // spike rather than a glide, and a very short envelope so it hits
        // and cuts rather than sustains - the most abrasive preset here
        { "Feral Screech", {
            { "vowel", 0.0f }, { "vowel_drift", 3.0f },
            { "formant_resonance", 0.95f }, { "formant_drive", 0.9f }, { "growl", 0.9f },
            { "swoop_amount", -12.0f }, { "swoop_time", 0.02f },
            { "vibrato_rate", 8.0f }, { "vibrato_depth", 0.5f },
            { "amp_attack", 0.001f }, { "amp_decay", 0.05f }, { "amp_sustain", 0.1f }, { "amp_release", 0.15f },
            { "master_drive", 0.6f },
        }},
    };
    return presets;
}

// writes every factory preset to disk via the normal savePreset() path (so
// they're indistinguishable from user-saved presets from that point on) -
// call once, only when the preset library is empty (first run).
inline void seedFactoryPresets()
{
    for (const FactoryPreset& fp : factoryPresets())
    {
        float values[kParamCount];
        for (uint32_t i = 0; i < kParamCount; ++i)
            values[i] = getParamInfo(i).def;

        for (const auto& kv : fp.overrides)
        {
            for (uint32_t i = 0; i < kParamCount; ++i)
            {
                // kParamOutLevel is the eye-brightness meter, not a real
                // parameter - it has no symbol worth matching against and
                // must never be overridden by a factory preset
                if (i == kParamOutLevel)
                    continue;

                if (std::strcmp(getParamInfo(i).symbol, kv.first) == 0)
                {
                    values[i] = kv.second;
                    break;
                }
            }
        }

        savePreset(fp.name, values);
    }
}

} // namespace ui
} // namespace sideous
