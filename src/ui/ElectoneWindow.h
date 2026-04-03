#pragma once
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>

#include "UIEventQueue.h"
#include "LcdPanel.h"
#include "ControlPanel.h"
#include "FootpadPanel.h"
#include "VoiceEditorWindow.h"

#include "common/IMidiDevice.h"
#include "common/IAudioEngine.h"
#include "common/MidiToChineseInstrumentMapper.h"
#include "common/SamplerEngine.h"
#include "common/VoiceSlotEngine.h"
#include "common/TransportEngine.h"
#include "common/DrumPattern.h"
#include "common/DrumPatternBank.h"
#include "common/DrumSequencer.h"
#include "common/DrumKitEngine.h"
#include "common/MidiFileParser.h"
#include "common/MidiFilePlayer.h"
#include "common/IMidiOutput.h"

namespace em::ui {

// Forward declarations for Phase 4-5
class StepSequencerWindow;

/// Main ElectoneManager window.
/// Owns the UI layout and bridges FLTK events to the C++ audio engine.
class ElectoneWindow : public Fl_Window {
public:
    /// Construct the window.
    /// @param midi   Owning pointer to platform MIDI device (may be null)
    /// @param audio  Owning pointer to platform audio engine (may be null)
    ElectoneWindow(std::unique_ptr<em::IMidiDevice>  midi,
                   std::unique_ptr<em::IAudioEngine> audio);
    ~ElectoneWindow() override;

    /// Called once after show() to start audio/MIDI engines.
    bool startEngines();

    /// Stop engines (called on close).
    void stopEngines();

private:
    // ── Layout helpers ─────────────────────────────────────────────────────
    void buildLayout();
    void buildMenuBar();

    // ── FLTK timer callback (polls UIEventQueue ~ 60 Hz) ─────────────────
    static void timerCb(void* data);
    void        onTimer();

    // ── Menu callbacks ────────────────────────────────────────────────────
    static void menuCb(Fl_Widget* w, void* data);
    void        handleMenu(const char* path);

    // ── Engine event handlers ─────────────────────────────────────────────
    void onNoteEvent   (const NoteEvent&         ev);
    void onMidiMessage (const em::MidiMessage&   msg);
    void onAudioState  (const AudioState&        state);

    // ── Widgets ───────────────────────────────────────────────────────────
    Fl_Menu_Bar*               _menuBar    = nullptr;
    LcdPanel*                  _lcd        = nullptr;
    ControlPanel*              _ctrl       = nullptr;
    FootpadPanel*              _footpad    = nullptr;
    VoiceEditorWindow*         _voiceEditor = nullptr;
    StepSequencerWindow*       _stepSeqWin  = nullptr;

    // ── Backend ───────────────────────────────────────────────────────────
    std::unique_ptr<em::IMidiDevice>           _midi;
    std::unique_ptr<em::IAudioEngine>          _audio;
    em::MidiToChineseInstrumentMapper          _mapper;

    static constexpr int kNumSlots = 8;
    em::VoiceSlotEngine                        _slots[kNumSlots];

    // ── Rhythm / Transport engine ─────────────────────────────────────────
    em::TransportEngine                        _transport;
    em::DrumSequencer                          _drumSeq;
    em::DrumKitEngine                          _drumKit;
    em::MidiFilePlayer                         _midiPlayer;

    // ── MIDI output (Phase 2) ─────────────────────────────────────────────
    std::unique_ptr<em::IMidiOutput>           _midiOut;

    // ── Thread communication ──────────────────────────────────────────────
    UIEventQueue                               _queue;
    em::RingBuffer<em::MidiMessage, 256>       _midiQueue;

    // ── Processing thread ─────────────────────────────────────────────────
    std::thread                                _processingThread;
    std::atomic<bool>                          _stopProcessing{false};
    void processingThreadFunc();

    // ── State ─────────────────────────────────────────────────────────────
    LcdState    _lcdState;
    AudioState  _audioState;

    static constexpr double kTimerInterval = 1.0 / 60.0; // 60 Hz UI refresh
};

} // namespace em::ui
