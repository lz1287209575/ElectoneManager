#include "DrumKitEngine.h"
#include <cstring>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace em {

DrumKitEngine::DrumKitEngine() {
    // Initialize all voices as inactive
    for (int i = 0; i < kMaxVoices; ++i) {
        _voices[i].active = false;
        _voices[i].noiseSeed = 12345u + static_cast<uint32_t>(i) * 7919u;
    }
}

void DrumKitEngine::trigger(int instrument, uint8_t velocity) noexcept {
    if (instrument < 0 || instrument >= kMaxVoices) return;
    if (velocity == 0) return;

    Voice& v    = _voices[instrument];
    v.active    = true;
    v.velocity  = velocity / 127.0f;
    v.phase     = 0.0;
    v.phase2    = 0.0;
    v.envLevel  = 1.0f;
    v.pitchEnv  = 1.0f;
    v.noiseState = 0.0f;
    v.age       = 0.0f;

    // Set envelope characteristics per instrument type
    // Instrument indices: 0=BD 1=SD 2=SS 3=CHH 4=OHH 5=PHH 6=CR 7=RD 8=LT 9=HT 10=CLP 11=CB
    switch (instrument) {
    case 0: // Bass Drum — slow decay, pitch sweep
        v.envDecay   = 0.9992f;
        v.pitchDecay = 0.997f;
        break;
    case 1: // Snare — medium decay
        v.envDecay   = 0.9985f;
        v.pitchDecay = 0.999f;
        break;
    case 2: // Side Stick — very short
        v.envDecay   = 0.998f;
        v.pitchDecay = 0.999f;
        break;
    case 3: // Closed Hi-Hat — short
        v.envDecay   = 0.998f;
        v.pitchDecay = 1.0f;
        break;
    case 4: // Open Hi-Hat — longer
        v.envDecay   = 0.9994f;
        v.pitchDecay = 1.0f;
        break;
    case 5: // Pedal Hi-Hat — medium short
        v.envDecay   = 0.9988f;
        v.pitchDecay = 1.0f;
        break;
    case 6: // Crash — long
        v.envDecay   = 0.99965f;
        v.pitchDecay = 1.0f;
        break;
    case 7: // Ride — medium long
        v.envDecay   = 0.9995f;
        v.pitchDecay = 1.0f;
        break;
    case 8: // Low Tom
        v.envDecay   = 0.999f;
        v.pitchDecay = 0.998f;
        break;
    case 9: // High Tom
        v.envDecay   = 0.999f;
        v.pitchDecay = 0.998f;
        break;
    case 10: // Clap — short burst with flutter
        v.envDecay   = 0.9985f;
        v.pitchDecay = 1.0f;
        break;
    case 11: // Cowbell — medium
        v.envDecay   = 0.999f;
        v.pitchDecay = 1.0f;
        break;
    }
}

float DrumKitEngine::renderVoiceSample(int instrument, Voice& v, double sampleRate) noexcept {
    if (!v.active) return 0.0f;

    float sample = 0.0f;
    float dt = 1.0f / static_cast<float>(sampleRate);
    constexpr float twoPi = static_cast<float>(2.0 * M_PI);

    switch (instrument) {
    case 0: { // Bass Drum: sine sweep from ~150Hz down to ~50Hz + click
        float freq = 50.0f + 100.0f * v.pitchEnv;
        v.phase += freq / sampleRate;
        if (v.phase > 1.0) v.phase -= 1.0;
        sample = std::sin(twoPi * static_cast<float>(v.phase));
        // Click transient in first 2ms
        if (v.age < 0.002f) {
            sample += 0.5f * noise(v.noiseSeed) * (1.0f - v.age / 0.002f);
        }
        sample *= 1.2f;  // Boost kick level
        break;
    }
    case 1: { // Snare: sine body (~200Hz) + noise
        float freq = 180.0f + 40.0f * v.pitchEnv;
        v.phase += freq / sampleRate;
        if (v.phase > 1.0) v.phase -= 1.0;
        float body = 0.4f * std::sin(twoPi * static_cast<float>(v.phase));
        float nz   = 0.6f * noise(v.noiseSeed);
        // Simple high-pass on noise
        float hp = nz - v.noiseState;
        v.noiseState = nz * 0.3f;
        sample = body + hp;
        break;
    }
    case 2: { // Side Stick: short click (high-freq sine + noise burst)
        float freq = 800.0f;
        v.phase += freq / sampleRate;
        if (v.phase > 1.0) v.phase -= 1.0;
        sample = 0.5f * std::sin(twoPi * static_cast<float>(v.phase));
        if (v.age < 0.003f) {
            sample += 0.5f * noise(v.noiseSeed);
        }
        break;
    }
    case 3: // Closed Hi-Hat: band-passed noise, short
    case 5: { // Pedal Hi-Hat: similar to closed
        float nz = noise(v.noiseSeed);
        // Simple band-pass: HP then LP simulation
        float hp = nz - v.noiseState;
        v.noiseState += (nz - v.noiseState) * 0.15f;
        sample = hp * 0.8f;
        break;
    }
    case 4: { // Open Hi-Hat: band-passed noise, longer
        float nz = noise(v.noiseSeed);
        float hp = nz - v.noiseState;
        v.noiseState += (nz - v.noiseState) * 0.08f;
        sample = hp * 0.7f;
        break;
    }
    case 6: { // Crash: metallic noise + FM synthesis
        float nz = noise(v.noiseSeed);
        float hp = nz - v.noiseState;
        v.noiseState += (nz - v.noiseState) * 0.05f;
        // Add metallic ring
        v.phase += 340.0 / sampleRate;
        v.phase2 += 800.0 / sampleRate;
        if (v.phase > 1.0) v.phase -= 1.0;
        if (v.phase2 > 1.0) v.phase2 -= 1.0;
        float ring = std::sin(twoPi * static_cast<float>(v.phase) +
                              2.0f * std::sin(twoPi * static_cast<float>(v.phase2)));
        sample = 0.4f * hp + 0.3f * ring;
        break;
    }
    case 7: { // Ride: metallic FM, cleaner than crash
        v.phase += 400.0 / sampleRate;
        v.phase2 += 620.0 / sampleRate;
        if (v.phase > 1.0) v.phase -= 1.0;
        if (v.phase2 > 1.0) v.phase2 -= 1.0;
        float ring = std::sin(twoPi * static_cast<float>(v.phase) +
                              1.5f * std::sin(twoPi * static_cast<float>(v.phase2)));
        float nz = 0.15f * noise(v.noiseSeed);
        sample = 0.6f * ring + nz;
        break;
    }
    case 8: { // Low Tom: sine ~100Hz with pitch decay
        float freq = 80.0f + 60.0f * v.pitchEnv;
        v.phase += freq / sampleRate;
        if (v.phase > 1.0) v.phase -= 1.0;
        sample = std::sin(twoPi * static_cast<float>(v.phase));
        // Slight noise transient
        if (v.age < 0.005f) {
            sample += 0.2f * noise(v.noiseSeed);
        }
        break;
    }
    case 9: { // High Tom: sine ~200Hz with pitch decay
        float freq = 160.0f + 80.0f * v.pitchEnv;
        v.phase += freq / sampleRate;
        if (v.phase > 1.0) v.phase -= 1.0;
        sample = std::sin(twoPi * static_cast<float>(v.phase));
        if (v.age < 0.004f) {
            sample += 0.2f * noise(v.noiseSeed);
        }
        break;
    }
    case 10: { // Clap: multi-burst noise
        float nz = noise(v.noiseSeed);
        // Create a "flutter" by gating noise in 3 short bursts
        float t = v.age;
        float gate = 0.0f;
        if (t < 0.01f)                           gate = 1.0f;
        else if (t > 0.015f && t < 0.025f)       gate = 0.8f;
        else if (t > 0.030f && t < 0.040f)       gate = 0.6f;
        else if (t > 0.045f)                      gate = 0.4f;  // tail
        sample = nz * gate;
        // HP filter
        float hp = sample - v.noiseState;
        v.noiseState += (sample - v.noiseState) * 0.2f;
        sample = hp;
        break;
    }
    case 11: { // Cowbell: two detuned square-ish waves
        float f1 = 545.0f, f2 = 815.0f;  // Classic 808 cowbell frequencies
        v.phase += f1 / sampleRate;
        v.phase2 += f2 / sampleRate;
        if (v.phase > 1.0) v.phase -= 1.0;
        if (v.phase2 > 1.0) v.phase2 -= 1.0;
        // Square wave approximation via hard clipping of sine
        float sq1 = (std::sin(twoPi * static_cast<float>(v.phase)) > 0.0f) ? 0.5f : -0.5f;
        float sq2 = (std::sin(twoPi * static_cast<float>(v.phase2)) > 0.0f) ? 0.5f : -0.5f;
        sample = 0.5f * (sq1 + sq2);
        break;
    }
    default:
        break;
    }

    // Apply envelope
    sample *= v.envLevel * v.velocity;
    v.envLevel  *= v.envDecay;
    v.pitchEnv  *= v.pitchDecay;
    v.age       += dt;

    // Kill voice when envelope is very low
    if (v.envLevel < 0.001f) {
        v.active = false;
    }

    return sample;
}

void DrumKitEngine::render(float* outputBuffer, uint32_t numFrames, double sampleRate) noexcept {
    for (uint32_t i = 0; i < numFrames; ++i) {
        float mix = 0.0f;
        for (int inst = 0; inst < kMaxVoices; ++inst) {
            if (_voices[inst].active) {
                mix += renderVoiceSample(inst, _voices[inst], sampleRate);
            }
        }
        outputBuffer[i] += mix * _volume;
    }
}

} // namespace em
