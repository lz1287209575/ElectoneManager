#pragma once
#include <vector>
#include <mutex>
#include <functional>
#include "common/MidiMessage.h"
#include "common/RingBuffer.h"

namespace em::ui {

/// Audio / MIDI engine state snapshot for the UI thread.
struct AudioState {
    bool    running         = false;
    double  latencyMs       = 0.0;
    double  sampleRate      = 0.0;
    uint32_t bufferSize     = 0;
    bool    underrun        = false;
    std::string deviceName;
};

/// Note-on/off state for keyboard highlight.
struct NoteEvent {
    uint8_t note     = 0;
    uint8_t velocity = 0;
    bool    isOn     = false;
};

// ──────────────────────────────────────────────────────────────────────────────
/// Thread-safe one-way event channel: audio/MIDI threads → FLTK UI thread.
/// Uses a lock-free RingBuffer for NoteEvents and a mutex-protected queue for
/// less-frequent AudioState updates (state changes are rare, not per-sample).
// ──────────────────────────────────────────────────────────────────────────────

class UIEventQueue {
public:
    // ── Called from MIDI / processing thread ──────────────────────────────
    void postNote(uint8_t note, uint8_t velocity, bool isOn) noexcept {
        NoteEvent ev{note, velocity, isOn};
        _noteRing.push(ev); // lock-free
    }

    void postMidiMessage(const em::MidiMessage& msg) noexcept {
        _midiRing.push(msg); // lock-free
    }

    void postAudioState(const AudioState& state) {
        std::lock_guard<std::mutex> lk(_stateMutex);
        _pendingState = state;
        _stateUpdated = true;
    }

    // ── Called from FLTK UI main thread (timer callback) ──────────────────
    void pollNotes(std::vector<NoteEvent>& out) noexcept {
        NoteEvent ev;
        while (_noteRing.pop(ev)) out.push_back(ev);
    }

    void pollMidiMessages(std::vector<em::MidiMessage>& out) noexcept {
        em::MidiMessage msg;
        while (_midiRing.pop(msg)) out.push_back(msg);
    }

    bool pollAudioState(AudioState& out) {
        std::lock_guard<std::mutex> lk(_stateMutex);
        if (!_stateUpdated) return false;
        out = _pendingState;
        _stateUpdated = false;
        return true;
    }

private:
    em::RingBuffer<NoteEvent,       512>  _noteRing;
    em::RingBuffer<em::MidiMessage, 256>  _midiRing;

    std::mutex  _stateMutex;
    AudioState  _pendingState;
    bool        _stateUpdated = false;
};

} // namespace em::ui
