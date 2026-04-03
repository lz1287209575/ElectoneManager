import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/engine_controller.dart';
import '../utils/colors.dart';

class StatusBar extends ConsumerWidget {
  const StatusBar({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(engineProvider);

    return Container(
      height: 36,
      color: EmColors.surface,
      padding: const EdgeInsets.symmetric(horizontal: 12),
      child: Row(
        children: [
          Flexible(
            child: Text(
            'ElectoneManager',
            style: TextStyle(
              color: EmColors.accent,
              fontSize: 14,
              fontWeight: FontWeight.bold,
              letterSpacing: 1.2,
            ),
            overflow: TextOverflow.ellipsis,
          ),
          ),
          const SizedBox(width: 8),
          Text(
            'ELS-02C',
            style: TextStyle(
              color: EmColors.textSecondary,
              fontSize: 11,
            ),
          ),
          const Spacer(),
          // Audio status
          _Led(active: state.audioRunning, color: EmColors.ledGreen),
          const SizedBox(width: 4),
          Text(
            state.audioRunning
                ? '${state.latencyMs.toStringAsFixed(1)} ms'
                : 'Audio Off',
            style: TextStyle(
              color: state.audioRunning
                  ? EmColors.textSecondary
                  : EmColors.ledRed,
              fontSize: 11,
            ),
          ),
          const SizedBox(width: 16),
          // MIDI status
          _Led(active: state.midiConnected, color: EmColors.ledCyan),
          const SizedBox(width: 4),
          Text(
            state.midiConnected ? 'MIDI' : 'No MIDI',
            style: TextStyle(
              color: state.midiConnected
                  ? EmColors.accentCyan
                  : EmColors.textSecondary,
              fontSize: 11,
            ),
          ),
        ],
      ),
    );
  }
}

class _Led extends StatelessWidget {
  const _Led({required this.active, required this.color});
  final bool active;
  final Color color;

  @override
  Widget build(BuildContext context) {
    return Container(
      width: 8,
      height: 8,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        color: active ? color : EmColors.ledOff,
        boxShadow: active
            ? [BoxShadow(color: color.withOpacity(0.6), blurRadius: 4)]
            : null,
      ),
    );
  }
}
