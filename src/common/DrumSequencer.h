#pragma once
#include "DrumPattern.h"
#include "RingBuffer.h"
#include <cstdint>
#include <cstring>

namespace em {

/// A single drum trigger event (audio-thread safe, no allocations).
struct DrumTrigger {
    int     instrument = -1;   ///< Index into kDrumInstruments (0-11)
    uint8_t velocity   = 0;    ///< MIDI velocity (1-127)
};

/// Fixed-size buffer of drum triggers for one tick (audio-thread safe).
struct TriggerBuffer {
    static constexpr int kMaxTriggers = kNumDrumInstruments;  // max 12 simultaneous
    DrumTrigger triggers[kMaxTriggers] = {};
    int count = 0;

    void clear() noexcept { count = 0; }

    void add(int instrument, uint8_t velocity) noexcept {
        if (count < kMaxTriggers) {
            triggers[count++] = { instrument, velocity };
        }
    }
};

/// Command from UI thread to modify the sequencer pattern.
struct SeqEditCommand {
    enum Type : uint8_t {
        SetPattern,     ///< Load a new full pattern
        ToggleStep,     ///< Toggle a single step
        SetStep,        ///< Set a step to specific velocity
        ClearPattern,   ///< Clear the current pattern
    };
    Type    type       = SetPattern;
    int     instrument = 0;
    int     step       = 0;
    uint8_t velocity   = 0;
    DrumPattern pattern;  ///< Used by SetPattern
};

// ─────────────────────────────────────────────────────────────────────────────
/// Drum sequencer: processes transport ticks and emits drum triggers.
/// Designed for audio-thread use — no dynamic allocations in onTick().
///
/// UI edits are sent via a RingBuffer of SeqEditCommands.
// ─────────────────────────────────────────────────────────────────────────────
class DrumSequencer {
public:
    DrumSequencer() = default;

    // ── UI thread API ────────────────────────────────────────────────────────

    /// Load a new pattern (sent to audio thread via ring buffer).
    void setPattern(const DrumPattern& pat) noexcept {
        SeqEditCommand cmd;
        cmd.type    = SeqEditCommand::SetPattern;
        cmd.pattern = pat;
        _editRing.push(cmd);
    }

    /// Toggle a single step (sent to audio thread via ring buffer).
    void toggleStep(int instrument, int step) noexcept {
        SeqEditCommand cmd;
        cmd.type       = SeqEditCommand::ToggleStep;
        cmd.instrument = instrument;
        cmd.step       = step;
        _editRing.push(cmd);
    }

    /// Set a step to a specific velocity.
    void setStep(int instrument, int step, uint8_t velocity) noexcept {
        SeqEditCommand cmd;
        cmd.type       = SeqEditCommand::SetStep;
        cmd.instrument = instrument;
        cmd.step       = step;
        cmd.velocity   = velocity;
        _editRing.push(cmd);
    }

    /// Clear the current pattern.
    void clearPattern() noexcept {
        SeqEditCommand cmd;
        cmd.type = SeqEditCommand::ClearPattern;
        _editRing.push(cmd);
    }

    /// Get a read-only copy of the current pattern (for UI display).
    /// NOTE: This is not perfectly thread-safe with the audio thread writing,
    /// but DrumPattern is POD and worst case is a single-frame visual glitch.
    const DrumPattern& currentPattern() const noexcept { return _pattern; }

    /// Get the current step position (for UI playback cursor).
    int currentStep() const noexcept { return _lastStep; }

    // ── Audio thread API ─────────────────────────────────────────────────────

    /// Called once per transport tick.  Fills `out` with any triggers for this tick.
    /// @param tick   Absolute tick count from TransportEngine
    /// @param ppqn   Pulses per quarter note (480)
    /// @param out    TriggerBuffer to fill (caller must clear first)
    void onTick(uint64_t tick, int ppqn, TriggerBuffer& out) noexcept {
        // 1. Drain edit commands
        SeqEditCommand cmd;
        while (_editRing.pop(cmd)) {
            switch (cmd.type) {
            case SeqEditCommand::SetPattern:
                _pattern = cmd.pattern;
                break;
            case SeqEditCommand::ToggleStep:
                _pattern.toggle(cmd.instrument, cmd.step);
                break;
            case SeqEditCommand::SetStep:
                _pattern.set(cmd.instrument, cmd.step, cmd.velocity);
                break;
            case SeqEditCommand::ClearPattern:
                _pattern.clear();
                break;
            }
        }

        // 2. Check if this tick falls on a step boundary
        int step = _pattern.tickToStep(tick, ppqn);
        if (step < 0) return;

        _lastStep = step;

        // 3. Emit triggers for all instruments that have a hit on this step
        for (int inst = 0; inst < kNumDrumInstruments; ++inst) {
            uint8_t vel = _pattern.grid[inst][step];
            if (vel > 0) {
                out.add(inst, vel);
            }
        }
    }

private:
    DrumPattern _pattern;   ///< Current pattern (audio thread writes via ring)
    int         _lastStep = -1;

    /// Edit commands from UI thread (capacity must be power of 2).
    /// SeqEditCommand is large (contains DrumPattern ~420 bytes), so keep ring small.
    RingBuffer<SeqEditCommand, 16> _editRing;
};

} // namespace em
