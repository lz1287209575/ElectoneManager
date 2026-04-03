#pragma once
#include "MidiFileParser.h"
#include "MidiMessage.h"
#include <vector>
#include <algorithm>
#include <functional>
#include <cstdint>

namespace em {

/// Callback for emitting note events during playback.
using MidiPlayerNoteCallback = std::function<void(const MidiMessage& msg)>;

/// Callback for tempo changes embedded in the MIDI file.
using MidiPlayerTempoCallback = std::function<void(float bpm)>;

// ─────────────────────────────────────────────────────────────────────────────
/// MIDI file playback engine.
/// Driven by TransportEngine ticks.  On each tick, emits any MIDI events
/// whose absolute tick position has been reached.
///
/// Not audio-thread safe by itself — designed to be called from the
/// TransportEngine tick callback (which runs on the audio thread).
/// All callbacks should be lock-free and fast.
// ─────────────────────────────────────────────────────────────────────────────
class MidiFilePlayer {
public:
    MidiFilePlayer() = default;

    /// Load a parsed MIDI file.  Merges all tracks into a single sorted event list.
    bool load(const MidiFile& file) {
        _events.clear();
        _cursor   = 0;
        _loaded   = false;
        _ppqn     = file.ppqn;
        _totalTicks = file.totalTicks();

        // Merge all tracks into one sorted event list
        for (auto& track : file.tracks) {
            for (auto& ev : track.events) {
                _events.push_back(ev);
            }
        }

        // Sort by absolute tick
        std::sort(_events.begin(), _events.end(),
            [](const MidiFileEvent& a, const MidiFileEvent& b) {
                return a.absTick < b.absTick;
            });

        _loaded = !_events.empty();
        return _loaded;
    }

    /// Load from a file path (convenience).
    bool loadFile(const std::string& path) {
        MidiFile file;
        if (!parseMidiFile(path, file)) return false;
        return load(file);
    }

    /// Set callbacks.
    void setNoteCallback(MidiPlayerNoteCallback cb)   { _noteCb  = std::move(cb); }
    void setTempoCallback(MidiPlayerTempoCallback cb) { _tempoCb = std::move(cb); }

    /// Called once per transport tick (audio thread).
    /// Emits all events at this tick position.
    /// @param tick  Absolute tick from TransportEngine
    void onTick(uint64_t tick) {
        if (!_loaded) return;

        // Scale tick from transport PPQN (480) to file PPQN if different
        // Both use 480 by default, but handle mismatch
        uint64_t fileTick = tick;
        if (_ppqn != 480 && _ppqn > 0) {
            fileTick = (tick * _ppqn) / 480;
        }

        while (_cursor < _events.size() && _events[_cursor].absTick <= fileTick) {
            const auto& ev = _events[_cursor];

            if (ev.isMeta) {
                // Handle tempo meta events (type 0x51)
                if (ev.metaType == 0x51 && ev.metaData.size() == 3 && _tempoCb) {
                    uint32_t usPerBeat =
                        (static_cast<uint32_t>(ev.metaData[0]) << 16) |
                        (static_cast<uint32_t>(ev.metaData[1]) << 8)  |
                         ev.metaData[2];
                    if (usPerBeat > 0) {
                        float bpm = 60000000.0f / static_cast<float>(usPerBeat);
                        _tempoCb(bpm);
                    }
                }
            } else {
                // Channel message → emit
                if (_noteCb) _noteCb(ev.msg);
            }

            _cursor++;
        }
    }

    /// Reset playback to the beginning.
    void rewind() { _cursor = 0; }

    /// Seek to a specific tick position.
    void seekToTick(uint64_t tick) {
        // Find the first event at or after this tick
        _cursor = 0;
        uint64_t fileTick = tick;
        if (_ppqn != 480 && _ppqn > 0) {
            fileTick = (tick * _ppqn) / 480;
        }
        while (_cursor < _events.size() && _events[_cursor].absTick < fileTick) {
            _cursor++;
        }
    }

    /// Get playback progress as a fraction [0.0, 1.0].
    float getProgressFraction() const {
        if (!_loaded || _totalTicks == 0) return 0.0f;
        if (_cursor >= _events.size()) return 1.0f;
        float pos = static_cast<float>(_events[_cursor].absTick);
        return pos / static_cast<float>(_totalTicks);
    }

    /// Get total duration in beats.
    double totalBeats() const {
        if (_ppqn <= 0) return 0.0;
        return static_cast<double>(_totalTicks) / _ppqn;
    }

    bool isLoaded() const { return _loaded; }
    bool isFinished() const { return _loaded && _cursor >= _events.size(); }

private:
    std::vector<MidiFileEvent> _events;
    size_t                     _cursor     = 0;
    bool                       _loaded     = false;
    int                        _ppqn       = 480;
    uint64_t                   _totalTicks = 0;

    MidiPlayerNoteCallback  _noteCb;
    MidiPlayerTempoCallback _tempoCb;
};

} // namespace em
