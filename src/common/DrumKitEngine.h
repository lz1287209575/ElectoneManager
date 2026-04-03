#pragma once
#include "DrumPattern.h"
#include "DrumSequencer.h"
#include <cstdint>
#include <cmath>

namespace em {

// ─────────────────────────────────────────────────────────────────────────────
/// Drum kit synthesizer: 12 independent drum voices, each producing a
/// procedurally-generated percussion sound (no samples needed).
///
/// Audio-thread safe: no dynamic allocations in trigger() or render().
/// Each voice uses simple FM/subtractive synthesis:
///   - Kick: sine sweep (low freq) + click transient
///   - Snare: noise + sine body
///   - Hi-hat: band-passed noise
///   - Toms: sine with pitch decay
///   - Clap: filtered noise burst
///   - Cowbell/Ride/Crash: metallic FM synthesis
// ─────────────────────────────────────────────────────────────────────────────
class DrumKitEngine {
public:
    DrumKitEngine();
    ~DrumKitEngine() = default;

    /// Trigger a drum instrument.
    /// @param instrument  Index into kDrumInstruments (0-11)
    /// @param velocity    MIDI velocity (1-127)
    void trigger(int instrument, uint8_t velocity) noexcept;

    /// Render mixed drum audio into output buffer (ADDITIVE).
    /// Caller must have zeroed the buffer first.
    void render(float* outputBuffer, uint32_t numFrames, double sampleRate) noexcept;

    /// Set master volume (0.0 - 1.0).
    void setVolume(float vol) noexcept { _volume = vol; }
    float volume() const noexcept { return _volume; }

private:
    static constexpr int kMaxVoices = kNumDrumInstruments;  // 12

    /// Per-voice state (one for each drum instrument).
    struct Voice {
        bool     active     = false;
        float    velocity   = 0.0f;   ///< 0.0-1.0
        double   phase      = 0.0;    ///< Primary oscillator phase
        double   phase2     = 0.0;    ///< Secondary oscillator phase
        float    envLevel   = 0.0f;   ///< Amplitude envelope
        float    envDecay   = 0.995f; ///< Envelope decay coefficient
        float    pitchEnv   = 0.0f;   ///< Pitch envelope (for kicks/toms)
        float    pitchDecay = 0.99f;  ///< Pitch envelope decay
        float    noiseState = 0.0f;   ///< Noise filter state
        float    age        = 0.0f;   ///< Time since trigger (seconds)
        uint32_t noiseSeed  = 12345;  ///< Per-voice PRNG seed
    };

    Voice _voices[kMaxVoices] = {};
    float _volume = 0.7f;

    /// Simple PRNG for noise generation (audio thread safe).
    static float noise(uint32_t& seed) noexcept {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(static_cast<int32_t>(seed)) / 2147483648.0f;
    }

    /// Render a single voice for one sample.
    float renderVoiceSample(int instrument, Voice& v, double sampleRate) noexcept;
};

} // namespace em
