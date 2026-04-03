import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/engine_controller.dart';
import '../utils/colors.dart';

class TransportBar extends ConsumerStatefulWidget {
  const TransportBar({super.key});

  @override
  ConsumerState<TransportBar> createState() => _TransportBarState();
}

class _TransportBarState extends ConsumerState<TransportBar> {
  bool _dragging = false;
  double _localBpm = 120.0;

  @override
  Widget build(BuildContext context) {
    final state = ref.watch(engineProvider);
    final engine = ref.read(engineProvider.notifier);
    final transport = state.transport;

    if (!_dragging) {
      _localBpm = transport.bpm;
    }

    return Container(
      height: 56,
      color: EmColors.surface,
      padding: const EdgeInsets.symmetric(horizontal: 12),
      child: Row(
        children: [
          // Play/Stop buttons
          _TransportButton(
            icon: Icons.stop_rounded,
            active: !transport.isPlaying,
            color: EmColors.ledRed,
            onTap: engine.stop,
          ),
          const SizedBox(width: 6),
          _TransportButton(
            icon: Icons.play_arrow_rounded,
            active: transport.isPlaying,
            color: EmColors.ledGreen,
            onTap: engine.play,
          ),
          const SizedBox(width: 16),
          // Bar:Beat display
          Container(
            width: 72,
            height: 32,
            decoration: BoxDecoration(
              color: EmColors.background,
              borderRadius: BorderRadius.circular(4),
              border: Border.all(color: EmColors.divider),
            ),
            alignment: Alignment.center,
            child: Text(
              '${transport.bar.toString().padLeft(3, '0')} : ${transport.beat}',
              style: TextStyle(
                color: EmColors.accent,
                fontFamily: 'monospace',
                fontSize: 14,
                fontWeight: FontWeight.bold,
                letterSpacing: 1,
              ),
            ),
          ),
          const SizedBox(width: 16),
          // Tempo slider
          Expanded(
            child: SliderTheme(
              data: SliderTheme.of(context).copyWith(
                activeTrackColor: EmColors.accent,
                inactiveTrackColor: EmColors.sliderTrack,
                thumbColor: EmColors.accent,
                overlayColor: EmColors.accent.withOpacity(0.2),
                trackHeight: 3,
                thumbShape:
                    const RoundSliderThumbShape(enabledThumbRadius: 7),
              ),
              child: Slider(
                value: _localBpm.clamp(40.0, 240.0),
                min: 40.0,
                max: 240.0,
                onChangeStart: (_) => setState(() => _dragging = true),
                onChanged: (v) => setState(() => _localBpm = v),
                onChangeEnd: (v) {
                  setState(() => _dragging = false);
                  engine.setTempo(v);
                },
              ),
            ),
          ),
          const SizedBox(width: 8),
          // BPM number
          SizedBox(
            width: 48,
            child: Text(
              '${_localBpm.round()}',
              textAlign: TextAlign.center,
              style: TextStyle(
                color: EmColors.textPrimary,
                fontSize: 16,
                fontWeight: FontWeight.bold,
              ),
            ),
          ),
          Text(
            'BPM',
            style: TextStyle(
              color: EmColors.textSecondary,
              fontSize: 11,
            ),
          ),
        ],
      ),
    );
  }
}

class _TransportButton extends StatelessWidget {
  const _TransportButton({
    required this.icon,
    required this.active,
    required this.color,
    required this.onTap,
  });

  final IconData icon;
  final bool active;
  final Color color;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 100),
        width: 36,
        height: 36,
        decoration: BoxDecoration(
          color: active
              ? color.withOpacity(0.25)
              : EmColors.surfaceVariant,
          borderRadius: BorderRadius.circular(6),
          border: Border.all(
            color: active ? color : EmColors.divider,
            width: active ? 1.5 : 1,
          ),
          boxShadow: active
              ? [BoxShadow(color: color.withOpacity(0.4), blurRadius: 6)]
              : null,
        ),
        child: Icon(
          icon,
          color: active ? color : EmColors.textSecondary,
          size: 20,
        ),
      ),
    );
  }
}
