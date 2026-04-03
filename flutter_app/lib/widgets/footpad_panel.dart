import 'package:flutter/material.dart';
import '../utils/colors.dart';

class FootpadPanel extends StatefulWidget {
  const FootpadPanel({super.key, this.vertical = false});

  final bool vertical;

  @override
  State<FootpadPanel> createState() => _FootpadPanelState();
}

class _FootpadPanelState extends State<FootpadPanel> {
  double _expression = 0.5;
  bool _swL = false;
  bool _swR = false;

  @override
  Widget build(BuildContext context) {
    return Container(
      color: EmColors.surface,
      padding: const EdgeInsets.all(8),
      child: widget.vertical
          ? _buildVertical()
          : _buildHorizontal(),
    );
  }

  Widget _buildVertical() {
    return Row(
      children: [
        // Vertical expression slider
        Column(
          children: [
            Text(
              'EXP',
              style: TextStyle(color: EmColors.textSecondary, fontSize: 10),
            ),
            const SizedBox(height: 4),
            Expanded(
              child: RotatedBox(
                quarterTurns: -1,
                child: _expressionSlider(),
              ),
            ),
          ],
        ),
        const SizedBox(width: 12),
        // Buttons column
        Column(
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          children: _footButtons(),
        ),
      ],
    );
  }

  Widget _buildHorizontal() {
    return Row(
      children: [
        Text(
          'EXP',
          style: TextStyle(color: EmColors.textSecondary, fontSize: 10),
        ),
        const SizedBox(width: 8),
        Expanded(child: _expressionSlider()),
        const SizedBox(width: 12),
        ..._footButtons(),
      ],
    );
  }

  Widget _expressionSlider() {
    return SliderTheme(
      data: SliderTheme.of(context).copyWith(
        activeTrackColor: EmColors.accentCyan,
        inactiveTrackColor: EmColors.sliderTrack,
        thumbColor: EmColors.accentCyan,
        overlayColor: EmColors.accentCyan.withOpacity(0.2),
        trackHeight: 4,
        thumbShape: const RoundSliderThumbShape(enabledThumbRadius: 8),
      ),
      child: Slider(
        value: _expression,
        min: 0,
        max: 1,
        onChanged: (v) => setState(() => _expression = v),
      ),
    );
  }

  List<Widget> _footButtons() {
    return [
      _FootButton(
        label: 'SW.L',
        active: _swL,
        onTap: () => setState(() => _swL = !_swL),
      ),
      const SizedBox(width: 6, height: 6),
      _FootButton(
        label: 'SW.R',
        active: _swR,
        onTap: () => setState(() => _swR = !_swR),
      ),
    ];
  }
}

class _FootButton extends StatelessWidget {
  const _FootButton({
    required this.label,
    required this.active,
    required this.onTap,
  });

  final String label;
  final bool active;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 80),
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
        decoration: BoxDecoration(
          color: active
              ? EmColors.accent.withOpacity(0.25)
              : EmColors.surfaceVariant,
          borderRadius: BorderRadius.circular(5),
          border: Border.all(
            color: active ? EmColors.accent : EmColors.divider,
            width: active ? 1.5 : 1,
          ),
        ),
        child: Text(
          label,
          style: TextStyle(
            color: active ? EmColors.accent : EmColors.textSecondary,
            fontSize: 11,
            fontWeight: FontWeight.bold,
          ),
        ),
      ),
    );
  }
}
