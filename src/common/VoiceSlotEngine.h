#pragma once
#include "SamplerEngine.h"
#include <vector>
#include <memory>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace em {

/// Per-layer metadata (UI thread).
struct LayerInfo {
    std::string        instrumentName = "Sine";
    float              mixVolume      = 1.0f;
    bool               muted          = false;
    std::vector<float> waveformData;   ///< owned waveform buffer (each layer independent)
};

/// Thin wrapper: one voice slot owns up to 8 independent SamplerEngine layers.
/// Audio render mixes all unmuted layers into the output buffer.
/// MIDI fan-out sends noteOn/noteOff to every unmuted layer simultaneously.
class VoiceSlotEngine {
public:
    static constexpr int kMaxLayers = 8;

    VoiceSlotEngine() = default;
    ~VoiceSlotEngine() = default;

    // Non-copyable, movable
    VoiceSlotEngine(const VoiceSlotEngine&) = delete;
    VoiceSlotEngine& operator=(const VoiceSlotEngine&) = delete;
    VoiceSlotEngine(VoiceSlotEngine&&) = default;
    VoiceSlotEngine& operator=(VoiceSlotEngine&&) = default;

    // ── Layer management (UI thread) ─────────────────────────────────────

    /// Add a new layer. Returns its index, or -1 if at max.
    int addLayer() {
        if (static_cast<int>(_layers.size()) >= kMaxLayers) return -1;
        _layers.push_back(std::make_unique<SamplerEngine>());
        _infos.push_back({});
        return static_cast<int>(_layers.size()) - 1;
    }

    /// Remove layer at index. Returns false if out of range.
    bool removeLayer(int idx) {
        if (idx < 0 || idx >= static_cast<int>(_layers.size())) return false;
        if (_layers.size() <= 1) return false;  // keep at least one layer
        _layers.erase(_layers.begin() + idx);
        _infos.erase(_infos.begin() + idx);
        return true;
    }

    int layerCount() const { return static_cast<int>(_layers.size()); }

    SamplerEngine* layer(int idx) {
        if (idx < 0 || idx >= static_cast<int>(_layers.size())) return nullptr;
        return _layers[idx].get();
    }
    const SamplerEngine* layer(int idx) const {
        if (idx < 0 || idx >= static_cast<int>(_layers.size())) return nullptr;
        return _layers[idx].get();
    }

    LayerInfo& layerInfo(int idx)             { return _infos[idx]; }
    const LayerInfo& layerInfo(int idx) const { return _infos[idx]; }

    // ── MIDI fan-out (processing thread) ─────────────────────────────────

    void noteOn(uint8_t note, uint8_t velocity) noexcept {
        for (int i = 0; i < static_cast<int>(_layers.size()); ++i) {
            if (!_infos[i].muted)
                _layers[i]->noteOn(note, velocity);
        }
    }

    void noteOff(uint8_t note) noexcept {
        for (int i = 0; i < static_cast<int>(_layers.size()); ++i) {
            if (!_infos[i].muted)
                _layers[i]->noteOff(note);
        }
    }

    void allNotesOff() noexcept {
        for (auto& l : _layers)
            l->allNotesOff();
    }

    // ── Expression params broadcast (processing thread) ──────────────────

    void setPitchBend(float v)      noexcept { for (auto& l : _layers) l->setPitchBend(v); }
    void setVibratoDepth(float v)   noexcept { for (auto& l : _layers) l->setVibratoDepth(v); }
    void setFilterCutoff(float v)   noexcept { for (auto& l : _layers) l->setFilterCutoff(v); }
    void setExpressionGain(float v) noexcept { for (auto& l : _layers) l->setExpressionGain(v); }

    // ── Audio render (audio thread) ──────────────────────────────────────

    /// Mix all unmuted layers into outputBuffer (ADDITIVE — caller must zero first).
    void render(float* outputBuffer, uint32_t numFrames, double sampleRate) noexcept {
        if (_layers.empty()) return;

        // Ensure mix buffer is large enough
        if (_mixBuf.size() < numFrames)
            _mixBuf.resize(numFrames);

        bool first = true;
        for (int i = 0; i < static_cast<int>(_layers.size()); ++i) {
            if (_infos[i].muted) continue;

            float vol = _infos[i].mixVolume;

            if (first) {
                // First active layer: render directly into output
                _layers[i]->render(outputBuffer, numFrames, sampleRate);
                if (vol != 1.0f) {
                    for (uint32_t s = 0; s < numFrames; ++s)
                        outputBuffer[s] *= vol;
                }
                first = false;
            } else {
                // Subsequent layers: render into temp buffer, then accumulate
                std::memset(_mixBuf.data(), 0, numFrames * sizeof(float));
                _layers[i]->render(_mixBuf.data(), numFrames, sampleRate);
                for (uint32_t s = 0; s < numFrames; ++s)
                    outputBuffer[s] += _mixBuf[s] * vol;
            }
        }

        // If no layer was active, zero output
        if (first) {
            std::memset(outputBuffer, 0, numFrames * sizeof(float));
        }
    }

    // ── Test waveform generation ─────────────────────────────────────────

    /// Generate a test waveform with harmonics based on layer index.
    /// Layer 0: pure sine, Layer 1: sine+3rd, Layer 2: sine+3rd+5th, etc.
    static void generateLayerWaveform(int layerIndex, std::vector<float>& out,
                                       int sampleRate = 48000, float baseFreq = 440.0f)
    {
        generateWaveformWithHarmonics(layerIndex + 1, out, sampleRate, baseFreq);
    }

    /// Generate a test waveform with a specific number of odd harmonics.
    /// numHarmonics=1 → pure sine, 2 → sine+3rd (hollow), 3 → +5th (reedy), etc.
    static void generateWaveformWithHarmonics(int numHarmonics, std::vector<float>& out,
                                               int sampleRate = 48000, float baseFreq = 440.0f)
    {
        if (numHarmonics < 1) numHarmonics = 1;
        int len = sampleRate;  // 1 second
        out.resize(len);

        for (int i = 0; i < len; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            float sample = 0.0f;
            float ampSum = 0.0f;
            for (int h = 0; h < numHarmonics; ++h) {
                // Use odd harmonics: 1, 3, 5, 7, ...
                int harmonic = 1 + h * 2;
                float amp = 1.0f / harmonic;  // decreasing amplitude
                sample += amp * static_cast<float>(std::sin(2.0 * M_PI * baseFreq * harmonic * t));
                ampSum += amp;
            }
            // Normalize to ~0.5 peak
            out[i] = 0.5f * sample / std::max(ampSum, 0.001f);
        }
    }

    // ── Convenience: get layer names for LCD display ─────────────────────

    std::vector<std::string> getLayerNames() const {
        std::vector<std::string> names;
        names.reserve(_infos.size());
        for (auto& info : _infos)
            names.push_back(info.instrumentName);
        return names;
    }

private:
    std::vector<std::unique_ptr<SamplerEngine>> _layers;
    std::vector<LayerInfo>                       _infos;
    std::vector<float>                           _mixBuf;  // reusable temp buffer
};

} // namespace em
