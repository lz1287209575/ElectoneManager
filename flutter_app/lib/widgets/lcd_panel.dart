import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/engine_controller.dart';
import '../utils/colors.dart';
import '../utils/constants.dart';
import 'voice_card.dart';

class LcdPanel extends ConsumerWidget {
  const LcdPanel({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(engineProvider);

    return Container(
      color: EmColors.background,
      padding: const EdgeInsets.all(8),
      child: Column(
        children: [
          // Title row
          Padding(
            padding: const EdgeInsets.only(bottom: 6),
            child: Row(
              children: [
                Text(
                  'VOICE SLOTS',
                  style: TextStyle(
                    color: EmColors.textSecondary,
                    fontSize: 10,
                    letterSpacing: 1.5,
                  ),
                ),
                const Spacer(),
                Text(
                  'SLOT ${state.selectedSlot + 1}',
                  style: TextStyle(
                    color: EmColors.accent,
                    fontSize: 10,
                    letterSpacing: 1,
                  ),
                ),
              ],
            ),
          ),
          // 2×4 grid of voice cards
          SizedBox(
            height: 200,
            child: GridView.builder(
              physics: const NeverScrollableScrollPhysics(),
              gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                crossAxisCount: 2,
                mainAxisSpacing: 6,
                crossAxisSpacing: 6,
                childAspectRatio: 2.4,
              ),
              itemCount: EmConstants.voiceSlotCount,
              itemBuilder: (_, i) => VoiceCard(slotIndex: i),
            ),
          ),
        ],
      ),
    );
  }
}
