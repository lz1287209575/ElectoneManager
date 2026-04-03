#pragma once
#include "RingBuffer.h"
#include "MidiMessage.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <cmath>

namespace em {

/// Transport command from UI thread to audio thread.
struct TransportCommand {
    enum Type : uint8_t {
        Play,
        Stop,
        SetTempo,
        SetPosition,
    };
    Type     type  = Play;
    float    value = 0.0f;  ///< tempo (BPM) or position (beat)
};

/// Beat information from audio thread to UI thread.
struct BeatInfo {
    int      bar         = 1;    ///< 1-based bar number
    int      beat        = 1;    ///< 1-based beat within bar
    int      beatsPerBar = 4;    ///< time signature numerator
    float    bpm         = 120.0f;
    bool     playing     = false;
    uint64_t totalTicks  = 0;    ///< absolute tick count since start
    double   totalBeats  = 0.0;  ///< absolute beat count (for progress)
};

/// Callback signature: called once per tick on the audio thread.
/// @param tick  Absolute tick count since transport started
using TickCallback = std::function<void(uint64_t tick)>;

// ─────────────────────────────────────────────────────────────────────────────
/// Master transport clock.  Runs on the audio thread (processAudioBlock).
/// Uses 480 PPQN (pulses per quarter note).
///
/// Communication:
///   UI thread → audio thread:  TransportCommand via RingBuffer
///   Audio thread → UI thread:  BeatInfo via RingBuffer (polled by timer)
///
/// The transport also sends MIDI Realtime messages (Clock/Start/Stop) to an
/// optional MIDI output for controlling external instruments.
// ─────────────────────────────────────────────────────────────────────────────
class TransportEngine {
public:
    static constexpr int kPPQN = 480;  ///< Pulses per quarter note

    TransportEngine() = default;

    // ── UI thread API ────────────────────────────────────────────────────────

    /// Send a play command to the audio thread.
    void play() noexcept {
        TransportCommand cmd;
        cmd.type = TransportCommand::Play;
        _cmdRing.push(cmd);
    }

    /// Send a stop command to the audio thread.
    void stop() noexcept {
        TransportCommand cmd;
        cmd.type = TransportCommand::Stop;
        _cmdRing.push(cmd);
    }

    /// Set tempo (BPM). Thread-safe.
    void setTempo(float bpm) noexcept {
        TransportCommand cmd;
        cmd.type  = TransportCommand::SetTempo;
        cmd.value = bpm;
        _cmdRing.push(cmd);
    }

    /// Set playback position (in beats from start). Thread-safe.
    void setPosition(float beat) noexcept {
        TransportCommand cmd;
        cmd.type  = TransportCommand::SetPosition;
        cmd.value = beat;
        _cmdRing.push(cmd);
    }

    /// Poll the latest beat info from the audio thread.
    /// Returns true if new info was available.
    bool pollBeatInfo(BeatInfo& out) noexcept {
        BeatInfo info;
        bool got = false;
        while (_beatRing.pop(info)) {
            out = info;
            got = true;
        }
        return got;
    }

    /// Register a tick callback (called on audio thread for each tick).
    void setTickCallback(TickCallback cb) { _tickCb = std::move(cb); }

    /// Set beats per bar (time signature numerator). Default 4.
    void setBeatsPerBar(int bpb) noexcept { _beatsPerBar.store(bpb, std::memory_order_relaxed); }

    /// Get current playing state (relaxed read, for UI display).
    bool isPlaying() const noexcept { return _playing.load(std::memory_order_relaxed); }

    /// Get current BPM (relaxed read).
    float currentBpm() const noexcept { return _bpm.load(std::memory_order_relaxed); }

    // ── MIDI output for external instrument sync ────────────────────────────

    /// Callback for sending raw MIDI bytes to an output port.
    using MidiOutputCallback = std::function<void(const uint8_t* data, int len)>;
    void setMidiOutputCallback(MidiOutputCallback cb) { _midiOutCb = std::move(cb); }

    // ── Audio thread API ─────────────────────────────────────────────────────

    /// Call this from the audio render callback.
    /// Advances the internal clock and fires tick callbacks.
    void processAudioBlock(uint32_t numFrames, double sampleRate) noexcept {
        // 1. Drain command ring
        TransportCommand cmd;
        while (_cmdRing.pop(cmd)) {
            switch (cmd.type) {
            case TransportCommand::Play:
                if (!_playing.load(std::memory_order_relaxed)) {
                    _playing.store(true, std::memory_order_relaxed);
                    _tickAccumulator = 0.0;
                    sendMidiRealtime(kMidiStart);
                }
                break;
            case TransportCommand::Stop:
                if (_playing.load(std::memory_order_relaxed)) {
                    _playing.store(false, std::memory_order_relaxed);
                    _tick = 0;
                    _beatCounter = 0;
                    sendMidiRealtime(kMidiStop);
                }
                break;
            case TransportCommand::SetTempo:
                if (cmd.value >= 20.0f && cmd.value <= 300.0f)
                    _bpm.store(cmd.value, std::memory_order_relaxed);
                break;
            case TransportCommand::SetPosition: {
                uint64_t newTick = static_cast<uint64_t>(cmd.value * kPPQN);
                _tick = newTick;
                _beatCounter = 0;
                break;
            }
            }
        }

        if (!_playing.load(std::memory_order_relaxed))
            return;

        // 2. Advance the clock
        float bpm = _bpm.load(std::memory_order_relaxed);
        double ticksPerSample = (bpm * kPPQN) / (60.0 * sampleRate);

        for (uint32_t i = 0; i < numFrames; ++i) {
            _tickAccumulator += ticksPerSample;

            while (_tickAccumulator >= 1.0) {
                _tickAccumulator -= 1.0;

                // Fire tick callback
                if (_tickCb) _tickCb(_tick);

                // MIDI Clock: 24 ppqn (every kPPQN/24 = 20 ticks)
                if ((_tick % (kPPQN / 24)) == 0) {
                    sendMidiRealtime(kMidiClock);
                }

                // Beat info update: once per beat
                if ((_tick % kPPQN) == 0) {
                    _beatCounter++;
                    int bpb = _beatsPerBar.load(std::memory_order_relaxed);
                    if (bpb < 1) bpb = 4;

                    BeatInfo info;
                    info.bar        = static_cast<int>((_beatCounter - 1) / bpb) + 1;
                    info.beat       = static_cast<int>((_beatCounter - 1) % bpb) + 1;
                    info.beatsPerBar = bpb;
                    info.bpm        = bpm;
                    info.playing    = true;
                    info.totalTicks = _tick;
                    info.totalBeats = static_cast<double>(_tick) / kPPQN;
                    _beatRing.push(info);
                }

                _tick++;
            }
        }
    }

private:
    void sendMidiRealtime(uint8_t byte) noexcept {
        if (_midiOutCb) {
            _midiOutCb(&byte, 1);
        }
    }

    // ── Command / beat ring buffers ──────────────────────────────────────────
    RingBuffer<TransportCommand, 64> _cmdRing;
    RingBuffer<BeatInfo, 64>         _beatRing;

    // ── Clock state (audio thread only) ──────────────────────────────────────
    uint64_t _tick           = 0;
    uint64_t _beatCounter    = 0;
    double   _tickAccumulator = 0.0;

    // ── Shared atomic params ─────────────────────────────────────────────────
    std::atomic<bool>  _playing     {false};
    std::atomic<float> _bpm         {120.0f};
    std::atomic<int>   _beatsPerBar {4};

    // ── Callbacks ────────────────────────────────────────────────────────────
    TickCallback        _tickCb;
    MidiOutputCallback  _midiOutCb;
};

} // namespace em
