import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/engine_controller.dart';
import '../utils/colors.dart';
import '../utils/constants.dart';
import '../widgets/drum_grid.dart';

class DrumSequencerScreen extends ConsumerWidget {
  const DrumSequencerScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(engineProvider);
    final engine = ref.read(engineProvider.notifier);
    return Scaffold(
      backgroundColor: EmColors.background,
      appBar: AppBar(
        backgroundColor: EmColors.surface,
        foregroundColor: EmColors.textPrimary,
        title: const Text('节奏编程器'),
        actions: [
          TextButton(
            onPressed: () {
              showDialog(
                context: context,
                builder: (_) => AlertDialog(
                  backgroundColor: EmColors.surface,
                  title: Text(
                    '清除节拍',
                    style: TextStyle(color: EmColors.textPrimary),
                  ),
                  content: Text(
                    '确认清除当前节拍模式？',
                    style: TextStyle(color: EmColors.textSecondary),
                  ),
                  actions: [
                    TextButton(
                      onPressed: () => Navigator.pop(context),
                      child: Text('取消',
                          style: TextStyle(color: EmColors.textSecondary)),
                    ),
                    TextButton(
                      onPressed: () {
                        engine.clearDrumPattern();
                        Navigator.pop(context);
                      },
                      child: Text('清除',
                          style: TextStyle(color: EmColors.ledRed)),
                    ),
                  ],
                ),
              );
            },
            child: Text(
              '清除',
              style: TextStyle(color: EmColors.ledRed),
            ),
          ),
        ],
      ),
      body: Column(
        children: [
          // Preset selector
          Container(
            height: 50,
            color: EmColors.surface,
            padding: const EdgeInsets.symmetric(horizontal: 12),
            child: Row(
              children: [
                Text(
                  '预设:',
                  style: TextStyle(color: EmColors.textSecondary, fontSize: 12),
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: ListView.builder(
                    scrollDirection: Axis.horizontal,
                    itemCount: EmConstants.rhythmPresetNames.length,
                    itemBuilder: (_, i) {
                      final sel = i == state.drumPresetIndex;
                      return GestureDetector(
                        onTap: () => engine.setDrumPreset(i),
                        child: AnimatedContainer(
                          duration: const Duration(milliseconds: 80),
                          margin: const EdgeInsets.symmetric(
                              horizontal: 3, vertical: 8),
                          padding: const EdgeInsets.symmetric(
                              horizontal: 10, vertical: 2),
                          decoration: BoxDecoration(
                            color: sel
                                ? EmColors.accent.withOpacity(0.2)
                                : EmColors.surfaceVariant,
                            borderRadius: BorderRadius.circular(4),
                            border: Border.all(
                              color: sel ? EmColors.accent : EmColors.divider,
                            ),
                          ),
                          child: Text(
                            EmConstants.rhythmPresetNames[i],
                            style: TextStyle(
                              color: sel
                                  ? EmColors.accent
                                  : EmColors.textSecondary,
                              fontSize: 11,
                              fontWeight: sel
                                  ? FontWeight.bold
                                  : FontWeight.normal,
                            ),
                          ),
                        ),
                      );
                    },
                  ),
                ),
              ],
            ),
          ),
          // Pattern name
          Padding(
            padding:
                const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
            child: Row(
              children: [
                Text(
                  state.drumPattern.patternName,
                  style: TextStyle(
                    color: EmColors.textSecondary,
                    fontSize: 11,
                    fontStyle: FontStyle.italic,
                  ),
                ),
                const Spacer(),
                // Step count label
                ...List.generate(EmConstants.drumSteps, (i) {
                  if (i % 4 != 0) return const SizedBox.shrink();
                  return SizedBox(
                    width: 20,
                    child: Text(
                      '${i + 1}',
                      style: TextStyle(
                        color: EmColors.textDim,
                        fontSize: 9,
                      ),
                      textAlign: TextAlign.center,
                    ),
                  );
                }),
              ],
            ),
          ),
          // Drum grid
          const Expanded(
            child: Padding(
              padding: EdgeInsets.symmetric(horizontal: 8, vertical: 4),
              child: DrumGrid(),
            ),
          ),
          // Row legend
          Padding(
            padding: const EdgeInsets.only(bottom: 8, left: 8, right: 8),
            child: Wrap(
              spacing: 8,
              runSpacing: 4,
              children: List.generate(EmConstants.drumRows, (i) {
                final color = EmColors.drumRowColors[i];
                return Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Container(
                      width: 8,
                      height: 8,
                      decoration: BoxDecoration(
                        color: color,
                        shape: BoxShape.circle,
                      ),
                    ),
                    const SizedBox(width: 3),
                    Text(
                      EmConstants.drumInstrumentFullNames[i],
                      style: TextStyle(
                        color: EmColors.textSecondary,
                        fontSize: 10,
                      ),
                    ),
                  ],
                );
              }),
            ),
          ),
        ],
      ),
    );
  }
}

// Thin shim — avoids direct dependency on EmBridge in the screen
int EmBridge_presetCount() {
  try {
    return EmConstants.rhythmPresetNames.length;
  } catch (_) {
    return 16;
  }
}
