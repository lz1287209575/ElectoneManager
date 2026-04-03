import 'dart:async';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../bindings/em_bridge.dart';

/// Holds the scrollable log lines (newest at the bottom).
/// Capped at [_kMaxLines] to avoid unbounded growth.
class LogNotifier extends StateNotifier<List<String>> {
  LogNotifier() : super(const []) {
    _timer = Timer.periodic(
      const Duration(milliseconds: 100), // 10 Hz
      (_) => _drain(),
    );
  }

  static const int _kMaxLines = 500;
  final EmBridge _bridge = EmBridge.instance;
  Timer? _timer;

  void _drain() {
    final batch = <String>[];
    // Drain up to 64 lines per tick to avoid blocking the UI thread
    for (int i = 0; i < 64; i++) {
      final line = _bridge.logPoll();
      if (line == null) break;
      batch.add(line);
    }
    if (batch.isEmpty) return;

    final next = [...state, ...batch];
    state = next.length > _kMaxLines
        ? next.sublist(next.length - _kMaxLines)
        : next;
  }

  void clear() => state = const [];

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }
}

final logProvider = StateNotifierProvider<LogNotifier, List<String>>(
  (ref) => LogNotifier(),
);
