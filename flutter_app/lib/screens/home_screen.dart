import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:file_picker/file_picker.dart';
import '../providers/engine_controller.dart';
import '../providers/log_provider.dart';
import '../utils/colors.dart';
import '../utils/constants.dart';
import '../utils/b00_parser.dart';
import '../widgets/status_bar.dart';
import '../widgets/transport_bar.dart';
import '../widgets/footpad_panel.dart';
import '../widgets/voice_card.dart';
import 'drum_sequencer_screen.dart';

class HomeScreen extends ConsumerStatefulWidget {
  const HomeScreen({super.key});

  @override
  ConsumerState<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends ConsumerState<HomeScreen>
    with SingleTickerProviderStateMixin {
  late final TabController _tabController;

  // Voice category selector
  int _categoryIndex = 0;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 4, vsync: this);
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: EmColors.background,
      body: SafeArea(
        child: Column(
          children: [
            // ── Top bar: status + tab switcher ──────────────────────────────
            _buildTopBar(),

            // ── Main content (tabs) ──────────────────────────────────────────
            Expanded(
              child: TabBarView(
                controller: _tabController,
                physics: const NeverScrollableScrollPhysics(),
                children: [
                  _buildVoiceTab(),
                  _buildRhythmTab(),
                  _buildEditorsTab(),
                  const _LogPanel(),
                ],
              ),
            ),

            // ── Transport + Footpad ──────────────────────────────────────────
            const TransportBar(),
            const FootpadPanel(),
          ],
        ),
      ),
    );
  }

  // ── Top bar: status info on left, tabs on right ──────────────────────────
  Widget _buildTopBar() {
    return Container(
      height: 36,
      color: EmColors.surface,
      child: Row(
        children: [
          // Status bar content (engine name etc.)
          const Expanded(child: StatusBar()),
          // B00 import button
          IconButton(
            icon: const Icon(Icons.folder_open, size: 16),
            color: EmColors.textSecondary,
            tooltip: 'Import B00',
            padding: const EdgeInsets.symmetric(horizontal: 4),
            constraints: const BoxConstraints(minWidth: 28, minHeight: 28),
            onPressed: _importB00,
          ),
          // Piano sync button
          Consumer(builder: (context, ref, _) {
            final connected = ref.watch(
                engineProvider.select((s) => s.midiOutputConnected));
            return IconButton(
              icon: Icon(
                connected ? Icons.piano : Icons.piano_off,
                size: 16,
              ),
              color: connected ? Colors.greenAccent : EmColors.textSecondary,
              tooltip: connected ? '已连接钢琴 (点击断开)' : '连接钢琴',
              padding: const EdgeInsets.symmetric(horizontal: 4),
              constraints: const BoxConstraints(minWidth: 28, minHeight: 28),
              onPressed: () => _showPianoSyncDialog(connected),
            );
          }),
          // Tab selector (音色 / 乐谱 / 节奏)
          TabBar(
            controller: _tabController,
            isScrollable: true,
            labelColor: EmColors.accent,
            unselectedLabelColor: EmColors.textSecondary,
            indicatorColor: EmColors.accent,
            indicatorWeight: 2,
            labelStyle: const TextStyle(
              fontSize: 12,
              fontWeight: FontWeight.bold,
            ),
            tabs: const [
              Tab(text: '音色'),
              Tab(text: '乐谱'),
              Tab(text: '节奏'),
              Tab(text: 'LOG'),
            ],
          ),
        ],
      ),
    );
  }

  Future<void> _importB00() async {
    final result = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['B00', 'b00'],
      dialogTitle: '选择 B00 Registration 文件',
    );
    if (result == null || result.files.isEmpty) return;
    final path = result.files.first.path;
    if (path == null) return;

    // Quick validation
    final isValid = await B00Parser.isValidB00(path);
    if (!mounted) return;
    if (!isValid) {
      _showB00Status('文件格式无效，不是合法的B00文件', isError: true);
      return;
    }

    // Parse to get info
    final parseResult = await B00Parser.parse(path);
    if (!mounted) return;
    if (!parseResult.isValid) {
      _showB00Status('解析失败: ${parseResult.error}', isError: true);
      return;
    }

    // Show confirmation dialog with file info
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (ctx) => _B00ImportDialog(
        filePath: path,
        fileName: result.files.first.name,
        parseResult: parseResult,
      ),
    );
    if (confirmed != true || !mounted) return;

    // Let user pick an optional MID file
    String? midPath;
    final midResult = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['mid', 'MID', 'midi', 'MIDI'],
      dialogTitle: '选择对应的 MID 伴奏文件 (可跳过)',
    );
    if (midResult != null && midResult.files.isNotEmpty) {
      midPath = midResult.files.first.path;
    }
    if (!mounted) return;

    // Load B00 (sends SysEx + loads MID if provided + updates LCD state)
    final engine = ref.read(engineProvider.notifier);
    final status = await engine.loadB00(path, midPath: midPath);
    if (mounted) {
      _showB00Status(status);
    }
  }

  void _showB00Status(String message, {bool isError = false}) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message, style: const TextStyle(fontSize: 12)),
        backgroundColor: isError ? Colors.red.shade800 : EmColors.surface,
        duration: const Duration(seconds: 4),
        behavior: SnackBarBehavior.floating,
      ),
    );
  }

  Future<void> _showPianoSyncDialog(bool currentlyConnected) async {
    final engine = ref.read(engineProvider.notifier);

    if (currentlyConnected) {
      // Confirm disconnect
      final ok = await showDialog<bool>(
        context: context,
        builder: (ctx) => AlertDialog(
          backgroundColor: EmColors.surface,
          title: Text('断开钢琴连接',
              style: TextStyle(color: EmColors.textPrimary, fontSize: 14)),
          content: Text('断开后MID播放事件将不再发送到钢琴。',
              style: TextStyle(color: EmColors.textSecondary, fontSize: 12)),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: Text('取消', style: TextStyle(color: EmColors.textSecondary)),
            ),
            TextButton(
              onPressed: () => Navigator.pop(ctx, true),
              child: const Text('断开', style: TextStyle(color: Colors.redAccent)),
            ),
          ],
        ),
      );
      if (ok == true && mounted) {
        engine.disconnectMidiOutput();
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: const Text('已断开钢琴连接', style: TextStyle(fontSize: 12)),
            backgroundColor: EmColors.surface,
            duration: const Duration(seconds: 2),
          ),
        );
      }
      return;
    }

    // List available ports and let user choose
    final ports = engine.getMidiOutputPorts();
    if (!mounted) return;

    if (ports.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: const Text('未找到MIDI输出设备', style: TextStyle(fontSize: 12)),
          backgroundColor: Colors.red.shade800,
          duration: const Duration(seconds: 3),
        ),
      );
      return;
    }

    final chosenIndex = await showDialog<int>(
      context: context,
      builder: (ctx) => AlertDialog(
        backgroundColor: EmColors.surface,
        title: Text('连接到钢琴',
            style: TextStyle(color: EmColors.textPrimary, fontSize: 14)),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('选择MIDI输出端口:',
                style: TextStyle(color: EmColors.textSecondary, fontSize: 12)),
            const SizedBox(height: 8),
            ...List.generate(ports.length, (i) => ListTile(
              dense: true,
              title: Text(ports[i],
                  style: TextStyle(color: EmColors.textPrimary, fontSize: 12)),
              leading: Icon(Icons.piano, color: EmColors.accent, size: 16),
              onTap: () => Navigator.pop(ctx, i),
            )),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: Text('取消', style: TextStyle(color: EmColors.textSecondary)),
          ),
        ],
      ),
    );

    if (chosenIndex == null || !mounted) return;
    final ok = engine.connectMidiOutput(chosenIndex);
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(
          ok ? '已连接: ${ports[chosenIndex]}  — MID播放将同步到钢琴'
              : '连接失败: ${ports[chosenIndex]}',
          style: const TextStyle(fontSize: 12),
        ),
        backgroundColor: ok ? EmColors.surface : Colors.red.shade800,
        duration: const Duration(seconds: 3),
      ),
    );
  }

  // ── Voice tab ────────────────────────────────────────────────────────────
  Widget _buildVoiceTab() {
    return Column(
      children: [
        // Voice slot cards — fixed height based on content
        _buildVoiceSlotGrid(),
        // LCD info bar (Rhythm / Registration / Shift / Tempo)
        const _LcdInfoBar(),
        // Section selector + category buttons — fills remaining space
        Expanded(
          child: _buildControlArea(),
        ),
      ],
    );
  }

  // 8 voice slot cards — 2 rows × 4 cols, height auto-expands with layers
  Widget _buildVoiceSlotGrid() {
    const gap = 3.0;
    return Container(
      color: EmColors.background,
      padding: const EdgeInsets.all(gap),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          // Row 1: slots 0-3
          Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: List.generate(4, (i) => Expanded(
              child: Padding(
                padding: EdgeInsets.only(right: i < 3 ? gap : 0),
                child: VoiceCard(slotIndex: i),
              ),
            )),
          ),
          const SizedBox(height: gap),
          // Row 2: slots 4-7
          Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: List.generate(4, (i) => Expanded(
              child: Padding(
                padding: EdgeInsets.only(right: i < 3 ? gap : 0),
                child: VoiceCard(slotIndex: i + 4),
              ),
            )),
          ),
        ],
      ),
    );
  }

  // Control area: section selector on left, category grid on right
  Widget _buildControlArea() {
    return Row(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        // Section selector column
        SizedBox(
          width: 82,
          child: _buildSectionSelector(),
        ),
        // Category button grid
        Expanded(
          child: _buildCategoryGrid(),
        ),
      ],
    );
  }

  // Left: 8 voice slots — UPPER 1/2, LOWER 1/2, LEAD 1/2, PEDAL 1/2
  Widget _buildSectionSelector() {
    // Each entry: (line1, line2, slot index)
    const slots = [
      ('UPPER VOICE', '1  上键盘音色', 0),
      ('UPPER VOICE', '2  上键盘音色', 1),
      ('LOWER VOICE', '1  下键盘音色', 2),
      ('LOWER VOICE', '2  下键盘音色', 3),
      ('LEAD VOICE',  '1  旋律1',   4),
      ('LEAD VOICE',  '2  旋律2',   5),
      ('PEDAL VOICE', '1  脚踏1',   6),
      ('PEDAL VOICE', '2  脚踏2',   7),
    ];
    return Consumer(
      builder: (context, ref, _) {
        final selectedSlot = ref.watch(engineProvider).selectedSlot;
        return Container(
          color: EmColors.surface,
          child: Column(
            children: List.generate(slots.length, (i) {
              final selected = selectedSlot == slots[i].$3;
              return Expanded(
                child: GestureDetector(
                  onTap: () =>
                      ref.read(engineProvider.notifier).selectSlot(slots[i].$3),
                  child: AnimatedContainer(
                    duration: const Duration(milliseconds: 80),
                    decoration: BoxDecoration(
                      color: selected
                          ? EmColors.accent.withOpacity(0.15)
                          : Colors.transparent,
                      border: Border(
                        left: BorderSide(
                          color: selected
                              ? EmColors.accent
                              : Colors.transparent,
                          width: 3,
                        ),
                        bottom:
                            BorderSide(color: EmColors.divider, width: 0.5),
                      ),
                    ),
                    padding: const EdgeInsets.symmetric(
                        horizontal: 6, vertical: 2),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Text(
                          slots[i].$1,
                          style: TextStyle(
                            color: selected
                                ? EmColors.accent
                                : EmColors.textSecondary,
                            fontSize: 9,
                            fontWeight: FontWeight.bold,
                            letterSpacing: 0.3,
                          ),
                        ),
                        Text(
                          slots[i].$2,
                          style: TextStyle(
                            color: EmColors.textDim,
                            fontSize: 8,
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              );
            }),
          ),
        );
      },
    );
  }

  // Right: voice category buttons — 3 rows × 4 cols, always fills available space
  Widget _buildCategoryGrid() {
    final cats = EmConstants.voiceCategories; // 12 items
    const rows = 3;
    const cols = 4;
    const gap = 3.0;

    return Container(
      color: EmColors.background,
      padding: const EdgeInsets.all(gap),
      child: Column(
        children: List.generate(rows, (row) {
          return Expanded(
            child: Padding(
              padding: EdgeInsets.only(bottom: row < rows - 1 ? gap : 0),
              child: Row(
                children: List.generate(cols, (col) {
                  final i = row * cols + col;
                  final selected = _categoryIndex == i;
                  final cat = cats[i];
                  return Expanded(
                    child: Padding(
                      padding: EdgeInsets.only(right: col < cols - 1 ? gap : 0),
                      child: GestureDetector(
                        onTap: () => setState(() => _categoryIndex = i),
                        child: AnimatedContainer(
                          duration: const Duration(milliseconds: 80),
                          decoration: BoxDecoration(
                            color: selected
                                ? const Color(0xFF8B2A2A)
                                : const Color(0xFF6B2020),
                            borderRadius: BorderRadius.circular(4),
                            border: Border.all(
                              color: selected
                                  ? const Color(0xFFCC4444)
                                  : const Color(0xFF4A1515),
                              width: selected ? 1.5 : 1,
                            ),
                            boxShadow: selected
                                ? [
                                    BoxShadow(
                                      color: Colors.red.withOpacity(0.3),
                                      blurRadius: 4,
                                    )
                                  ]
                                : null,
                          ),
                          child: Stack(
                            children: [
                              // Top-left: Chinese label
                              Positioned(
                                top: 4,
                                left: 6,
                                child: Text(
                                  cat['cn']!,
                                  style: TextStyle(
                                    color: Colors.white.withOpacity(0.55),
                                    fontSize: 9,
                                  ),
                                ),
                              ),
                              // Top-right: LED dot
                              Positioned(
                                top: 5,
                                right: 6,
                                child: Container(
                                  width: 6,
                                  height: 6,
                                  decoration: BoxDecoration(
                                    shape: BoxShape.circle,
                                    color: selected
                                        ? EmColors.ledGreen
                                        : Colors.black.withOpacity(0.5),
                                    border: Border.all(
                                      color: selected
                                          ? EmColors.ledGreen.withOpacity(0.6)
                                          : Colors.white.withOpacity(0.2),
                                    ),
                                  ),
                                ),
                              ),
                              // Center: English label
                              Center(
                                child: Text(
                                  cat['en']!,
                                  style: TextStyle(
                                    color: selected
                                        ? Colors.white
                                        : Colors.white.withOpacity(0.75),
                                    fontSize: 13,
                                    fontWeight: FontWeight.w600,
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
                    ),
                  );
                }),
              ),
            ),
          );
        }),
      ),
    );
  }

  Widget _buildRhythmTab() => const DrumSequencerScreen();

  Widget _buildEditorsTab() {
    return Center(
      child: Text(
        '乐谱编辑器',
        style: TextStyle(color: EmColors.textPrimary, fontSize: 16),
      ),
    );
  }
}

// ── Log Panel ─────────────────────────────────────────────────────────────────

class _LogPanel extends ConsumerStatefulWidget {
  const _LogPanel();

  @override
  ConsumerState<_LogPanel> createState() => _LogPanelState();
}

class _LogPanelState extends ConsumerState<_LogPanel> {
  final ScrollController _scroll = ScrollController();
  bool _autoScroll = true;
  // Filter: null = show all, otherwise only lines containing this prefix
  String? _filter;

  static const _filters = <String?>[null, '[MID] NoteOn', '[MID] PC', '[MID] CC', '[MID] Tempo'];
  static const _filterLabels = ['ALL', 'NOTE', 'PC', 'CC', 'TEMPO'];

  @override
  void dispose() {
    _scroll.dispose();
    super.dispose();
  }

  void _scrollToBottom() {
    if (!_autoScroll) return;
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (_scroll.hasClients) {
        _scroll.jumpTo(_scroll.position.maxScrollExtent);
      }
    });
  }

  // Colour-code log lines
  Color _lineColor(String line) {
    if (line.contains('NoteOn'))  return const Color(0xFF88FF88);
    if (line.contains('NoteOff')) return const Color(0xFF448844);
    if (line.contains('[MID] PC'))    return const Color(0xFFFFCC44);
    if (line.contains('[MID] Tempo')) return const Color(0xFF44CCFF);
    if (line.contains('[MID] CC'))    return const Color(0xFFFF8844);
    if (line.contains('[MIDI]'))      return const Color(0xFFCCAA88);
    return const Color(0xFFAAAAAA);
  }

  @override
  Widget build(BuildContext context) {
    final lines = ref.watch(logProvider);
    final filtered = _filter == null
        ? lines
        : lines.where((l) => l.contains(_filter!)).toList();

    // Auto-scroll whenever lines change
    if (_autoScroll && filtered.isNotEmpty) _scrollToBottom();

    return Container(
      color: const Color(0xFF0A0A0A),
      child: Column(
        children: [
          // ── Toolbar ──────────────────────────────────────────────────────
          Container(
            height: 32,
            color: const Color(0xFF141414),
            padding: const EdgeInsets.symmetric(horizontal: 8),
            child: Row(
              children: [
                Text('LOG',
                    style: TextStyle(
                      color: EmColors.accent,
                      fontSize: 11,
                      fontWeight: FontWeight.bold,
                      letterSpacing: 1.5,
                    )),
                const SizedBox(width: 8),
                Text('${filtered.length} lines',
                    style: TextStyle(color: EmColors.textDim, fontSize: 10)),
                const Spacer(),
                // Filter chips
                ...List.generate(_filters.length, (i) {
                  final selected = _filter == _filters[i];
                  return Padding(
                    padding: const EdgeInsets.only(left: 4),
                    child: GestureDetector(
                      onTap: () => setState(() => _filter = _filters[i]),
                      child: Container(
                        padding: const EdgeInsets.symmetric(
                            horizontal: 7, vertical: 2),
                        decoration: BoxDecoration(
                          color: selected
                              ? EmColors.accent.withOpacity(0.25)
                              : Colors.transparent,
                          border: Border.all(
                            color: selected
                                ? EmColors.accent
                                : EmColors.divider,
                          ),
                          borderRadius: BorderRadius.circular(3),
                        ),
                        child: Text(
                          _filterLabels[i],
                          style: TextStyle(
                            color: selected
                                ? EmColors.accent
                                : EmColors.textDim,
                            fontSize: 9,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                      ),
                    ),
                  );
                }),
                const SizedBox(width: 8),
                // Auto-scroll toggle
                GestureDetector(
                  onTap: () => setState(() => _autoScroll = !_autoScroll),
                  child: Container(
                    padding: const EdgeInsets.symmetric(
                        horizontal: 7, vertical: 2),
                    decoration: BoxDecoration(
                      color: _autoScroll
                          ? EmColors.ledGreen.withOpacity(0.2)
                          : Colors.transparent,
                      border: Border.all(
                        color: _autoScroll
                            ? EmColors.ledGreen
                            : EmColors.divider,
                      ),
                      borderRadius: BorderRadius.circular(3),
                    ),
                    child: Text('AUTO',
                        style: TextStyle(
                          color: _autoScroll
                              ? EmColors.ledGreen
                              : EmColors.textDim,
                          fontSize: 9,
                          fontWeight: FontWeight.bold,
                        )),
                  ),
                ),
                const SizedBox(width: 4),
                // Copy button
                GestureDetector(
                  onTap: () {
                    Clipboard.setData(ClipboardData(
                        text: filtered.join('\n')));
                  },
                  child: Container(
                    padding: const EdgeInsets.symmetric(
                        horizontal: 7, vertical: 2),
                    decoration: BoxDecoration(
                      border: Border.all(color: EmColors.divider),
                      borderRadius: BorderRadius.circular(3),
                    ),
                    child: Text('COPY',
                        style: TextStyle(
                            color: EmColors.textDim,
                            fontSize: 9,
                            fontWeight: FontWeight.bold)),
                  ),
                ),
                const SizedBox(width: 4),
                // Clear button
                GestureDetector(
                  onTap: () =>
                      ref.read(logProvider.notifier).clear(),
                  child: Container(
                    padding: const EdgeInsets.symmetric(
                        horizontal: 7, vertical: 2),
                    decoration: BoxDecoration(
                      border: Border.all(color: EmColors.divider),
                      borderRadius: BorderRadius.circular(3),
                    ),
                    child: Text('CLEAR',
                        style: TextStyle(
                            color: EmColors.textDim,
                            fontSize: 9,
                            fontWeight: FontWeight.bold)),
                  ),
                ),
              ],
            ),
          ),
          // ── Log lines ────────────────────────────────────────────────────
          Expanded(
            child: filtered.isEmpty
                ? Center(
                    child: Text('等待日志输出...',
                        style: TextStyle(
                            color: EmColors.textDim, fontSize: 12)))
                : ListView.builder(
                    controller: _scroll,
                    padding: const EdgeInsets.symmetric(
                        horizontal: 8, vertical: 4),
                    itemCount: filtered.length,
                    itemBuilder: (context, i) {
                      final line = filtered[i];
                      return Text(
                        line,
                        style: TextStyle(
                          color: _lineColor(line),
                          fontSize: 11,
                          fontFamily: 'monospace',
                          height: 1.4,
                        ),
                      );
                    },
                  ),
          ),
        ],
      ),
    );
  }
}

// ── LCD Info Bar (B00/MID names + Registration buttons) ──────────────────────

class _LcdInfoBar extends ConsumerWidget {
  const _LcdInfoBar();

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(engineProvider);
    final transport = state.transport;
    final rhythmName = EmConstants
        .rhythmPresetNames[state.drumPresetIndex % EmConstants.rhythmPresetNames.length];
    final midName = state.loadedMidName ?? '---';
    final b00Name = state.loadedB00Name != null
        ? state.loadedB00Name!.replaceAll(RegExp(r'\.[Bb]00$'), '')
        : '---';
    final currentReg = state.currentRegistration;

    return Container(
      color: const Color(0xFF1A1A1A),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          // Row 1: Rhythm | Song (MID) | REG (B00)
          Container(
            height: 28,
            decoration: BoxDecoration(
              border: Border(
                bottom: BorderSide(color: EmColors.divider, width: 0.5),
              ),
            ),
            child: Row(
              children: [
                _LcdCell(label: 'RHYTHM', value: rhythmName, flex: 3),
                VerticalDivider(color: EmColors.divider, width: 1),
                _LcdCell(label: 'SONG', value: midName, flex: 4),
                VerticalDivider(color: EmColors.divider, width: 1),
                _LcdCell(label: 'REG', value: b00Name, flex: 4),
              ],
            ),
          ),
          // Row 2: Tempo | progress + play/pause tap | 8 Registration buttons
          SizedBox(
            height: 40,
            child: Row(
              children: [
                // Tempo display
                Container(
                  width: 90,
                  padding: const EdgeInsets.symmetric(horizontal: 8),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Text(
                        'TEMPO',
                        style: TextStyle(
                          color: EmColors.textDim,
                          fontSize: 8,
                          letterSpacing: 1,
                        ),
                      ),
                      Text(
                        transport.bpm.round().toString(),
                        style: TextStyle(
                          color: EmColors.accent,
                          fontSize: 20,
                          fontWeight: FontWeight.bold,
                          fontFamily: 'monospace',
                          height: 1.1,
                        ),
                      ),
                    ],
                  ),
                ),
                VerticalDivider(color: EmColors.divider, width: 1),
                // MIDI progress + play/pause (tap to toggle)
                Expanded(
                  child: GestureDetector(
                    onTap: () {
                      final notifier = ref.read(engineProvider.notifier);
                      if (transport.isPlaying) {
                        notifier.stop();
                      } else {
                        notifier.play();
                      }
                    },
                    child: Container(
                      color: Colors.black,
                      padding: const EdgeInsets.symmetric(horizontal: 8),
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          // Progress bar
                          if (transport.midiLoaded)
                            ClipRRect(
                              borderRadius: BorderRadius.circular(2),
                              child: LinearProgressIndicator(
                                value: transport.midiProgress.clamp(0.0, 1.0),
                                minHeight: 4,
                                backgroundColor: Colors.white.withOpacity(0.1),
                                valueColor: AlwaysStoppedAnimation<Color>(
                                  transport.isPlaying
                                      ? EmColors.ledGreen
                                      : EmColors.textSecondary,
                                ),
                              ),
                            )
                          else
                            Container(
                              height: 4,
                              decoration: BoxDecoration(
                                color: Colors.white.withOpacity(0.08),
                                borderRadius: BorderRadius.circular(2),
                              ),
                            ),
                          const SizedBox(height: 4),
                          Row(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              Icon(
                                transport.isPlaying
                                    ? Icons.pause
                                    : Icons.play_arrow,
                                color: transport.isPlaying
                                    ? EmColors.ledGreen
                                    : EmColors.textDim,
                                size: 16,
                              ),
                              const SizedBox(width: 4),
                              Text(
                                transport.isPlaying ? 'PLAY' : 'STOP',
                                style: TextStyle(
                                  color: transport.isPlaying
                                      ? EmColors.ledGreen
                                      : EmColors.ledRed,
                                  fontSize: 11,
                                  fontWeight: FontWeight.bold,
                                  letterSpacing: 1.5,
                                ),
                              ),
                            ],
                          ),
                        ],
                      ),
                    ),
                  ),
                ),
                VerticalDivider(color: EmColors.divider, width: 1),
                // 8 Registration buttons
                SizedBox(
                  width: 180,
                  child: Row(
                    children: List.generate(8, (i) {
                      final reg = i + 1;
                      final isActive = currentReg == reg;
                      return Expanded(
                        child: GestureDetector(
                          onTap: () {
                            ref.read(engineProvider.notifier).switchRegistration(reg);
                          },
                          child: AnimatedContainer(
                            duration: const Duration(milliseconds: 100),
                            margin: const EdgeInsets.symmetric(
                                horizontal: 1, vertical: 4),
                            decoration: BoxDecoration(
                              color: isActive
                                  ? EmColors.accent
                                  : const Color(0xFF2A2020),
                              borderRadius: BorderRadius.circular(3),
                              border: Border.all(
                                color: isActive
                                    ? EmColors.accent
                                    : EmColors.divider,
                                width: isActive ? 1.5 : 1,
                              ),
                              boxShadow: isActive
                                  ? [
                                      BoxShadow(
                                        color: EmColors.accent.withOpacity(0.4),
                                        blurRadius: 4,
                                      )
                                    ]
                                  : null,
                            ),
                            alignment: Alignment.center,
                            child: Text(
                              '$reg',
                              style: TextStyle(
                                color: isActive
                                    ? Colors.black
                                    : EmColors.textDim,
                                fontSize: 11,
                                fontWeight: isActive
                                    ? FontWeight.bold
                                    : FontWeight.normal,
                              ),
                            ),
                          ),
                        ),
                      );
                    }),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _LcdCell extends StatelessWidget {
  const _LcdCell({
    required this.label,
    required this.value,
    required this.flex,
  });
  final String label;
  final String value;
  final int flex;

  @override
  Widget build(BuildContext context) {
    return Expanded(
      flex: flex,
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 8),
        child: Row(
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [
            Text(
              label,
              style: TextStyle(
                color: EmColors.textDim,
                fontSize: 8,
                letterSpacing: 0.8,
              ),
            ),
            const SizedBox(width: 6),
            Expanded(
              child: Text(
                value,
                style: TextStyle(
                  color: EmColors.textSecondary,
                  fontSize: 11,
                  fontWeight: FontWeight.w500,
                ),
                overflow: TextOverflow.ellipsis,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// ── B00 Import confirmation dialog ────────────────────────────────────────────

class _B00ImportDialog extends StatelessWidget {
  const _B00ImportDialog({
    required this.filePath,
    required this.fileName,
    required this.parseResult,
  });

  final String filePath;
  final String fileName;
  final B00ParseResult parseResult;

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      backgroundColor: EmColors.surface,
      title: Text(
        'Import B00 Registration',
        style: TextStyle(color: EmColors.textPrimary, fontSize: 14),
      ),
      content: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          _infoRow('文件', fileName),
          _infoRow('大小', '${(parseResult.totalBytes / 1024).toStringAsFixed(1)} KB'),
          _infoRow('SysEx消息', '${parseResult.messages.length} 条'),
          _infoRow('Registration组', '${parseResult.registrationCount} 个'),
          const SizedBox(height: 12),
          Text(
            '将通过MIDI输出把所有Registration参数发送给连接的电子琴。',
            style: TextStyle(color: EmColors.textSecondary, fontSize: 11),
          ),
          const SizedBox(height: 4),
          Text(
            '注意：请确保MIDI输出已连接到ELS-02C。',
            style: TextStyle(color: EmColors.textDim, fontSize: 10),
          ),
        ],
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context, false),
          child: Text('取消', style: TextStyle(color: EmColors.textSecondary)),
        ),
        ElevatedButton(
          onPressed: () => Navigator.pop(context, true),
          style: ElevatedButton.styleFrom(
            backgroundColor: EmColors.accent,
            foregroundColor: Colors.black,
          ),
          child: const Text('发送到钢琴', style: TextStyle(fontSize: 12)),
        ),
      ],
    );
  }

  Widget _infoRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2),
      child: Row(
        children: [
          SizedBox(
            width: 90,
            child: Text(
              label,
              style: TextStyle(color: EmColors.textDim, fontSize: 11),
            ),
          ),
          Text(
            value,
            style: TextStyle(
              color: EmColors.textPrimary,
              fontSize: 11,
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }
}
