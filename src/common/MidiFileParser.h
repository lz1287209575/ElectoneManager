#pragma once
#include "MidiMessage.h"
#include <string>
#include <vector>
#include <cstdint>

namespace em {

/// A single event in a parsed MIDI file.
struct MidiFileEvent {
    uint64_t   absTick  = 0;     ///< Absolute tick position
    MidiMessage msg;              ///< The MIDI message
    bool       isMeta   = false;  ///< True for meta events
    uint8_t    metaType = 0;      ///< Meta event type (if isMeta)
    std::vector<uint8_t> metaData; ///< Meta event data (e.g., tempo bytes)
};

/// A parsed MIDI track.
struct MidiTrack {
    std::vector<MidiFileEvent> events;
};

/// Parsed Standard MIDI File structure.
struct MidiFile {
    int format     = 0;        ///< SMF format (0 or 1)
    int numTracks  = 0;
    int ppqn       = 480;      ///< Pulses per quarter note (ticks per beat)
    std::vector<MidiTrack> tracks;

    /// Total duration in ticks.
    uint64_t totalTicks() const {
        uint64_t maxTick = 0;
        for (auto& tr : tracks) {
            if (!tr.events.empty()) {
                auto last = tr.events.back().absTick;
                if (last > maxTick) maxTick = last;
            }
        }
        return maxTick;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
/// Parse a Standard MIDI File (SMF format 0 or 1).
/// Returns true on success, filling `outFile`.
// ─────────────────────────────────────────────────────────────────────────────
bool parseMidiFile(const std::string& path, MidiFile& outFile);

} // namespace em
