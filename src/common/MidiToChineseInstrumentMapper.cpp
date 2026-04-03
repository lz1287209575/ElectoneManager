#include "MidiToChineseInstrumentMapper.h"

namespace em {

// ──────────────────────────────────────────────────────────────────────────────
// GM program → Chinese instrument name
// Covers the 128 General MIDI programs with plausible Chinese equivalents.
// Programs without a close Chinese counterpart map to the nearest timbral match.
// ──────────────────────────────────────────────────────────────────────────────
const std::unordered_map<int, std::string> MidiToChineseInstrumentMapper::kProgramMap = {
    // Piano family → 钢琴 / 古琴
    {0,  "钢琴"},      {1,  "明亮钢琴"},  {2,  "电钢琴"},
    {3,  "酒吧钢琴"},  {4,  "电钢琴Ⅱ"},  {5,  "电钢琴Ⅲ"},
    {6,  "羽管键琴"},  {7,  "古琴"},

    // Chromatic Perc → 扬琴 / 钟琴
    {8,  "钢片琴"},    {9,  "钟琴"},     {10, "音乐盒"},
    {11, "颤音琴"},    {12, "马林巴"},   {13, "木琴"},
    {14, "管钟"},      {15, "扬琴"},

    // Organ → 管风琴
    {16, "管风琴"},    {17, "打击管风琴"},{18, "摇滚管风琴"},
    {19, "教堂管风琴"},{20, "簧片管风琴"},{21, "口风琴"},
    {22, "手风琴"},    {23, "口琴"},

    // Guitar
    {24, "尼龙弦吉他"},{25, "钢弦吉他"}, {26, "爵士吉他"},
    {27, "清音吉他"},  {28, "闷音吉他"}, {29, "过载吉他"},
    {30, "失真吉他"},  {31, "泛音吉他"},

    // Bass
    {32, "低音提琴"},  {33, "电贝斯"},   {34, "指拨贝斯"},
    {35, "拨片贝斯"},  {36, "无品贝斯"}, {37, "清音贝斯"},
    {38, "闷音贝斯"},  {39, "合成贝斯"},

    // Strings → 二胡 / 提琴
    {40, "小提琴"},    {41, "中提琴"},   {42, "大提琴"},
    {43, "低音提琴"},  {44, "弦乐震音"}, {45, "拨弦"},
    {46, "竖琴"},      {47, "定音鼓"},

    // Ensemble
    {48, "弦乐合奏Ⅰ"},{49, "弦乐合奏Ⅱ"},{50, "合成弦乐Ⅰ"},
    {51, "合成弦乐Ⅱ"},{52, "合唱"},     {53, "人声哼唱"},
    {54, "合成人声"},  {55, "管弦乐打击"},

    // Brass
    {56, "小号"},      {57, "长号"},     {58, "大号"},
    {59, "弱音小号"},  {60, "法国号"},   {61, "铜管合奏"},
    {62, "合成铜管Ⅰ"},{63, "合成铜管Ⅱ"},

    // Reed → 中国管乐
    {64, "高音萨克斯"},{65, "中音萨克斯"},{66, "次中音萨克斯"},
    {67, "上低音萨克斯"},{68,"双簧管"},  {69, "英国管"},
    {70, "巴松管"},    {71, "单簧管"},

    // Pipe → 笛子 / 箫
    {72, "短笛"},      {73, "长笛"},     {74, "竹笛"},
    {75, "陶笛"},      {76, "竖笛"},     {77, "排箫"},
    {78, "吹口瓶"},    {79, "尺八"},

    // Synth Lead
    {80, "方波合成"},  {81, "锯齿波"},   {82, "钢琴合成"},
    {83, "独奏合成"},  {84, "五度合成"}, {85, "音色合成"},
    {86, "贝斯合成"},  {87, "铅笔合成"},

    // Synth Pad
    {88, "新世纪"},    {89, "暖音"},     {90, "复音"},
    {91, "合唱垫"},    {92, "弓弦"},     {93, "金属"},
    {94, "光晕"},      {95, "扫弦"},

    // Synth FX / Ethnic
    {96, "雨声"},      {97, "音轨"},     {98, "水晶"},
    {99, "大气"},      {100,"明亮"},     {101,"幻想"},
    {102,"魔幻"},      {103,"科幻"},

    // Ethnic → 琵琶 / 古筝
    {104,"西塔琴"},    {105,"班卓琴"},   {106,"三味线"},
    {107,"古筝"},      {108,"卡林巴"},   {109,"风笛"},
    {110,"小提琴拨弦"},{111,"锡塔琴"},

    // Percussive / Sound FX
    {112,"铃鼓"},      {113,"木鱼"},     {114,"塔iko鼓"},
    {115,"旋律打击"},  {116,"合成鼓"},   {117,"逆转钹"},
    {118,"吉他拨弦"},  {119,"换把噪音"},

    // More FX
    {120,"吉他弦音"},  {121,"呼吸音"},   {122,"海浪声"},
    {123,"鸟鸣"},      {124,"电话铃"},   {125,"直升机"},
    {126,"掌声"},      {127,"枪声"},
};

// ──────────────────────────────────────────────────────────────────────────────
MidiToChineseInstrumentMapper::MidiToChineseInstrumentMapper() = default;

void MidiToChineseInstrumentMapper::process(const MidiMessage& msg) noexcept {
    switch (msg.type) {
    case MidiType::PitchBend:
        // 14-bit horizontal aftertouch: data1=LSB, data2=MSB → [-1.0, 1.0]
        _pitchBend.store(msg.pitchBendFloat(), std::memory_order_release);
        break;

    case MidiType::ControlChange:
        switch (msg.data1) {
        case 1:   // Mod wheel → vibrato depth
            _vibratoDepth.store(msg.data2 / 127.0f, std::memory_order_release);
            break;
        case 11:  // Expression CC#11
            _expressionGain.store(msg.data2 / 127.0f, std::memory_order_release);
            break;
        case 74:  // Filter cutoff (Brightness CC#74)
            _filterCutoff.store(msg.data2 / 127.0f, std::memory_order_release);
            break;
        default:
            break;
        }
        break;

    case MidiType::ProgramChange:
        _program.store(msg.data1 & 0x7F, std::memory_order_release);
        break;

    default:
        break;
    }
}

InstrumentParams MidiToChineseInstrumentMapper::getParams() const noexcept {
    InstrumentParams p;
    p.pitchBend      = _pitchBend.load(std::memory_order_acquire);
    p.vibratoDepth   = _vibratoDepth.load(std::memory_order_acquire);
    p.filterCutoff   = _filterCutoff.load(std::memory_order_acquire);
    p.expressionGain = _expressionGain.load(std::memory_order_acquire);
    p.programNumber  = _program.load(std::memory_order_acquire);
    p.instrumentName = programToChineseInstrument(p.programNumber);
    return p;
}

std::string MidiToChineseInstrumentMapper::programToChineseInstrument(
        int program) noexcept {
    auto it = kProgramMap.find(program & 0x7F);
    return (it != kProgramMap.end()) ? it->second : "未知音色";
}

} // namespace em
