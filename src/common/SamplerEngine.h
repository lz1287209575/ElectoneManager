#pragma once
#include <vector>
#include <atomic>
#include <cstdint>
#include <cmath>

namespace em {

/// Single-waveform looping sampler with SVF filter, 2-band EQ, LFO, and
/// proper 4-stage ADSR.  Designed for real-time audio render callbacks.
///
/// Parameters are set atomically from the MIDI/processing thread and
/// consumed lock-free by the audio thread.
class SamplerEngine {
public:
    static constexpr int kMaxVoices = 8;

    SamplerEngine();
    ~SamplerEngine() = default;

    // ── Waveform data ──────────────────────────────────────────────────────
    void setWaveform(const float* samples, uint32_t numSamples, double sampleRate);
    void setLoopPoints(uint32_t loopStart, uint32_t loopEnd) noexcept;

    // ── Voice control (MIDI thread) ────────────────────────────────────────
    void noteOn(uint8_t note, uint8_t velocity)  noexcept;
    void noteOff(uint8_t note)                   noexcept;
    void allNotesOff()                           noexcept;

    // ── Expression params (MIDI/processing thread, atomic) ────────────────
    void setPitchBend(float bend)       noexcept;  ///< -1.0 to 1.0
    void setVibratoDepth(float depth)   noexcept;  ///< 0.0 to 1.0
    void setFilterCutoff(float norm)    noexcept;  ///< 0.0 to 1.0 normalized
    void setExpressionGain(float gain)  noexcept;  ///< 0.0 to 1.0

    // ── ADSR params (real-time safe atomic stores) ────────────────────────
    void setAdsrAttack (float seconds)  noexcept;  ///< 0.001 – 2.0 s
    void setAdsrDecay  (float seconds)  noexcept;  ///< 0.001 – 2.0 s
    void setAdsrSustain(float level)    noexcept;  ///< 0.0 – 1.0
    void setAdsrRelease(float seconds)  noexcept;  ///< 0.001 – 5.0 s

    // ── New ELS-02C params ────────────────────────────────────────────────
    // Page 1: Basic
    void setPan(float pan)              noexcept;  ///< -1.0 (L) to 1.0 (R)
    void setOctaveShift(int shift)      noexcept;  ///< -2 to +2
    void setReverbSend(float send)      noexcept;  ///< 0.0 – 1.0

    // Page 2: Vibrato / Slide
    void setVibratoDelay(float seconds) noexcept;  ///< 0.0 – 2.0 s
    void setVibratoSpeed(float hz)      noexcept;  ///< Hz

    // Page 5: Fine tune
    void setFineTune(float cents)       noexcept;  ///< -64 to +63 cents

    // Page 6: Voice Edit
    void setResonance(float norm)       noexcept;  ///< 0.0 – 1.0 normalized
    void setEqLowFreq(float hz)         noexcept;  ///< 32 – 2000 Hz
    void setEqLowGain(float dB)         noexcept;  ///< -12 – +12 dB
    void setEqHighFreq(float hz)        noexcept;  ///< 500 – 16000 Hz
    void setEqHighGain(float dB)        noexcept;  ///< -12 – +12 dB
    void setLfoWave(int wave)           noexcept;  ///< 0=sine,1=saw,2=tri,3=squ,4=rnd
    void setLfoSpeed(float hz)          noexcept;  ///< Hz
    void setLfoPmd(float depth)         noexcept;  ///< pitch mod depth 0–400
    void setLfoFmd(float depth)         noexcept;  ///< filter mod depth 0–4800
    void setLfoAmd(float depth)         noexcept;  ///< amplitude mod depth 0–128

    // ── Getters (for editor initialisation, UI thread) ────────────────────
    float getExpressionGain() const noexcept { return _expressionGain.load(std::memory_order_relaxed); }
    float getFilterCutoff()   const noexcept { return _filterCutoff.load  (std::memory_order_relaxed); }
    float getVibratoDepth()   const noexcept { return _vibratoDepth.load  (std::memory_order_relaxed); }
    float getPitchBend()      const noexcept { return _pitchBend.load     (std::memory_order_relaxed); }
    float getAdsrAttack()     const noexcept { return _adsrAttackS.load   (std::memory_order_relaxed); }
    float getAdsrDecay()      const noexcept { return _adsrDecayS.load    (std::memory_order_relaxed); }
    float getAdsrSustain()    const noexcept { return _adsrSustainV.load  (std::memory_order_relaxed); }
    float getAdsrRelease()    const noexcept { return _adsrReleaseS.load  (std::memory_order_relaxed); }

    // New getters
    float getPan()            const noexcept { return _pan.load           (std::memory_order_relaxed); }
    int   getOctaveShift()    const noexcept { return _octaveShift.load   (std::memory_order_relaxed); }
    float getReverbSend()     const noexcept { return _reverbSend.load    (std::memory_order_relaxed); }
    float getVibratoDelay()   const noexcept { return _vibratoDelay.load  (std::memory_order_relaxed); }
    float getVibratoSpeed()   const noexcept { return _vibratoSpeed.load  (std::memory_order_relaxed); }
    float getFineTune()       const noexcept { return _fineTune.load      (std::memory_order_relaxed); }
    float getResonance()      const noexcept { return _resonance.load     (std::memory_order_relaxed); }
    float getEqLowFreq()      const noexcept { return _eqLowFreq.load    (std::memory_order_relaxed); }
    float getEqLowGain()      const noexcept { return _eqLowGain.load    (std::memory_order_relaxed); }
    float getEqHighFreq()     const noexcept { return _eqHighFreq.load   (std::memory_order_relaxed); }
    float getEqHighGain()     const noexcept { return _eqHighGain.load   (std::memory_order_relaxed); }
    int   getLfoWave()        const noexcept { return _lfoWave.load       (std::memory_order_relaxed); }
    float getLfoSpeed()       const noexcept { return _lfoSpeed.load      (std::memory_order_relaxed); }
    float getLfoPmd()         const noexcept { return _lfoPmd.load        (std::memory_order_relaxed); }
    float getLfoFmd()         const noexcept { return _lfoFmd.load        (std::memory_order_relaxed); }
    float getLfoAmd()         const noexcept { return _lfoAmd.load        (std::memory_order_relaxed); }

    // ── Audio thread ──────────────────────────────────────────────────────
    void render(float* outputBuffer, uint32_t numFrames, double sampleRate) noexcept;

private:
    // ── Voice state ────────────────────────────────────────────────────────
    struct Voice {
        bool     active      = false;
        uint8_t  note        = 0;
        float    velocity    = 0.0f;
        double   phase       = 0.0;
        double   phaseStep   = 1.0;
        // Proper 4-stage ADSR
        enum class Env { Attack, Decay, Sustain, Release, Idle } envState = Env::Idle;
        float    envLevel    = 0.0f;
        // Per-voice SVF filter state
        float    svfLow      = 0.0f;
        float    svfBand     = 0.0f;
        // Per-voice vibrato delay counter (seconds elapsed since note-on)
        float    noteAge     = 0.0f;
        // Per-voice EQ biquad states (low shelf)
        float    eqLoX1 = 0, eqLoX2 = 0, eqLoY1 = 0, eqLoY2 = 0;
        // Per-voice EQ biquad states (high shelf)
        float    eqHiX1 = 0, eqHiX2 = 0, eqHiY1 = 0, eqHiY2 = 0;
    };

    Voice _voices[kMaxVoices] {};

    // ── Waveform ──────────────────────────────────────────────────────────
    const float* _waveData    = nullptr;
    uint32_t     _waveLen     = 0;
    double       _waveSR      = 48000.0;
    uint32_t     _loopStart   = 0;
    uint32_t     _loopEnd     = 0;

    // ── Atomic expression params ──────────────────────────────────────────
    std::atomic<float> _pitchBend      {0.0f};
    std::atomic<float> _vibratoDepth   {0.0f};
    std::atomic<float> _filterCutoff   {1.0f};
    std::atomic<float> _expressionGain {1.0f};

    // ── Atomic ADSR params ────────────────────────────────────────────────
    std::atomic<float> _adsrAttackS    {0.005f};
    std::atomic<float> _adsrDecayS     {0.100f};
    std::atomic<float> _adsrSustainV   {1.000f};
    std::atomic<float> _adsrReleaseS   {0.080f};

    // ── New atomic params ─────────────────────────────────────────────────
    std::atomic<float> _pan            {0.0f};     // -1.0 (L) to 1.0 (R)
    std::atomic<int>   _octaveShift    {0};        // -2 to +2
    std::atomic<float> _fineTune       {0.0f};     // -64 to +63 cents
    std::atomic<float> _reverbSend     {0.5f};     // 0.0–1.0
    std::atomic<float> _vibratoDelay   {0.0f};     // seconds
    std::atomic<float> _vibratoSpeed   {5.5f};     // Hz
    std::atomic<float> _resonance      {0.0f};     // 0.0–1.0 normalized
    std::atomic<float> _eqLowGain      {0.0f};     // -12 to +12 dB
    std::atomic<float> _eqHighGain     {0.0f};     // -12 to +12 dB
    std::atomic<float> _eqLowFreq      {200.0f};   // Hz
    std::atomic<float> _eqHighFreq     {4000.0f};  // Hz
    std::atomic<int>   _lfoWave        {0};        // 0=sine,1=saw,2=tri,3=squ,4=rnd
    std::atomic<float> _lfoSpeed       {5.5f};     // Hz
    std::atomic<float> _lfoPmd         {0.0f};     // pitch mod depth
    std::atomic<float> _lfoFmd         {0.0f};     // filter mod depth
    std::atomic<float> _lfoAmd         {0.0f};     // amplitude mod depth

    // ── LFO state (audio thread only) ─────────────────────────────────────
    double _lfoPhase = 0.0;
    uint32_t _lfoRandSeed = 12345;

    // ── Internal helpers ──────────────────────────────────────────────────
    float sampleInterpolated(double pos) const noexcept;
    static float noteToHz(uint8_t note) noexcept;

    /// Generate LFO waveform value (-1..+1) for given phase (0..1) and wave type.
    float lfoWaveform(double phase, int waveType) noexcept;
};

} // namespace em
