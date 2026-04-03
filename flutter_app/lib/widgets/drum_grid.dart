import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/engine_controller.dart';
import '../utils/colors.dart';
import '../utils/constants.dart';

class DrumGrid extends ConsumerWidget {
  const DrumGrid({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(engineProvider);
    final engine = ref.read(engineProvider.notifier);
    final pattern = state.drumPattern;
    final currentStep = state.transport.currentStep;
    final isPlaying = state.transport.isPlaying;

    return LayoutBuilder(builder: (context, constraints) {
      return Column(
        children: List.generate(EmConstants.drumRows, (row) {
          final rowColor = EmColors.drumRowColors[row];
          return Expanded(
            child: Row(
              children: [
                // Row label
                SizedBox(
                  width: 40,
                  child: Text(
                    EmConstants.drumInstrumentNames[row],
                    style: TextStyle(
                      color: rowColor,
                      fontSize: 10,
                      fontWeight: FontWeight.bold,
                    ),
                    textAlign: TextAlign.right,
                  ),
                ),
                const SizedBox(width: 4),
                // Steps
                ...List.generate(EmConstants.drumSteps, (step) {
                  final active = pattern.isActive(row, step);
                  final isCurrent = isPlaying && step == currentStep;
                  return Expanded(
                    child: GestureDetector(
                      onTap: () => engine.toggleDrumStep(row, step),
                      child: AnimatedContainer(
                        duration: const Duration(milliseconds: 60),
                        margin: const EdgeInsets.all(1.5),
                        decoration: BoxDecoration(
                          color: active
                              ? rowColor.withOpacity(0.85)
                              : isCurrent
                                  ? EmColors.surfaceVariant.withOpacity(0.9)
                                  : EmColors.surface,
                          borderRadius: BorderRadius.circular(2),
                          border: Border.all(
                            color: isCurrent
                                ? EmColors.accent.withOpacity(0.6)
                                : active
                                    ? rowColor.withOpacity(0.4)
                                    : EmColors.divider,
                            width: isCurrent ? 1.5 : 1,
                          ),
                          boxShadow: active
                              ? [
                                  BoxShadow(
                                    color: rowColor.withOpacity(0.5),
                                    blurRadius: 4,
                                  )
                                ]
                              : null,
                        ),
                        // Beat group separator: groups of 4
                        child: step % 4 == 0 && step > 0
                            ? Align(
                                alignment: Alignment.centerLeft,
                                child: Container(
                                  width: 1,
                                  height: double.infinity,
                                  color: EmColors.divider,
                                ),
                              )
                            : null,
                      ),
                    ),
                  );
                }),
              ],
            ),
          );
        }),
      );
    });
  }
}
