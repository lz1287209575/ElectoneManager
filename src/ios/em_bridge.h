// em_bridge.h — Pure C API bridge between Swift and C++ engine
// All C++ types are hidden behind the opaque EMEngine pointer.
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ─── Opaque engine handle ────────────────────────────────────────────────────
typedef struct EMEngine EMEngine;

// Beat/tempo snapshot polled by Swift at 60 Hz
typedef struct {
    float   bpm;
    int     bar;
    int     beat;
    bool    isPlaying;
    float   midiProgress;   // 0.0 – 1.0
    int     currentStep;    // drum sequencer step (0-15)
    bool    audioRunning;
    bool    midiConnected;
    double  latencyMs;
} EMBeatInfo;

// 12 drum rows × 16 steps, each byte is velocity (0 = off)
typedef struct {
    uint8_t grid[12][16];
    int     numRows;
    int     numSteps;
    char    patternName[64];
} EMDrumGrid;

// ─── Lifecycle ───────────────────────────────────────────────────────────────
EMEngine* em_create(void);
void      em_destroy(EMEngine* e);

bool      em_start_audio(EMEngine* e);
void      em_stop_audio(EMEngine* e);

bool      em_start_midi(EMEngine* e);
void      em_stop_midi(EMEngine* e);

// ─── Transport ───────────────────────────────────────────────────────────────
void      em_transport_play(EMEngine* e);
void      em_transport_stop(EMEngine* e);
void      em_transport_set_tempo(EMEngine* e, float bpm);
void      em_transport_set_position(EMEngine* e, float beatPos);
bool      em_transport_is_playing(EMEngine* e);
float     em_transport_get_tempo(EMEngine* e);

// ─── Beat info poll (call every display frame) ────────────────────────────────
void      em_poll_beat_info(EMEngine* e, EMBeatInfo* out);

// ─── Drum machine ────────────────────────────────────────────────────────────
int       em_drum_preset_count(void);
void      em_drum_set_preset(EMEngine* e, int presetIndex);
int       em_drum_get_preset(EMEngine* e);
const char* em_drum_get_preset_name(int presetIndex);
void      em_drum_toggle_step(EMEngine* e, int row, int step);
void      em_drum_clear_pattern(EMEngine* e);
void      em_drum_get_grid(EMEngine* e, EMDrumGrid* out);
int       em_drum_current_step(EMEngine* e);

// ─── MIDI file player ────────────────────────────────────────────────────────
bool      em_midi_player_load(EMEngine* e, const char* path);
void      em_midi_player_rewind(EMEngine* e);
float     em_midi_player_progress(EMEngine* e);  // 0.0 – 1.0
double    em_midi_player_total_beats(EMEngine* e);
bool      em_midi_player_is_loaded(EMEngine* e);
void      em_midi_player_seek(EMEngine* e, float fraction);

// ─── Voice slots (0-7) ───────────────────────────────────────────────────────
int       em_voice_layer_count(EMEngine* e, int slot);
bool      em_voice_add_layer(EMEngine* e, int slot);
void      em_voice_remove_layer(EMEngine* e, int slot, int layerIndex);
void      em_voice_set_layer_name(EMEngine* e, int slot, int layerIndex, const char* name);
// Writes name into buf (max bufLen bytes)
void      em_voice_get_layer_name(EMEngine* e, int slot, int layerIndex, char* buf, int bufLen);
void      em_voice_all_notes_off(EMEngine* e, int slot);
void      em_voice_set_program(EMEngine* e, int slot, int gmProgram);
// Returns GM program number for a slot
int       em_voice_get_program(EMEngine* e, int slot);

// ─── MIDI ports ──────────────────────────────────────────────────────────────
int       em_midi_port_count(EMEngine* e);
void      em_midi_port_name(EMEngine* e, int index, char* buf, int bufLen);
bool      em_midi_port_open(EMEngine* e, int index);
bool      em_midi_port_is_open(EMEngine* e);

// ─── Audio status ─────────────────────────────────────────────────────────────
bool      em_audio_is_running(EMEngine* e);
double    em_audio_latency_ms(EMEngine* e);

// ─── SamplerEngine parameters (slot 0-7, layer 0-7) ──────────────────────────
// ADSR
void      em_sampler_set_attack(EMEngine* e, int slot, int layer, float seconds);
float     em_sampler_get_attack(EMEngine* e, int slot, int layer);
void      em_sampler_set_decay(EMEngine* e, int slot, int layer, float seconds);
float     em_sampler_get_decay(EMEngine* e, int slot, int layer);
void      em_sampler_set_sustain(EMEngine* e, int slot, int layer, float level);
float     em_sampler_get_sustain(EMEngine* e, int slot, int layer);
void      em_sampler_set_release(EMEngine* e, int slot, int layer, float seconds);
float     em_sampler_get_release(EMEngine* e, int slot, int layer);

// Tone
void      em_sampler_set_pan(EMEngine* e, int slot, int layer, float pan);
float     em_sampler_get_pan(EMEngine* e, int slot, int layer);
void      em_sampler_set_octave_shift(EMEngine* e, int slot, int layer, int shift);
int       em_sampler_get_octave_shift(EMEngine* e, int slot, int layer);
void      em_sampler_set_fine_tune(EMEngine* e, int slot, int layer, float cents);
float     em_sampler_get_fine_tune(EMEngine* e, int slot, int layer);
void      em_sampler_set_reverb_send(EMEngine* e, int slot, int layer, float send);
float     em_sampler_get_reverb_send(EMEngine* e, int slot, int layer);

// Filter
void      em_sampler_set_filter_cutoff(EMEngine* e, int slot, int layer, float norm);
float     em_sampler_get_filter_cutoff(EMEngine* e, int slot, int layer);
void      em_sampler_set_resonance(EMEngine* e, int slot, int layer, float norm);
float     em_sampler_get_resonance(EMEngine* e, int slot, int layer);

// EQ
void      em_sampler_set_eq_low_freq(EMEngine* e, int slot, int layer, float hz);
float     em_sampler_get_eq_low_freq(EMEngine* e, int slot, int layer);
void      em_sampler_set_eq_low_gain(EMEngine* e, int slot, int layer, float dB);
float     em_sampler_get_eq_low_gain(EMEngine* e, int slot, int layer);
void      em_sampler_set_eq_high_freq(EMEngine* e, int slot, int layer, float hz);
float     em_sampler_get_eq_high_freq(EMEngine* e, int slot, int layer);
void      em_sampler_set_eq_high_gain(EMEngine* e, int slot, int layer, float dB);
float     em_sampler_get_eq_high_gain(EMEngine* e, int slot, int layer);

// LFO
void      em_sampler_set_lfo_wave(EMEngine* e, int slot, int layer, int wave);
int       em_sampler_get_lfo_wave(EMEngine* e, int slot, int layer);
void      em_sampler_set_lfo_speed(EMEngine* e, int slot, int layer, float hz);
float     em_sampler_get_lfo_speed(EMEngine* e, int slot, int layer);
void      em_sampler_set_lfo_pmd(EMEngine* e, int slot, int layer, float depth);
float     em_sampler_get_lfo_pmd(EMEngine* e, int slot, int layer);
void      em_sampler_set_lfo_fmd(EMEngine* e, int slot, int layer, float depth);
float     em_sampler_get_lfo_fmd(EMEngine* e, int slot, int layer);
void      em_sampler_set_lfo_amd(EMEngine* e, int slot, int layer, float depth);
float     em_sampler_get_lfo_amd(EMEngine* e, int slot, int layer);

// Vibrato
void      em_sampler_set_vibrato_depth(EMEngine* e, int slot, int layer, float depth);
float     em_sampler_get_vibrato_depth(EMEngine* e, int slot, int layer);
void      em_sampler_set_vibrato_delay(EMEngine* e, int slot, int layer, float seconds);
float     em_sampler_get_vibrato_delay(EMEngine* e, int slot, int layer);
void      em_sampler_set_vibrato_speed(EMEngine* e, int slot, int layer, float hz);
float     em_sampler_get_vibrato_speed(EMEngine* e, int slot, int layer);

// ─── VoiceSlot realtime control ───────────────────────────────────────────────
void      em_voice_note_on(EMEngine* e, int slot, uint8_t note, uint8_t velocity);
void      em_voice_note_off(EMEngine* e, int slot, uint8_t note);
void      em_voice_set_pitch_bend(EMEngine* e, int slot, float bend);
void      em_voice_set_expression(EMEngine* e, int slot, float gain);

// ─── DrumKit direct trigger ───────────────────────────────────────────────────
void      em_drum_kit_trigger(EMEngine* e, int voiceIndex, uint8_t velocity);
void      em_drum_kit_set_volume(EMEngine* e, float volume);

// ─── Drum step velocity ───────────────────────────────────────────────────────
void      em_drum_set_step_velocity(EMEngine* e, int row, int step, uint8_t velocity);
uint8_t   em_drum_get_step_velocity(EMEngine* e, int row, int step);

// ─── MIDI output (send to hardware) ──────────────────────────────────────────
int       em_midi_output_port_count(EMEngine* e);
void      em_midi_output_port_name(EMEngine* e, int index, char* buf, int bufLen);
bool      em_midi_output_open(EMEngine* e, int index);
void      em_midi_output_send_note_on(EMEngine* e, uint8_t channel, uint8_t note, uint8_t velocity);
void      em_midi_output_send_note_off(EMEngine* e, uint8_t channel, uint8_t note);
void      em_midi_output_send_cc(EMEngine* e, uint8_t channel, uint8_t cc, uint8_t value);
void      em_midi_output_send_program_change(EMEngine* e, uint8_t channel, uint8_t program);
void      em_midi_output_send_bank_program(EMEngine* e, uint8_t channel, uint8_t bankMSB, uint8_t bankLSB, uint8_t program);

// ─── MIDI feedback (get last program change from incoming MIDI) ─────────────────
uint8_t   em_midi_get_last_program_change(EMEngine* e, int slot);

// ─── SysEx support (Yamaha XG) ─────────────────────────────────────────────────
void      em_midi_output_send_sysex(EMEngine* e, const uint8_t* data, int len);

bool      em_midi_output_is_open(EMEngine* e);
void      em_midi_output_close(EMEngine* e);

// ─── Log ring buffer (poll from Dart at ~10 Hz) ─────────────────────────────────
// Pops the oldest log line into buf. Returns 1 if a line was available, 0 if empty.
// buf must be at least bufLen bytes. Lines are null-terminated.
int       em_log_poll(char* buf, int bufLen);

#ifdef __cplusplus
} // extern "C"
#endif
