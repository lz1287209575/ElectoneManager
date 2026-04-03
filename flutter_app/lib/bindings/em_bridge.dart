import 'dart:ffi' as ffi;
import 'dart:io';
import 'package:ffi/ffi.dart';

// ─── Native structs ─────────────────────────────────────────────────────────

final class EMBeatInfo extends ffi.Struct {
  @ffi.Float()
  external double bpm;

  @ffi.Int32()
  external int bar;

  @ffi.Int32()
  external int beat;

  @ffi.Bool()
  external bool isPlaying;

  @ffi.Float()
  external double midiProgress;

  @ffi.Int32()
  external int currentStep;

  @ffi.Bool()
  external bool audioRunning;

  @ffi.Bool()
  external bool midiConnected;

  @ffi.Double()
  external double latencyMs;
}

final class EMDrumGrid extends ffi.Struct {
  @ffi.Array(12 * 16)
  external ffi.Array<ffi.Uint8> grid;

  @ffi.Int32()
  external int numRows;

  @ffi.Int32()
  external int numSteps;

  @ffi.Array(64)
  external ffi.Array<ffi.Uint8> patternName;
}

// ─── Native function typedefs ────────────────────────────────────────────────

typedef _EmCreateNative = ffi.Pointer<ffi.Void> Function();
typedef _EmCreateDart = ffi.Pointer<ffi.Void> Function();

typedef _EmDestroyNative = ffi.Void Function(ffi.Pointer<ffi.Void> e);
typedef _EmDestroyDart = void Function(ffi.Pointer<ffi.Void> e);

typedef _EmBoolNative = ffi.Bool Function(ffi.Pointer<ffi.Void> e);
typedef _EmBoolDart = bool Function(ffi.Pointer<ffi.Void> e);

typedef _EmVoidNative = ffi.Void Function(ffi.Pointer<ffi.Void> e);
typedef _EmVoidDart = void Function(ffi.Pointer<ffi.Void> e);

typedef _EmSetTempoNative = ffi.Void Function(ffi.Pointer<ffi.Void> e, ffi.Float bpm);
typedef _EmSetTempoDart = void Function(ffi.Pointer<ffi.Void> e, double bpm);

typedef _EmGetTempoNative = ffi.Float Function(ffi.Pointer<ffi.Void> e);
typedef _EmGetTempoDart = double Function(ffi.Pointer<ffi.Void> e);

typedef _EmPollBeatInfoNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Pointer<EMBeatInfo> out);
typedef _EmPollBeatInfoDart = void Function(
    ffi.Pointer<ffi.Void> e, ffi.Pointer<EMBeatInfo> out);

typedef _EmDrumPresetCountNative = ffi.Int32 Function();
typedef _EmDrumPresetCountDart = int Function();

typedef _EmDrumSetPresetNative = ffi.Void Function(ffi.Pointer<ffi.Void> e, ffi.Int32 idx);
typedef _EmDrumSetPresetDart = void Function(ffi.Pointer<ffi.Void> e, int idx);

typedef _EmDrumGetPresetNative = ffi.Int32 Function(ffi.Pointer<ffi.Void> e);
typedef _EmDrumGetPresetDart = int Function(ffi.Pointer<ffi.Void> e);

typedef _EmDrumGetPresetNameNative = ffi.Pointer<ffi.Char> Function(ffi.Int32 idx);
typedef _EmDrumGetPresetNameDart = ffi.Pointer<ffi.Char> Function(int idx);

typedef _EmDrumToggleStepNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 row, ffi.Int32 step);
typedef _EmDrumToggleStepDart = void Function(
    ffi.Pointer<ffi.Void> e, int row, int step);

typedef _EmDrumGetGridNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Pointer<EMDrumGrid> out);
typedef _EmDrumGetGridDart = void Function(
    ffi.Pointer<ffi.Void> e, ffi.Pointer<EMDrumGrid> out);

typedef _EmMidiPlayerLoadNative = ffi.Bool Function(
    ffi.Pointer<ffi.Void> e, ffi.Pointer<ffi.Char> path);
typedef _EmMidiPlayerLoadDart = bool Function(
    ffi.Pointer<ffi.Void> e, ffi.Pointer<ffi.Char> path);

typedef _EmMidiPlayerProgressNative = ffi.Float Function(ffi.Pointer<ffi.Void> e);
typedef _EmMidiPlayerProgressDart = double Function(ffi.Pointer<ffi.Void> e);

typedef _EmMidiPlayerSeekNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Float fraction);
typedef _EmMidiPlayerSeekDart = void Function(
    ffi.Pointer<ffi.Void> e, double fraction);

typedef _EmVoiceLayerCountNative = ffi.Int32 Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot);
typedef _EmVoiceLayerCountDart = int Function(
    ffi.Pointer<ffi.Void> e, int slot);

typedef _EmVoiceAddLayerNative = ffi.Bool Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot);
typedef _EmVoiceAddLayerDart = bool Function(
    ffi.Pointer<ffi.Void> e, int slot);

typedef _EmVoiceRemoveLayerNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot, ffi.Int32 layerIndex);
typedef _EmVoiceRemoveLayerDart = void Function(
    ffi.Pointer<ffi.Void> e, int slot, int layerIndex);

typedef _EmVoiceSetProgramNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot, ffi.Int32 program);
typedef _EmVoiceSetProgramDart = void Function(
    ffi.Pointer<ffi.Void> e, int slot, int program);

typedef _EmVoiceGetProgramNative = ffi.Int32 Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot);
typedef _EmVoiceGetProgramDart = int Function(
    ffi.Pointer<ffi.Void> e, int slot);

typedef _EmVoiceGetLayerNameNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot, ffi.Int32 layer,
    ffi.Pointer<ffi.Char> buf, ffi.Int32 bufLen);
typedef _EmVoiceGetLayerNameDart = void Function(
    ffi.Pointer<ffi.Void> e, int slot, int layer,
    ffi.Pointer<ffi.Char> buf, int bufLen);

typedef _EmVoiceNoteOnNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot, ffi.Uint8 note, ffi.Uint8 velocity);
typedef _EmVoiceNoteOnDart = void Function(
    ffi.Pointer<ffi.Void> e, int slot, int note, int velocity);

typedef _EmVoiceNoteOffNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot, ffi.Uint8 note);
typedef _EmVoiceNoteOffDart = void Function(
    ffi.Pointer<ffi.Void> e, int slot, int note);

typedef _EmSamplerSetNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot, ffi.Int32 layer, ffi.Float value);
typedef _EmSamplerSetDart = void Function(
    ffi.Pointer<ffi.Void> e, int slot, int layer, double value);

typedef _EmSamplerGetNative = ffi.Float Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot, ffi.Int32 layer);
typedef _EmSamplerGetDart = double Function(
    ffi.Pointer<ffi.Void> e, int slot, int layer);

typedef _EmSamplerGetIntNative = ffi.Int32 Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot, ffi.Int32 layer);
typedef _EmSamplerGetIntDart = int Function(
    ffi.Pointer<ffi.Void> e, int slot, int layer);

typedef _EmSamplerSetIntNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot, ffi.Int32 layer, ffi.Int32 value);
typedef _EmSamplerSetIntDart = void Function(
    ffi.Pointer<ffi.Void> e, int slot, int layer, int value);

typedef _EmMidiPortCountNative = ffi.Int32 Function(ffi.Pointer<ffi.Void> e);
typedef _EmMidiPortCountDart = int Function(ffi.Pointer<ffi.Void> e);

typedef _EmMidiPortNameNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 index,
    ffi.Pointer<ffi.Char> buf, ffi.Int32 bufLen);
typedef _EmMidiPortNameDart = void Function(
    ffi.Pointer<ffi.Void> e, int index,
    ffi.Pointer<ffi.Char> buf, int bufLen);

typedef _EmMidiPortOpenNative = ffi.Bool Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 index);
typedef _EmMidiPortOpenDart = bool Function(
    ffi.Pointer<ffi.Void> e, int index);

// ─── MIDI output typedefs ──────────────────────────────────────────────────

typedef _EmMidiOutputPortCountNative = ffi.Int32 Function(
    ffi.Pointer<ffi.Void> e);
typedef _EmMidiOutputPortCountDart = int Function(
    ffi.Pointer<ffi.Void> e);

typedef _EmMidiOutputPortNameNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 index,
    ffi.Pointer<ffi.Char> buf, ffi.Int32 bufLen);
typedef _EmMidiOutputPortNameDart = void Function(
    ffi.Pointer<ffi.Void> e, int index,
    ffi.Pointer<ffi.Char> buf, int bufLen);

typedef _EmMidiOutputOpenNative = ffi.Bool Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 index);
typedef _EmMidiOutputOpenDart = bool Function(
    ffi.Pointer<ffi.Void> e, int index);

typedef _EmMidiOutputSendNoteOnNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Uint8 channel, ffi.Uint8 note, ffi.Uint8 velocity);
typedef _EmMidiOutputSendNoteOnDart = void Function(
    ffi.Pointer<ffi.Void> e, int channel, int note, int velocity);

typedef _EmMidiOutputSendNoteOffNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Uint8 channel, ffi.Uint8 note);
typedef _EmMidiOutputSendNoteOffDart = void Function(
    ffi.Pointer<ffi.Void> e, int channel, int note);

typedef _EmMidiOutputSendCCNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Uint8 channel, ffi.Uint8 cc, ffi.Uint8 value);
typedef _EmMidiOutputSendCCDart = void Function(
    ffi.Pointer<ffi.Void> e, int channel, int cc, int value);

typedef _EmMidiOutputSendProgramChangeNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Uint8 channel, ffi.Uint8 program);
typedef _EmMidiOutputSendProgramChangeDart = void Function(
    ffi.Pointer<ffi.Void> e, int channel, int program);

typedef _EmMidiOutputSendBankProgramNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Uint8 channel, ffi.Uint8 bankMSB, ffi.Uint8 bankLSB, ffi.Uint8 program);
typedef _EmMidiOutputSendBankProgramDart = void Function(
    ffi.Pointer<ffi.Void> e, int channel, int bankMSB, int bankLSB, int program);

typedef _EmMidiGetLastProgramChangeNative = ffi.Uint8 Function(
    ffi.Pointer<ffi.Void> e, ffi.Int32 slot);
typedef _EmMidiGetLastProgramChangeDart = int Function(
    ffi.Pointer<ffi.Void> e, int slot);

typedef _EmMidiOutputSendSysexNative = ffi.Void Function(
    ffi.Pointer<ffi.Void> e, ffi.Pointer<ffi.Uint8> data, ffi.Int32 len);
typedef _EmMidiOutputSendSysexDart = void Function(
    ffi.Pointer<ffi.Void> e, ffi.Pointer<ffi.Uint8> data, int len);

typedef _EmMidiOutputIsOpenNative = ffi.Bool Function(ffi.Pointer<ffi.Void> e);
typedef _EmMidiOutputIsOpenDart   = bool Function(ffi.Pointer<ffi.Void> e);
typedef _EmMidiOutputCloseNative  = ffi.Void Function(ffi.Pointer<ffi.Void> e);
typedef _EmMidiOutputCloseDart    = void Function(ffi.Pointer<ffi.Void> e);

// Log ring buffer poll (no engine pointer — global)
typedef _EmLogPollNative = ffi.Int32 Function(ffi.Pointer<ffi.Char> buf, ffi.Int32 len);
typedef _EmLogPollDart   = int Function(ffi.Pointer<ffi.Char> buf, int len);

typedef _EmAudioLatencyNative = ffi.Double Function(ffi.Pointer<ffi.Void> e);
typedef _EmAudioLatencyDart = double Function(ffi.Pointer<ffi.Void> e);

// ─── EmBridge singleton ──────────────────────────────────────────────────────

class EmBridge {
  EmBridge._() {
    _lib = _loadLibrary();
    _engine = _lib
        .lookupFunction<_EmCreateNative, _EmCreateDart>('em_create')();
  }

  static final EmBridge instance = EmBridge._();

  late final ffi.DynamicLibrary _lib;
  late ffi.Pointer<ffi.Void> _engine;

  bool get hasEngine =>
      _engine != ffi.nullptr && _engine.address != 0;

  // ── Library loading ────────────────────────────────────────────────────────

  static ffi.DynamicLibrary _loadLibrary() {
    if (Platform.isMacOS) {
      return ffi.DynamicLibrary.open('libem_engine.dylib');
    } else if (Platform.isIOS) {
      return ffi.DynamicLibrary.process();
    } else if (Platform.isAndroid || Platform.isLinux) {
      return ffi.DynamicLibrary.open('libem_engine.so');
    } else if (Platform.isWindows) {
      return ffi.DynamicLibrary.open('em_engine.dll');
    }
    throw UnsupportedError('Platform not supported: ${Platform.operatingSystem}');
  }

  // ── Cached function lookups ────────────────────────────────────────────────

  late final _emDestroy =
      _lib.lookupFunction<_EmDestroyNative, _EmDestroyDart>('em_destroy');
  late final _emStartAudio =
      _lib.lookupFunction<_EmBoolNative, _EmBoolDart>('em_start_audio');
  late final _emStopAudio =
      _lib.lookupFunction<_EmVoidNative, _EmVoidDart>('em_stop_audio');
  late final _emStartMidi =
      _lib.lookupFunction<_EmBoolNative, _EmBoolDart>('em_start_midi');
  late final _emStopMidi =
      _lib.lookupFunction<_EmVoidNative, _EmVoidDart>('em_stop_midi');
  late final _emTransportPlay =
      _lib.lookupFunction<_EmVoidNative, _EmVoidDart>('em_transport_play');
  late final _emTransportStop =
      _lib.lookupFunction<_EmVoidNative, _EmVoidDart>('em_transport_stop');
  late final _emTransportIsPlaying =
      _lib.lookupFunction<_EmBoolNative, _EmBoolDart>('em_transport_is_playing');
  late final _emTransportSetTempo =
      _lib.lookupFunction<_EmSetTempoNative, _EmSetTempoDart>('em_transport_set_tempo');
  late final _emTransportGetTempo =
      _lib.lookupFunction<_EmGetTempoNative, _EmGetTempoDart>('em_transport_get_tempo');
  late final _emPollBeatInfo =
      _lib.lookupFunction<_EmPollBeatInfoNative, _EmPollBeatInfoDart>('em_poll_beat_info');
  late final _emDrumPresetCount =
      _lib.lookupFunction<_EmDrumPresetCountNative, _EmDrumPresetCountDart>('em_drum_preset_count');
  late final _emDrumSetPreset =
      _lib.lookupFunction<_EmDrumSetPresetNative, _EmDrumSetPresetDart>('em_drum_set_preset');
  late final _emDrumGetPreset =
      _lib.lookupFunction<_EmDrumGetPresetNative, _EmDrumGetPresetDart>('em_drum_get_preset');
  late final _emDrumGetPresetName =
      _lib.lookupFunction<_EmDrumGetPresetNameNative, _EmDrumGetPresetNameDart>('em_drum_get_preset_name');
  late final _emDrumToggleStep =
      _lib.lookupFunction<_EmDrumToggleStepNative, _EmDrumToggleStepDart>('em_drum_toggle_step');
  late final _emDrumClearPattern =
      _lib.lookupFunction<_EmVoidNative, _EmVoidDart>('em_drum_clear_pattern');
  late final _emDrumGetGrid =
      _lib.lookupFunction<_EmDrumGetGridNative, _EmDrumGetGridDart>('em_drum_get_grid');
  late final _emMidiPlayerLoad =
      _lib.lookupFunction<_EmMidiPlayerLoadNative, _EmMidiPlayerLoadDart>('em_midi_player_load');
  late final _emMidiPlayerRewind =
      _lib.lookupFunction<_EmVoidNative, _EmVoidDart>('em_midi_player_rewind');
  late final _emMidiPlayerProgress =
      _lib.lookupFunction<_EmMidiPlayerProgressNative, _EmMidiPlayerProgressDart>('em_midi_player_progress');
  late final _emMidiPlayerIsLoaded =
      _lib.lookupFunction<_EmBoolNative, _EmBoolDart>('em_midi_player_is_loaded');
  late final _emMidiPlayerSeek =
      _lib.lookupFunction<_EmMidiPlayerSeekNative, _EmMidiPlayerSeekDart>('em_midi_player_seek');
  late final _emVoiceLayerCount =
      _lib.lookupFunction<_EmVoiceLayerCountNative, _EmVoiceLayerCountDart>('em_voice_layer_count');
  late final _emVoiceAddLayer =
      _lib.lookupFunction<_EmVoiceAddLayerNative, _EmVoiceAddLayerDart>('em_voice_add_layer');
  late final _emVoiceRemoveLayer =
      _lib.lookupFunction<_EmVoiceRemoveLayerNative, _EmVoiceRemoveLayerDart>('em_voice_remove_layer');
  late final _emVoiceSetProgram =
      _lib.lookupFunction<_EmVoiceSetProgramNative, _EmVoiceSetProgramDart>('em_voice_set_program');
  late final _emVoiceGetProgram =
      _lib.lookupFunction<_EmVoiceGetProgramNative, _EmVoiceGetProgramDart>('em_voice_get_program');
  late final _emVoiceAllNotesOff =
      _lib.lookupFunction<_EmVoiceLayerCountNative, _EmVoiceLayerCountDart>('em_voice_all_notes_off');
  late final _emVoiceGetLayerName =
      _lib.lookupFunction<_EmVoiceGetLayerNameNative, _EmVoiceGetLayerNameDart>('em_voice_get_layer_name');
  late final _emVoiceNoteOn =
      _lib.lookupFunction<_EmVoiceNoteOnNative, _EmVoiceNoteOnDart>('em_voice_note_on');
  late final _emVoiceNoteOff =
      _lib.lookupFunction<_EmVoiceNoteOffNative, _EmVoiceNoteOffDart>('em_voice_note_off');
  late final _emMidiPortCount =
      _lib.lookupFunction<_EmMidiPortCountNative, _EmMidiPortCountDart>('em_midi_port_count');
  late final _emMidiPortName =
      _lib.lookupFunction<_EmMidiPortNameNative, _EmMidiPortNameDart>('em_midi_port_name');
  late final _emMidiPortOpen =
      _lib.lookupFunction<_EmMidiPortOpenNative, _EmMidiPortOpenDart>('em_midi_port_open');
  late final _emMidiPortIsOpen =
      _lib.lookupFunction<_EmBoolNative, _EmBoolDart>('em_midi_port_is_open');
  
  // ── MIDI output functions ──────────────────────────────────────────────────
  late final _emMidiOutputPortCount =
      _lib.lookupFunction<_EmMidiOutputPortCountNative, _EmMidiOutputPortCountDart>('em_midi_output_port_count');
  late final _emMidiOutputPortName =
      _lib.lookupFunction<_EmMidiOutputPortNameNative, _EmMidiOutputPortNameDart>('em_midi_output_port_name');
  late final _emMidiOutputOpen =
      _lib.lookupFunction<_EmMidiOutputOpenNative, _EmMidiOutputOpenDart>('em_midi_output_open');
  late final _emMidiOutputSendNoteOn =
      _lib.lookupFunction<_EmMidiOutputSendNoteOnNative, _EmMidiOutputSendNoteOnDart>('em_midi_output_send_note_on');
  late final _emMidiOutputSendNoteOff =
      _lib.lookupFunction<_EmMidiOutputSendNoteOffNative, _EmMidiOutputSendNoteOffDart>('em_midi_output_send_note_off');
  late final _emMidiOutputSendCC =
      _lib.lookupFunction<_EmMidiOutputSendCCNative, _EmMidiOutputSendCCDart>('em_midi_output_send_cc');
  late final _emMidiOutputSendProgramChange =
      _lib.lookupFunction<_EmMidiOutputSendProgramChangeNative, _EmMidiOutputSendProgramChangeDart>('em_midi_output_send_program_change');
  late final _emMidiOutputSendBankProgram =
      _lib.lookupFunction<_EmMidiOutputSendBankProgramNative, _EmMidiOutputSendBankProgramDart>('em_midi_output_send_bank_program');

  // ── MIDI feedback functions ────────────────────────────────────────────────
  late final _emMidiGetLastProgramChange =
      _lib.lookupFunction<_EmMidiGetLastProgramChangeNative, _EmMidiGetLastProgramChangeDart>('em_midi_get_last_program_change');
  
  // ── SysEx support ──────────────────────────────────────────────────────────
  late final _emMidiOutputSendSysex =
      _lib.lookupFunction<_EmMidiOutputSendSysexNative, _EmMidiOutputSendSysexDart>('em_midi_output_send_sysex');

  late final _emMidiOutputIsOpen =
      _lib.lookupFunction<_EmMidiOutputIsOpenNative, _EmMidiOutputIsOpenDart>('em_midi_output_is_open');

  late final _emMidiOutputClose =
      _lib.lookupFunction<_EmMidiOutputCloseNative, _EmMidiOutputCloseDart>('em_midi_output_close');

  // ── Log ring buffer ────────────────────────────────────────────────────────
  late final _emLogPoll =
      _lib.lookupFunction<_EmLogPollNative, _EmLogPollDart>('em_log_poll');

  late final _emAudioIsRunning =
      _lib.lookupFunction<_EmBoolNative, _EmBoolDart>('em_audio_is_running');
  late final _emAudioLatencyMs =
      _lib.lookupFunction<_EmAudioLatencyNative, _EmAudioLatencyDart>('em_audio_latency_ms');

  // Sampler — float set/get (shared signature, looked up by name at call time
  // using the concrete typedefs so the FFI constraint is satisfied)
  _EmSamplerSetDart _samplerSetFn(String name) =>
      _lib.lookupFunction<_EmSamplerSetNative, _EmSamplerSetDart>(name);
  _EmSamplerGetDart _samplerGetFn(String name) =>
      _lib.lookupFunction<_EmSamplerGetNative, _EmSamplerGetDart>(name);
  _EmSamplerSetIntDart _samplerSetIntFn(String name) =>
      _lib.lookupFunction<_EmSamplerSetIntNative, _EmSamplerSetIntDart>(name);
  _EmSamplerGetIntDart _samplerGetIntFn(String name) =>
      _lib.lookupFunction<_EmSamplerGetIntNative, _EmSamplerGetIntDart>(name);

  void _samplerSet(String name, int slot, int layer, double value) {
    if (!hasEngine) return;
    _samplerSetFn(name)(_engine, slot, layer, value);
  }

  double _samplerGet(String name, int slot, int layer) {
    if (!hasEngine) return 0.0;
    return _samplerGetFn(name)(_engine, slot, layer);
  }

  void _samplerSetInt(String name, int slot, int layer, int value) {
    if (!hasEngine) return;
    _samplerSetIntFn(name)(_engine, slot, layer, value);
  }

  int _samplerGetInt(String name, int slot, int layer) {
    if (!hasEngine) return 0;
    return _samplerGetIntFn(name)(_engine, slot, layer);
  }

  // ── Lifecycle ──────────────────────────────────────────────────────────────

  bool startAudio() {
    if (!hasEngine) return false;
    return _emStartAudio(_engine);
  }

  void stopAudio() {
    if (!hasEngine) return;
    _emStopAudio(_engine);
  }

  bool startMidi() {
    if (!hasEngine) return false;
    return _emStartMidi(_engine);
  }

  void stopMidi() {
    if (!hasEngine) return;
    _emStopMidi(_engine);
  }

  void dispose() {
    if (!hasEngine) return;
    _emDestroy(_engine);
    _engine = ffi.nullptr;
  }

  // ── Transport ──────────────────────────────────────────────────────────────

  void transportPlay() {
    if (!hasEngine) return;
    _emTransportPlay(_engine);
  }

  void transportStop() {
    if (!hasEngine) return;
    _emTransportStop(_engine);
  }

  void transportSetTempo(double bpm) {
    if (!hasEngine) return;
    _emTransportSetTempo(_engine, bpm);
  }

  bool transportIsPlaying() {
    if (!hasEngine) return false;
    return _emTransportIsPlaying(_engine);
  }

  double transportGetTempo() {
    if (!hasEngine) return 120.0;
    return _emTransportGetTempo(_engine);
  }

  // ── Beat info ──────────────────────────────────────────────────────────────

  void pollBeatInfo(ffi.Pointer<EMBeatInfo> out) {
    if (!hasEngine) return;
    _emPollBeatInfo(_engine, out);
  }

  // ── Drum machine ───────────────────────────────────────────────────────────

  int drumPresetCount() {
    return _emDrumPresetCount();
  }

  void drumSetPreset(int index) {
    if (!hasEngine) return;
    _emDrumSetPreset(_engine, index);
  }

  int drumGetPreset() {
    if (!hasEngine) return 0;
    return _emDrumGetPreset(_engine);
  }

  String drumGetPresetName(int index) {
    final ptr = _emDrumGetPresetName(index);
    if (ptr == ffi.nullptr) return 'Unknown';
    return ptr.cast<Utf8>().toDartString();
  }

  void drumToggleStep(int row, int step) {
    if (!hasEngine) return;
    _emDrumToggleStep(_engine, row, step);
  }

  void drumClearPattern() {
    if (!hasEngine) return;
    _emDrumClearPattern(_engine);
  }

  void drumGetGrid(ffi.Pointer<EMDrumGrid> out) {
    if (!hasEngine) return;
    _emDrumGetGrid(_engine, out);
  }

  // ── MIDI file player ───────────────────────────────────────────────────────

  bool midiPlayerLoad(String path) {
    if (!hasEngine) return false;
    return using((arena) {
      final pathPtr = path.toNativeUtf8(allocator: arena).cast<ffi.Char>();
      return _emMidiPlayerLoad(_engine, pathPtr);
    });
  }

  void midiPlayerRewind() {
    if (!hasEngine) return;
    _emMidiPlayerRewind(_engine);
  }

  double midiPlayerProgress() {
    if (!hasEngine) return 0.0;
    return _emMidiPlayerProgress(_engine);
  }

  bool midiPlayerIsLoaded() {
    if (!hasEngine) return false;
    return _emMidiPlayerIsLoaded(_engine);
  }

  void midiPlayerSeek(double fraction) {
    if (!hasEngine) return;
    _emMidiPlayerSeek(_engine, fraction);
  }

  // ── Voice slots ────────────────────────────────────────────────────────────

  int voiceLayerCount(int slot) {
    if (!hasEngine) return 0;
    return _emVoiceLayerCount(_engine, slot);
  }

  bool voiceAddLayer(int slot) {
    if (!hasEngine) return false;
    return _emVoiceAddLayer(_engine, slot);
  }

  void voiceRemoveLayer(int slot, int layerIndex) {
    if (!hasEngine) return;
    _emVoiceRemoveLayer(_engine, slot, layerIndex);
  }

  void voiceSetProgram(int slot, int program) {
    if (!hasEngine) return;
    _emVoiceSetProgram(_engine, slot, program);
  }

  int voiceGetProgram(int slot) {
    if (!hasEngine) return 0;
    return _emVoiceGetProgram(_engine, slot);
  }

  void voiceAllNotesOff(int slot) {
    if (!hasEngine) return;
    _emVoiceAllNotesOff(_engine, slot);
  }

  String voiceGetLayerName(int slot, int layer) {
    if (!hasEngine) return '';
    return using((arena) {
      final buf = arena.allocate<ffi.Char>(64);
      _emVoiceGetLayerName(_engine, slot, layer, buf, 64);
      return buf.cast<Utf8>().toDartString();
    });
  }

  void voiceNoteOn(int slot, int note, int velocity) {
    if (!hasEngine) return;
    _emVoiceNoteOn(_engine, slot, note, velocity);
  }

  void voiceNoteOff(int slot, int note) {
    if (!hasEngine) return;
    _emVoiceNoteOff(_engine, slot, note);
  }

  // ── Sampler parameters ─────────────────────────────────────────────────────

  void samplerSetAttack(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_attack', slot, layer, v);
  double samplerGetAttack(int slot, int layer) =>
      _samplerGet('em_sampler_get_attack', slot, layer);

  void samplerSetDecay(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_decay', slot, layer, v);
  double samplerGetDecay(int slot, int layer) =>
      _samplerGet('em_sampler_get_decay', slot, layer);

  void samplerSetSustain(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_sustain', slot, layer, v);
  double samplerGetSustain(int slot, int layer) =>
      _samplerGet('em_sampler_get_sustain', slot, layer);

  void samplerSetRelease(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_release', slot, layer, v);
  double samplerGetRelease(int slot, int layer) =>
      _samplerGet('em_sampler_get_release', slot, layer);

  void samplerSetPan(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_pan', slot, layer, v);
  double samplerGetPan(int slot, int layer) =>
      _samplerGet('em_sampler_get_pan', slot, layer);

  void samplerSetOctaveShift(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_octave_shift', slot, layer, v);
  double samplerGetOctaveShift(int slot, int layer) =>
      _samplerGet('em_sampler_get_octave_shift', slot, layer);

  void samplerSetFineTune(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_fine_tune', slot, layer, v);
  double samplerGetFineTune(int slot, int layer) =>
      _samplerGet('em_sampler_get_fine_tune', slot, layer);

  void samplerSetReverbSend(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_reverb_send', slot, layer, v);
  double samplerGetReverbSend(int slot, int layer) =>
      _samplerGet('em_sampler_get_reverb_send', slot, layer);

  void samplerSetFilterCutoff(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_filter_cutoff', slot, layer, v);
  double samplerGetFilterCutoff(int slot, int layer) =>
      _samplerGet('em_sampler_get_filter_cutoff', slot, layer);

  void samplerSetResonance(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_resonance', slot, layer, v);
  double samplerGetResonance(int slot, int layer) =>
      _samplerGet('em_sampler_get_resonance', slot, layer);

  void samplerSetEqLowFreq(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_eq_low_freq', slot, layer, v);
  double samplerGetEqLowFreq(int slot, int layer) =>
      _samplerGet('em_sampler_get_eq_low_freq', slot, layer);

  void samplerSetEqLowGain(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_eq_low_gain', slot, layer, v);
  double samplerGetEqLowGain(int slot, int layer) =>
      _samplerGet('em_sampler_get_eq_low_gain', slot, layer);

  void samplerSetEqHighFreq(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_eq_high_freq', slot, layer, v);
  double samplerGetEqHighFreq(int slot, int layer) =>
      _samplerGet('em_sampler_get_eq_high_freq', slot, layer);

  void samplerSetEqHighGain(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_eq_high_gain', slot, layer, v);
  double samplerGetEqHighGain(int slot, int layer) =>
      _samplerGet('em_sampler_get_eq_high_gain', slot, layer);

  void samplerSetLfoWave(int slot, int layer, int v) =>
      _samplerSetInt('em_sampler_set_lfo_wave', slot, layer, v);
  int samplerGetLfoWave(int slot, int layer) =>
      _samplerGetInt('em_sampler_get_lfo_wave', slot, layer);

  void samplerSetLfoSpeed(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_lfo_speed', slot, layer, v);
  double samplerGetLfoSpeed(int slot, int layer) =>
      _samplerGet('em_sampler_get_lfo_speed', slot, layer);

  void samplerSetLfoPmd(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_lfo_pmd', slot, layer, v);
  double samplerGetLfoPmd(int slot, int layer) =>
      _samplerGet('em_sampler_get_lfo_pmd', slot, layer);

  void samplerSetLfoFmd(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_lfo_fmd', slot, layer, v);
  double samplerGetLfoFmd(int slot, int layer) =>
      _samplerGet('em_sampler_get_lfo_fmd', slot, layer);

  void samplerSetLfoAmd(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_lfo_amd', slot, layer, v);
  double samplerGetLfoAmd(int slot, int layer) =>
      _samplerGet('em_sampler_get_lfo_amd', slot, layer);

  void samplerSetVibratoDepth(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_vibrato_depth', slot, layer, v);
  double samplerGetVibratoDepth(int slot, int layer) =>
      _samplerGet('em_sampler_get_vibrato_depth', slot, layer);

  void samplerSetVibratoDelay(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_vibrato_delay', slot, layer, v);
  double samplerGetVibratoDelay(int slot, int layer) =>
      _samplerGet('em_sampler_get_vibrato_delay', slot, layer);

  void samplerSetVibratoSpeed(int slot, int layer, double v) =>
      _samplerSet('em_sampler_set_vibrato_speed', slot, layer, v);
  double samplerGetVibratoSpeed(int slot, int layer) =>
      _samplerGet('em_sampler_get_vibrato_speed', slot, layer);

  // ── MIDI ports ─────────────────────────────────────────────────────────────

  int midiPortCount() {
    if (!hasEngine) return 0;
    return _emMidiPortCount(_engine);
  }

  String midiPortName(int index) {
    if (!hasEngine) return '';
    return using((arena) {
      final buf = arena.allocate<ffi.Char>(128);
      _emMidiPortName(_engine, index, buf, 128);
      return buf.cast<Utf8>().toDartString();
    });
  }

  bool midiPortOpen(int index) {
    if (!hasEngine) return false;
    return _emMidiPortOpen(_engine, index);
  }

  bool midiPortIsOpen() {
    if (!hasEngine) return false;
    return _emMidiPortIsOpen(_engine);
  }

  // ── MIDI Output ────────────────────────────────────────────────────────────

  int midiOutputPortCount() {
    if (!hasEngine) return 0;
    return _emMidiOutputPortCount(_engine);
  }

  String midiOutputPortName(int index) {
    if (!hasEngine) return '';
    return using((arena) {
      final buf = arena.allocate<ffi.Char>(128);
      _emMidiOutputPortName(_engine, index, buf, 128);
      return buf.cast<Utf8>().toDartString();
    });
  }

  bool midiOutputOpen(int index) {
    if (!hasEngine) return false;
    return _emMidiOutputOpen(_engine, index);
  }

  void midiOutputSendNoteOn(int channel, int note, int velocity) {
    if (!hasEngine) return;
    _emMidiOutputSendNoteOn(_engine, channel, note, velocity);
  }

  void midiOutputSendNoteOff(int channel, int note) {
    if (!hasEngine) return;
    _emMidiOutputSendNoteOff(_engine, channel, note);
  }

  void midiOutputSendCC(int channel, int cc, int value) {
    if (!hasEngine) return;
    _emMidiOutputSendCC(_engine, channel, cc, value);
  }

  void midiOutputSendProgramChange(int channel, int program) {
    if (!hasEngine) return;
    _emMidiOutputSendProgramChange(_engine, channel, program);
  }

  void midiOutputSendBankProgram(int channel, int bankMSB, int bankLSB, int program) {
    if (!hasEngine) return;
    _emMidiOutputSendBankProgram(_engine, channel, bankMSB, bankLSB, program);
  }

  // ── MIDI Feedback ──────────────────────────────────────────────────────────

  int midiGetLastProgramChange(int slot) {
    if (!hasEngine) return 0;
    return _emMidiGetLastProgramChange(_engine, slot);
  }

  // ── SysEx support ──────────────────────────────────────────────────────────

  void midiOutputSendSysex(List<int> data) {
    if (!hasEngine || data.isEmpty) return;
    final ptr = calloc<ffi.Uint8>(data.length);
    for (int i = 0; i < data.length; i++) {
      ptr[i] = data[i];
    }
    _emMidiOutputSendSysex(_engine, ptr, data.length);
    calloc.free(ptr);
  }

  bool midiOutputIsOpen() {
    if (!hasEngine) return false;
    return _emMidiOutputIsOpen(_engine);
  }

  void midiOutputClose() {
    if (!hasEngine) return;
    _emMidiOutputClose(_engine);
  }

  // ── Log ring buffer ────────────────────────────────────────────────────────

  /// Poll one log line from the C++ ring buffer.
  /// Returns null if the buffer is empty.
  String? logPoll() {
    return using((arena) {
      final buf = arena.allocate<ffi.Char>(128);
      final got = _emLogPoll(buf, 128);
      if (got == 0) return null;
      return buf.cast<Utf8>().toDartString();
    });
  }

  // ── Audio status ───────────────────────────────────────────────────────────

  bool audioIsRunning() {
    if (!hasEngine) return false;
    return _emAudioIsRunning(_engine);
  }

  double audioLatencyMs() {
    if (!hasEngine) return 0.0;
    return _emAudioLatencyMs(_engine);
  }
}
