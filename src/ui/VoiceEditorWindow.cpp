#include "VoiceEditorWindow.h"
#include "common/MidiToChineseInstrumentMapper.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace em::ui {

// ── Window dimensions ─────────────────────────────────────────────────────────
static constexpr int kWinW  = 1400;
static constexpr int kWinH  = 1120;

// ── Layout constants ──────────────────────────────────────────────────────────
static constexpr int kHeaderH   = 72;
static constexpr int kLayerBarH = 44;
static constexpr int kTabBarH   = 48;
static constexpr int kPageY     = kHeaderH + kLayerBarH + kTabBarH;
static constexpr int kPageH     = kWinH - kPageY;
static constexpr int kPad       = 12;
static constexpr int kGap       = 10;

// ── Palette ───────────────────────────────────────────────────────────────────
static const Fl_Color kWinBg     = fl_rgb_color(22,  24,  22);
static const Fl_Color kSectionBg = fl_rgb_color(34,  36,  34);
static const Fl_Color kTitleBg   = fl_rgb_color(44,  46,  44);
static const Fl_Color kTextHi    = fl_rgb_color(220, 222, 218);
static const Fl_Color kTextMid   = fl_rgb_color(140, 142, 138);
static const Fl_Color kAmber     = fl_rgb_color(230, 155, 30);
static const Fl_Color kTabInactiveBg = fl_rgb_color(38, 40, 38);
static const Fl_Color kTabBarBg      = fl_rgb_color(28, 30, 28);

// Role label helper
static const char* kSlotName[8] = {
    "UPPER KEYBOARD VOICE 1",
    "UPPER KEYBOARD VOICE 2",
    "LEAD VOICE 1",
    "LEAD VOICE 2",
    "LOWER KEYBOARD VOICE 1",
    "LOWER KEYBOARD VOICE 2",
    "PEDAL VOICE 1",
    "PEDAL VOICE 2",
};

static Fl_Color slotAccent(int slot) {
    switch (slot) {
    case 0: case 4: return fl_rgb_color(180, 65,  40);
    case 1: case 5: return fl_rgb_color(130, 135, 128);
    case 2: case 3: return fl_rgb_color(52,  112, 52);
    case 6: case 7: return fl_rgb_color(48,  108, 48);
    default:        return fl_rgb_color(100, 100, 100);
    }
}

// Tab page names
static const char* kTabLabels[6] = {
    "P.1 BASIC", "P.2 VIBRATO", "P.3 EFFECT 1",
    "P.4 EFFECT 2", "P.5 TRANSPOSE", "P.6 VOICE EDIT"
};

// ── Voice definitions for per-layer instrument selection ─────────────────────
struct VoiceDef {
    const char* label;
    int         program;    // GM program number (for name lookup, -1 = test tone)
    int         harmonics;  // number of odd harmonics in test waveform
};

// Organised into 4 category pages
static const char* kVoiceCatLabels[] = {
    "STRINGS / BRASS", "WIND / CHOIR", "KEYS / SYNTH", "ETHNIC / TEST"
};

static const VoiceDef kVoiceCats[][7] = {
    // Category 0: Strings / Brass
    {
        { "Strings 1",   40,  5 },
        { "Strings 2",   48,  6 },
        { "Violin",      40,  4 },
        { "Cello",       42,  5 },
        { "Brass 1",     56,  7 },
        { "Trumpet",     56,  6 },
        { "Trombone",    57,  5 },
    },
    // Category 1: Wind / Choir
    {
        { "Flute",       73,  2 },
        { "Oboe",        68,  6 },
        { "Clarinet",    71,  4 },
        { "Bamboo",      74,  3 },
        { "Fr.Horn",     60,  4 },
        { "Choir",       52,  3 },
        { "Voice Pad",   53,  2 },
    },
    // Category 2: Keys / Synth
    {
        { "Piano",        0,  8 },
        { "E.Piano",      4,  5 },
        { "Organ",       16,  8 },
        { "Synth Pad",   89,  4 },
        { "Synth Lead",  80,  3 },
        { "Ac.Bass",     32,  2 },
        { "E.Bass",      33,  3 },
    },
    // Category 3: Ethnic / Test
    {
        { "Guzheng",    107,  6 },
        { "Erhu",        40,  3 },
        { "Perc",       112,  8 },
        { "Sine",        -1,  1 },
        { "Hollow",      -1,  2 },
        { "Reedy",       -1,  3 },
        { "Bright",      -1,  6 },
    },
};
static constexpr int kNumVoiceCatsConst = 4;
static constexpr int kVoicesPerCatConst = 7;

// ─────────────────────────────────────────────────────────────────────────────
Fl_Color VoiceEditorWindow::tabAccent(int page) const {
    switch (page) {
    case 0: return kAccentVolume;   // deep orange
    case 1: return kAccentVibrato;  // green
    case 2: return kAccentFilter;   // teal
    case 3: return kAccentFilter;   // teal
    case 4: return kAccentPitch;    // cobalt blue
    case 5: return kAccentAdsr;     // purple
    default: return kAccentVolume;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
VoiceEditorWindow::VoiceEditorWindow(int slot, const std::string& voiceName,
                                     em::VoiceSlotEngine* slotEngine)
    : Fl_Window(kWinW, kWinH)
    , _slot(slot)
    , _voiceName(voiceName)
    , _slotEngine(slotEngine)
{
    char title[80];
    snprintf(title, sizeof(title), "Voice Editor — Slot %d: %s",
             slot + 1, voiceName.c_str());
    copy_label(title);

    color(kWinBg);
    begin();
    buildLayout();
    end();
    resizable(nullptr);
    set_non_modal();
}

// ── buildLayout ───────────────────────────────────────────────────────────────
void VoiceEditorWindow::buildLayout() {
    // Close button (top-right of header)
    Fl_Button* closeBtn = new Fl_Button(kWinW - 160, 14, 140, 46, "✕  Close");
    closeBtn->color(fl_rgb_color(80, 28, 28));
    closeBtn->labelcolor(fl_rgb_color(230, 160, 160));
    closeBtn->labelfont(FL_HELVETICA_BOLD);
    closeBtn->labelsize(18);
    closeBtn->box(FL_FLAT_BOX);
    closeBtn->callback(closeCb, this);

    // Layer bar (between header and tab bar)
    buildLayerBar();

    // Tab bar
    buildTabBar();

    // Build all 6 pages
    int px = 0, py = kPageY, pw = kWinW, ph = kPageH;
    buildPage1(px, py, pw, ph);
    buildPage2(px, py, pw, ph);
    buildPage3(px, py, pw, ph);
    buildPage4(px, py, pw, ph);
    buildPage5(px, py, pw, ph);
    buildPage6(px, py, pw, ph);

    // Show first page
    showPage(0);
}

// ── Tab bar ──────────────────────────────────────────────────────────────────
void VoiceEditorWindow::buildTabBar() {
    int tabW = kWinW / kNumPages;
    for (int i = 0; i < kNumPages; ++i) {
        int tx = i * tabW;
        int tw = (i == kNumPages - 1) ? (kWinW - tx) : tabW; // last tab gets remainder
        _tabBtns[i] = new Fl_Button(tx, kHeaderH + kLayerBarH, tw, kTabBarH, kTabLabels[i]);
        _tabBtns[i]->box(FL_FLAT_BOX);
        _tabBtns[i]->labelfont(FL_HELVETICA_BOLD);
        _tabBtns[i]->labelsize(16);
        _tabBtns[i]->callback(tabCb, this);
    }
}

// ── Layer bar ─────────────────────────────────────────────────────────────────
static const Fl_Color kLayerBarBg     = fl_rgb_color(24, 26, 24);
static const Fl_Color kLayerActive    = fl_rgb_color(230, 155, 30);
static const Fl_Color kLayerInactive  = fl_rgb_color(50, 52, 50);

void VoiceEditorWindow::buildLayerBar() {
    _layerBarGroup = new Fl_Group(0, kHeaderH, kWinW, kLayerBarH);
    _layerBarGroup->box(FL_FLAT_BOX);
    _layerBarGroup->color(kLayerBarBg);

    int bx = 10;
    int bw = 48, bh = 36;
    int by = kHeaderH + (kLayerBarH - bh) / 2;

    // L1..L8 buttons
    for (int i = 0; i < kMaxLayers; ++i) {
        char lbl[4];
        snprintf(lbl, sizeof(lbl), "L%d", i + 1);
        _layerBtns[i] = new Fl_Button(bx + i * (bw + 3), by, bw, bh);
        _layerBtns[i]->copy_label(lbl);
        _layerBtns[i]->box(FL_FLAT_BOX);
        _layerBtns[i]->labelfont(FL_HELVETICA_BOLD);
        _layerBtns[i]->labelsize(16);
        _layerBtns[i]->callback(layerBtnCb, this);
    }

    // [+] button
    int addX = bx + kMaxLayers * (bw + 3) + 10;
    _addLayerBtn = new Fl_Button(addX, by, 40, bh, "+");
    _addLayerBtn->box(FL_FLAT_BOX);
    _addLayerBtn->color(fl_rgb_color(40, 80, 40));
    _addLayerBtn->labelcolor(fl_rgb_color(100, 220, 100));
    _addLayerBtn->labelfont(FL_HELVETICA_BOLD);
    _addLayerBtn->labelsize(20);
    _addLayerBtn->callback(addLayerCb, this);

    // [-] button
    _removeLayerBtn = new Fl_Button(addX + 44, by, 40, bh, "\xe2\x88\x92");
    _removeLayerBtn->box(FL_FLAT_BOX);
    _removeLayerBtn->color(fl_rgb_color(80, 40, 40));
    _removeLayerBtn->labelcolor(fl_rgb_color(220, 100, 100));
    _removeLayerBtn->labelfont(FL_HELVETICA_BOLD);
    _removeLayerBtn->labelsize(20);
    _removeLayerBtn->callback(removeLayerCb, this);

    _layerBarGroup->end();
    refreshLayerBar();
}

void VoiceEditorWindow::refreshLayerBar() {
    int count = _slotEngine ? _slotEngine->layerCount() : 0;
    for (int i = 0; i < kMaxLayers; ++i) {
        if (!_layerBtns[i]) continue;
        if (i < count) {
            _layerBtns[i]->show();
            if (i == _currentLayer) {
                _layerBtns[i]->color(kLayerActive);
                _layerBtns[i]->labelcolor(fl_rgb_color(30, 30, 30));
            } else {
                _layerBtns[i]->color(kLayerInactive);
                _layerBtns[i]->labelcolor(kTextMid);
            }
            _layerBtns[i]->redraw();
        } else {
            _layerBtns[i]->hide();
        }
    }
    // Disable remove if only 1 layer
    if (_removeLayerBtn) {
        if (count <= 1) {
            _removeLayerBtn->deactivate();
        } else {
            _removeLayerBtn->activate();
        }
    }
    // Disable add if at max
    if (_addLayerBtn) {
        if (count >= kMaxLayers) {
            _addLayerBtn->deactivate();
        } else {
            _addLayerBtn->activate();
        }
    }
}

void VoiceEditorWindow::selectLayer(int idx) {
    if (!_slotEngine || idx < 0 || idx >= _slotEngine->layerCount()) return;
    _currentLayer = idx;
    refreshLayerBar();
    reloadSliderValues();
}

void VoiceEditorWindow::reloadSliderValues() {
    auto* s = currentSampler();
    if (!s) return;

    // Page 1: Basic — sync voice button highlight
    if (_slotEngine && _currentLayer < _slotEngine->layerCount()) {
        const auto& name = _slotEngine->layerInfo(_currentLayer).instrumentName;
        // Find which category the current voice is in, and switch to it
        for (int c = 0; c < kNumVoiceCatsConst; ++c) {
            for (int v = 0; v < kVoicesPerCatConst; ++v) {
                if (name == kVoiceCats[c][v].label) {
                    showVoiceCat(c);
                    goto voiceFound;
                }
            }
        }
        voiceFound:;
    }
    if (_volumeSlider)     _volumeSlider->value(s->getExpressionGain() * 127.0);
    if (_panSlider)        _panSlider->value((s->getPan() + 1.0) * 3.0);
    if (_feetChoice)       _feetChoice->value(std::clamp(s->getOctaveShift() + 2, 0, 3));
    if (_reverbSendSlider) _reverbSendSlider->value(s->getReverbSend() * 24.0);

    // Page 2: Vibrato
    if (_vibDelaySlider)   _vibDelaySlider->value(std::clamp(static_cast<double>(s->getVibratoDelay() * 7.0f), 0.0, 14.0));
    if (_vibDepthSlider)   _vibDepthSlider->value(std::clamp(static_cast<double>(s->getVibratoDepth() * 14.0f), 0.0, 14.0));
    if (_vibSpeedSlider) {
        double vibSpeedVal = std::clamp((static_cast<double>(s->getVibratoSpeed()) - 1.0) / (12.0 - 1.0) * 14.0, 0.0, 14.0);
        _vibSpeedSlider->value(vibSpeedVal);
    }

    // Page 5: Transpose / Tune
    if (_transposeSlider) _transposeSlider->value(std::clamp(static_cast<double>(s->getPitchBend() * 12.0f), -6.0, 6.0));
    if (_detuneSlider)    _detuneSlider->value(static_cast<double>(s->getFineTune()));

    // Page 6: Voice Edit
    if (_eqLoFreqSlider)  _eqLoFreqSlider->value(s->getEqLowFreq());
    if (_eqLoGainSlider)  _eqLoGainSlider->value(s->getEqLowGain());
    if (_eqHiFreqSlider)  _eqHiFreqSlider->value(s->getEqHighFreq());
    if (_eqHiGainSlider)  _eqHiGainSlider->value(s->getEqHighGain());
    if (_attackSlider)    _attackSlider->value(s->getAdsrAttack() * 1000.0);
    if (_decaySlider)     _decaySlider->value(s->getAdsrDecay() * 1000.0);
    if (_sustainSlider)   _sustainSlider->value(s->getAdsrSustain() * 100.0);
    if (_releaseSlider)   _releaseSlider->value(s->getAdsrRelease() * 1000.0);
    if (_filterSlider)    _filterSlider->value(s->getFilterCutoff() * 100.0);
    if (_resonanceSlider) _resonanceSlider->value(s->getResonance() * 100.0);
    if (_lfoWaveChoice)   _lfoWaveChoice->value(s->getLfoWave());
    if (_lfoSpeedSlider)  _lfoSpeedSlider->value(s->getLfoSpeed());
    if (_lfoPmdSlider)    _lfoPmdSlider->value(s->getLfoPmd());
    if (_lfoFmdSlider)    _lfoFmdSlider->value(s->getLfoFmd());
    if (_lfoAmdSlider)    _lfoAmdSlider->value(s->getLfoAmd());

    redraw();
}

// ── showPage ─────────────────────────────────────────────────────────────────
void VoiceEditorWindow::showPage(int pageIndex) {
    _currentPage = pageIndex;
    for (int i = 0; i < kNumPages; ++i) {
        if (_pages[i]) {
            if (i == pageIndex)
                _pages[i]->show();
            else
                _pages[i]->hide();
        }
        // Update tab appearance
        if (_tabBtns[i]) {
            if (i == pageIndex) {
                _tabBtns[i]->color(tabAccent(i));
                _tabBtns[i]->labelcolor(FL_WHITE);
            } else {
                _tabBtns[i]->color(kTabInactiveBg);
                _tabBtns[i]->labelcolor(kTextMid);
            }
            _tabBtns[i]->redraw();
        }
    }
    redraw();
}

// ── Page 1: BASIC ────────────────────────────────────────────────────────────
void VoiceEditorWindow::buildPage1(int x, int y, int w, int h) {
    _pages[0] = new Fl_Group(x, y, w, h);
    _pages[0]->box(FL_FLAT_BOX);
    _pages[0]->color(kWinBg);

    constexpr int secH   = 42;
    constexpr int slH    = 36;
    constexpr int slGap  = 52;
    constexpr int voiceSecH = 160;

    int col1X = kPad;
    int col1W = w / 3 - kPad;
    int col2X = w / 3 + kGap / 2;
    int col2W = w / 3 - kGap;
    int col3X = 2 * w / 3 + kGap / 2;
    int col3W = w / 3 - kPad - kGap / 2;

    // ── Voice / Instrument selector (button grid) ──
    int voiceY = y + kPad;
    buildVoiceSelector(kPad, voiceY, w - kPad * 2, voiceSecH);

    int sectionY = voiceY + voiceSecH + kGap;

    // ── Volume ──
    float initVol = currentSampler() ? currentSampler()->getExpressionGain() * 127.0f : 127.0f;
    _volumeSlider = makeSlider(col1X + 8, sectionY + secH + 16, col1W - 16, slH,
                               "Volume (0\xe2\x80\x93" "127)", 0.0, 127.0, initVol, kAmber);

    // ── Pan ──
    float initPan = currentSampler() ? currentSampler()->getPan() : 0.0f;
    double panVal = (initPan + 1.0) * 3.0;
    _panSlider = makeSlider(col2X + 8, sectionY + secH + 16, col2W - 16, slH,
                            "Pan  L3 \xe2\x86\x90 C \xe2\x86\x92 R3", 0.0, 6.0, panVal, kAmber);
    _panSlider->step(1.0);

    // ── Feet / Octave ──
    _feetChoice = new Fl_Choice(col3X + 8, sectionY + secH + 16, col3W - 16, slH, "Feet / Octave");
    _feetChoice->align(FL_ALIGN_TOP_LEFT);
    _feetChoice->labelcolor(kTextMid);
    _feetChoice->labelsize(12);
    _feetChoice->textsize(18);
    _feetChoice->color(fl_rgb_color(50, 52, 50));
    _feetChoice->textcolor(kTextHi);
    _feetChoice->add("PRESET (0)");
    _feetChoice->add("16' (-1 oct)");
    _feetChoice->add("8' (normal)");
    _feetChoice->add("4' (+1 oct)");
    int initOct = currentSampler() ? currentSampler()->getOctaveShift() + 2 : 2;
    _feetChoice->value(std::clamp(initOct, 0, 3));
    _feetChoice->callback(choiceCb, this);

    // ── Touch controls ──
    int touchY = sectionY + secH + slH + slGap + 24;

    _initTouchSlider = makeSlider(col1X + 8, touchY + secH + 16, col1W - 16, slH,
                                  "Initial Touch (0\xe2\x80\x93" "14)", 0.0, 14.0, 7.0, kAmber);
    _initTouchSlider->step(1.0);

    _afterTouchSlider = makeSlider(col1X + 8, touchY + secH + slH + slGap + 16, col1W - 16, slH,
                                   "After Touch (0\xe2\x80\x93" "14)", 0.0, 14.0, 7.0, kAmber);
    _afterTouchSlider->step(1.0);

    _horizTouchSlider = makeSlider(col2X + 8, touchY + secH + 16, col2W - 16, slH,
                                   "Horizontal Touch (0\xe2\x80\x93" "14)", 0.0, 14.0, 0.0, kAmber);
    _horizTouchSlider->step(1.0);

    _atPitchSlider = makeSlider(col2X + 8, touchY + secH + slH + slGap + 16, col2W - 16, slH,
                                "AT \xe2\x86\x92 Pitch (-14 ~ +14)", -14.0, 14.0, 0.0, kAmber);
    _atPitchSlider->step(1.0);

    // ── Reverb Send ──
    float initRev = currentSampler() ? currentSampler()->getReverbSend() * 24.0f : 12.0f;
    _reverbSendSlider = makeSlider(col3X + 8, touchY + secH + 16, col3W - 16, slH,
                                   "Reverb Send (0\xe2\x80\x93" "24)", 0.0, 24.0, initRev, kAmber);
    _reverbSendSlider->step(1.0);

    _pages[0]->end();
}

// ── Voice selector: category tabs + button grid ──────────────────────────────
void VoiceEditorWindow::buildVoiceSelector(int bx, int by, int bw, int bh) {
    constexpr int catTabH = 42;
    constexpr int gridPad = 4;

    // Category tab buttons across the top
    int catTabW = bw / kNumVoiceCats;
    for (int c = 0; c < kNumVoiceCats; ++c) {
        int tx = bx + c * catTabW;
        int tw = (c == kNumVoiceCats - 1) ? (bx + bw - tx) : catTabW;
        _voiceCatBtns[c] = new Fl_Button(tx, by, tw, catTabH, kVoiceCatLabels[c]);
        _voiceCatBtns[c]->box(FL_FLAT_BOX);
        _voiceCatBtns[c]->labelfont(FL_HELVETICA_BOLD);
        _voiceCatBtns[c]->labelsize(16);
        _voiceCatBtns[c]->callback(voiceCatCb, this);
    }

    // Grid area below category tabs
    int gridY = by + catTabH + 2;
    int gridH = bh - catTabH - 2;

    for (int c = 0; c < kNumVoiceCats; ++c) {
        _voiceCatPages[c] = new Fl_Group(bx, gridY, bw, gridH);
        _voiceCatPages[c]->box(FL_FLAT_BOX);
        _voiceCatPages[c]->color(fl_rgb_color(30, 32, 30));

        // Layout: single row of buttons
        int btnGap = 4;
        int btnW = (bw - gridPad * 2 - btnGap * (kVoicesPerCat - 1)) / kVoicesPerCat;
        int btnH = gridH - gridPad * 2;
        if (btnH > 50) btnH = 50;
        int btnY = gridY + (gridH - btnH) / 2;

        for (int v = 0; v < kVoicesPerCat; ++v) {
            int btnX = bx + gridPad + v * (btnW + btnGap);
            _voiceBtns[c][v] = new Fl_Button(btnX, btnY, btnW, btnH,
                                              kVoiceCats[c][v].label);
            _voiceBtns[c][v]->box(FL_FLAT_BOX);
            _voiceBtns[c][v]->color(fl_rgb_color(50, 52, 50));
            _voiceBtns[c][v]->labelcolor(kTextHi);
            _voiceBtns[c][v]->labelfont(FL_HELVETICA);
            _voiceBtns[c][v]->labelsize(18);
            _voiceBtns[c][v]->callback(voiceBtnCb, this);
        }
        _voiceCatPages[c]->end();
    }

    showVoiceCat(0);
}

void VoiceEditorWindow::showVoiceCat(int cat) {
    _currentVoiceCat = cat;
    for (int c = 0; c < kNumVoiceCats; ++c) {
        if (_voiceCatPages[c]) {
            if (c == cat) _voiceCatPages[c]->show();
            else          _voiceCatPages[c]->hide();
        }
        if (_voiceCatBtns[c]) {
            if (c == cat) {
                _voiceCatBtns[c]->color(kAccentVolume);
                _voiceCatBtns[c]->labelcolor(FL_WHITE);
            } else {
                _voiceCatBtns[c]->color(fl_rgb_color(40, 42, 40));
                _voiceCatBtns[c]->labelcolor(kTextMid);
            }
            _voiceCatBtns[c]->redraw();
        }
    }
    // Highlight the voice button matching the current layer's instrument
    if (_slotEngine && _currentLayer < _slotEngine->layerCount()) {
        const auto& name = _slotEngine->layerInfo(_currentLayer).instrumentName;
        for (int v = 0; v < kVoicesPerCat; ++v) {
            bool match = (name == kVoiceCats[cat][v].label);
            _voiceBtns[cat][v]->color(match ? kAmber : fl_rgb_color(50, 52, 50));
            _voiceBtns[cat][v]->labelcolor(match ? fl_rgb_color(30, 30, 30) : kTextHi);
            _voiceBtns[cat][v]->redraw();
        }
    }
}

// ── Page 2: VIBRATO / SLIDE ──────────────────────────────────────────────────
void VoiceEditorWindow::buildPage2(int x, int y, int w, int h) {
    _pages[1] = new Fl_Group(x, y, w, h);
    _pages[1]->box(FL_FLAT_BOX);
    _pages[1]->color(kWinBg);

    constexpr int secH  = 42;
    constexpr int slH   = 36;
    constexpr int slGap = 52;

    int halfW = w / 2 - kPad;

    // ── Vibrato section (left half) ──
    int vx = kPad + 8, vw = halfW - 16;
    int vy = y + kPad + secH + 16;

    // Vibrato Mode toggle
    _vibModeBtn = new Fl_Light_Button(vx, vy, 280, slH, "PRESET / USER");
    _vibModeBtn->box(FL_FLAT_BOX);
    _vibModeBtn->color(fl_rgb_color(50, 52, 50));
    _vibModeBtn->labelcolor(kTextMid);
    _vibModeBtn->labelsize(16);
    _vibModeBtn->selection_color(fl_rgb_color(50, 200, 80));

    float initVibDelay = currentSampler() ? currentSampler()->getVibratoDelay() * 7.0f : 0.0f;
    _vibDelaySlider = makeSlider(vx, vy + slH + slGap, vw, slH,
                                 "Vibrato Delay (0–14)", 0.0, 14.0,
                                 std::clamp(static_cast<double>(initVibDelay), 0.0, 14.0),
                                 fl_rgb_color(50, 200, 80));
    _vibDelaySlider->step(1.0);

    float initVibDepth = currentSampler() ? currentSampler()->getVibratoDepth() * 14.0f : 0.0f;
    _vibDepthSlider = makeSlider(vx, vy + (slH + slGap) * 2, vw, slH,
                                 "Vibrato Depth (0–14)", 0.0, 14.0,
                                 std::clamp(static_cast<double>(initVibDepth), 0.0, 14.0),
                                 fl_rgb_color(50, 200, 80));
    _vibDepthSlider->step(1.0);

    float initVibSpeed = currentSampler() ? currentSampler()->getVibratoSpeed() : 5.5f;
    // Map Hz to 0–14 range: 0=1Hz, 14=12Hz approx
    double vibSpeedVal = std::clamp((initVibSpeed - 1.0) / (12.0 - 1.0) * 14.0, 0.0, 14.0);
    _vibSpeedSlider = makeSlider(vx, vy + (slH + slGap) * 3, vw, slH,
                                 "Vibrato Speed (0–14)", 0.0, 14.0, vibSpeedVal,
                                 fl_rgb_color(50, 200, 80));
    _vibSpeedSlider->step(1.0);

    // ── Slide section (right half) ──
    int sx = w / 2 + kGap / 2 + 8, sw = halfW - 16;
    int sy = y + kPad + secH + 16;

    _slideModeChoice = new Fl_Choice(sx, sy, 280, slH, "Slide Mode");
    _slideModeChoice->align(FL_ALIGN_TOP_LEFT);
    _slideModeChoice->labelcolor(kTextMid);
    _slideModeChoice->labelsize(12);
    _slideModeChoice->textsize(18);
    _slideModeChoice->color(fl_rgb_color(50, 52, 50));
    _slideModeChoice->textcolor(kTextHi);
    _slideModeChoice->add("OFF");
    _slideModeChoice->add("ON");
    _slideModeChoice->add("KNEE");
    _slideModeChoice->value(0);

    _slideTimeSlider = makeSlider(sx, sy + slH + slGap, sw, slH,
                                  "Slide Time (0–14)", 0.0, 14.0, 7.0,
                                  fl_rgb_color(50, 200, 80));
    _slideTimeSlider->step(1.0);

    // Touch Vibrato toggle
    _touchVibBtn = new Fl_Light_Button(sx, sy + (slH + slGap) * 2, 280, slH, "Touch Vibrato");
    _touchVibBtn->box(FL_FLAT_BOX);
    _touchVibBtn->color(fl_rgb_color(50, 52, 50));
    _touchVibBtn->labelcolor(kTextMid);
    _touchVibBtn->labelsize(16);
    _touchVibBtn->selection_color(fl_rgb_color(50, 200, 80));
    _touchVibBtn->value(0);

    _pages[1]->end();
}

// ── Page 3: EFFECT 1 (placeholder) ──────────────────────────────────────────
void VoiceEditorWindow::buildPage3(int x, int y, int w, int h) {
    _pages[2] = new Fl_Group(x, y, w, h);
    _pages[2]->box(FL_FLAT_BOX);
    _pages[2]->color(kWinBg);

    int cy = y + kPad + 30;

    _fx1CatChoice = new Fl_Choice(kPad + 8, cy + 16, 300, 36, "Effect 1 Category");
    _fx1CatChoice->align(FL_ALIGN_TOP_LEFT);
    _fx1CatChoice->labelcolor(kTextMid);
    _fx1CatChoice->labelsize(12);
    _fx1CatChoice->textsize(18);
    _fx1CatChoice->color(fl_rgb_color(50, 52, 50));
    _fx1CatChoice->textcolor(kTextHi);
    _fx1CatChoice->add("REVERB");
    _fx1CatChoice->add("DELAY");
    _fx1CatChoice->add("CHORUS");
    _fx1CatChoice->add("FLANGER");
    _fx1CatChoice->add("PHASER");
    _fx1CatChoice->add("TREMOLO");
    _fx1CatChoice->add("ROTARY");
    _fx1CatChoice->add("DISTORTION");
    _fx1CatChoice->value(0);

    _fx1TypeChoice = new Fl_Choice(kPad + 8, cy + 70, 300, 36, "Effect 1 Type");
    _fx1TypeChoice->align(FL_ALIGN_TOP_LEFT);
    _fx1TypeChoice->labelcolor(kTextMid);
    _fx1TypeChoice->labelsize(12);
    _fx1TypeChoice->textsize(18);
    _fx1TypeChoice->color(fl_rgb_color(50, 52, 50));
    _fx1TypeChoice->textcolor(kTextHi);
    _fx1TypeChoice->add("(no types available)");
    _fx1TypeChoice->value(0);

    // Placeholder label
    Fl_Box* placeholder = new Fl_Box(kPad + 8, cy + 140, w - kPad * 2 - 16, 200,
        "Connect to ELS-02C to edit effects\n\nEffect parameters will be displayed here\nwhen the hardware connection is active.");
    placeholder->box(FL_FLAT_BOX);
    placeholder->color(fl_rgb_color(30, 32, 30));
    placeholder->labelcolor(kTextMid);
    placeholder->labelsize(18);
    placeholder->labelfont(FL_HELVETICA_ITALIC);
    placeholder->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

    _pages[2]->end();
}

// ── Page 4: EFFECT 2 (placeholder) ──────────────────────────────────────────
void VoiceEditorWindow::buildPage4(int x, int y, int w, int h) {
    _pages[3] = new Fl_Group(x, y, w, h);
    _pages[3]->box(FL_FLAT_BOX);
    _pages[3]->color(kWinBg);

    int cy = y + kPad + 30;

    _fx2CatChoice = new Fl_Choice(kPad + 8, cy + 16, 300, 36, "Effect 2 Category");
    _fx2CatChoice->align(FL_ALIGN_TOP_LEFT);
    _fx2CatChoice->labelcolor(kTextMid);
    _fx2CatChoice->labelsize(12);
    _fx2CatChoice->textsize(18);
    _fx2CatChoice->color(fl_rgb_color(50, 52, 50));
    _fx2CatChoice->textcolor(kTextHi);
    _fx2CatChoice->add("REVERB");
    _fx2CatChoice->add("DELAY");
    _fx2CatChoice->add("CHORUS");
    _fx2CatChoice->add("FLANGER");
    _fx2CatChoice->add("PHASER");
    _fx2CatChoice->add("TREMOLO");
    _fx2CatChoice->add("ROTARY");
    _fx2CatChoice->add("DISTORTION");
    _fx2CatChoice->value(0);

    _fx2TypeChoice = new Fl_Choice(kPad + 8, cy + 70, 300, 36, "Effect 2 Type");
    _fx2TypeChoice->align(FL_ALIGN_TOP_LEFT);
    _fx2TypeChoice->labelcolor(kTextMid);
    _fx2TypeChoice->labelsize(12);
    _fx2TypeChoice->textsize(18);
    _fx2TypeChoice->color(fl_rgb_color(50, 52, 50));
    _fx2TypeChoice->textcolor(kTextHi);
    _fx2TypeChoice->add("(no types available)");
    _fx2TypeChoice->value(0);

    Fl_Box* placeholder = new Fl_Box(kPad + 8, cy + 140, w - kPad * 2 - 16, 200,
        "Connect to ELS-02C to edit effects\n\nEffect parameters will be displayed here\nwhen the hardware connection is active.");
    placeholder->box(FL_FLAT_BOX);
    placeholder->color(fl_rgb_color(30, 32, 30));
    placeholder->labelcolor(kTextMid);
    placeholder->labelsize(18);
    placeholder->labelfont(FL_HELVETICA_ITALIC);
    placeholder->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

    _pages[3]->end();
}

// ── Page 5: TRANSPOSE / TUNE ────────────────────────────────────────────────
void VoiceEditorWindow::buildPage5(int x, int y, int w, int h) {
    _pages[4] = new Fl_Group(x, y, w, h);
    _pages[4]->box(FL_FLAT_BOX);
    _pages[4]->color(kWinBg);

    constexpr int slH = 36;
    int halfW = w / 2 - kPad;
    int secY = y + kPad + 30;

    // Transpose
    float initPitch = currentSampler() ? currentSampler()->getPitchBend() * 12.0f : 0.0f;
    // Clamp to -6..+6
    double transpVal = std::clamp(static_cast<double>(initPitch), -6.0, 6.0);
    _transposeSlider = makeSlider(kPad + 8, secY + 16, halfW - 16, slH,
                                  "Transpose (-6 ~ +6 semitones)",
                                  -6.0, 6.0, transpVal,
                                  fl_rgb_color(60, 130, 230));
    _transposeSlider->step(1.0);

    // Detune
    float initDetune = currentSampler() ? currentSampler()->getFineTune() : 0.0f;
    _detuneSlider = makeSlider(w / 2 + kGap / 2 + 8, secY + 16, halfW - 16, slH,
                               "Tune / Detune (-64 ~ +63 cents)",
                               -64.0, 63.0, static_cast<double>(initDetune),
                               fl_rgb_color(60, 130, 230));
    _detuneSlider->step(1.0);

    // Large visual labels for center detent indication
    Fl_Box* transLabel = new Fl_Box(kPad + 8, secY + 60, halfW - 16, 30,
        "← Flatten     0     Sharpen →");
    transLabel->labelcolor(kTextMid);
    transLabel->labelsize(16);
    transLabel->labelfont(FL_HELVETICA);

    Fl_Box* detuneLabel = new Fl_Box(w / 2 + kGap / 2 + 8, secY + 60, halfW - 16, 30,
        "← Flat     0     Sharp →");
    detuneLabel->labelcolor(kTextMid);
    detuneLabel->labelsize(16);
    detuneLabel->labelfont(FL_HELVETICA);

    _pages[4]->end();
}

// ── Page 6: VOICE EDIT ──────────────────────────────────────────────────────
void VoiceEditorWindow::buildPage6(int x, int y, int w, int h) {
    _pages[5] = new Fl_Group(x, y, w, h);
    _pages[5]->box(FL_FLAT_BOX);
    _pages[5]->color(kWinBg);

    constexpr int slH    = 28;
    constexpr int slGap  = 74;   // label(16) + padding(30) + slider(28)
    constexpr int secH   = 42;
    constexpr int secGap = 12;   // gap between sections

    // Layout: 2 columns for EQ, left col for ADSR, right for curve, bottom 3-col filter+LFO
    int halfW = w / 2 - kPad;

    int baseY = y + kPad;

    // ════ EQ section (2 columns) ════
    int eqY = baseY + secH + 8;

    float initLoFreq = currentSampler() ? currentSampler()->getEqLowFreq() : 200.0f;
    _eqLoFreqSlider = makeSlider(kPad + 8, eqY, halfW - 16, slH,
                                 "Low EQ Freq (32\xe2\x80\x93" "2000 Hz)",
                                 32.0, 2000.0, initLoFreq,
                                 fl_rgb_color(30, 170, 170));

    float initLoGain = currentSampler() ? currentSampler()->getEqLowGain() : 0.0f;
    _eqLoGainSlider = makeSlider(w / 2 + 4, eqY, halfW - 8, slH,
                                 "Low EQ Gain (-12 ~ +12 dB)",
                                 -12.0, 12.0, initLoGain,
                                 fl_rgb_color(30, 170, 170));

    float initHiFreq = currentSampler() ? currentSampler()->getEqHighFreq() : 4000.0f;
    _eqHiFreqSlider = makeSlider(kPad + 8, eqY + slGap, halfW - 16, slH,
                                 "High EQ Freq (500\xe2\x80\x93" "16000 Hz)",
                                 500.0, 16000.0, initHiFreq,
                                 fl_rgb_color(30, 170, 170));

    float initHiGain = currentSampler() ? currentSampler()->getEqHighGain() : 0.0f;
    _eqHiGainSlider = makeSlider(w / 2 + 4, eqY + slGap, halfW - 8, slH,
                                 "High EQ Gain (-12 ~ +12 dB)",
                                 -12.0, 12.0, initHiGain,
                                 fl_rgb_color(30, 170, 170));

    // ════ ADSR section ════
    int eqSecH = secH + 8 + slGap * 2 + slH;  // total EQ section pixel height
    int adsrBaseY = baseY + eqSecH + secGap;
    int adsrSliderW = w * 35 / 100;

    float iA = currentSampler() ? currentSampler()->getAdsrAttack()  * 1000.0f : 5.0f;
    float iD = currentSampler() ? currentSampler()->getAdsrDecay()   * 1000.0f : 100.0f;
    float iS = currentSampler() ? currentSampler()->getAdsrSustain() * 100.0f  : 100.0f;
    float iR = currentSampler() ? currentSampler()->getAdsrRelease() * 1000.0f : 80.0f;

    int adsrY0 = adsrBaseY + secH + 6;
    _attackSlider  = makeSlider(kPad + 8, adsrY0,              adsrSliderW - 16, slH,
                                "Attack (ms)", 1.0, 2000.0, iA, kAmber);
    _decaySlider   = makeSlider(kPad + 8, adsrY0 + slGap,     adsrSliderW - 16, slH,
                                "Decay (ms)", 1.0, 2000.0, iD, kAmber);
    _sustainSlider = makeSlider(kPad + 8, adsrY0 + slGap * 2, adsrSliderW - 16, slH,
                                "Sustain (%)", 0.0, 100.0, iS, kAmber);
    _releaseSlider = makeSlider(kPad + 8, adsrY0 + slGap * 3, adsrSliderW - 16, slH,
                                "Release (ms)", 1.0, 5000.0, iR, kAmber);

    // ADSR curve area (right of ADSR sliders)
    int curveX = kPad + 8 + adsrSliderW;
    int curveW = w - curveX - kPad - 8;
    int curveY = adsrY0;
    int curveH = slGap * 3 + slH;
    _adsrCurveX = curveX;
    _adsrCurveY = curveY;
    _adsrCurveW = curveW;
    _adsrCurveH = curveH;

    // ════ Filter + LFO section (3 columns) ════
    int filterBaseY = adsrY0 + slGap * 4 + secGap;
    int col1X = kPad + 8;
    int col1W = w / 3 - kPad - 4;
    int col2X = w / 3 + 4;
    int col2W = w / 3 - 8;
    int col3X = 2 * w / 3 + 4;
    int col3W = w / 3 - kPad - 4;

    int filterY0 = filterBaseY + secH + 6;

    float iF = currentSampler() ? currentSampler()->getFilterCutoff() * 100.0f : 100.0f;
    _filterSlider = makeSlider(col1X, filterY0, col1W, slH,
                               "Filter Cutoff (%)", 0.0, 100.0, iF,
                               fl_rgb_color(30, 170, 170));

    float initRes = currentSampler() ? currentSampler()->getResonance() * 100.0f : 0.0f;
    _resonanceSlider = makeSlider(col1X, filterY0 + slGap, col1W, slH,
                                  "Resonance (%)", 0.0, 100.0, initRes,
                                  fl_rgb_color(30, 170, 170));

    // LFO controls (col2 + col3)
    _lfoWaveChoice = new Fl_Choice(col2X, filterY0, 300, slH, "LFO Waveform");
    _lfoWaveChoice->align(FL_ALIGN_TOP_LEFT);
    _lfoWaveChoice->labelcolor(kTextMid);
    _lfoWaveChoice->labelsize(12);
    _lfoWaveChoice->textsize(18);
    _lfoWaveChoice->color(fl_rgb_color(50, 52, 50));
    _lfoWaveChoice->textcolor(kTextHi);
    _lfoWaveChoice->add("Sine");
    _lfoWaveChoice->add("Sawtooth");
    _lfoWaveChoice->add("Triangle");
    _lfoWaveChoice->add("Square");
    _lfoWaveChoice->add("Random");
    int initLfoWave = currentSampler() ? currentSampler()->getLfoWave() : 0;
    _lfoWaveChoice->value(initLfoWave);
    _lfoWaveChoice->callback(choiceCb, this);

    float initLfoSpd = currentSampler() ? currentSampler()->getLfoSpeed() : 5.5f;
    _lfoSpeedSlider = makeSlider(col2X, filterY0 + slGap, col2W, slH,
                                 "LFO Speed (Hz)", 0.1, 20.0, initLfoSpd,
                                 fl_rgb_color(120, 80, 180));

    float initPmd = currentSampler() ? currentSampler()->getLfoPmd() : 0.0f;
    _lfoPmdSlider = makeSlider(col3X, filterY0, col3W, slH,
                               "LFO PMD (Pitch) 0\xe2\x80\x93" "400", 0.0, 400.0, initPmd,
                               fl_rgb_color(120, 80, 180));

    float initFmd = currentSampler() ? currentSampler()->getLfoFmd() : 0.0f;
    _lfoFmdSlider = makeSlider(col3X, filterY0 + slGap, col3W, slH,
                               "LFO FMD (Filter) 0\xe2\x80\x93" "4800", 0.0, 4800.0, initFmd,
                               fl_rgb_color(120, 80, 180));

    float initAmd = currentSampler() ? currentSampler()->getLfoAmd() : 0.0f;
    _lfoAmdSlider = makeSlider(col3X, filterY0 + slGap * 2, col3W, slH,
                               "LFO AMD (Amp) 0\xe2\x80\x93" "128", 0.0, 128.0, initAmd,
                               fl_rgb_color(120, 80, 180));

    _pages[5]->end();
}

// ── makeSlider ────────────────────────────────────────────────────────────────
Fl_Slider* VoiceEditorWindow::makeSlider(int sx, int sy, int sw, int sh,
                                          const char* label,
                                          double lo, double hi, double val,
                                          Fl_Color fillColor)
{
    auto* s = new Fl_Slider(FL_HOR_FILL_SLIDER, sx, sy, sw, sh, nullptr);
    s->bounds(lo, hi);
    s->value(val);
    s->color(fl_rgb_color(50, 52, 50));
    s->selection_color(fillColor);
    s->box(FL_FLAT_BOX);
    s->copy_label(label);
    s->labelcolor(kTextMid);
    s->labelsize(12);
    s->align(FL_ALIGN_TOP_LEFT);
    s->callback(sliderCb, this);
    return s;
}

// ── draw ──────────────────────────────────────────────────────────────────────
void VoiceEditorWindow::draw() {
    // Window background
    fl_color(kWinBg);
    fl_rectf(0, 0, kWinW, kWinH);

    // ── Header bar ────────────────────────────────────────────────────────
    Fl_Color accent = slotAccent(_slot);
    fl_color(accent);
    fl_rectf(0, 0, kWinW, 4);
    fl_color(fl_rgb_color(32, 34, 32));
    fl_rectf(0, 4, kWinW, kHeaderH - 4);

    // Slot colour dot
    fl_color(accent);
    fl_rectf(kPad, 14, 26, 26);
    fl_color(fl_color_average(accent, FL_WHITE, 0.30f));
    fl_rectf(kPad + 1, 14, 26, 5);
    fl_color(fl_color_average(accent, kWinBg, 0.50f));
    fl_rect(kPad, 14, 26, 26);

    // Slot number inside dot
    {
        char buf[4]; snprintf(buf, sizeof(buf), "%d", _slot + 1);
        fl_font(FL_HELVETICA_BOLD, 15);
        fl_color(FL_WHITE);
        fl_draw(buf, kPad, 14, 26, 26, FL_ALIGN_CENTER);
    }

    // Title text
    fl_font(FL_HELVETICA_BOLD, 24);
    fl_color(kTextHi);
    {
        const char* roleName = (_slot >= 0 && _slot < 8) ? kSlotName[_slot] : "VOICE";
        fl_draw(roleName, kPad + 36, 8, kWinW - kPad * 2 - 120, 26,
                FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    }
    fl_font(FL_HELVETICA, 16);
    fl_color(fl_color_average(accent, FL_WHITE, 0.55f));
    fl_draw(_voiceName.c_str(), kPad + 36, 30, kWinW - kPad * 2 - 120, 22,
            FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    // Separator under header
    fl_color(fl_rgb_color(55, 57, 55));
    fl_line(0, kHeaderH - 1, kWinW - 1, kHeaderH - 1);

    // Layer bar background
    fl_color(kLayerBarBg);
    fl_rectf(0, kHeaderH, kWinW, kLayerBarH);

    // Tab bar background
    fl_color(kTabBarBg);
    fl_rectf(0, kHeaderH + kLayerBarH, kWinW, kTabBarH);

    // Active tab accent underline
    if (_currentPage >= 0 && _currentPage < kNumPages) {
        Fl_Color tabAcc = tabAccent(_currentPage);
        int tabW = kWinW / kNumPages;
        int tx = _currentPage * tabW;
        int tw = (_currentPage == kNumPages - 1) ? (kWinW - tx) : tabW;
        fl_color(tabAcc);
        fl_rectf(tx, kHeaderH + kLayerBarH + kTabBarH - 3, tw, 3);
    }

    // ── Page-specific section backgrounds ────────────────────────────────
    if (_currentPage == 0) {
        // Page 1: Basic — draw section backgrounds
        constexpr int voiceSecH = 160;
        drawSection(kPad, kPageY + kPad, kWinW - kPad * 2, voiceSecH, "VOICE / INSTRUMENT", kAccentVolume);

        int secBase = kPageY + kPad + voiceSecH + kGap;
        drawSection(kPad, secBase, kWinW / 3 - kPad, 90, "VOLUME", kAccentVolume);
        drawSection(kWinW / 3 + kGap / 2, secBase, kWinW / 3 - kGap, 90, "PAN", kAccentVolume);
        drawSection(2 * kWinW / 3 + kGap / 2, secBase, kWinW / 3 - kPad - kGap / 2, 90, "FEET / OCTAVE", kAccentVolume);

        int touchY = secBase + 90 + 24;
        drawSection(kPad, touchY, kWinW / 3 - kPad, 160, "TOUCH SENSITIVITY", kAccentVolume);
        drawSection(kWinW / 3 + kGap / 2, touchY, kWinW / 3 - kGap, 160, "TOUCH PARAMETERS", kAccentVolume);
        drawSection(2 * kWinW / 3 + kGap / 2, touchY, kWinW / 3 - kPad - kGap / 2, 90, "REVERB SEND", kAccentVolume);
    }
    else if (_currentPage == 1) {
        int halfW = kWinW / 2 - kPad;
        drawSection(kPad, kPageY + kPad, halfW, kPageH - kPad * 2, "VIBRATO", kAccentVibrato);
        drawSection(kWinW / 2 + kGap / 2, kPageY + kPad, halfW, kPageH - kPad * 2, "SLIDE / ARTICULATION", kAccentVibrato);
    }
    else if (_currentPage == 2) {
        drawSection(kPad, kPageY + kPad, kWinW - kPad * 2, kPageH - kPad * 2, "EFFECT 1", kAccentFilter);
    }
    else if (_currentPage == 3) {
        drawSection(kPad, kPageY + kPad, kWinW - kPad * 2, kPageH - kPad * 2, "EFFECT 2", kAccentFilter);
    }
    else if (_currentPage == 4) {
        int halfW = kWinW / 2 - kPad;
        drawSection(kPad, kPageY + kPad, halfW, 140, "TRANSPOSE", kAccentPitch);
        drawSection(kWinW / 2 + kGap / 2, kPageY + kPad, halfW, 140, "TUNE / DETUNE", kAccentPitch);
    }
    else if (_currentPage == 5) {
        // Voice Edit page sections — match buildPage6 layout
        constexpr int secH2 = 42, slGap2 = 74, slH2 = 28, secGap2 = 12;

        int baseY2 = kPageY + kPad;
        int eqSecH2 = secH2 + 8 + slGap2 + slH2 + 4;
        drawSection(kPad, baseY2, kWinW - kPad * 2, eqSecH2, "EQUALIZER", kAccentFilter);

        int adsrBaseY2 = baseY2 + eqSecH2 + secGap2;
        int adsrH2 = secH2 + 6 + slGap2 * 3 + slH2 + 4;
        drawSection(kPad, adsrBaseY2, kWinW - kPad * 2, adsrH2, "ENVELOPE (ADSR)", kAccentAdsr);

        int filterBaseY2 = adsrBaseY2 + secH2 + 6 + slGap2 * 4 + secGap2;
        int filterH2 = kWinH - filterBaseY2 - kPad;
        if (filterH2 < 80) filterH2 = 80;
        drawSection(kPad, filterBaseY2, kWinW / 3 - kPad, filterH2, "FILTER", kAccentFilter);
        drawSection(kWinW / 3 + 4, filterBaseY2, 2 * kWinW / 3 - kPad - 4, filterH2, "LFO MODULATION", kAccentAdsr);
    }

    // ── ADSR curve (only on Page 6) ─────────────────────────────────────
    if (_currentPage == 5 && _adsrCurveW > 20 && _adsrCurveH > 20)
        drawAdsrCurve(_adsrCurveX, _adsrCurveY, _adsrCurveW, _adsrCurveH);

    // Draw child widgets on top
    draw_children();
}

// ── drawSection ───────────────────────────────────────────────────────────────
void VoiceEditorWindow::drawSection(int sx, int sy, int sw, int sh,
                                     const char* title, Fl_Color accent) {
    fl_color(kSectionBg);
    fl_rectf(sx, sy, sw, sh);

    constexpr int kTH = 28;
    fl_color(kTitleBg);
    fl_rectf(sx, sy, sw, kTH);

    fl_color(accent);
    fl_rectf(sx, sy, 3, sh);

    fl_font(FL_HELVETICA_BOLD, 14);
    fl_color(fl_color_average(accent, FL_WHITE, 0.55f));
    fl_draw(title, sx + 8, sy, sw - 12, kTH, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    fl_color(fl_rgb_color(55, 57, 55));
    fl_rect(sx, sy, sw, sh);
}

// ── drawAdsrCurve ─────────────────────────────────────────────────────────────
void VoiceEditorWindow::drawAdsrCurve(int cx, int cy, int cw, int ch) {
    fl_color(fl_rgb_color(26, 28, 26));
    fl_rectf(cx, cy, cw, ch);
    fl_color(fl_rgb_color(50, 52, 50));
    fl_rect(cx, cy, cw, ch);

    if (!_attackSlider || !_decaySlider || !_sustainSlider || !_releaseSlider)
        return;

    double attackMs  = _attackSlider ->value();
    double decayMs   = _decaySlider  ->value();
    double sustainPct= _sustainSlider->value();
    double releaseMs = _releaseSlider ->value();

    double totalMs = attackMs + decayMs + 500.0 + releaseMs;
    if (totalMs < 1.0) totalMs = 1.0;

    double fA  = attackMs  / totalMs;
    double fD  = decayMs   / totalMs;
    double fS  = 500.0     / totalMs;
    (void)releaseMs;

    double sLvl = sustainPct / 100.0;

    int x0 = cx + 4,  x1 = cx + 4 + static_cast<int>(fA * (cw - 8));
    int x2 = x1 + static_cast<int>(fD * (cw - 8));
    int x3 = x2 + static_cast<int>(fS * (cw - 8));
    int x4 = cx + cw - 4;

    int yBot = cy + ch - 6;
    int yTop = cy + 6;
    int yRange = yBot - yTop;
    int ySust = yBot - static_cast<int>(sLvl * yRange);

    // Grid lines
    fl_color(fl_rgb_color(42, 44, 42));
    for (int gx = cx + cw / 4; gx < cx + cw; gx += cw / 4)
        fl_line(gx, cy + 1, gx, cy + ch - 2);
    for (int gy = cy + ch / 4; gy < cy + ch; gy += ch / 4)
        fl_line(cx + 1, gy, cx + cw - 2, gy);

    // Filled area under curve
    fl_color(fl_rgb_color(80, 40, 100));
    for (int px = x0; px < x1 && px < x4; ++px) {
        double t = (x1 > x0) ? (double)(px - x0) / (x1 - x0) : 1.0;
        int pTop = yBot - static_cast<int>(t * yRange);
        fl_line(px, pTop, px, yBot);
    }
    for (int px = x1; px < x2 && px < x4; ++px) {
        double t = (x2 > x1) ? (double)(px - x1) / (x2 - x1) : 1.0;
        int pTop = yTop + static_cast<int>(t * (ySust - yTop));
        fl_line(px, pTop, px, yBot);
    }
    for (int px = x2; px < x3 && px < x4; ++px)
        fl_line(px, ySust, px, yBot);
    for (int px = x3; px < x4; ++px) {
        double t = (x4 > x3) ? (double)(px - x3) / (x4 - x3) : 1.0;
        int pTop = ySust + static_cast<int>(t * (yBot - ySust));
        fl_line(px, pTop, px, yBot);
    }

    // Curve line
    fl_color(kAmber);
    fl_line_style(FL_SOLID, 2);
    fl_line(x0, yBot, x1, yTop);
    fl_line(x1, yTop, x2, ySust);
    fl_line(x2, ySust, x3, ySust);
    fl_line(x3, ySust, x4, yBot);
    fl_line_style(FL_SOLID, 0);

    // Knob dots
    fl_color(fl_color_average(kAmber, FL_WHITE, 0.30f));
    fl_pie(x1 - 4, yTop - 4, 8, 8, 0, 360);
    fl_pie(x2 - 4, ySust - 4, 8, 8, 0, 360);
    fl_pie(x3 - 4, ySust - 4, 8, 8, 0, 360);

    // Phase labels
    fl_font(FL_HELVETICA, 16);
    fl_color(kTextMid);
    fl_draw("A",  x0 + 2, cy + 2, 20, 14, FL_ALIGN_LEFT);
    fl_draw("D",  x1 + 2, cy + 2, 20, 14, FL_ALIGN_LEFT);
    fl_draw("S",  x2 + 2, cy + 2, 20, 14, FL_ALIGN_LEFT);
    fl_draw("R",  x3 + 2, cy + 2, 20, 14, FL_ALIGN_LEFT);
}

// ── Callbacks ─────────────────────────────────────────────────────────────────
void VoiceEditorWindow::sliderCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<VoiceEditorWindow*>(data);
    if (!self->currentSampler()) return;

    auto* s = static_cast<Fl_Slider*>(w);
    double v = s->value();

    // ── Page 1: Basic ──
    if (s == self->_volumeSlider) {
        self->currentSampler()->setExpressionGain(static_cast<float>(v / 127.0));
    } else if (s == self->_panSlider) {
        // 0..6 → -1..+1
        self->currentSampler()->setPan(static_cast<float>((v / 3.0) - 1.0));
    } else if (s == self->_reverbSendSlider) {
        self->currentSampler()->setReverbSend(static_cast<float>(v / 24.0));
    }
    // Touch sliders: display-only for now (no engine mapping)

    // ── Page 2: Vibrato / Slide ──
    else if (s == self->_vibDelaySlider) {
        // 0–14 → 0–2 seconds
        self->currentSampler()->setVibratoDelay(static_cast<float>(v / 7.0));
    } else if (s == self->_vibDepthSlider) {
        self->currentSampler()->setVibratoDepth(static_cast<float>(v / 14.0));
    } else if (s == self->_vibSpeedSlider) {
        // 0–14 → 1–12 Hz
        float hz = 1.0f + static_cast<float>(v / 14.0) * 11.0f;
        self->currentSampler()->setVibratoSpeed(hz);
    }

    // ── Page 5: Transpose / Tune ──
    else if (s == self->_transposeSlider) {
        self->currentSampler()->setPitchBend(static_cast<float>(v / 12.0));
    } else if (s == self->_detuneSlider) {
        self->currentSampler()->setFineTune(static_cast<float>(v));
    }

    // ── Page 6: Voice Edit ──
    // EQ
    else if (s == self->_eqLoFreqSlider) {
        self->currentSampler()->setEqLowFreq(static_cast<float>(v));
    } else if (s == self->_eqLoGainSlider) {
        self->currentSampler()->setEqLowGain(static_cast<float>(v));
    } else if (s == self->_eqHiFreqSlider) {
        self->currentSampler()->setEqHighFreq(static_cast<float>(v));
    } else if (s == self->_eqHiGainSlider) {
        self->currentSampler()->setEqHighGain(static_cast<float>(v));
    }
    // ADSR
    else if (s == self->_attackSlider) {
        float sec = static_cast<float>(v / 1000.0);
        if (sec < 0.001f) sec = 0.001f;
        self->currentSampler()->setAdsrAttack(sec);
    } else if (s == self->_decaySlider) {
        float sec = static_cast<float>(v / 1000.0);
        if (sec < 0.001f) sec = 0.001f;
        self->currentSampler()->setAdsrDecay(sec);
    } else if (s == self->_sustainSlider) {
        self->currentSampler()->setAdsrSustain(static_cast<float>(v / 100.0));
    } else if (s == self->_releaseSlider) {
        float sec = static_cast<float>(v / 1000.0);
        if (sec < 0.001f) sec = 0.001f;
        self->currentSampler()->setAdsrRelease(sec);
    }
    // Filter
    else if (s == self->_filterSlider) {
        self->currentSampler()->setFilterCutoff(static_cast<float>(v / 100.0));
    } else if (s == self->_resonanceSlider) {
        self->currentSampler()->setResonance(static_cast<float>(v / 100.0));
    }
    // LFO
    else if (s == self->_lfoSpeedSlider) {
        self->currentSampler()->setLfoSpeed(static_cast<float>(v));
    } else if (s == self->_lfoPmdSlider) {
        self->currentSampler()->setLfoPmd(static_cast<float>(v));
    } else if (s == self->_lfoFmdSlider) {
        self->currentSampler()->setLfoFmd(static_cast<float>(v));
    } else if (s == self->_lfoAmdSlider) {
        self->currentSampler()->setLfoAmd(static_cast<float>(v));
    }

    // Redraw ADSR curve when any ADSR slider moves (only on Page 6)
    if (self->_currentPage == 5 &&
        (s == self->_attackSlider  || s == self->_decaySlider ||
         s == self->_sustainSlider || s == self->_releaseSlider)) {
        self->redraw();
    }
}

void VoiceEditorWindow::choiceCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<VoiceEditorWindow*>(data);
    if (!self->currentSampler()) return;

    if (w == self->_feetChoice) {
        int val = self->_feetChoice->value();
        // 0=PRESET(0), 1=16'(-1), 2=8'(0), 3=4'(+1)
        int shifts[] = {0, -1, 0, 1};
        self->currentSampler()->setOctaveShift(shifts[std::clamp(val, 0, 3)]);
    } else if (w == self->_lfoWaveChoice) {
        self->currentSampler()->setLfoWave(self->_lfoWaveChoice->value());
    }
    // Effect choices are display-only
}

void VoiceEditorWindow::tabCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<VoiceEditorWindow*>(data);
    for (int i = 0; i < kNumPages; ++i) {
        if (w == self->_tabBtns[i]) {
            self->showPage(i);
            return;
        }
    }
}

void VoiceEditorWindow::closeCb(Fl_Widget* /*w*/, void* data) {
    static_cast<VoiceEditorWindow*>(data)->hide();
}

void VoiceEditorWindow::layerBtnCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<VoiceEditorWindow*>(data);
    for (int i = 0; i < kMaxLayers; ++i) {
        if (w == self->_layerBtns[i]) {
            self->selectLayer(i);
            return;
        }
    }
}

void VoiceEditorWindow::addLayerCb(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<VoiceEditorWindow*>(data);
    if (!self->_slotEngine) return;

    int idx = self->_slotEngine->addLayer();
    if (idx < 0) return;

    // Default new layer to the first voice (Strings 1, cat=0, voice=0)
    self->selectLayer(idx);
    self->applyVoiceToCurrentLayer(0, 0);
}

void VoiceEditorWindow::removeLayerCb(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<VoiceEditorWindow*>(data);
    if (!self->_slotEngine) return;

    int count = self->_slotEngine->layerCount();
    if (count <= 1) return;

    self->_slotEngine->removeLayer(self->_currentLayer);
    // Clamp current layer to valid range
    if (self->_currentLayer >= self->_slotEngine->layerCount())
        self->_currentLayer = self->_slotEngine->layerCount() - 1;
    self->selectLayer(self->_currentLayer);
}

void VoiceEditorWindow::voiceChoiceCb(Fl_Widget* /*w*/, void* /*data*/) {
    // Legacy — no longer used (replaced by voiceBtnCb)
}

void VoiceEditorWindow::voiceCatCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<VoiceEditorWindow*>(data);
    for (int c = 0; c < kNumVoiceCatsConst; ++c) {
        if (w == self->_voiceCatBtns[c]) {
            self->showVoiceCat(c);
            return;
        }
    }
}

void VoiceEditorWindow::voiceBtnCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<VoiceEditorWindow*>(data);
    if (!self->_slotEngine) return;
    int cat = self->_currentVoiceCat;
    for (int v = 0; v < kVoicesPerCatConst; ++v) {
        if (w == self->_voiceBtns[cat][v]) {
            self->applyVoiceToCurrentLayer(cat, v);
            return;
        }
    }
}

void VoiceEditorWindow::applyVoiceToCurrentLayer(int voiceIndex) {
    // Overload: find cat/voice from flat index (used by addLayerCb default)
    // voiceIndex is cat*kVoicesPerCat + v, but for simplicity just use cat=0, v=0
    applyVoiceToCurrentLayer(0, voiceIndex);
}

void VoiceEditorWindow::applyVoiceToCurrentLayer(int cat, int voice) {
    if (!_slotEngine) return;
    if (_currentLayer < 0 || _currentLayer >= _slotEngine->layerCount()) return;
    if (cat < 0 || cat >= kNumVoiceCatsConst) return;
    if (voice < 0 || voice >= kVoicesPerCatConst) return;

    const VoiceDef& vd = kVoiceCats[cat][voice];
    auto& info = _slotEngine->layerInfo(_currentLayer);

    // Set the instrument name
    info.instrumentName = vd.label;

    // Generate waveform with the voice's harmonic recipe
    em::VoiceSlotEngine::generateWaveformWithHarmonics(
        vd.harmonics, info.waveformData, EM_SAMPLE_RATE, 440.0f);
    _slotEngine->layer(_currentLayer)->setWaveform(
        info.waveformData.data(),
        static_cast<uint32_t>(info.waveformData.size()),
        EM_SAMPLE_RATE);

    // Refresh the button highlight
    showVoiceCat(cat);
}

} // namespace em::ui
