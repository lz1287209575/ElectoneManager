// em_bridge.cpp — Implementation of the C bridge layer for iOS, macOS, and Flutter.
// Mirrors ElectoneWindow::startEngines() wiring, but without any FLTK dependency.
#if defined(EM_IOS) || defined(EM_MACOS) || defined(EM_FLUTTER)

#include "em_bridge.h"

#include "common/SamplerEngine.h"
#include "common/VoiceSlotEngine.h"
#include "common/TransportEngine.h"
#include "common/DrumPattern.h"
#include "common/DrumPatternBank.h"
#include "common/DrumSequencer.h"
#include "common/DrumKitEngine.h"
#include "common/MidiFileParser.h"
#include "common/MidiFilePlayer.h"
#include "common/MidiToChineseInstrumentMapper.h"
#include "common/IMidiDevice.h"
#include "common/IAudioEngine.h"
#include "common/IMidiOutput.h"
#include "common/RingBuffer.h"
#include "common/MidiMessage.h"

#include "platform/apple/CoreAudioEngine.h"
#include "platform/apple/CoreMidiDevice.h"
#include "platform/apple/CoreMidiOutput.h"

#include <cstring>
#include <cstdio>
#include <algorithm>
#include <atomic>
#include <mutex>

// ─── Global log ring buffer ─────────────────────────────────────────────────
// Lock-free single-producer (audio thread printf), single-consumer (Dart poll).
// Uses a fixed 256-slot ring of 128-byte lines.
namespace {
    constexpr int kLogSlots = 256;
    constexpr int kLogLineLen = 128;
    char     gLogBuf[kLogSlots][kLogLineLen];
    std::atomic<int> gLogWrite{0};
    std::atomic<int> gLogRead{0};

    // Called from audio thread — must be lock-free and fast.
    void em_log(const char* line) {
        int w = gLogWrite.load(std::memory_order_relaxed);
        int next = (w + 1) % kLogSlots;
        // If full, advance read pointer to drop oldest
        if (next == gLogRead.load(std::memory_order_acquire))
            gLogRead.fetch_add(1, std::memory_order_relaxed);
        std::strncpy(gLogBuf[w], line, kLogLineLen - 1);
        gLogBuf[w][kLogLineLen - 1] = '\0';
        gLogWrite.store(next, std::memory_order_release);
    }
} // namespace
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

// ─── Preset name table ────────────────────────────────────────────────────────
static const char* kPresetNames[] = {
    "March", "Waltz", "Swing", "Pop", "R&B", "Latin", "Ethnic"
};
static constexpr int kNumPresets = 7;

// ─── Engine struct ────────────────────────────────────────────────────────────
struct EMEngine {
    // Platform layer
    em::CoreAudioEngine                      audio;
    std::unique_ptr<em::CoreMidiDevice>      midi;
    std::unique_ptr<em::CoreMidiOutput>      midiOut;

    // Common layer
    em::TransportEngine                      transport;
    em::DrumSequencer                        drumSeq;
    em::DrumKitEngine                        drumKit;
    em::MidiFilePlayer                       midiPlayer;
    em::VoiceSlotEngine                      slots[8];
    em::MidiToChineseInstrumentMapper        mapper;

    // MIDI ring buffer
    em::RingBuffer<em::MidiMessage, 256>     midiRing;

    // Processing thread
    std::thread                              processingThread;
    std::atomic<bool>                        stopProcessing{false};

    // Cached state for poll
    std::atomic<int>                         currentPreset{3}; // Pop default
    std::atomic<int>                         currentProgram[8];
    std::atomic<uint8_t>                     lastProgramChange[8];  // Track incoming PC from piano

    // Last beat info (for em_poll_beat_info)
    em::BeatInfo                             lastBeat;
    float                                    midiProgress{0.f};

    EMEngine() {
        for (int i = 0; i < 8; ++i) currentProgram[i].store(0);
    }
};

// ─── Lifecycle ────────────────────────────────────────────────────────────────
EMEngine* em_create() {
    return new EMEngine();
}

void em_destroy(EMEngine* e) {
    if (!e) return;
    em_stop_audio(e);
    em_stop_midi(e);
    delete e;
}

bool em_start_audio(EMEngine* e) {
    if (!e) return false;

    // Default slot 0: Strings voice
    e->slots[0].addLayer();
    {
        auto& info = e->slots[0].layerInfo(0);
        em::VoiceSlotEngine::generateWaveformWithHarmonics(
            5, info.waveformData, EM_SAMPLE_RATE, 440.0f);
        e->slots[0].layer(0)->setWaveform(
            info.waveformData.data(),
            static_cast<uint32_t>(info.waveformData.size()),
            EM_SAMPLE_RATE);
        info.instrumentName = "Strings 1";
    }

    // Default drum pattern (Pop)
    auto defaultPattern = em::DrumPatternBank::getPreset(3);
    e->drumSeq.setPattern(defaultPattern);
    e->transport.setTempo(120.0f);
    e->transport.setBeatsPerBar(4);

    // MIDI file player callbacks
    e->midiPlayer.setNoteCallback([e](const em::MidiMessage& msg) {
        uint8_t channel = msg.channel & 0x0F;
        int slotIdx = (channel < 8) ? channel : 0;
        char line[kLogLineLen];

        // ── Method C: Forward MID events to hardware piano via MIDI Output ───
        // If a MIDI output port is open, send the raw bytes to the ELS-02C so
        // the piano plays/changes voice in sync with the app progress bar.
        // Falls back to internal SamplerEngine slots when no piano is connected.
        bool forwardedToHardware = false;
        if (e->midiOut && e->midiOut->isOpen()) {
            uint8_t raw[3];
            int len = msg.toRaw(raw);
            e->midiOut->sendRaw(raw, len);
            forwardedToHardware = true;
        }

        if (msg.isNoteOn()) {
            if (!forwardedToHardware)
                e->slots[slotIdx].noteOn(msg.data1, msg.data2);
            std::snprintf(line, sizeof(line), "[MID] NoteOn  ch=%d note=%3d vel=%3d%s",
                          channel + 1, msg.data1, msg.data2,
                          forwardedToHardware ? " →HW" : "");
        } else if (msg.isNoteOff()) {
            if (!forwardedToHardware)
                e->slots[slotIdx].noteOff(msg.data1);
            std::snprintf(line, sizeof(line), "[MID] NoteOff ch=%d note=%3d%s",
                          channel + 1, msg.data1,
                          forwardedToHardware ? " →HW" : "");
        } else if (msg.isProgramChange()) {
            e->lastProgramChange[slotIdx].store(msg.data1, std::memory_order_relaxed);
            std::snprintf(line, sizeof(line), "[MID] PC      ch=%d prog=%3d%s",
                          channel + 1, msg.data1,
                          forwardedToHardware ? " →HW" : "");
        } else if (msg.isControlChange()) {
            std::snprintf(line, sizeof(line), "[MID] CC      ch=%d cc=%3d val=%3d%s",
                          channel + 1, msg.data1, msg.data2,
                          forwardedToHardware ? " →HW" : "");
        } else {
            std::snprintf(line, sizeof(line), "[MID] MSG     ch=%d type=0x%02X d1=%3d d2=%3d%s",
                          channel + 1,
                          static_cast<uint8_t>(msg.type),
                          msg.data1, msg.data2,
                          forwardedToHardware ? " →HW" : "");
        }
        em_log(line);
    });
    e->midiPlayer.setTempoCallback([e](float bpm) {
        e->transport.setTempo(bpm);
        char line[kLogLineLen];
        std::snprintf(line, sizeof(line), "[MID] Tempo   bpm=%.1f", bpm);
        em_log(line);
    });

    // Transport tick → drum seq + MIDI player
    e->transport.setTickCallback([e](uint64_t tick) {
        em::TriggerBuffer triggers;
        triggers.clear();
        e->drumSeq.onTick(tick, em::TransportEngine::kPPQN, triggers);
        for (int i = 0; i < triggers.count; ++i)
            e->drumKit.trigger(triggers.triggers[i].instrument,
                               triggers.triggers[i].velocity);
        e->midiPlayer.onTick(tick);
    });

    // Audio render callback
    e->audio.setBufferSize(EM_BUFFER_SIZE);
    e->audio.setSampleRate(EM_SAMPLE_RATE);
    e->audio.selectDefaultYamahaDevice();
    e->audio.setRenderCallback([e](float* out, uint32_t n, double sr) {
        std::memset(out, 0, n * sizeof(float));
        e->transport.processAudioBlock(n, sr);
        e->drumKit.render(out, n, sr);
        for (int s = 0; s < 8; ++s)
            e->slots[s].render(out, n, sr);
    });

    // Processing thread
    e->stopProcessing = false;
    e->processingThread = std::thread([e]() {
        em::MidiMessage msg;
        while (!e->stopProcessing) {
            while (e->midiRing.pop(msg)) {
                // Route to appropriate slot based on MIDI channel
                // Channels 0-7 map to slots 0-7 (Upper1/Upper2/Lower1/Lower2/Lead1/Lead2/Pedal1/Pedal2)
                uint8_t channel = msg.channel & 0x0F;
                int slotIdx = (channel < 8) ? channel : 0;
                
                // Process expression params through mapper
                e->mapper.process(msg);
                auto params = e->mapper.getParams();
                
                // Apply expression params to the appropriate slot
                e->slots[slotIdx].setPitchBend(params.pitchBend);
                e->slots[slotIdx].setVibratoDepth(params.vibratoDepth);
                e->slots[slotIdx].setFilterCutoff(params.filterCutoff);
                e->slots[slotIdx].setExpressionGain(params.expressionGain);
                
                // Handle note on/off
                if (msg.isNoteOn())
                    e->slots[slotIdx].noteOn(msg.data1, msg.data2);
                else if (msg.isNoteOff())
                    e->slots[slotIdx].noteOff(msg.data1);
                
                // Track program changes for UI feedback
                if (msg.isProgramChange()) {
                    e->lastProgramChange[slotIdx] = msg.data1;
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    });

    return e->audio.startStream();
}

void em_stop_audio(EMEngine* e) {
    if (!e) return;
    e->transport.stop();
    e->stopProcessing = true;
    if (e->processingThread.joinable())
        e->processingThread.join();
    e->audio.stopStream();
}

bool em_start_midi(EMEngine* e) {
    if (!e) return false;
    e->midi = std::make_unique<em::CoreMidiDevice>();
    auto ports = e->midi->getAvailablePorts();
    bool opened = !ports.empty() &&
                  (e->midi->openPortByName("Yamaha")   ||
                   e->midi->openPortByName("Electone") ||
                   e->midi->openPort(0));
    e->midi->setMessageCallback([e](const em::MidiMessage& msg) {
        e->midiRing.push(msg);
    });
    if (opened) e->midi->startListening();
    return opened;
}

void em_stop_midi(EMEngine* e) {
    if (!e || !e->midi) return;
    e->midi->stopListening();
    e->midi->closePort();
    e->midi.reset();
}

// ─── Transport ────────────────────────────────────────────────────────────────
void em_transport_play(EMEngine* e)                    { if (e) e->transport.play(); }
void em_transport_stop(EMEngine* e)                    { if (e) e->transport.stop(); }
void em_transport_set_tempo(EMEngine* e, float bpm)    { if (e) e->transport.setTempo(bpm); }
void em_transport_set_position(EMEngine* e, float b)   { if (e) e->transport.setPosition(b); }
bool em_transport_is_playing(EMEngine* e)              { return e && e->transport.isPlaying(); }
float em_transport_get_tempo(EMEngine* e)              { return e ? e->transport.currentBpm() : 120.f; }

// ─── Beat info poll ───────────────────────────────────────────────────────────
void em_poll_beat_info(EMEngine* e, EMBeatInfo* out) {
    if (!e || !out) return;
    em::BeatInfo info;
    if (e->transport.pollBeatInfo(info))
        e->lastBeat = info;

    // Update midi progress
    if (e->midiPlayer.isLoaded()) {
        double total = e->midiPlayer.totalBeats();
        if (total > 0.0)
            e->midiProgress = static_cast<float>(e->lastBeat.totalBeats / total);
        e->midiProgress = std::min(e->midiProgress, 1.f);
    }

    out->bpm           = e->lastBeat.bpm;
    out->bar           = e->lastBeat.bar;
    out->beat          = e->lastBeat.beat;
    out->isPlaying     = e->lastBeat.playing;
    out->midiProgress  = e->midiProgress;
    out->currentStep   = e->drumSeq.currentStep();
    out->audioRunning  = e->audio.isRunning();
    out->midiConnected = e->midi && e->midi->isOpen();
    out->latencyMs     = e->audio.getLatencyMs();
}

// ─── Drum machine ─────────────────────────────────────────────────────────────
int   em_drum_preset_count()                        { return kNumPresets; }
const char* em_drum_get_preset_name(int i)          { return (i >= 0 && i < kNumPresets) ? kPresetNames[i] : ""; }

void em_drum_set_preset(EMEngine* e, int idx) {
    if (!e || idx < 0 || idx >= kNumPresets) return;
    auto pat = em::DrumPatternBank::getPreset(idx);
    e->drumSeq.setPattern(pat);
    e->transport.setBeatsPerBar(pat.beatsPerBar);
    e->currentPreset.store(idx);
}

int em_drum_get_preset(EMEngine* e) {
    return e ? e->currentPreset.load() : 0;
}

void em_drum_toggle_step(EMEngine* e, int row, int step) {
    if (e) e->drumSeq.toggleStep(row, step);
}

void em_drum_clear_pattern(EMEngine* e) {
    if (e) e->drumSeq.clearPattern();
}

void em_drum_get_grid(EMEngine* e, EMDrumGrid* out) {
    if (!e || !out) return;
    const auto& pat = e->drumSeq.currentPattern();
    out->numRows  = em::kNumDrumInstruments;
    out->numSteps = pat.steps;
    std::strncpy(out->patternName, pat.name, sizeof(out->patternName) - 1);
    out->patternName[sizeof(out->patternName) - 1] = '\0';
    for (int r = 0; r < em::kNumDrumInstruments; ++r)
        for (int s = 0; s < pat.steps && s < 16; ++s)
            out->grid[r][s] = pat.grid[r][s];
}

int em_drum_current_step(EMEngine* e) {
    return e ? e->drumSeq.currentStep() : -1;
}

// ─── MIDI file player ─────────────────────────────────────────────────────────
bool em_midi_player_load(EMEngine* e, const char* path) {
    if (!e || !path) return false;
    return e->midiPlayer.loadFile(path);
}

void em_midi_player_rewind(EMEngine* e) {
    if (!e) return;
    e->midiPlayer.rewind();
    e->transport.setPosition(0.f);
    e->midiProgress = 0.f;
}

float  em_midi_player_progress(EMEngine* e)      { return e ? e->midiProgress : 0.f; }
double em_midi_player_total_beats(EMEngine* e)   { return e ? e->midiPlayer.totalBeats() : 0.0; }
bool   em_midi_player_is_loaded(EMEngine* e)     { return e && e->midiPlayer.isLoaded(); }

void em_midi_player_seek(EMEngine* e, float fraction) {
    if (!e || !e->midiPlayer.isLoaded()) return;
    double totalBeats = e->midiPlayer.totalBeats();
    e->transport.setPosition(static_cast<float>(totalBeats * fraction));
    e->midiProgress = fraction;
}

// ─── Voice slots ──────────────────────────────────────────────────────────────
int  em_voice_layer_count(EMEngine* e, int slot) {
    return (e && slot >= 0 && slot < 8) ? e->slots[slot].layerCount() : 0;
}

bool em_voice_add_layer(EMEngine* e, int slot) {
    if (!e || slot < 0 || slot >= 8) return false;
    return e->slots[slot].addLayer();
}

void em_voice_remove_layer(EMEngine* e, int slot, int layerIndex) {
    if (e && slot >= 0 && slot < 8)
        e->slots[slot].removeLayer(layerIndex);
}

void em_voice_set_layer_name(EMEngine* e, int slot, int layerIndex, const char* name) {
    if (!e || slot < 0 || slot >= 8 || !name) return;
    auto& info = e->slots[slot].layerInfo(layerIndex);
    info.instrumentName = name;
}

void em_voice_get_layer_name(EMEngine* e, int slot, int layerIndex, char* buf, int bufLen) {
    if (!e || slot < 0 || slot >= 8 || !buf || bufLen <= 0) return;
    const auto& info = e->slots[slot].layerInfo(layerIndex);
    std::strncpy(buf, info.instrumentName.c_str(), bufLen - 1);
    buf[bufLen - 1] = '\0';
}

void em_voice_all_notes_off(EMEngine* e, int slot) {
    if (e && slot >= 0 && slot < 8)
        e->slots[slot].allNotesOff();
}

void em_voice_set_program(EMEngine* e, int slot, int gmProgram) {
    if (!e || slot < 0 || slot >= 8) return;
    e->currentProgram[slot].store(gmProgram);
    // Generate a simple waveform for the chosen GM program
    if (e->slots[slot].layerCount() == 0) e->slots[slot].addLayer();
    auto& info = e->slots[slot].layerInfo(0);
    // Harmonic count based on GM family
    int harmonics = (gmProgram < 8)  ? 1 :   // piano-like
                    (gmProgram < 24) ? 3 :   // chromatic/organ
                    (gmProgram < 40) ? 2 :   // guitar/bass
                    (gmProgram < 56) ? 5 :   // strings
                                       4;    // everything else
    em::VoiceSlotEngine::generateWaveformWithHarmonics(
        harmonics, info.waveformData, EM_SAMPLE_RATE, 440.0f);
    e->slots[slot].layer(0)->setWaveform(
        info.waveformData.data(),
        static_cast<uint32_t>(info.waveformData.size()),
        EM_SAMPLE_RATE);
    info.instrumentName = em::MidiToChineseInstrumentMapper::programToChineseInstrument(gmProgram);
}

int em_voice_get_program(EMEngine* e, int slot) {
    if (!e || slot < 0 || slot >= 8) return 0;
    return e->currentProgram[slot].load();
}

// ─── MIDI ports ───────────────────────────────────────────────────────────────
int em_midi_port_count(EMEngine* e) {
    if (!e || !e->midi) return 0;
    return static_cast<int>(e->midi->getAvailablePorts().size());
}

void em_midi_port_name(EMEngine* e, int index, char* buf, int bufLen) {
    if (!e || !e->midi || !buf || bufLen <= 0) return;
    auto ports = e->midi->getAvailablePorts();
    if (index >= 0 && index < static_cast<int>(ports.size()))
        std::strncpy(buf, ports[index].c_str(), bufLen - 1);
    buf[bufLen - 1] = '\0';
}

bool em_midi_port_open(EMEngine* e, int index) {
    return e && e->midi && e->midi->openPort(static_cast<unsigned>(index));
}

bool em_midi_port_is_open(EMEngine* e) {
    return e && e->midi && e->midi->isOpen();
}

// ─── Audio status ──────────────────────────────────────────────────────────────
bool   em_audio_is_running(EMEngine* e)    { return e && e->audio.isRunning(); }
double em_audio_latency_ms(EMEngine* e)    { return e ? e->audio.getLatencyMs() : 0.0; }

// ─── SamplerEngine parameters ─────────────────────────────────────────────────
// Helper macro: resolve sampler pointer, return default on failure
#define EM_SAMPLER(e, slot, layer) \
    (((e) && (slot) >= 0 && (slot) < 8) ? (e)->slots[(slot)].layer((layer)) : nullptr)

// ADSR
void  em_sampler_set_attack(EMEngine* e, int slot, int layer, float v)  { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setAdsrAttack(v);   }
float em_sampler_get_attack(EMEngine* e, int slot, int layer)           { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getAdsrAttack()   : 0.f; }
void  em_sampler_set_decay(EMEngine* e, int slot, int layer, float v)   { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setAdsrDecay(v);    }
float em_sampler_get_decay(EMEngine* e, int slot, int layer)            { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getAdsrDecay()    : 0.f; }
void  em_sampler_set_sustain(EMEngine* e, int slot, int layer, float v) { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setAdsrSustain(v);  }
float em_sampler_get_sustain(EMEngine* e, int slot, int layer)          { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getAdsrSustain()  : 0.f; }
void  em_sampler_set_release(EMEngine* e, int slot, int layer, float v) { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setAdsrRelease(v);  }
float em_sampler_get_release(EMEngine* e, int slot, int layer)          { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getAdsrRelease()  : 0.f; }

// Tone
void  em_sampler_set_pan(EMEngine* e, int slot, int layer, float v)          { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setPan(v);         }
float em_sampler_get_pan(EMEngine* e, int slot, int layer)                   { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getPan()         : 0.f; }
void  em_sampler_set_octave_shift(EMEngine* e, int slot, int layer, int v)   { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setOctaveShift(v);  }
int   em_sampler_get_octave_shift(EMEngine* e, int slot, int layer)          { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getOctaveShift() : 0;   }
void  em_sampler_set_fine_tune(EMEngine* e, int slot, int layer, float v)    { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setFineTune(v);      }
float em_sampler_get_fine_tune(EMEngine* e, int slot, int layer)             { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getFineTune()     : 0.f; }
void  em_sampler_set_reverb_send(EMEngine* e, int slot, int layer, float v)  { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setReverbSend(v);    }
float em_sampler_get_reverb_send(EMEngine* e, int slot, int layer)           { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getReverbSend()   : 0.f; }

// Filter
void  em_sampler_set_filter_cutoff(EMEngine* e, int slot, int layer, float v) { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setFilterCutoff(v);  }
float em_sampler_get_filter_cutoff(EMEngine* e, int slot, int layer)          { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getFilterCutoff() : 0.f; }
void  em_sampler_set_resonance(EMEngine* e, int slot, int layer, float v)     { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setResonance(v);      }
float em_sampler_get_resonance(EMEngine* e, int slot, int layer)              { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getResonance()     : 0.f; }

// EQ
void  em_sampler_set_eq_low_freq(EMEngine* e, int slot, int layer, float v)  { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setEqLowFreq(v);   }
float em_sampler_get_eq_low_freq(EMEngine* e, int slot, int layer)           { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getEqLowFreq()  : 0.f; }
void  em_sampler_set_eq_low_gain(EMEngine* e, int slot, int layer, float v)  { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setEqLowGain(v);   }
float em_sampler_get_eq_low_gain(EMEngine* e, int slot, int layer)           { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getEqLowGain()  : 0.f; }
void  em_sampler_set_eq_high_freq(EMEngine* e, int slot, int layer, float v) { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setEqHighFreq(v);  }
float em_sampler_get_eq_high_freq(EMEngine* e, int slot, int layer)          { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getEqHighFreq() : 0.f; }
void  em_sampler_set_eq_high_gain(EMEngine* e, int slot, int layer, float v) { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setEqHighGain(v);  }
float em_sampler_get_eq_high_gain(EMEngine* e, int slot, int layer)          { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getEqHighGain() : 0.f; }

// LFO
void  em_sampler_set_lfo_wave(EMEngine* e, int slot, int layer, int v)    { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setLfoWave(v);   }
int   em_sampler_get_lfo_wave(EMEngine* e, int slot, int layer)           { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getLfoWave()  : 0;   }
void  em_sampler_set_lfo_speed(EMEngine* e, int slot, int layer, float v) { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setLfoSpeed(v);  }
float em_sampler_get_lfo_speed(EMEngine* e, int slot, int layer)          { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getLfoSpeed() : 0.f; }
void  em_sampler_set_lfo_pmd(EMEngine* e, int slot, int layer, float v)   { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setLfoPmd(v);    }
float em_sampler_get_lfo_pmd(EMEngine* e, int slot, int layer)            { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getLfoPmd()   : 0.f; }
void  em_sampler_set_lfo_fmd(EMEngine* e, int slot, int layer, float v)   { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setLfoFmd(v);    }
float em_sampler_get_lfo_fmd(EMEngine* e, int slot, int layer)            { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getLfoFmd()   : 0.f; }
void  em_sampler_set_lfo_amd(EMEngine* e, int slot, int layer, float v)   { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setLfoAmd(v);    }
float em_sampler_get_lfo_amd(EMEngine* e, int slot, int layer)            { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getLfoAmd()   : 0.f; }

// Vibrato
void  em_sampler_set_vibrato_depth(EMEngine* e, int slot, int layer, float v)   { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setVibratoDepth(v);  }
float em_sampler_get_vibrato_depth(EMEngine* e, int slot, int layer)            { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getVibratoDepth() : 0.f; }
void  em_sampler_set_vibrato_delay(EMEngine* e, int slot, int layer, float v)   { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setVibratoDelay(v);  }
float em_sampler_get_vibrato_delay(EMEngine* e, int slot, int layer)            { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getVibratoDelay() : 0.f; }
void  em_sampler_set_vibrato_speed(EMEngine* e, int slot, int layer, float v)   { auto* s = EM_SAMPLER(e,slot,layer); if (s) s->setVibratoSpeed(v);  }
float em_sampler_get_vibrato_speed(EMEngine* e, int slot, int layer)            { auto* s = EM_SAMPLER(e,slot,layer); return s ? s->getVibratoSpeed() : 0.f; }

#undef EM_SAMPLER

// ─── VoiceSlot realtime control ───────────────────────────────────────────────
void em_voice_note_on(EMEngine* e, int slot, uint8_t note, uint8_t velocity) {
    if (e && slot >= 0 && slot < 8) e->slots[slot].noteOn(note, velocity);
}
void em_voice_note_off(EMEngine* e, int slot, uint8_t note) {
    if (e && slot >= 0 && slot < 8) e->slots[slot].noteOff(note);
}
void em_voice_set_pitch_bend(EMEngine* e, int slot, float bend) {
    if (e && slot >= 0 && slot < 8) e->slots[slot].setPitchBend(bend);
}
void em_voice_set_expression(EMEngine* e, int slot, float gain) {
    if (e && slot >= 0 && slot < 8) e->slots[slot].setExpressionGain(gain);
}

// ─── DrumKit direct trigger ───────────────────────────────────────────────────
void em_drum_kit_trigger(EMEngine* e, int voiceIndex, uint8_t velocity) {
    if (e) e->drumKit.trigger(voiceIndex, velocity);
}
void em_drum_kit_set_volume(EMEngine* e, float volume) {
    if (e) e->drumKit.setVolume(volume);
}

// ─── Drum step velocity ───────────────────────────────────────────────────────
void em_drum_set_step_velocity(EMEngine* e, int row, int step, uint8_t velocity) {
    if (e) e->drumSeq.setStep(row, step, velocity);
}
uint8_t em_drum_get_step_velocity(EMEngine* e, int row, int step) {
    if (!e) return 0;
    const auto& pat = e->drumSeq.currentPattern();
    if (row < 0 || row >= em::kNumDrumInstruments || step < 0 || step >= pat.steps) return 0;
    return pat.grid[row][step];
}

// ─── MIDI output ──────────────────────────────────────────────────────────────
int em_midi_output_port_count(EMEngine* e) {
    if (!e || !e->midiOut) return 0;
    return static_cast<int>(e->midiOut->getAvailablePorts().size());
}

void em_midi_output_port_name(EMEngine* e, int index, char* buf, int bufLen) {
    if (!e || !e->midiOut || !buf || bufLen <= 0) return;
    auto ports = e->midiOut->getAvailablePorts();
    if (index >= 0 && index < static_cast<int>(ports.size()))
        std::strncpy(buf, ports[index].c_str(), bufLen - 1);
    buf[bufLen - 1] = '\0';
}

bool em_midi_output_open(EMEngine* e, int index) {
    if (!e) return false;
    if (!e->midiOut) e->midiOut = std::make_unique<em::CoreMidiOutput>();
    return e->midiOut->openPort(static_cast<unsigned>(index));
}

void em_midi_output_send_note_on(EMEngine* e, uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!e || !e->midiOut || !e->midiOut->isOpen()) return;
    e->midiOut->sendMessage(em::MidiMessage::noteOn(channel & 0x0F, note, velocity));
}

void em_midi_output_send_note_off(EMEngine* e, uint8_t channel, uint8_t note) {
    if (!e || !e->midiOut || !e->midiOut->isOpen()) return;
    e->midiOut->sendMessage(em::MidiMessage::noteOff(channel & 0x0F, note));
}

void em_midi_output_send_cc(EMEngine* e, uint8_t channel, uint8_t cc, uint8_t value) {
    if (!e || !e->midiOut || !e->midiOut->isOpen()) return;
    e->midiOut->sendMessage(em::MidiMessage::cc(channel & 0x0F, cc, value));
}

void em_midi_output_send_program_change(EMEngine* e, uint8_t channel, uint8_t program) {
    if (!e || !e->midiOut || !e->midiOut->isOpen()) return;
    e->midiOut->sendMessage(em::MidiMessage::programChange(channel & 0x0F, program));
}

void em_midi_output_send_bank_program(EMEngine* e, uint8_t channel, uint8_t bankMSB, uint8_t bankLSB, uint8_t program) {
    if (!e || !e->midiOut || !e->midiOut->isOpen()) return;
    // Send Bank Select MSB (CC#0)
    e->midiOut->sendMessage(em::MidiMessage::cc(channel & 0x0F, 0, bankMSB));
    // Send Bank Select LSB (CC#32)
    e->midiOut->sendMessage(em::MidiMessage::cc(channel & 0x0F, 32, bankLSB));
    // Send Program Change
    e->midiOut->sendMessage(em::MidiMessage::programChange(channel & 0x0F, program));
}

uint8_t em_midi_get_last_program_change(EMEngine* e, int slot) {
    if (!e || slot < 0 || slot >= 8) return 0;
    return e->lastProgramChange[slot].load(std::memory_order_relaxed);
}

void em_midi_output_send_sysex(EMEngine* e, const uint8_t* data, int len) {
    if (!e || !e->midiOut || !e->midiOut->isOpen() || !data || len <= 0) return;
    e->midiOut->sendRaw(data, len);
}

bool em_midi_output_is_open(EMEngine* e) {
    if (!e || !e->midiOut) return false;
    return e->midiOut->isOpen();
}

void em_midi_output_close(EMEngine* e) {
    if (!e || !e->midiOut) return;
    e->midiOut->closePort();
}

// ─── Log ring buffer poll ────────────────────────────────────────────────────

int em_log_poll(char* buf, int bufLen) {
    int r = gLogRead.load(std::memory_order_acquire);
    int w = gLogWrite.load(std::memory_order_acquire);
    if (r == w) return 0; // empty
    std::strncpy(buf, gLogBuf[r], bufLen - 1);
    buf[bufLen - 1] = '\0';
    gLogRead.store((r + 1) % kLogSlots, std::memory_order_release);
    return 1;
}

#endif // EM_IOS || EM_MACOS || EM_FLUTTER
