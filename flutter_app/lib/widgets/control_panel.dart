import 'package:flutter/material.dart';
import '../utils/colors.dart';
import '../utils/constants.dart';

class ControlPanel extends StatefulWidget {
  const ControlPanel({super.key, this.onCategorySelected});

  final void Function(int categoryIndex)? onCategorySelected;

  @override
  State<ControlPanel> createState() => _ControlPanelState();
}

class _ControlPanelState extends State<ControlPanel>
    with SingleTickerProviderStateMixin {
  late final TabController _tabController;
  int _selectedCategory = 0;

  static const _tabs = ['音色', '节奏', '注册', '移调'];

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: _tabs.length, vsync: this);
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        // Tab bar
        Container(
          color: EmColors.surface,
          child: TabBar(
            controller: _tabController,
            labelColor: EmColors.accent,
            unselectedLabelColor: EmColors.textSecondary,
            indicatorColor: EmColors.accent,
            indicatorWeight: 2,
            labelStyle: const TextStyle(
              fontSize: 13,
              fontWeight: FontWeight.bold,
            ),
            tabs: _tabs.map((t) => Tab(text: t)).toList(),
          ),
        ),
        // Tab content
        SizedBox(
          height: 280,
          child: TabBarView(
            controller: _tabController,
            children: [
              _VoiceCategoryGrid(
                selectedIndex: _selectedCategory,
                onSelected: (i) {
                  setState(() => _selectedCategory = i);
                  widget.onCategorySelected?.call(i);
                },
              ),
              _PlaceholderTab(label: '节奏类型'),
              _PlaceholderTab(label: '注册记忆'),
              _PlaceholderTab(label: '移调设置'),
            ],
          ),
        ),
      ],
    );
  }
}

class _VoiceCategoryGrid extends StatelessWidget {
  const _VoiceCategoryGrid({
    required this.selectedIndex,
    required this.onSelected,
  });

  final int selectedIndex;
  final void Function(int) onSelected;

  @override
  Widget build(BuildContext context) {
    return GridView.builder(
      padding: const EdgeInsets.all(8),
      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
        crossAxisCount: 3,
        mainAxisSpacing: 6,
        crossAxisSpacing: 6,
        childAspectRatio: 1.6,
      ),
      itemCount: EmConstants.voiceCategories.length,
      itemBuilder: (_, i) {
        final cat = EmConstants.voiceCategories[i];
        final isSelected = i == selectedIndex;
        return _CategoryButton(
          cn: cat['cn']!,
          en: cat['en']!,
          selected: isSelected,
          onTap: () => onSelected(i),
        );
      },
    );
  }
}

class _CategoryButton extends StatelessWidget {
  const _CategoryButton({
    required this.cn,
    required this.en,
    required this.selected,
    required this.onTap,
  });

  final String cn;
  final String en;
  final bool selected;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 100),
        decoration: BoxDecoration(
          color: selected
              ? EmColors.voiceButton.withOpacity(0.9)
              : EmColors.voiceButton.withOpacity(0.55),
          borderRadius: BorderRadius.circular(6),
          border: Border.all(
            color: selected
                ? EmColors.accent.withOpacity(0.8)
                : EmColors.voiceButton.withOpacity(0.8),
            width: selected ? 1.5 : 1,
          ),
          boxShadow: selected
              ? [
                  BoxShadow(
                    color: EmColors.accent.withOpacity(0.2),
                    blurRadius: 6,
                  )
                ]
              : null,
        ),
        padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 4),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Text(
              cn,
              style: TextStyle(
                color: EmColors.textSecondary,
                fontSize: 10,
              ),
            ),
            Center(
              child: Text(
                en,
                style: TextStyle(
                  color: selected ? EmColors.textPrimary : EmColors.textSecondary,
                  fontSize: 12,
                  fontWeight: FontWeight.w600,
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _PlaceholderTab extends StatelessWidget {
  const _PlaceholderTab({required this.label});
  final String label;

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Text(
        label,
        style: TextStyle(
          color: EmColors.textSecondary,
          fontSize: 14,
        ),
      ),
    );
  }
}
