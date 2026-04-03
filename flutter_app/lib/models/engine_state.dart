import 'package:flutter/foundation.dart';
import 'voice_slot.dart';
import 'drum_pattern.dart';

@immutable
class TransportState {
  const TransportState({
    required this.isPlaying,
    required this.bpm,
    required this.bar,
    required this.beat,
    required this.currentStep,
    required this.midiProgress,
    required this.midiLoaded,
  });

  final bool isPlaying;
  final double bpm;
  final int bar;
  final int beat;
  final int currentStep;
  final double midiProgress;
  final bool midiLoaded;

  static const TransportState initial = TransportState(
    isPlaying: false,
    bpm: 120.0,
    bar: 1,
    beat: 1,
    currentStep: 0,
    midiProgress: 0.0,
    midiLoaded: false,
  );

  TransportState copyWith({
    bool? isPlaying,
    double? bpm,
    int? bar,
    int? beat,
    int? currentStep,
    double? midiProgress,
    bool? midiLoaded,
  }) {
    return TransportState(
      isPlaying: isPlaying ?? this.isPlaying,
      bpm: bpm ?? this.bpm,
      bar: bar ?? this.bar,
      beat: beat ?? this.beat,
      currentStep: currentStep ?? this.currentStep,
      midiProgress: midiProgress ?? this.midiProgress,
      midiLoaded: midiLoaded ?? this.midiLoaded,
    );
  }

  @override
  bool operator ==(Object other) {
    if (identical(this, other)) return true;
    return other is TransportState &&
        other.isPlaying == isPlaying &&
        other.bpm == bpm &&
        other.bar == bar &&
        other.beat == beat &&
        other.currentStep == currentStep &&
        other.midiProgress == midiProgress &&
        other.midiLoaded == midiLoaded;
  }

  @override
  int get hashCode =>
      Object.hash(isPlaying, bpm, bar, beat, currentStep, midiProgress, midiLoaded);
}

@immutable
class EngineState {
  const EngineState({
    required this.transport,
    required this.audioRunning,
    required this.midiConnected,
    required this.latencyMs,
    required this.drumPattern,
    required this.voiceSlots,
    required this.selectedSlot,
    required this.drumPresetIndex,
    this.loadedB00Name,
    this.loadedMidPath,
    this.loadedMidName,
    this.currentRegistration = 0,
    this.midiOutputConnected = false,
  });

  final TransportState transport;
  final bool audioRunning;
  final bool midiConnected;
  /// True when a MIDI output port is open (piano sync active)
  final bool midiOutputConnected;
  final double latencyMs;
  final DrumPattern drumPattern;
  final List<VoiceSlotInfo> voiceSlots;
  final int selectedSlot;
  final int drumPresetIndex;
  final String? loadedB00Name;
  final String? loadedMidPath;
  final String? loadedMidName;
  /// Currently selected Registration button (0 = none, 1-8 = buttons)
  final int currentRegistration;

  static EngineState initial() {
    return EngineState(
      transport: TransportState.initial,
      audioRunning: false,
      midiConnected: false,
      midiOutputConnected: false,
      latencyMs: 0.0,
      drumPattern: DrumPattern.empty(),
      voiceSlots: List.generate(
        8,
        (i) => VoiceSlotInfo(
          slotIndex: i,
          programNumber: 0,
          programName: 'Acoustic Grand Piano',
          layerCount: 1,
          layerNames: const ['Layer 0'],
        ),
      ),
      selectedSlot: 0,
      drumPresetIndex: 0,
    );
  }

  EngineState copyWith({
    TransportState? transport,
    bool? audioRunning,
    bool? midiConnected,
    bool? midiOutputConnected,
    double? latencyMs,
    DrumPattern? drumPattern,
    List<VoiceSlotInfo>? voiceSlots,
    int? selectedSlot,
    int? drumPresetIndex,
    Object? loadedB00Name = _sentinel,
    Object? loadedMidPath = _sentinel,
    Object? loadedMidName = _sentinel,
    int? currentRegistration,
  }) {
    return EngineState(
      transport: transport ?? this.transport,
      audioRunning: audioRunning ?? this.audioRunning,
      midiConnected: midiConnected ?? this.midiConnected,
      midiOutputConnected: midiOutputConnected ?? this.midiOutputConnected,
      latencyMs: latencyMs ?? this.latencyMs,
      drumPattern: drumPattern ?? this.drumPattern,
      voiceSlots: voiceSlots ?? this.voiceSlots,
      selectedSlot: selectedSlot ?? this.selectedSlot,
      drumPresetIndex: drumPresetIndex ?? this.drumPresetIndex,
      loadedB00Name: loadedB00Name == _sentinel
          ? this.loadedB00Name
          : loadedB00Name as String?,
      loadedMidPath: loadedMidPath == _sentinel
          ? this.loadedMidPath
          : loadedMidPath as String?,
      loadedMidName: loadedMidName == _sentinel
          ? this.loadedMidName
          : loadedMidName as String?,
      currentRegistration: currentRegistration ?? this.currentRegistration,
    );
  }

  static const Object _sentinel = Object();
}
