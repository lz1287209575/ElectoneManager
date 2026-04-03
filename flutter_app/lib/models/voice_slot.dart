import 'package:flutter/foundation.dart';

@immutable
class VoiceSlotInfo {
  const VoiceSlotInfo({
    required this.slotIndex,
    required this.programNumber,
    required this.programName,
    required this.layerCount,
    this.layerNames = const [],
    this.enabled = true,
    this.layerEnabled = const [],
  });

  final int slotIndex;
  final int programNumber;
  final String programName;
  final int layerCount;
  final List<String> layerNames;
  final bool enabled;
  final List<bool> layerEnabled;

  /// Returns layerEnabled, padding with true if shorter than layerCount.
  List<bool> get effectiveLayerEnabled => List.generate(
        layerCount,
        (i) => i < layerEnabled.length ? layerEnabled[i] : true,
      );

  VoiceSlotInfo copyWith({
    int? slotIndex,
    int? programNumber,
    String? programName,
    int? layerCount,
    List<String>? layerNames,
    bool? enabled,
    List<bool>? layerEnabled,
  }) {
    return VoiceSlotInfo(
      slotIndex: slotIndex ?? this.slotIndex,
      programNumber: programNumber ?? this.programNumber,
      programName: programName ?? this.programName,
      layerCount: layerCount ?? this.layerCount,
      layerNames: layerNames ?? this.layerNames,
      enabled: enabled ?? this.enabled,
      layerEnabled: layerEnabled ?? this.layerEnabled,
    );
  }

  static const VoiceSlotInfo empty = VoiceSlotInfo(
    slotIndex: 0,
    programNumber: 0,
    programName: 'Acoustic Grand Piano',
    layerCount: 1,
    layerNames: ['Layer 0'],
  );

  @override
  bool operator ==(Object other) {
    if (identical(this, other)) return true;
    return other is VoiceSlotInfo &&
        other.slotIndex == slotIndex &&
        other.programNumber == programNumber &&
        other.programName == programName &&
        other.layerCount == layerCount &&
        other.enabled == enabled;
  }

  @override
  int get hashCode =>
      Object.hash(slotIndex, programNumber, programName, layerCount, enabled);
}
