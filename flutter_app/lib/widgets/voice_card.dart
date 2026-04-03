import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/engine_controller.dart';
import '../utils/colors.dart';
import '../utils/constants.dart';
import '../screens/voice_editor_screen.dart';

class VoiceCard extends ConsumerStatefulWidget {
  const VoiceCard({super.key, required this.slotIndex});
  final int slotIndex;

  @override
  ConsumerState<VoiceCard> createState() => _VoiceCardState();
}

class _VoiceCardState extends ConsumerState<VoiceCard>
    with SingleTickerProviderStateMixin {
  bool _expanded = false;
  late final AnimationController _ctrl;

  // 卡片始终固定总高度
  static const double _cardH = 140.0;
  static const double _layerH = 48.0;

  @override
  void initState() {
    super.initState();
    _ctrl = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 180),
    );
  }

  @override
  void dispose() {
    _ctrl.dispose();
    super.dispose();
  }

  void _toggle() {
    setState(() => _expanded = !_expanded);
    if (_expanded) _ctrl.forward();
    else _ctrl.reverse();
  }

  void _openEditor(int layer) {
    ref.read(engineProvider.notifier).selectSlot(widget.slotIndex);
    Navigator.of(context).push(MaterialPageRoute(
      builder: (_) => VoiceEditorScreen(
        slotIndex: widget.slotIndex,
        initialLayer: layer,
      ),
      fullscreenDialog: true,
    ));
  }

  static Color _cardBg(int slot) {
    if (slot < 2) return EmColors.cardUpper;
    if (slot < 4) return EmColors.cardLower;
    if (slot < 6) return EmColors.cardLead;
    return EmColors.cardPedal;
  }

  @override
  Widget build(BuildContext context) {
    final state = ref.watch(engineProvider);
    final slot = state.voiceSlots[widget.slotIndex];
    final isSelected = state.selectedSlot == widget.slotIndex;
    final enabled = slot.enabled;
    final layerEnabled = slot.effectiveLayerEnabled;
    final accent = isSelected ? EmColors.accent : EmColors.accentCyan;

    return SizedBox(
      height: _cardH,
      child: AnimatedOpacity(
        duration: const Duration(milliseconds: 150),
        opacity: enabled ? 1.0 : 0.5,
        child: DecoratedBox(
          decoration: BoxDecoration(
            color: _cardBg(widget.slotIndex),
            borderRadius: BorderRadius.circular(6),
            border: Border.all(
              color: isSelected ? EmColors.accent : EmColors.divider,
              width: isSelected ? 1.5 : 0.5,
            ),
            boxShadow: isSelected && enabled
                ? [BoxShadow(
                    color: EmColors.accent.withOpacity(0.2),
                    blurRadius: 6,
                    spreadRadius: 0,
                  )]
                : null,
          ),
          child: ClipRRect(
            borderRadius: BorderRadius.circular(6),
            child: Material(
              color: Colors.transparent,
              child: Stack(
                children: [
                  // ── 主体内容（单击展开，长按编辑）
                  Positioned.fill(
                    bottom: _expanded ? _layerH : 0,
                    child: InkWell(
                      onTap: _toggle,
                      onLongPress: () => _openEditor(0),
                      child: Padding(
                        padding: const EdgeInsets.fromLTRB(10, 8, 8, 6),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            // 顶行
                            Row(
                              crossAxisAlignment: CrossAxisAlignment.center,
                              children: [
                                Text(
                                  EmConstants.slotRoleNames[widget.slotIndex],
                                  style: TextStyle(
                                    color: EmColors.textSecondary,
                                    fontSize: 9,
                                    letterSpacing: 0.6,
                                    fontWeight: FontWeight.w500,
                                  ),
                                ),
                                const SizedBox(width: 4),
                                Text(
                                  EmConstants.slotRoleNamesCn[widget.slotIndex],
                                  style: TextStyle(
                                    color: EmColors.textDim,
                                    fontSize: 9,
                                  ),
                                ),
                                const Spacer(),
                                // 展开指示箭头
                                AnimatedRotation(
                                  turns: _expanded ? 0.5 : 0,
                                  duration: const Duration(milliseconds: 180),
                                  child: Icon(
                                    Icons.expand_more_rounded,
                                    color: EmColors.textDim,
                                    size: 15,
                                  ),
                                ),
                                const SizedBox(width: 6),
                                // 启用/禁用开关（小方块风格）
                                _SlotToggle(
                                  enabled: enabled,
                                  accent: accent,
                                  onTap: () => ref
                                      .read(engineProvider.notifier)
                                      .toggleSlotEnabled(widget.slotIndex),
                                ),
                              ],
                            ),
                            const SizedBox(height: 5),
                            // 音色名
                            Expanded(
                              child: Text(
                                slot.programName,
                                style: TextStyle(
                                  color: enabled
                                      ? EmColors.textPrimary
                                      : EmColors.textSecondary,
                                  fontSize: 18,
                                  fontWeight: FontWeight.w600,
                                  height: 1.2,
                                ),
                                maxLines: 2,
                                overflow: TextOverflow.ellipsis,
                              ),
                            ),
                            // 底部 P 编号 + 层数
                            Row(
                              crossAxisAlignment: CrossAxisAlignment.center,
                              children: [
                                Text(
                                  'P${slot.programNumber.toString().padLeft(3, '0')}',
                                  style: TextStyle(
                                    color: EmColors.accentCyan,
                                    fontSize: 9,
                                    fontFamily: 'monospace',
                                    fontWeight: FontWeight.w500,
                                  ),
                                ),
                                if (slot.layerCount > 1) ...[
                                  const SizedBox(width: 6),
                                  Container(
                                    padding: const EdgeInsets.symmetric(
                                        horizontal: 4, vertical: 1),
                                    decoration: BoxDecoration(
                                      color: accent.withOpacity(0.15),
                                      borderRadius: BorderRadius.circular(3),
                                    ),
                                    child: Text(
                                      '${slot.layerCount} layers',
                                      style: TextStyle(
                                        color: accent,
                                        fontSize: 8,
                                        fontWeight: FontWeight.w600,
                                      ),
                                    ),
                                  ),
                                ],
                              ],
                            ),
                          ],
                        ),
                      ),
                    ),
                  ),

                  // ── 底部层条（动画滑入）
                  Positioned(
                    left: 0,
                    right: 0,
                    bottom: 0,
                    child: AnimatedContainer(
                      duration: const Duration(milliseconds: 180),
                      curve: Curves.easeInOut,
                      height: _expanded ? _layerH : 0,
                      child: SingleChildScrollView(
                        physics: const NeverScrollableScrollPhysics(),
                        child: Container(
                          height: _layerH,
                          decoration: BoxDecoration(
                            color: Colors.black.withOpacity(0.25),
                            border: Border(
                              top: BorderSide(
                                  color: EmColors.divider.withOpacity(0.5),
                                  width: 0.5),
                            ),
                          ),
                          child: Row(
                            children: [
                              Expanded(
                                child: ListView.builder(
                                  scrollDirection: Axis.horizontal,
                                  padding: const EdgeInsets.symmetric(
                                      horizontal: 6, vertical: 6),
                                  itemCount: slot.layerCount,
                                  itemBuilder: (_, i) {
                                    final name = i < slot.layerNames.length
                                        ? slot.layerNames[i]
                                        : 'Layer ${i + 1}';
                                    final lOn = layerEnabled[i];
                                    return _LayerChip(
                                      index: i,
                                      name: name,
                                      enabled: lOn,
                                      accent: accent,
                                      onTap: () => _openEditor(i),
                                      onToggle: () => ref
                                          .read(engineProvider.notifier)
                                          .toggleLayerEnabled(
                                              widget.slotIndex, i),
                                    );
                                  },
                                ),
                              ),
                              // 添加层
                              GestureDetector(
                                onTap: () => ref
                                    .read(engineProvider.notifier)
                                    .addLayer(widget.slotIndex),
                                child: SizedBox(
                                  width: 30,
                                  child: Icon(
                                    Icons.add_rounded,
                                    size: 16,
                                    color: accent.withOpacity(0.7),
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }
}

// ── 启用/禁用小开关 ─────────────────────────────────────────────────────────

class _SlotToggle extends StatelessWidget {
  const _SlotToggle({
    required this.enabled,
    required this.accent,
    required this.onTap,
  });

  final bool enabled;
  final Color accent;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 150),
        width: 28,
        height: 14,
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(7),
          color: enabled
              ? accent.withOpacity(0.3)
              : Colors.black.withOpacity(0.3),
          border: Border.all(
            color: enabled ? accent.withOpacity(0.6) : EmColors.divider,
            width: 0.5,
          ),
        ),
        child: Stack(
          children: [
            AnimatedPositioned(
              duration: const Duration(milliseconds: 150),
              curve: Curves.easeInOut,
              left: enabled ? 15 : 1,
              top: 1,
              child: Container(
                width: 12,
                height: 12,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  color: enabled ? accent : EmColors.textDim,
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// ── 层卡片 ──────────────────────────────────────────────────────────────────

class _LayerChip extends StatelessWidget {
  const _LayerChip({
    required this.index,
    required this.name,
    required this.enabled,
    required this.accent,
    required this.onTap,
    required this.onToggle,
  });

  final int index;
  final String name;
  final bool enabled;
  final Color accent;
  final VoidCallback onTap;
  final VoidCallback onToggle;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: AnimatedOpacity(
        duration: const Duration(milliseconds: 150),
        opacity: enabled ? 1.0 : 0.4,
        child: Container(
          width: 68,
          margin: const EdgeInsets.only(right: 4),
          decoration: BoxDecoration(
            color: index == 0
                ? accent.withOpacity(0.15)
                : Colors.white.withOpacity(0.06),
            borderRadius: BorderRadius.circular(4),
            border: Border.all(
              color: index == 0
                  ? accent.withOpacity(0.4)
                  : Colors.white.withOpacity(0.1),
              width: 0.5,
            ),
          ),
          padding: const EdgeInsets.fromLTRB(5, 3, 3, 3),
          child: Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      'L${index + 1}',
                      style: TextStyle(
                        color: index == 0 ? accent : EmColors.textDim,
                        fontSize: 8,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const SizedBox(height: 2),
                    Text(
                      name,
                      style: TextStyle(
                        color: EmColors.textPrimary,
                        fontSize: 9,
                        fontWeight: index == 0
                            ? FontWeight.w600
                            : FontWeight.normal,
                      ),
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                    ),
                  ],
                ),
              ),
              // Layer 启用小点
              GestureDetector(
                onTap: onToggle,
                behavior: HitTestBehavior.opaque,
                child: Padding(
                  padding: const EdgeInsets.only(top: 2),
                  child: Container(
                    width: 6,
                    height: 6,
                    decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      color: enabled
                          ? (index == 0 ? accent : EmColors.accentCyan)
                          : EmColors.divider,
                    ),
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
