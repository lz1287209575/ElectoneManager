#pragma once
#include <cstdint>
#include <cstring>
#include <string>

namespace em {

/// GM Drum Map — standard note numbers for drum instruments.
struct DrumInstrument {
    uint8_t     note;       ///< MIDI note number (GM drum map)
    const char* name;       ///< Human-readable name
    const char* shortName;  ///< 3-4 char abbreviation for UI
};

/// 12 drum instruments used in the sequencer grid.
static constexpr int kNumDrumInstruments = 12;

static const DrumInstrument kDrumInstruments[kNumDrumInstruments] = {
    { 36, "Bass Drum 1",      "BD"  },
    { 38, "Snare Drum",       "SD"  },
    { 37, "Side Stick",       "SS"  },
    { 42, "Closed Hi-Hat",    "CHH" },
    { 46, "Open Hi-Hat",      "OHH" },
    { 44, "Pedal Hi-Hat",     "PHH" },
    { 49, "Crash Cymbal 1",   "CR"  },
    { 51, "Ride Cymbal 1",    "RD"  },
    { 45, "Low Tom",          "LT"  },
    { 48, "High Mid Tom",     "HT"  },
    { 39, "Hand Clap",        "CLP" },
    { 56, "Cowbell",          "CB"  },
};

/// Maximum number of steps in a drum pattern.
static constexpr int kMaxDrumSteps = 32;

// ─────────────────────────────────────────────────────────────────────────────
/// Drum pattern: a grid of 12 instruments × up to 32 steps.
/// Each cell holds a velocity (0 = off, 1-127 = on at that velocity).
/// POD-like: no heap allocations, safe for audio-thread use.
// ─────────────────────────────────────────────────────────────────────────────
struct DrumPattern {
    /// Pattern name (for UI display).
    char name[32] = {};

    /// Velocity grid: [instrument][step]. 0 = off.
    uint8_t grid[kNumDrumInstruments][kMaxDrumSteps] = {};

    /// Number of active steps in this pattern (8, 16, or 32).
    int steps       = 16;

    /// Steps per beat (4 = 16th notes, 2 = 8th notes).
    int stepsPerBeat = 4;

    /// Beats per bar (time signature numerator).
    int beatsPerBar  = 4;

    /// Default velocity for new hits.
    uint8_t defaultVelocity = 100;

    /// Clear the entire grid.
    void clear() noexcept {
        std::memset(grid, 0, sizeof(grid));
    }

    /// Set a single step. Returns the new velocity.
    uint8_t toggle(int instrument, int step) noexcept {
        if (instrument < 0 || instrument >= kNumDrumInstruments) return 0;
        if (step < 0 || step >= steps) return 0;
        if (grid[instrument][step] > 0) {
            grid[instrument][step] = 0;
            return 0;
        } else {
            grid[instrument][step] = defaultVelocity;
            return defaultVelocity;
        }
    }

    /// Set a step to a specific velocity.
    void set(int instrument, int step, uint8_t velocity) noexcept {
        if (instrument < 0 || instrument >= kNumDrumInstruments) return;
        if (step < 0 || step >= steps) return;
        grid[instrument][step] = velocity;
    }

    /// Get velocity at a step.
    uint8_t get(int instrument, int step) const noexcept {
        if (instrument < 0 || instrument >= kNumDrumInstruments) return 0;
        if (step < 0 || step >= steps) return 0;
        return grid[instrument][step];
    }

    /// Calculate which step index corresponds to a given transport tick.
    /// @param tick  Absolute tick count from TransportEngine
    /// @param ppqn  Pulses per quarter note (480)
    /// @return Step index (0..steps-1), or -1 if not on a step boundary
    int tickToStep(uint64_t tick, int ppqn) const noexcept {
        // Ticks per step = ppqn / stepsPerBeat
        int ticksPerStep = ppqn / stepsPerBeat;
        if (ticksPerStep <= 0) return -1;

        if ((tick % ticksPerStep) != 0) return -1;

        int totalStepsSinceStart = static_cast<int>(tick / ticksPerStep);
        return totalStepsSinceStart % steps;
    }
};

} // namespace em
