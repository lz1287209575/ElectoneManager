#include "SamplerEngine.h"
#include <cmath>
#include <cstring>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace em {

// ──────────────────────────────────────────────────────────────────────────────
SamplerEngine::SamplerEngine() {
    allNotesOff();
}

void SamplerEngine::setWaveform(const float* samples, uint32_t numSamples,
                                 double sampleRate) {
    _waveData  = samples;
    _waveLen   = numSamples;
    _waveSR    = sampleRate;
    _loopStart = 0;
    _loopEnd   = (numSamples > 0) ? numSamples - 1 : 0;
}

void SamplerEngine::setLoopPoints(uint32_t loopStart, uint32_t loopEnd) noexcept {
    if (_waveLen == 0) return;
    _loopStart = std::min(loopStart, _waveLen - 1);
    _loopEnd   = std::min(loopEnd,   _waveLen - 1);
    if (_loopStart > _loopEnd) std::swap(_loopStart, _loopEnd);
}

// ── Voice control ─────────────────────────────────────────────────────────

void SamplerEngine::noteOn(uint8_t note, uint8_t velocity) noexcept {
    int slot = -1;
    for (int i = 0; i < kMaxVoices; ++i) {
        if (!_voices[i].active) { slot = i; break; }
    }
    if (slot < 0) slot = 0; // steal first voice

    Voice& v       = _voices[slot];
    v.active       = true;
    v.note         = note;
    v.velocity     = velocity / 127.0f;
    v.phase        = _loopStart;
    v.envState     = Voice::Env::Attack;
    v.envLevel     = 0.0f;
    v.svfLow       = 0.0f;
    v.svfBand      = 0.0f;
    v.noteAge      = 0.0f;
    v.eqLoX1 = v.eqLoX2 = v.eqLoY1 = v.eqLoY2 = 0.0f;
    v.eqHiX1 = v.eqHiX2 = v.eqHiY1 = v.eqHiY2 = 0.0f;
}

void SamplerEngine::noteOff(uint8_t note) noexcept {
    for (auto& v : _voices) {
        if (v.active && v.note == note && v.envState != Voice::Env::Release) {
            v.envState = Voice::Env::Release;
        }
    }
}

void SamplerEngine::allNotesOff() noexcept {
    for (auto& v : _voices) {
        v.active   = false;
        v.envState = Voice::Env::Idle;
        v.envLevel = 0.0f;
        v.svfLow   = 0.0f;
        v.svfBand  = 0.0f;
        v.noteAge  = 0.0f;
    }
}

// ── Expression setters ───────────────────────────────────────────────────────

void SamplerEngine::setPitchBend(float bend) noexcept {
    _pitchBend.store(bend, std::memory_order_release);
}
void SamplerEngine::setVibratoDepth(float depth) noexcept {
    _vibratoDepth.store(depth, std::memory_order_release);
}
void SamplerEngine::setFilterCutoff(float norm) noexcept {
    _filterCutoff.store(norm, std::memory_order_release);
}
void SamplerEngine::setExpressionGain(float gain) noexcept {
    _expressionGain.store(gain, std::memory_order_release);
}

void SamplerEngine::setAdsrAttack(float seconds) noexcept {
    _adsrAttackS.store(seconds, std::memory_order_release);
}
void SamplerEngine::setAdsrDecay(float seconds) noexcept {
    _adsrDecayS.store(seconds, std::memory_order_release);
}
void SamplerEngine::setAdsrSustain(float level) noexcept {
    _adsrSustainV.store(level, std::memory_order_release);
}
void SamplerEngine::setAdsrRelease(float seconds) noexcept {
    _adsrReleaseS.store(seconds, std::memory_order_release);
}

// ── New ELS-02C param setters ────────────────────────────────────────────────

void SamplerEngine::setPan(float pan) noexcept {
    _pan.store(std::clamp(pan, -1.0f, 1.0f), std::memory_order_release);
}
void SamplerEngine::setOctaveShift(int shift) noexcept {
    _octaveShift.store(std::clamp(shift, -2, 2), std::memory_order_release);
}
void SamplerEngine::setReverbSend(float send) noexcept {
    _reverbSend.store(std::clamp(send, 0.0f, 1.0f), std::memory_order_release);
}
void SamplerEngine::setVibratoDelay(float seconds) noexcept {
    _vibratoDelay.store(std::max(seconds, 0.0f), std::memory_order_release);
}
void SamplerEngine::setVibratoSpeed(float hz) noexcept {
    _vibratoSpeed.store(std::max(hz, 0.1f), std::memory_order_release);
}
void SamplerEngine::setFineTune(float cents) noexcept {
    _fineTune.store(std::clamp(cents, -64.0f, 63.0f), std::memory_order_release);
}
void SamplerEngine::setResonance(float norm) noexcept {
    _resonance.store(std::clamp(norm, 0.0f, 1.0f), std::memory_order_release);
}
void SamplerEngine::setEqLowFreq(float hz) noexcept {
    _eqLowFreq.store(std::clamp(hz, 32.0f, 2000.0f), std::memory_order_release);
}
void SamplerEngine::setEqLowGain(float dB) noexcept {
    _eqLowGain.store(std::clamp(dB, -12.0f, 12.0f), std::memory_order_release);
}
void SamplerEngine::setEqHighFreq(float hz) noexcept {
    _eqHighFreq.store(std::clamp(hz, 500.0f, 16000.0f), std::memory_order_release);
}
void SamplerEngine::setEqHighGain(float dB) noexcept {
    _eqHighGain.store(std::clamp(dB, -12.0f, 12.0f), std::memory_order_release);
}
void SamplerEngine::setLfoWave(int wave) noexcept {
    _lfoWave.store(std::clamp(wave, 0, 4), std::memory_order_release);
}
void SamplerEngine::setLfoSpeed(float hz) noexcept {
    _lfoSpeed.store(std::max(hz, 0.1f), std::memory_order_release);
}
void SamplerEngine::setLfoPmd(float depth) noexcept {
    _lfoPmd.store(std::max(depth, 0.0f), std::memory_order_release);
}
void SamplerEngine::setLfoFmd(float depth) noexcept {
    _lfoFmd.store(std::max(depth, 0.0f), std::memory_order_release);
}
void SamplerEngine::setLfoAmd(float depth) noexcept {
    _lfoAmd.store(std::max(depth, 0.0f), std::memory_order_release);
}

// ── LFO waveform generator ──────────────────────────────────────────────────

float SamplerEngine::lfoWaveform(double phase, int waveType) noexcept {
    // phase is 0..1, output is -1..+1
    switch (waveType) {
    case 0: // sine
        return static_cast<float>(std::sin(2.0 * M_PI * phase));
    case 1: // sawtooth (ramp up)
        return static_cast<float>(2.0 * phase - 1.0);
    case 2: // triangle
        return static_cast<float>(phase < 0.5 ? (4.0 * phase - 1.0) : (3.0 - 4.0 * phase));
    case 3: // square
        return (phase < 0.5f) ? 1.0f : -1.0f;
    case 4: { // random (sample-and-hold style, update each cycle)
        // Simple pseudo-random using xorshift
        _lfoRandSeed ^= _lfoRandSeed << 13;
        _lfoRandSeed ^= _lfoRandSeed >> 17;
        _lfoRandSeed ^= _lfoRandSeed << 5;
        return static_cast<float>(_lfoRandSeed) / static_cast<float>(0xFFFFFFFFu) * 2.0f - 1.0f;
    }
    default:
        return 0.0f;
    }
}

// ── Render ───────────────────────────────────────────────────────────────────

void SamplerEngine::render(float* outputBuffer, uint32_t numFrames,
                            double sampleRate) noexcept {
    if (_waveData == nullptr || _waveLen == 0) return;

    // ── Read all atomics once per render block ──────────────────────────
    const float pitchBend    = _pitchBend.load(std::memory_order_acquire);
    const float vibratoDepth = _vibratoDepth.load(std::memory_order_acquire);
    const float filterNorm   = _filterCutoff.load(std::memory_order_acquire);
    const float exprGain     = _expressionGain.load(std::memory_order_acquire);

    const float pan          = _pan.load(std::memory_order_acquire);
    const int   octaveShift  = _octaveShift.load(std::memory_order_acquire);
    const float fineTune     = _fineTune.load(std::memory_order_acquire);
    const float vibDelay     = _vibratoDelay.load(std::memory_order_acquire);
    const float vibSpeed     = _vibratoSpeed.load(std::memory_order_acquire);
    const float resonance    = _resonance.load(std::memory_order_acquire);

    const float eqLoFreq     = _eqLowFreq.load(std::memory_order_acquire);
    const float eqLoGain     = _eqLowGain.load(std::memory_order_acquire);
    const float eqHiFreq     = _eqHighFreq.load(std::memory_order_acquire);
    const float eqHiGain     = _eqHighGain.load(std::memory_order_acquire);

    const int   lfoWaveType  = _lfoWave.load(std::memory_order_acquire);
    const float lfoSpeedHz   = _lfoSpeed.load(std::memory_order_acquire);
    const float lfoPmd       = _lfoPmd.load(std::memory_order_acquire);
    const float lfoFmd       = _lfoFmd.load(std::memory_order_acquire);
    const float lfoAmd       = _lfoAmd.load(std::memory_order_acquire);

    // ── ADSR coefficients ───────────────────────────────────────────────
    const float attackSec    = _adsrAttackS.load  (std::memory_order_relaxed);
    const float decaySec     = _adsrDecayS.load   (std::memory_order_relaxed);
    const float sustainLevel = _adsrSustainV.load (std::memory_order_relaxed);
    const float releaseSec   = _adsrReleaseS.load (std::memory_order_relaxed);

    const float attackCoeff  = 1.0f / (attackSec  * static_cast<float>(sampleRate));
    const float decayCoeff   = 1.0f / (decaySec   * static_cast<float>(sampleRate));
    const float releaseCoeff = 1.0f / (releaseSec * static_cast<float>(sampleRate));

    // ── SVF filter coefficients ─────────────────────────────────────────
    // Map filterNorm [0,1] → cutoff frequency [200 Hz, sampleRate*0.45]
    const float minHz  = 200.0f;
    const float maxHz  = static_cast<float>(sampleRate) * 0.45f;
    const float cutoffHz = minHz + filterNorm * (maxHz - minHz);
    // Resonance: map 0..1 → Q from 0.5 (no resonance) to 20 (high resonance)
    const float qVal = 0.5f + resonance * 19.5f;
    const float svfQ = 1.0f / qVal;

    // ── Low-shelf EQ biquad coefficients ────────────────────────────────
    // Using simple first-order shelving approximation for efficiency
    float eqLoA[3] = {1, 0, 0}, eqLoB[3] = {1, 0, 0};
    {
        float A  = std::pow(10.0f, eqLoGain / 40.0f); // sqrt of linear gain
        float w0 = 2.0f * static_cast<float>(M_PI) * eqLoFreq / static_cast<float>(sampleRate);
        float cs = std::cos(w0);
        float sn = std::sin(w0);
        float al = sn / (2.0f * 0.707f); // Q = 0.707 (Butterworth)
        float t  = 2.0f * std::sqrt(A) * al;
        float a0 = (A+1) + (A-1)*cs + t;
        eqLoB[0] = (A * ((A+1) - (A-1)*cs + t)) / a0;
        eqLoB[1] = (2*A * ((A-1) - (A+1)*cs))   / a0;
        eqLoB[2] = (A * ((A+1) - (A-1)*cs - t)) / a0;
        eqLoA[0] = 1.0f;
        eqLoA[1] = (-2 * ((A-1) + (A+1)*cs))    / a0;
        eqLoA[2] = ((A+1) + (A-1)*cs - t)       / a0;
    }

    // ── High-shelf EQ biquad coefficients ───────────────────────────────
    float eqHiA[3] = {1, 0, 0}, eqHiB[3] = {1, 0, 0};
    {
        float A  = std::pow(10.0f, eqHiGain / 40.0f);
        float w0 = 2.0f * static_cast<float>(M_PI) * eqHiFreq / static_cast<float>(sampleRate);
        float cs = std::cos(w0);
        float sn = std::sin(w0);
        float al = sn / (2.0f * 0.707f);
        float t  = 2.0f * std::sqrt(A) * al;
        float a0 = (A+1) - (A-1)*cs + t;
        eqHiB[0] = (A * ((A+1) + (A-1)*cs + t)) / a0;
        eqHiB[1] = (-2*A * ((A-1) + (A+1)*cs))  / a0;
        eqHiB[2] = (A * ((A+1) + (A-1)*cs - t)) / a0;
        eqHiA[0] = 1.0f;
        eqHiA[1] = (2 * ((A-1) - (A+1)*cs))     / a0;
        eqHiA[2] = ((A+1) - (A-1)*cs - t)       / a0;
    }

    // ── Pan coefficients (constant-power panning) ───────────────────────
    const float panAngle = (pan + 1.0f) * 0.5f; // 0..1
    const float panL = std::cos(panAngle * static_cast<float>(M_PI) * 0.5f);
    const float panR = std::sin(panAngle * static_cast<float>(M_PI) * 0.5f);

    // ── LFO increment ───────────────────────────────────────────────────
    const double lfoInc = lfoSpeedHz / sampleRate;

    // ── Per-sample inverse ──────────────────────────────────────────────
    const float invSR = 1.0f / static_cast<float>(sampleRate);

    for (uint32_t frame = 0; frame < numFrames; ++frame) {
        // ── LFO ─────────────────────────────────────────────────────────
        float lfoVal = lfoWaveform(_lfoPhase, lfoWaveType);
        _lfoPhase = std::fmod(_lfoPhase + lfoInc, 1.0);

        // LFO modulation amounts (normalized)
        // PMD: map 0–400 to ±semitones of pitch modulation
        const float lfoPitchMod  = lfoVal * (lfoPmd / 400.0f) * 2.0f; // ±2 semitones max
        // FMD: map 0–4800 to filter cutoff modulation in normalized units
        const float lfoFilterMod = lfoVal * (lfoFmd / 4800.0f);
        // AMD: map 0–128 to amplitude modulation depth
        const float lfoAmpMod    = 1.0f - (lfoAmd / 128.0f) * (1.0f - (lfoVal + 1.0f) * 0.5f);

        // ── Vibrato (shared phase, separate from LFO) ───────────────────
        // Reuse vibrato from the old code but with configurable speed
        static double vibratoPhase = 0.0;
        const double  vibratoInc   = vibSpeed / sampleRate;

        const double vibratoMod =
            vibratoDepth * std::sin(2.0 * M_PI * vibratoPhase);
        vibratoPhase = std::fmod(vibratoPhase + vibratoInc, 1.0);

        // ── SVF cutoff with LFO filter modulation ───────────────────────
        float modCutoffHz = cutoffHz * (1.0f + lfoFilterMod);
        modCutoffHz = std::clamp(modCutoffHz, 20.0f, maxHz);
        float modSvfF = 2.0f * std::sin(static_cast<float>(M_PI) * modCutoffHz
                                          / static_cast<float>(sampleRate));
        modSvfF = std::min(modSvfF, 1.9f); // stability clamp

        float mixL = 0.0f, mixR = 0.0f;

        for (auto& v : _voices) {
            if (!v.active) continue;

            // ── Envelope (proper 4-stage ADSR) ──────────────────────────
            switch (v.envState) {
            case Voice::Env::Attack:
                v.envLevel += attackCoeff;
                if (v.envLevel >= 1.0f) {
                    v.envLevel = 1.0f;
                    v.envState = Voice::Env::Decay;
                }
                break;
            case Voice::Env::Decay:
                v.envLevel -= decayCoeff * (v.envLevel - sustainLevel);
                if (v.envLevel <= sustainLevel + 0.001f) {
                    v.envLevel = sustainLevel;
                    v.envState = Voice::Env::Sustain;
                }
                break;
            case Voice::Env::Sustain:
                v.envLevel = sustainLevel;
                break;
            case Voice::Env::Release:
                v.envLevel -= releaseCoeff;
                if (v.envLevel <= 0.0f) {
                    v.envLevel = 0.0f;
                    v.envState = Voice::Env::Idle;
                    v.active   = false;
                    continue;
                }
                break;
            default:
                v.active = false;
                continue;
            }

            // ── Vibrato with delay ──────────────────────────────────────
            v.noteAge += invSR;
            float effectiveVibratoMod = 0.0f;
            if (v.noteAge >= vibDelay) {
                effectiveVibratoMod = static_cast<float>(vibratoMod) * 0.5f;
            }

            // ── Pitch (note + bend + octave shift + fine tune + vibrato + LFO) ──
            const float pitchSemitones =
                static_cast<float>(v.note - 69)
                + pitchBend * 2.0f
                + static_cast<float>(octaveShift * 12)
                + fineTune / 100.0f
                + effectiveVibratoMod
                + lfoPitchMod;
            const double pitchRatio = std::pow(2.0, pitchSemitones / 12.0);
            v.phaseStep = pitchRatio * (_waveSR / sampleRate);

            // ── Sample read (linear interpolation) ──────────────────────
            float s = sampleInterpolated(v.phase);

            // ── SVF filter (2-pole state variable) ──────────────────────
            v.svfLow  += modSvfF * v.svfBand;
            float svfHigh = s - v.svfLow - svfQ * v.svfBand;
            v.svfBand += modSvfF * svfHigh;
            s = v.svfLow; // low-pass output

            // ── 2-band EQ (biquad shelving filters) ─────────────────────
            // Low shelf
            {
                float y = eqLoB[0]*s + eqLoB[1]*v.eqLoX1 + eqLoB[2]*v.eqLoX2
                        - eqLoA[1]*v.eqLoY1 - eqLoA[2]*v.eqLoY2;
                v.eqLoX2 = v.eqLoX1; v.eqLoX1 = s;
                v.eqLoY2 = v.eqLoY1; v.eqLoY1 = y;
                s = y;
            }
            // High shelf
            {
                float y = eqHiB[0]*s + eqHiB[1]*v.eqHiX1 + eqHiB[2]*v.eqHiX2
                        - eqHiA[1]*v.eqHiY1 - eqHiA[2]*v.eqHiY2;
                v.eqHiX2 = v.eqHiX1; v.eqHiX1 = s;
                v.eqHiY2 = v.eqHiY1; v.eqHiY1 = y;
                s = y;
            }

            // ── Gain (envelope × velocity × expression × LFO amp mod) ──
            const float sample = s * v.envLevel * v.velocity * exprGain * lfoAmpMod;

            // ── Pan ─────────────────────────────────────────────────────
            mixL += sample * panL;
            mixR += sample * panR;

            // ── Advance phase ───────────────────────────────────────────
            v.phase += v.phaseStep;
            const double loopLen = static_cast<double>(_loopEnd - _loopStart);
            if (loopLen > 0 && v.phase > static_cast<double>(_loopEnd)) {
                v.phase -= loopLen;
            }
        }

        // Write interleaved stereo (accumulate)
        outputBuffer[frame * 2 + 0] += mixL;
        outputBuffer[frame * 2 + 1] += mixR;
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

float SamplerEngine::sampleInterpolated(double pos) const noexcept {
    if (_waveLen == 0) return 0.0f;
    const uint32_t i0 = static_cast<uint32_t>(pos) % _waveLen;
    const uint32_t i1 = (i0 + 1) % _waveLen;
    const float    t  = static_cast<float>(pos - static_cast<double>(i0));
    return _waveData[i0] * (1.0f - t) + _waveData[i1] * t;
}

float SamplerEngine::noteToHz(uint8_t note) noexcept {
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

} // namespace em
