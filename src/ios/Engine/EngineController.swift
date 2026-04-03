// EngineController.swift
// @Observable singleton that owns the C++ engine and drives the SwiftUI state machine.
// Requires iOS 17+ for @Observable macro.

import Foundation
import AVFoundation
import QuartzCore

@Observable
final class EngineController {

    // ── Shared singleton ─────────────────────────────────────────────────────
    static let shared = EngineController()

    // ── Published state (read by SwiftUI views) ─────────────────────────────
    var isPlaying: Bool = false
    var bpm: Float = 120.0
    var bar: Int = 1
    var beat: Int = 1
    var midiProgress: Float = 0.0
    var midiLoaded: Bool = false
    var audioRunning: Bool = false
    var midiConnected: Bool = false
    var latencyMs: Double = 0.0

    var currentStep: Int = -1
    var drumGrid: EMDrumGrid = EMDrumGrid()
    var currentPreset: Int = 3  // Pop

    // Voice slot names (8 slots)
    var slotNames: [String] = Array(repeating: "---", count: 8)
    var slotPrograms: [Int32] = Array(repeating: 0, count: 8)

    // MIDI port list
    var midiPorts: [String] = []

    // ── Engine pointer (also used by views for direct C calls) ───────────────
    var engine: OpaquePointer?
    private var displayLink: CADisplayLink?

    // ─────────────────────────────────────────────────────────────────────────
    private init() {}

    // ── Start/stop ───────────────────────────────────────────────────────────
    func start() {
        guard engine == nil else { return }
        engine = em_create()

        // Configure AVAudioSession for low-latency playback
        let session = AVAudioSession.sharedInstance()
        try? session.setCategory(.playback, mode: .default, options: [.mixWithOthers])
        try? session.setPreferredIOBufferDuration(Double(256) / Double(48000))
        try? session.setActive(true)

        em_start_audio(engine)
        em_start_midi(engine)

        // Refresh MIDI port list
        refreshMidiPorts()

        // Initial drum grid snapshot
        if let e = engine {
            em_drum_get_grid(e, &drumGrid)
        }

        // Initial slot names
        refreshSlotNames()

        // Start 60 Hz polling
        displayLink = CADisplayLink(target: self, selector: #selector(onDisplayLink))
        displayLink?.preferredFrameRateRange = CAFrameRateRange(minimum: 30, maximum: 60, preferred: 60)
        displayLink?.add(to: .main, forMode: .common)
    }

    func stop() {
        displayLink?.invalidate()
        displayLink = nil
        em_stop_audio(engine)
        em_stop_midi(engine)
        em_destroy(engine)
        engine = nil
    }

    // ── 60 Hz poll ────────────────────────────────────────────────────────────
    @objc private func onDisplayLink() {
        guard let e = engine else { return }

        var info = EMBeatInfo()
        em_poll_beat_info(e, &info)

        isPlaying     = info.isPlaying
        bpm           = info.bpm
        bar           = Int(info.bar)
        beat          = Int(info.beat)
        midiProgress  = info.midiProgress
        audioRunning  = info.audioRunning
        midiConnected = info.midiConnected
        latencyMs     = info.latencyMs
        currentStep   = Int(info.currentStep)
        midiLoaded    = em_midi_player_is_loaded(e)

        // Update drum grid (copy on every frame — it's a fixed 12×16 struct)
        em_drum_get_grid(e, &drumGrid)
    }

    // ── Transport actions ─────────────────────────────────────────────────────
    func play() {
        guard let e = engine else { return }
        if isPlaying {
            em_transport_stop(e)
        } else {
            em_transport_play(e)
        }
    }

    func stop_transport() {
        guard let e = engine else { return }
        em_transport_stop(e)
        em_midi_player_rewind(e)
    }

    func setTempo(_ newBpm: Float) {
        guard let e = engine else { return }
        em_transport_set_tempo(e, newBpm)
        bpm = newBpm
    }

    // ── Drum actions ──────────────────────────────────────────────────────────
    func selectPreset(_ index: Int) {
        guard let e = engine else { return }
        em_drum_set_preset(e, Int32(index))
        currentPreset = index
        em_drum_get_grid(e, &drumGrid)
    }

    func toggleStep(row: Int, step: Int) {
        guard let e = engine else { return }
        em_drum_toggle_step(e, Int32(row), Int32(step))
    }

    func clearPattern() {
        guard let e = engine else { return }
        em_drum_clear_pattern(e)
    }

    var presetCount: Int { Int(em_drum_preset_count()) }

    func presetName(_ index: Int) -> String {
        guard let cstr = em_drum_get_preset_name(Int32(index)) else { return "" }
        return String(cString: cstr)
    }

    // ── MIDI file actions ─────────────────────────────────────────────────────
    func loadMidi(url: URL) {
        guard let e = engine else { return }
        url.path.withCString { cpath in
            _ = em_midi_player_load(e, cpath)
        }
        midiLoaded = em_midi_player_is_loaded(e)
    }

    func rewindMidi() {
        guard let e = engine else { return }
        em_midi_player_rewind(e)
    }

    func seekMidi(fraction: Float) {
        guard let e = engine else { return }
        em_midi_player_seek(e, fraction)
    }

    // ── Voice slot actions ────────────────────────────────────────────────────
    func setSlotProgram(slot: Int, gmProgram: Int) {
        guard let e = engine, slot >= 0, slot < 8 else { return }
        em_voice_set_program(e, Int32(slot), Int32(gmProgram))
        slotPrograms[slot] = Int32(gmProgram)
        refreshSlotNames()
    }

    func allNotesOff(slot: Int) {
        guard let e = engine, slot >= 0, slot < 8 else { return }
        em_voice_all_notes_off(e, Int32(slot))
    }

    // ── MIDI port actions ─────────────────────────────────────────────────────
    func refreshMidiPorts() {
        guard let e = engine else { return }
        let count = Int(em_midi_port_count(e))
        midiPorts = (0..<count).map { idx in
            var buf = [CChar](repeating: 0, count: 128)
            em_midi_port_name(e, Int32(idx), &buf, 128)
            return String(cString: buf)
        }
    }

    func openMidiPort(_ index: Int) {
        guard let e = engine else { return }
        _ = em_midi_port_open(e, Int32(index))
    }

    // ── Helpers ───────────────────────────────────────────────────────────────
    private func refreshSlotNames() {
        guard let e = engine else { return }
        for slot in 0..<8 {
            var buf = [CChar](repeating: 0, count: 64)
            if em_voice_layer_count(e, Int32(slot)) > 0 {
                em_voice_get_layer_name(e, Int32(slot), 0, &buf, 64)
                slotNames[slot] = String(cString: buf)
            } else {
                slotNames[slot] = "---"
            }
        }
    }
}
