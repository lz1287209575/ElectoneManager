import 'dart:async';
import 'dart:ffi' as ffi;
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../bindings/em_bridge.dart';
import '../models/engine_state.dart';
import '../models/drum_pattern.dart';
import '../models/voice_slot.dart';
import '../utils/constants.dart';
import '../utils/b00_parser.dart';

class EngineNotifier extends StateNotifier<EngineState> {
  EngineNotifier() : super(EngineState.initial()) {
    _init();
  }

  final EmBridge _bridge = EmBridge.instance;
  Timer? _pollTimer;
  ffi.Pointer<EMBeatInfo>? _beatInfoPtr;
  ffi.Pointer<EMDrumGrid>? _drumGridPtr;
  String? _cachedB00Path;
  B00ParseResult? _cachedB00Result;

  void _init() {
    _beatInfoPtr = calloc<EMBeatInfo>();
    _drumGridPtr = calloc<EMDrumGrid>();

    _bridge.startAudio();
    _bridge.startMidi();

    // Sync initial voice state
    _syncVoiceSlots();

    // Start 60 Hz polling
    _pollTimer = Timer.periodic(
      const Duration(microseconds: 16667), // ~60 Hz
      (_) => _poll(),
    );
  }

  void _poll() {
    if (!_bridge.hasEngine) return;

    final ptr = _beatInfoPtr;
    if (ptr == null) return;

    _bridge.pollBeatInfo(ptr);
    final info = ptr.ref;

    final transport = state.transport.copyWith(
      isPlaying: info.isPlaying,
      bpm: info.bpm.toDouble(),
      bar: info.bar,
      beat: info.beat,
      currentStep: info.currentStep,
      midiProgress: info.midiProgress.toDouble(),
    );

    // Check for incoming MIDI program changes from piano
    bool anyProgramChanged = false;
    final updatedSlots = List<VoiceSlotInfo>.from(state.voiceSlots);
    for (int i = 0; i < 8; i++) {
      final lastPC = _bridge.midiGetLastProgramChange(i);
      if (lastPC > 0 && updatedSlots[i].programNumber != lastPC) {
        final programName = lastPC < EmConstants.gmProgramNames.length
            ? EmConstants.gmProgramNames[lastPC]
            : 'Program $lastPC';
        updatedSlots[i] = updatedSlots[i].copyWith(
          programNumber: lastPC,
          programName: programName,
        );
        anyProgramChanged = true;
      }
    }

    state = state.copyWith(
      transport: transport,
      audioRunning: info.audioRunning,
      midiConnected: info.midiConnected,
      midiOutputConnected: _bridge.midiOutputIsOpen(),
      latencyMs: info.latencyMs,
      voiceSlots: anyProgramChanged ? updatedSlots : state.voiceSlots,
    );
  }

  void _syncVoiceSlots() {
    final slots = List<VoiceSlotInfo>.generate(8, (slot) {
      final program = _bridge.voiceGetProgram(slot);
      final layerCount = _bridge.voiceLayerCount(slot);
      final layers = List<String>.generate(
        layerCount,
        (l) => _bridge.voiceGetLayerName(slot, l),
      );
      final programName = program < EmConstants.gmProgramNames.length
          ? EmConstants.gmProgramNames[program]
          : 'Program $program';
      return VoiceSlotInfo(
        slotIndex: slot,
        programNumber: program,
        programName: programName,
        layerCount: layerCount,
        layerNames: layers,
      );
    });
    state = state.copyWith(voiceSlots: slots);
  }

  void _syncDrumGrid() {
    final ptr = _drumGridPtr;
    if (ptr == null) return;
    _bridge.drumGetGrid(ptr);
    final g = ptr.ref;
    final grid = List.generate(
      g.numRows,
      (row) => List.generate(
        g.numSteps,
        (step) => g.grid[row * 16 + step],
      ),
    );
    final name = String.fromCharCodes(
      List.generate(64, (i) => g.patternName[i]).takeWhile((c) => c != 0),
    );
    state = state.copyWith(
      drumPattern: DrumPattern(
        grid: grid,
        patternName: name.isEmpty ? 'Pattern' : name,
        presetIndex: state.drumPresetIndex,
      ),
    );
  }

  // ── Transport ──────────────────────────────────────────────────────────────

  void play() {
    _bridge.transportPlay();
  }

  void stop() {
    _bridge.transportStop();
  }

  void setTempo(double bpm) {
    _bridge.transportSetTempo(bpm);
    state = state.copyWith(
      transport: state.transport.copyWith(bpm: bpm),
    );
  }

  // ── MIDI file player ───────────────────────────────────────────────────────

  void loadMidi(String path) {
    final ok = _bridge.midiPlayerLoad(path);
    state = state.copyWith(
      transport: state.transport.copyWith(
        midiLoaded: ok,
        midiProgress: ok ? 0.0 : state.transport.midiProgress,
      ),
    );
  }

  void seekMidi(double fraction) {
    _bridge.midiPlayerSeek(fraction);
  }

  void rewindMidi() {
    _bridge.midiPlayerRewind();
  }

  // ── Voice slots ────────────────────────────────────────────────────────────

  void selectSlot(int slot) {
    state = state.copyWith(selectedSlot: slot);
  }

  void setVoiceProgram(int slot, int program) {
    _bridge.voiceSetProgram(slot, program);
    final name = program < EmConstants.gmProgramNames.length
        ? EmConstants.gmProgramNames[program]
        : 'Program $program';
    final newSlots = List<VoiceSlotInfo>.from(state.voiceSlots);
    newSlots[slot] = newSlots[slot].copyWith(
      programNumber: program,
      programName: name,
    );
    state = state.copyWith(voiceSlots: newSlots);
  }

  void addLayer(int slot) {
    _bridge.voiceAddLayer(slot);
    _syncVoiceSlots();
  }

  void removeLayer(int slot, int layer) {
    _bridge.voiceRemoveLayer(slot, layer);
    _syncVoiceSlots();
  }

  void toggleSlotEnabled(int slot) {
    final newSlots = List<VoiceSlotInfo>.from(state.voiceSlots);
    newSlots[slot] = newSlots[slot].copyWith(enabled: !newSlots[slot].enabled);
    state = state.copyWith(voiceSlots: newSlots);
  }

  void toggleLayerEnabled(int slot, int layer) {
    final newSlots = List<VoiceSlotInfo>.from(state.voiceSlots);
    final s = newSlots[slot];
    final current = s.effectiveLayerEnabled;
    current[layer] = !current[layer];
    newSlots[slot] = s.copyWith(layerEnabled: current);
    state = state.copyWith(voiceSlots: newSlots);
  }

  void noteOn(int slot, int note, int velocity) {
    _bridge.voiceNoteOn(slot, note, velocity);
  }

  void noteOff(int slot, int note) {
    _bridge.voiceNoteOff(slot, note);
  }

  // ── Drum machine ───────────────────────────────────────────────────────────

  void toggleDrumStep(int row, int step) {
    _bridge.drumToggleStep(row, step);
    _syncDrumGrid();
  }

  void clearDrumPattern() {
    _bridge.drumClearPattern();
    _syncDrumGrid();
  }

  void setDrumPreset(int index) {
    _bridge.drumSetPreset(index);
    state = state.copyWith(drumPresetIndex: index);
    _syncDrumGrid();
  }

  // ── Sampler params ─────────────────────────────────────────────────────────

  void setSamplerAttack(int slot, int layer, double v) =>
      _bridge.samplerSetAttack(slot, layer, v);
  void setSamplerDecay(int slot, int layer, double v) =>
      _bridge.samplerSetDecay(slot, layer, v);
  void setSamplerSustain(int slot, int layer, double v) =>
      _bridge.samplerSetSustain(slot, layer, v);
  void setSamplerRelease(int slot, int layer, double v) =>
      _bridge.samplerSetRelease(slot, layer, v);
  void setSamplerPan(int slot, int layer, double v) =>
      _bridge.samplerSetPan(slot, layer, v);
  void setSamplerOctaveShift(int slot, int layer, double v) =>
      _bridge.samplerSetOctaveShift(slot, layer, v);
  void setSamplerFineTune(int slot, int layer, double v) =>
      _bridge.samplerSetFineTune(slot, layer, v);
  void setSamplerReverbSend(int slot, int layer, double v) =>
      _bridge.samplerSetReverbSend(slot, layer, v);
  void setSamplerFilterCutoff(int slot, int layer, double v) =>
      _bridge.samplerSetFilterCutoff(slot, layer, v);
  void setSamplerResonance(int slot, int layer, double v) =>
      _bridge.samplerSetResonance(slot, layer, v);
  void setSamplerEqLowFreq(int slot, int layer, double v) =>
      _bridge.samplerSetEqLowFreq(slot, layer, v);
  void setSamplerEqLowGain(int slot, int layer, double v) =>
      _bridge.samplerSetEqLowGain(slot, layer, v);
  void setSamplerEqHighFreq(int slot, int layer, double v) =>
      _bridge.samplerSetEqHighFreq(slot, layer, v);
  void setSamplerEqHighGain(int slot, int layer, double v) =>
      _bridge.samplerSetEqHighGain(slot, layer, v);
  void setSamplerLfoWave(int slot, int layer, int v) =>
      _bridge.samplerSetLfoWave(slot, layer, v);
  void setSamplerLfoSpeed(int slot, int layer, double v) =>
      _bridge.samplerSetLfoSpeed(slot, layer, v);
  void setSamplerLfoPmd(int slot, int layer, double v) =>
      _bridge.samplerSetLfoPmd(slot, layer, v);
  void setSamplerLfoFmd(int slot, int layer, double v) =>
      _bridge.samplerSetLfoFmd(slot, layer, v);
  void setSamplerLfoAmd(int slot, int layer, double v) =>
      _bridge.samplerSetLfoAmd(slot, layer, v);
  void setSamplerVibratoDepth(int slot, int layer, double v) =>
      _bridge.samplerSetVibratoDepth(slot, layer, v);
  void setSamplerVibratoDelay(int slot, int layer, double v) =>
      _bridge.samplerSetVibratoDelay(slot, layer, v);
  void setSamplerVibratoSpeed(int slot, int layer, double v) =>
      _bridge.samplerSetVibratoSpeed(slot, layer, v);

  double getSamplerAttack(int slot, int layer) =>
      _bridge.samplerGetAttack(slot, layer);
  double getSamplerDecay(int slot, int layer) =>
      _bridge.samplerGetDecay(slot, layer);
  double getSamplerSustain(int slot, int layer) =>
      _bridge.samplerGetSustain(slot, layer);
  double getSamplerRelease(int slot, int layer) =>
      _bridge.samplerGetRelease(slot, layer);
  double getSamplerPan(int slot, int layer) =>
      _bridge.samplerGetPan(slot, layer);
  double getSamplerOctaveShift(int slot, int layer) =>
      _bridge.samplerGetOctaveShift(slot, layer);
  double getSamplerFineTune(int slot, int layer) =>
      _bridge.samplerGetFineTune(slot, layer);
  double getSamplerReverbSend(int slot, int layer) =>
      _bridge.samplerGetReverbSend(slot, layer);
  double getSamplerFilterCutoff(int slot, int layer) =>
      _bridge.samplerGetFilterCutoff(slot, layer);
  double getSamplerResonance(int slot, int layer) =>
      _bridge.samplerGetResonance(slot, layer);
  double getSamplerEqLowFreq(int slot, int layer) =>
      _bridge.samplerGetEqLowFreq(slot, layer);
  double getSamplerEqLowGain(int slot, int layer) =>
      _bridge.samplerGetEqLowGain(slot, layer);
  double getSamplerEqHighFreq(int slot, int layer) =>
      _bridge.samplerGetEqHighFreq(slot, layer);
  double getSamplerEqHighGain(int slot, int layer) =>
      _bridge.samplerGetEqHighGain(slot, layer);
  int getSamplerLfoWave(int slot, int layer) =>
      _bridge.samplerGetLfoWave(slot, layer);
  double getSamplerLfoSpeed(int slot, int layer) =>
      _bridge.samplerGetLfoSpeed(slot, layer);
  double getSamplerLfoPmd(int slot, int layer) =>
      _bridge.samplerGetLfoPmd(slot, layer);
  double getSamplerLfoFmd(int slot, int layer) =>
      _bridge.samplerGetLfoFmd(slot, layer);
  double getSamplerLfoAmd(int slot, int layer) =>
      _bridge.samplerGetLfoAmd(slot, layer);
  double getSamplerVibratoDepth(int slot, int layer) =>
      _bridge.samplerGetVibratoDepth(slot, layer);
  double getSamplerVibratoDelay(int slot, int layer) =>
      _bridge.samplerGetVibratoDelay(slot, layer);
  double getSamplerVibratoSpeed(int slot, int layer) =>
      _bridge.samplerGetVibratoSpeed(slot, layer);

  // ── MIDI Output (Piano Sync) ────────────────────────────────────────────────

  /// Returns list of available MIDI output port names.
  List<String> getMidiOutputPorts() {
    final count = _bridge.midiOutputPortCount();
    return List.generate(count, (i) => _bridge.midiOutputPortName(i));
  }

  /// Open MIDI output port by index. Returns true on success.
  bool connectMidiOutput(int portIndex) {
    final ok = _bridge.midiOutputOpen(portIndex);
    state = state.copyWith(midiOutputConnected: ok);
    return ok;
  }

  /// Close the current MIDI output port.
  void disconnectMidiOutput() {
    _bridge.midiOutputClose();
    state = state.copyWith(midiOutputConnected: false);
  }

  // ── B00 Registration files ─────────────────────────────────────────────────

  /// Load a B00 file: parse it, optionally load a MID, update LCD state,
  /// send all small SysEx messages to piano, switch to registration 1.
  /// If [midPath] is provided it is used directly; otherwise the method tries
  /// to auto-discover a MID file in the same directory (may fail in sandbox).
  Future<String> loadB00(String filePath, {String? midPath}) async {
    if (!_bridge.hasEngine) return '未连接到引擎';

    final result = await B00Parser.parse(filePath);
    if (!result.isValid) return '解析失败: ${result.error}';

    // Cache for later registration switches
    _cachedB00Path = filePath;
    _cachedB00Result = result;

    final b00Name = filePath.split('/').last;

    // Use explicitly provided MID path first; fall back to auto-discovery
    String? resolvedMidPath = midPath;
    String? midName = midPath != null ? midPath.split('/').last : null;

    if (resolvedMidPath == null) {
      // Try auto-find — may silently fail under macOS App Sandbox
      try {
        final dir = File(filePath).parent;
        await for (final entity in dir.list()) {
          if (entity is File) {
            final name = entity.path.split('/').last;
            final lower = name.toLowerCase();
            if (lower.endsWith('.mid') || lower.endsWith('.midi')) {
              resolvedMidPath = entity.path;
              midName = name;
              break;
            }
          }
        }
      } catch (_) {}
    }

    // Update LCD state
    state = state.copyWith(
      loadedB00Name: b00Name,
      loadedMidPath: resolvedMidPath,
      loadedMidName: midName,
      currentRegistration: 1,
    );

    // Auto-load the MID file if found
    if (resolvedMidPath != null) {
      loadMidi(resolvedMidPath);
    }

    // Send registration 1 SysEx to piano
    await _sendRegistrationSysEx(result, 0);

    final midStatus = midName != null ? ', MID: $midName' : ', 未加载MID';
    return '已加载 $b00Name$midStatus (${result.registrationCount} 个Registration)';
  }

  /// Switch to a registration (1-8). Sends the corresponding SysEx group.
  Future<String> switchRegistration(int regNumber) async {
    if (regNumber < 1 || regNumber > 8) return 'Registration编号必须在1-8之间';

    final path = _cachedB00Path;
    final cached = _cachedB00Result;

    B00ParseResult result;
    if (cached != null && path != null) {
      result = cached;
    } else if (path != null) {
      result = await B00Parser.parse(path);
      _cachedB00Result = result;
    } else {
      return '未加载B00文件';
    }

    final regIndex = regNumber - 1;
    final msgs = result.registrationMessages(regIndex);
    if (msgs.isEmpty) return 'Registration $regNumber 不存在';

    state = state.copyWith(currentRegistration: regNumber);
    await _sendRegistrationSysEx(result, regIndex);
    return '已切换到 Registration $regNumber';
  }

  Future<void> _sendRegistrationSysEx(B00ParseResult result, int regIndex) async {
    final msgs = result.registrationMessages(regIndex)
        .where((m) => m.length < 2000)
        .toList();
    int sent = 0;
    for (final msg in msgs) {
      _bridge.midiOutputSendSysex(msg);
      sent++;
      if (sent % 10 == 0) {
        await Future<void>.delayed(const Duration(milliseconds: 20));
      }
    }
  }

  /// Send all SysEx messages from a B00 file to the connected piano.
  /// Returns a status message describing the result.
  Future<String> sendB00ToPiano(String filePath) async {
    if (!_bridge.hasEngine) return '未连接到引擎';

    final result = await B00Parser.parse(filePath);
    if (!result.isValid) {
      return '解析失败: ${result.error}';
    }

    // Skip the large bulk dump (first message, typically >50KB) when sending
    // to piano — it would take too long and may not be accepted.
    // Instead send only the registration parameter messages.
    final msgs = result.messages
        .where((m) => m.length < 2000)  // exclude large bulk dumps
        .toList();

    if (msgs.isEmpty) {
      return '文件中没有可发送的消息';
    }

    // Send each SysEx message with a small delay to avoid buffer overflow
    int sent = 0;
    for (final msg in msgs) {
      _bridge.midiOutputSendSysex(msg);
      sent++;
      // Small yield to avoid blocking UI thread
      if (sent % 10 == 0) {
        await Future<void>.delayed(const Duration(milliseconds: 20));
      }
    }

    return '已发送 $sent 条SysEx消息 (共 ${result.messages.length} 条, ${result.registrationCount} 个Registration)';
  }

  /// Send a specific registration group from a B00 file to the piano.
  Future<String> sendB00Registration(String filePath, int regIndex) async {
    if (!_bridge.hasEngine) return '未连接到引擎';

    final result = await B00Parser.parse(filePath);
    if (!result.isValid) {
      return '解析失败: ${result.error}';
    }

    final msgs = result.registrationMessages(regIndex);
    if (msgs.isEmpty) {
      return 'Registration $regIndex 不存在';
    }

    for (final msg in msgs) {
      _bridge.midiOutputSendSysex(msg);
    }

    return '已发送 Registration $regIndex (${msgs.length} 条消息)';
  }

  @override
  void dispose() {
    _pollTimer?.cancel();
    final bi = _beatInfoPtr;
    if (bi != null) calloc.free(bi);
    final dg = _drumGridPtr;
    if (dg != null) calloc.free(dg);
    _bridge.dispose();
    super.dispose();
  }
}

final engineProvider =
    StateNotifierProvider<EngineNotifier, EngineState>((ref) {
  return EngineNotifier();
});
