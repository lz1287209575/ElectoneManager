import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/engine_controller.dart';
import '../utils/colors.dart';
import '../utils/constants.dart';
import '../bindings/em_bridge.dart';

class VoiceEditorScreen extends ConsumerStatefulWidget {
  const VoiceEditorScreen({
    super.key,
    required this.slotIndex,
    this.initialLayer = 0,
  });

  final int slotIndex;
  final int initialLayer;

  @override
  ConsumerState<VoiceEditorScreen> createState() => _VoiceEditorScreenState();
}

class _VoiceEditorScreenState extends ConsumerState<VoiceEditorScreen>
    with SingleTickerProviderStateMixin {
  late final TabController _tabController;
  late int _selectedLayer;

  static const _tabs = ['ADSR', 'Tone', 'Filter', 'EQ', 'LFO', 'Effects'];

  @override
  void initState() {
    super.initState();
    _selectedLayer = widget.initialLayer;
    _tabController = TabController(length: _tabs.length, vsync: this);
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final state = ref.watch(engineProvider);
    final slot = state.voiceSlots[widget.slotIndex];
    final engine = ref.read(engineProvider.notifier);

    return Scaffold(
      backgroundColor: EmColors.background,
      appBar: AppBar(
        backgroundColor: EmColors.surface,
        foregroundColor: EmColors.textPrimary,
        title: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              EmConstants.slotRoleNames[widget.slotIndex],
              style: TextStyle(
                color: EmColors.accent,
                fontSize: 12,
                letterSpacing: 1,
              ),
            ),
            Text(
              slot.programName,
              style: const TextStyle(fontSize: 15, fontWeight: FontWeight.bold),
            ),
          ],
        ),
        actions: [
          // Program picker
          TextButton(
            onPressed: () => _showProgramPicker(context),
            child: Text(
              'P${slot.programNumber.toString().padLeft(3, '0')}',
              style: TextStyle(
                color: EmColors.accentCyan,
                fontFamily: 'monospace',
                fontSize: 13,
              ),
            ),
          ),
          const SizedBox(width: 8),
        ],
        bottom: TabBar(
          controller: _tabController,
          labelColor: EmColors.accent,
          unselectedLabelColor: EmColors.textSecondary,
          indicatorColor: EmColors.accent,
          indicatorWeight: 2,
          isScrollable: true,
          labelStyle: const TextStyle(fontSize: 12, fontWeight: FontWeight.bold),
          tabs: _tabs.map((t) => Tab(text: t)).toList(),
        ),
      ),
      body: Column(
        children: [
          // Layer selector
          if (slot.layerCount > 1)
            _LayerSelector(
              count: slot.layerCount,
              selected: _selectedLayer,
              onSelect: (l) => setState(() => _selectedLayer = l),
            ),
          Expanded(
            child: TabBarView(
              controller: _tabController,
              children: [
                _AdsrTab(
                    slot: widget.slotIndex,
                    layer: _selectedLayer,
                    engine: engine),
                _ToneTab(
                    slot: widget.slotIndex,
                    layer: _selectedLayer,
                    engine: engine),
                _FilterTab(
                    slot: widget.slotIndex,
                    layer: _selectedLayer,
                    engine: engine),
                _EqTab(
                    slot: widget.slotIndex,
                    layer: _selectedLayer,
                    engine: engine),
                _LfoTab(
                    slot: widget.slotIndex,
                    layer: _selectedLayer,
                    engine: engine),
                _EffectsTab(
                    slot: widget.slotIndex,
                    layer: _selectedLayer,
                    engine: engine),
              ],
            ),
          ),
          // Add/Remove layer
          Padding(
            padding: const EdgeInsets.all(8),
            child: Row(
              children: [
                _ActionButton(
                  label: '+ 添加层',
                  color: EmColors.accent,
                  onTap: () => engine.addLayer(widget.slotIndex),
                ),
                const SizedBox(width: 8),
                if (slot.layerCount > 1)
                  _ActionButton(
                    label: '- 移除层',
                    color: EmColors.ledRed,
                    onTap: () {
                      engine.removeLayer(widget.slotIndex, _selectedLayer);
                      if (_selectedLayer >= slot.layerCount - 1) {
                        setState(() => _selectedLayer = slot.layerCount - 2);
                      }
                    },
                  ),
                const Spacer(),
                _ActionButton(
                  label: '静音',
                  color: EmColors.textSecondary,
                  onTap: () => engine.noteOff(widget.slotIndex, 60),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  void _showProgramPicker(BuildContext context) {
    showModalBottomSheet(
      context: context,
      backgroundColor: EmColors.surface,
      isScrollControlled: true,
      builder: (_) => _ProgramPickerSheet(slotIndex: widget.slotIndex),
    );
  }
}

class _LayerSelector extends StatelessWidget {
  const _LayerSelector({
    required this.count,
    required this.selected,
    required this.onSelect,
  });

  final int count;
  final int selected;
  final void Function(int) onSelect;

  @override
  Widget build(BuildContext context) {
    return Container(
      height: 36,
      color: EmColors.surface,
      padding: const EdgeInsets.symmetric(horizontal: 8),
      child: Row(
        children: [
          Text(
            '层:',
            style: TextStyle(color: EmColors.textSecondary, fontSize: 12),
          ),
          const SizedBox(width: 8),
          ...List.generate(count, (i) {
            final sel = i == selected;
            return GestureDetector(
              onTap: () => onSelect(i),
              child: AnimatedContainer(
                duration: const Duration(milliseconds: 80),
                margin: const EdgeInsets.only(right: 6),
                padding:
                    const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
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
                  'L${i + 1}',
                  style: TextStyle(
                    color: sel ? EmColors.accent : EmColors.textSecondary,
                    fontSize: 12,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ),
            );
          }),
        ],
      ),
    );
  }
}

// ─── Parameter Slider helper ─────────────────────────────────────────────────

class _ParamSlider extends StatefulWidget {
  const _ParamSlider({
    required this.label,
    required this.value,
    required this.min,
    required this.max,
    required this.onChanged,
    this.unit = '',
    this.divisions,
  });

  final String label;
  final double value;
  final double min;
  final double max;
  final void Function(double) onChanged;
  final String unit;
  final int? divisions;

  @override
  State<_ParamSlider> createState() => _ParamSliderState();
}

class _ParamSliderState extends State<_ParamSlider> {
  late double _local;

  @override
  void initState() {
    super.initState();
    _local = widget.value;
  }

  @override
  void didUpdateWidget(_ParamSlider old) {
    super.didUpdateWidget(old);
    _local = widget.value;
  }

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4, horizontal: 8),
      child: Row(
        children: [
          SizedBox(
            width: 100,
            child: Text(
              widget.label,
              style: TextStyle(
                color: EmColors.textSecondary,
                fontSize: 12,
              ),
            ),
          ),
          Expanded(
            child: SliderTheme(
              data: SliderTheme.of(context).copyWith(
                activeTrackColor: EmColors.accent,
                inactiveTrackColor: EmColors.sliderTrack,
                thumbColor: EmColors.accent,
                trackHeight: 3,
                thumbShape:
                    const RoundSliderThumbShape(enabledThumbRadius: 6),
              ),
              child: Slider(
                value: _local.clamp(widget.min, widget.max),
                min: widget.min,
                max: widget.max,
                divisions: widget.divisions,
                onChanged: (v) => setState(() => _local = v),
                onChangeEnd: widget.onChanged,
              ),
            ),
          ),
          SizedBox(
            width: 56,
            child: Text(
              '${_local.toStringAsFixed(widget.max > 100 ? 0 : 3)}${widget.unit}',
              textAlign: TextAlign.right,
              style: TextStyle(
                color: EmColors.accentCyan,
                fontSize: 11,
                fontFamily: 'monospace',
              ),
            ),
          ),
        ],
      ),
    );
  }
}

// ─── Tab implementations ─────────────────────────────────────────────────────

class _AdsrTab extends StatelessWidget {
  const _AdsrTab(
      {required this.slot, required this.layer, required this.engine});
  final int slot;
  final int layer;
  final EngineNotifier engine;

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(8),
      children: [
        _ParamSlider(
          label: 'Attack',
          value: engine.getSamplerAttack(slot, layer),
          min: 0.001,
          max: 2.0,
          unit: 's',
          onChanged: (v) => engine.setSamplerAttack(slot, layer, v),
        ),
        _ParamSlider(
          label: 'Decay',
          value: engine.getSamplerDecay(slot, layer),
          min: 0.001,
          max: 2.0,
          unit: 's',
          onChanged: (v) => engine.setSamplerDecay(slot, layer, v),
        ),
        _ParamSlider(
          label: 'Sustain',
          value: engine.getSamplerSustain(slot, layer),
          min: 0,
          max: 1,
          onChanged: (v) => engine.setSamplerSustain(slot, layer, v),
        ),
        _ParamSlider(
          label: 'Release',
          value: engine.getSamplerRelease(slot, layer),
          min: 0.001,
          max: 5.0,
          unit: 's',
          onChanged: (v) => engine.setSamplerRelease(slot, layer, v),
        ),
      ],
    );
  }
}

class _ToneTab extends StatelessWidget {
  const _ToneTab(
      {required this.slot, required this.layer, required this.engine});
  final int slot;
  final int layer;
  final EngineNotifier engine;

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(8),
      children: [
        _ParamSlider(
          label: 'Pan',
          value: engine.getSamplerPan(slot, layer),
          min: -1,
          max: 1,
          onChanged: (v) => engine.setSamplerPan(slot, layer, v),
        ),
        _ParamSlider(
          label: 'Octave Shift',
          value: engine.getSamplerOctaveShift(slot, layer),
          min: -2,
          max: 2,
          divisions: 4,
          onChanged: (v) => engine.setSamplerOctaveShift(slot, layer, v),
        ),
        _ParamSlider(
          label: 'Fine Tune',
          value: engine.getSamplerFineTune(slot, layer),
          min: -64,
          max: 63,
          divisions: 127,
          unit: ' ct',
          onChanged: (v) => engine.setSamplerFineTune(slot, layer, v),
        ),
        _ParamSlider(
          label: 'Reverb Send',
          value: engine.getSamplerReverbSend(slot, layer),
          min: 0,
          max: 1,
          onChanged: (v) => engine.setSamplerReverbSend(slot, layer, v),
        ),
      ],
    );
  }
}

class _FilterTab extends StatelessWidget {
  const _FilterTab(
      {required this.slot, required this.layer, required this.engine});
  final int slot;
  final int layer;
  final EngineNotifier engine;

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(8),
      children: [
        _ParamSlider(
          label: 'Cutoff',
          value: engine.getSamplerFilterCutoff(slot, layer),
          min: 0,
          max: 1,
          onChanged: (v) => engine.setSamplerFilterCutoff(slot, layer, v),
        ),
        _ParamSlider(
          label: 'Resonance',
          value: engine.getSamplerResonance(slot, layer),
          min: 0,
          max: 1,
          onChanged: (v) => engine.setSamplerResonance(slot, layer, v),
        ),
      ],
    );
  }
}

class _EqTab extends StatelessWidget {
  const _EqTab(
      {required this.slot, required this.layer, required this.engine});
  final int slot;
  final int layer;
  final EngineNotifier engine;

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(8),
      children: [
        _ParamSlider(
          label: 'Low Freq',
          value: engine.getSamplerEqLowFreq(slot, layer),
          min: 32,
          max: 2000,
          unit: 'Hz',
          onChanged: (v) => engine.setSamplerEqLowFreq(slot, layer, v),
        ),
        _ParamSlider(
          label: 'Low Gain',
          value: engine.getSamplerEqLowGain(slot, layer),
          min: -12,
          max: 12,
          unit: 'dB',
          onChanged: (v) => engine.setSamplerEqLowGain(slot, layer, v),
        ),
        _ParamSlider(
          label: 'High Freq',
          value: engine.getSamplerEqHighFreq(slot, layer),
          min: 500,
          max: 16000,
          unit: 'Hz',
          onChanged: (v) => engine.setSamplerEqHighFreq(slot, layer, v),
        ),
        _ParamSlider(
          label: 'High Gain',
          value: engine.getSamplerEqHighGain(slot, layer),
          min: -12,
          max: 12,
          unit: 'dB',
          onChanged: (v) => engine.setSamplerEqHighGain(slot, layer, v),
        ),
      ],
    );
  }
}

class _LfoTab extends StatefulWidget {
  const _LfoTab(
      {required this.slot, required this.layer, required this.engine});
  final int slot;
  final int layer;
  final EngineNotifier engine;

  @override
  State<_LfoTab> createState() => _LfoTabState();
}

class _LfoTabState extends State<_LfoTab> {
  late int _waveIndex;

  @override
  void initState() {
    super.initState();
    _waveIndex = widget.engine.getSamplerLfoWave(widget.slot, widget.layer);
  }

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(8),
      children: [
        // Wave selector
        Padding(
          padding: const EdgeInsets.symmetric(vertical: 4, horizontal: 8),
          child: Row(
            children: [
              SizedBox(
                width: 100,
                child: Text(
                  'Wave',
                  style: TextStyle(
                    color: EmColors.textSecondary,
                    fontSize: 12,
                  ),
                ),
              ),
              ...List.generate(EmConstants.lfoWaveNames.length, (i) {
                final sel = i == _waveIndex;
                return Expanded(
                  child: GestureDetector(
                    onTap: () {
                      setState(() => _waveIndex = i);
                      widget.engine.setSamplerLfoWave(
                          widget.slot, widget.layer, i);
                    },
                    child: AnimatedContainer(
                      duration: const Duration(milliseconds: 80),
                      margin: const EdgeInsets.symmetric(horizontal: 2),
                      padding: const EdgeInsets.symmetric(vertical: 6),
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
                        EmConstants.lfoWaveNames[i],
                        textAlign: TextAlign.center,
                        style: TextStyle(
                          color: sel
                              ? EmColors.accent
                              : EmColors.textSecondary,
                          fontSize: 10,
                        ),
                      ),
                    ),
                  ),
                );
              }),
            ],
          ),
        ),
        _ParamSlider(
          label: 'Speed (Hz)',
          value: widget.engine.getSamplerLfoSpeed(widget.slot, widget.layer),
          min: 0.1,
          max: 20.0,
          unit: 'Hz',
          onChanged: (v) =>
              widget.engine.setSamplerLfoSpeed(widget.slot, widget.layer, v),
        ),
        _ParamSlider(
          label: 'PMD',
          value: widget.engine.getSamplerLfoPmd(widget.slot, widget.layer),
          min: 0,
          max: 1,
          onChanged: (v) =>
              widget.engine.setSamplerLfoPmd(widget.slot, widget.layer, v),
        ),
        _ParamSlider(
          label: 'FMD',
          value: widget.engine.getSamplerLfoFmd(widget.slot, widget.layer),
          min: 0,
          max: 1,
          onChanged: (v) =>
              widget.engine.setSamplerLfoFmd(widget.slot, widget.layer, v),
        ),
        _ParamSlider(
          label: 'AMD',
          value: widget.engine.getSamplerLfoAmd(widget.slot, widget.layer),
          min: 0,
          max: 1,
          onChanged: (v) =>
              widget.engine.setSamplerLfoAmd(widget.slot, widget.layer, v),
        ),
      ],
    );
  }
}

class _EffectsTab extends StatelessWidget {
  const _EffectsTab(
      {required this.slot, required this.layer, required this.engine});
  final int slot;
  final int layer;
  final EngineNotifier engine;

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(8),
      children: [
        _ParamSlider(
          label: 'Vibrato Depth',
          value: engine.getSamplerVibratoDepth(slot, layer),
          min: 0,
          max: 1,
          onChanged: (v) => engine.setSamplerVibratoDepth(slot, layer, v),
        ),
        _ParamSlider(
          label: 'Vibrato Delay',
          value: engine.getSamplerVibratoDelay(slot, layer),
          min: 0,
          max: 5,
          unit: 's',
          onChanged: (v) => engine.setSamplerVibratoDelay(slot, layer, v),
        ),
        _ParamSlider(
          label: 'Vibrato Speed',
          value: engine.getSamplerVibratoSpeed(slot, layer),
          min: 0.1,
          max: 20.0,
          unit: 'Hz',
          onChanged: (v) => engine.setSamplerVibratoSpeed(slot, layer, v),
        ),
      ],
    );
  }
}

// ─── Program picker ───────────────────────────────────────────────────────────

class _ProgramPickerSheet extends ConsumerStatefulWidget {
  const _ProgramPickerSheet({required this.slotIndex});
  final int slotIndex;

  @override
  ConsumerState<_ProgramPickerSheet> createState() => _ProgramPickerSheetState();
}

class _ProgramPickerSheetState extends ConsumerState<_ProgramPickerSheet> {
  String _selectedCategory = 'Piano';
  String _searchText = '';

  // Build list of unique categories from XG database
  List<String> get _categories {
    final cats = <String>{};
    for (final voice in EmConstants.xgVoices) {
      cats.add(voice['cat'] as String);
    }
    return cats.toList()..sort();
  }

  // Filter voices by category and search
  List<Map<dynamic, dynamic>> get _filteredVoices {
    return EmConstants.xgVoices.where((v) {
      final cat = v['cat'] as String;
      final cn = (v['cn'] as String).toLowerCase();
      final en = (v['en'] as String).toLowerCase();
      final search = _searchText.toLowerCase();

      return cat == _selectedCategory &&
          (cn.contains(search) || en.contains(search));
    }).toList();
  }

  void _selectVoice(Map<dynamic, dynamic> voice) {
    final engine = ref.read(engineProvider.notifier);
    final bankMSB = voice['bankMSB'] as int;
    final bankLSB = voice['bankLSB'] as int;
    final program = voice['program'] as int;

    // Set in engine (internal state)
    engine.setVoiceProgram(widget.slotIndex, program);

    // Send Bank Select + Program Change to MIDI output
    EmBridge.instance.midiOutputSendBankProgram(
      0, // channel 0 (will vary by slot in P2)
      bankMSB,
      bankLSB,
      program,
    );

    Navigator.of(context).pop();
  }

  @override
  Widget build(BuildContext context) {
    return DraggableScrollableSheet(
      expand: false,
      initialChildSize: 0.8,
      minChildSize: 0.4,
      maxChildSize: 0.95,
      builder: (_, controller) {
        return Container(
          color: EmColors.surface,
          child: Column(
            children: [
              // Header
              Padding(
                padding: const EdgeInsets.all(12),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      '选择Yamaha XG音色',
                      style: TextStyle(
                        color: EmColors.textPrimary,
                        fontSize: 15,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const SizedBox(height: 8),
                    // Search box
                    TextField(
                      onChanged: (v) => setState(() => _searchText = v),
                      decoration: InputDecoration(
                        hintText: '搜索音色...',
                        hintStyle: TextStyle(color: EmColors.textSecondary),
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(6),
                          borderSide: BorderSide(color: EmColors.divider),
                        ),
                        contentPadding: const EdgeInsets.symmetric(
                          horizontal: 10,
                          vertical: 8,
                        ),
                        prefixIcon: Icon(
                          Icons.search,
                          color: EmColors.textSecondary,
                          size: 18,
                        ),
                      ),
                      style: TextStyle(color: EmColors.textPrimary, fontSize: 13),
                    ),
                  ],
                ),
              ),
              Divider(color: EmColors.divider, height: 1),
              // Category selector
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 8),
                child: SizedBox(
                  height: 36,
                  child: ListView.builder(
                    scrollDirection: Axis.horizontal,
                    itemCount: _categories.length,
                    itemBuilder: (_, i) {
                      final cat = _categories[i];
                      final sel = cat == _selectedCategory;
                      return Padding(
                        padding: const EdgeInsets.symmetric(horizontal: 4),
                        child: GestureDetector(
                          onTap: () => setState(() => _selectedCategory = cat),
                          child: AnimatedContainer(
                            duration: const Duration(milliseconds: 80),
                            padding: const EdgeInsets.symmetric(
                              horizontal: 12,
                              vertical: 6,
                            ),
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
                              cat,
                              style: TextStyle(
                                color: sel ? EmColors.accent : EmColors.textSecondary,
                                fontSize: 11,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                          ),
                        ),
                      );
                    },
                  ),
                ),
              ),
              Divider(color: EmColors.divider, height: 1),
              // Voice list
              Expanded(
                child: _filteredVoices.isEmpty
                    ? Center(
                        child: Text(
                          '未找到音色',
                          style: TextStyle(color: EmColors.textSecondary),
                        ),
                      )
                    : ListView.builder(
                        controller: controller,
                        itemCount: _filteredVoices.length,
                        itemBuilder: (_, i) {
                          final voice = _filteredVoices[i];
                          final bankMSB = voice['bankMSB'] as int;
                          final bankLSB = voice['bankLSB'] as int;
                          final program = voice['program'] as int;
                          final cn = voice['cn'] as String;
                          final en = voice['en'] as String;

                          return ListTile(
                            dense: true,
                            leading: Column(
                              mainAxisAlignment: MainAxisAlignment.center,
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Text(
                                  'B:$bankMSB,$bankLSB',
                                  style: TextStyle(
                                    color: EmColors.accentCyan,
                                    fontFamily: 'monospace',
                                    fontSize: 10,
                                  ),
                                ),
                                Text(
                                  'P:${program.toString().padLeft(3, '0')}',
                                  style: TextStyle(
                                    color: EmColors.accentCyan,
                                    fontFamily: 'monospace',
                                    fontSize: 10,
                                  ),
                                ),
                              ],
                            ),
                            title: Text(
                              cn,
                              style: TextStyle(
                                color: EmColors.textPrimary,
                                fontSize: 13,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                            subtitle: Text(
                              en,
                              style: TextStyle(
                                color: EmColors.textSecondary,
                                fontSize: 11,
                              ),
                            ),
                            onTap: () => _selectVoice(voice),
                          );
                        },
                      ),
              ),
            ],
          ),
        );
      },
    );
  }
}

class _ActionButton extends StatelessWidget {
  const _ActionButton({
    required this.label,
    required this.color,
    required this.onTap,
  });

  final String label;
  final Color color;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
        decoration: BoxDecoration(
          color: color.withOpacity(0.15),
          borderRadius: BorderRadius.circular(6),
          border: Border.all(color: color.withOpacity(0.6)),
        ),
        child: Text(
          label,
          style: TextStyle(color: color, fontSize: 12, fontWeight: FontWeight.bold),
        ),
      ),
    );
  }
}
