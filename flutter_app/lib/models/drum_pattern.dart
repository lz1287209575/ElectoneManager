import 'package:flutter/foundation.dart';

@immutable
class DrumPattern {
  const DrumPattern({
    required this.grid,
    required this.patternName,
    required this.presetIndex,
  });

  /// 12 rows × 16 steps; value is velocity (0 = off, 1-127 = on)
  final List<List<int>> grid;
  final String patternName;
  final int presetIndex;

  static DrumPattern empty() {
    return DrumPattern(
      grid: List.generate(12, (_) => List.filled(16, 0)),
      patternName: 'Empty',
      presetIndex: 0,
    );
  }

  bool isActive(int row, int step) {
    if (row < 0 || row >= grid.length) return false;
    if (step < 0 || step >= grid[row].length) return false;
    return grid[row][step] > 0;
  }

  int velocity(int row, int step) {
    if (row < 0 || row >= grid.length) return 0;
    if (step < 0 || step >= grid[row].length) return 0;
    return grid[row][step];
  }

  DrumPattern copyWith({
    List<List<int>>? grid,
    String? patternName,
    int? presetIndex,
  }) {
    return DrumPattern(
      grid: grid ?? this.grid,
      patternName: patternName ?? this.patternName,
      presetIndex: presetIndex ?? this.presetIndex,
    );
  }

  @override
  bool operator ==(Object other) {
    if (identical(this, other)) return true;
    return other is DrumPattern &&
        other.patternName == patternName &&
        other.presetIndex == presetIndex;
  }

  @override
  int get hashCode => Object.hash(patternName, presetIndex);
}
