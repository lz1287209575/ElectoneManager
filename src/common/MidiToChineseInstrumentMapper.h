#pragma once
#include <string>
#include <unordered_map>
#include <atomic>
#include "MidiMessage.h"

namespace em {

/// Parameters exposed to the SamplerEngine for expression control.
struct InstrumentParams {
    float pitchBend      = 0.0f;  ///< -1.0 to 1.0 (horizontal aftertouch)
    float vibratoDepth   = 0.0f;  ///< 0.0 to 1.0
    float filterCutoff   = 1.0f;  ///< 0.0 (fully closed) to 1.0 (fully open)
    float expressionGain = 1.0f;  ///< 0.0 to 1.0 (CC#11)
    int   programNumber  = 0;     ///< General MIDI program 0-127
    std::string instrumentName;   ///< Resolved Chinese instrument name
};

/// Maps ELS-02C MIDI messages to Chinese instrument parameters.
/// Thread-safe: params are written atomically from MIDI thread,
/// read from audio thread via atomicSnapshot().
class MidiToChineseInstrumentMapper {
public:
    MidiToChineseInstrumentMapper();

    /// Process an incoming MIDI message and update internal params.
    void process(const MidiMessage& msg) noexcept;

    /// Get a snapshot of the current params (for the audio thread).
    /// Note: uses acquire/release on individual members — safe for SPSC.
    InstrumentParams getParams() const noexcept;

    /// Map a General MIDI program number to a Chinese instrument name.
    static std::string programToChineseInstrument(int program) noexcept;

private:
    // Atomic storage for each param
    std::atomic<float> _pitchBend      {0.0f};
    std::atomic<float> _vibratoDepth   {0.0f};
    std::atomic<float> _filterCutoff   {1.0f};
    std::atomic<float> _expressionGain {1.0f};
    std::atomic<int>   _program        {0};

    static const std::unordered_map<int, std::string> kProgramMap;
};

} // namespace em
