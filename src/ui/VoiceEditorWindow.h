#pragma once
#include <FL/Fl_Window.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Light_Button.H>
#include <string>
#include "common/SamplerEngine.h"
#include "common/VoiceSlotEngine.h"

namespace em::ui {

/// Tabbed voice parameter editor matching ELS-02C Voice Condition pages.
/// Opens when the user double-clicks a voice card on the LCD panel.
/// 6 tabs: P.1 Basic, P.2 Vibrato/Slide, P.3 Effect1, P.4 Effect2,
///         P.5 Transpose/Tune, P.6 Voice Edit (deep edit).
class VoiceEditorWindow : public Fl_Window {
public:
    VoiceEditorWindow(int slot, const std::string& voiceName,
                      em::VoiceSlotEngine* slotEngine);

    void draw() override;

private:
    int                    _slot;
    std::string            _voiceName;
    em::VoiceSlotEngine*   _slotEngine;
    int                    _currentLayer = 0;
    int                    _currentPage = 0;  // 0-5

    /// Helper: returns the SamplerEngine for the currently selected layer.
    em::SamplerEngine* currentSampler() {
        return _slotEngine ? _slotEngine->layer(_currentLayer) : nullptr;
    }

    // ── Tab buttons ──────────────────────────────────────────────────────
    static constexpr int kNumPages = 6;
    Fl_Button* _tabBtns[kNumPages] = {};

    // ── Layer bar buttons ────────────────────────────────────────────────
    static constexpr int kMaxLayers = 8;
    Fl_Button* _layerBtns[kMaxLayers] = {};
    Fl_Button* _addLayerBtn    = nullptr;
    Fl_Button* _removeLayerBtn = nullptr;
    Fl_Group*  _layerBarGroup  = nullptr;

    // ── Voice selector (Page 1 button grid) ──────────────────────────────
    static constexpr int kNumVoiceCats = 4;   ///< category tabs
    static constexpr int kVoicesPerCat = 7;   ///< max voices per category page
    Fl_Button* _voiceCatBtns[kNumVoiceCats] = {};
    Fl_Group*  _voiceCatPages[kNumVoiceCats] = {};
    Fl_Button* _voiceBtns[kNumVoiceCats][kVoicesPerCat] = {};
    int        _currentVoiceCat = 0;

    // ── Page groups (shown/hidden) ───────────────────────────────────────
    Fl_Group* _pages[kNumPages] = {};

    // ── Page 1: Basic ────────────────────────────────────────────────────
    Fl_Slider* _volumeSlider      = nullptr;
    Fl_Slider* _panSlider         = nullptr;
    Fl_Choice* _feetChoice        = nullptr;
    Fl_Slider* _initTouchSlider   = nullptr;
    Fl_Slider* _afterTouchSlider  = nullptr;
    Fl_Slider* _horizTouchSlider  = nullptr;
    Fl_Slider* _atPitchSlider     = nullptr;
    Fl_Slider* _reverbSendSlider  = nullptr;

    // ── Page 2: Vibrato / Slide ──────────────────────────────────────────
    Fl_Light_Button* _vibModeBtn  = nullptr;
    Fl_Slider* _vibDelaySlider    = nullptr;
    Fl_Slider* _vibDepthSlider    = nullptr;
    Fl_Slider* _vibSpeedSlider    = nullptr;
    Fl_Choice* _slideModeChoice   = nullptr;
    Fl_Slider* _slideTimeSlider   = nullptr;
    Fl_Light_Button* _touchVibBtn = nullptr;

    // ── Page 3 & 4: Effect placeholders ──────────────────────────────────
    Fl_Choice* _fx1CatChoice      = nullptr;
    Fl_Choice* _fx1TypeChoice     = nullptr;
    Fl_Choice* _fx2CatChoice      = nullptr;
    Fl_Choice* _fx2TypeChoice     = nullptr;

    // ── Page 5: Transpose / Tune ─────────────────────────────────────────
    Fl_Slider* _transposeSlider   = nullptr;
    Fl_Slider* _detuneSlider      = nullptr;

    // ── Page 6: Voice Edit ───────────────────────────────────────────────
    Fl_Slider* _eqLoFreqSlider    = nullptr;
    Fl_Slider* _eqLoGainSlider    = nullptr;
    Fl_Slider* _eqHiFreqSlider    = nullptr;
    Fl_Slider* _eqHiGainSlider    = nullptr;
    Fl_Slider* _attackSlider      = nullptr;
    Fl_Slider* _decaySlider       = nullptr;
    Fl_Slider* _sustainSlider     = nullptr;
    Fl_Slider* _releaseSlider     = nullptr;
    Fl_Slider* _filterSlider      = nullptr;
    Fl_Slider* _resonanceSlider   = nullptr;
    Fl_Choice* _lfoWaveChoice     = nullptr;
    Fl_Slider* _lfoSpeedSlider    = nullptr;
    Fl_Slider* _lfoPmdSlider      = nullptr;
    Fl_Slider* _lfoFmdSlider      = nullptr;
    Fl_Slider* _lfoAmdSlider      = nullptr;

    // ── Layout helpers ────────────────────────────────────────────────────
    void buildLayout();
    void buildLayerBar();
    void buildTabBar();
    void buildPage1(int x, int y, int w, int h);
    void buildPage2(int x, int y, int w, int h);
    void buildPage3(int x, int y, int w, int h);
    void buildPage4(int x, int y, int w, int h);
    void buildPage5(int x, int y, int w, int h);
    void buildPage6(int x, int y, int w, int h);
    void showPage(int pageIndex);
    void selectLayer(int idx);
    void refreshLayerBar();
    void reloadSliderValues();
    void showVoiceCat(int cat);
    void buildVoiceSelector(int x, int y, int w, int h);

    Fl_Slider* makeSlider(int x, int y, int w, int h,
                          const char* label,
                          double lo, double hi, double val,
                          Fl_Color fillColor);

    void drawSection(int x, int y, int w, int h,
                     const char* title, Fl_Color accent);

    void drawAdsrCurve(int x, int y, int w, int h);

    // ── FLTK callbacks ────────────────────────────────────────────────────
    static void sliderCb    (Fl_Widget* w, void* data);
    static void choiceCb    (Fl_Widget* w, void* data);
    static void tabCb       (Fl_Widget* w, void* data);
    static void closeCb     (Fl_Widget* w, void* data);
    static void layerBtnCb  (Fl_Widget* w, void* data);
    static void addLayerCb  (Fl_Widget* w, void* data);
    static void removeLayerCb(Fl_Widget* w, void* data);
    static void voiceChoiceCb(Fl_Widget* w, void* data);
    static void voiceCatCb  (Fl_Widget* w, void* data);
    static void voiceBtnCb  (Fl_Widget* w, void* data);

    /// Apply the selected voice to the current layer (generate waveform + update name).
    void applyVoiceToCurrentLayer(int voiceIndex);
    void applyVoiceToCurrentLayer(int cat, int voice);

    // ── Section geometry (filled in buildLayout) ──────────────────────────
    int _adsrCurveX = 0, _adsrCurveY = 0, _adsrCurveW = 0, _adsrCurveH = 0;

    // Accent colours per section
    static constexpr Fl_Color kAccentVolume    = 0xB83A1000u;  // deep orange
    static constexpr Fl_Color kAccentPitch     = 0x1A4EA000u;  // cobalt blue
    static constexpr Fl_Color kAccentAdsr      = 0x6B1E8000u;  // purple
    static constexpr Fl_Color kAccentFilter    = 0x006B6B00u;  // teal
    static constexpr Fl_Color kAccentVibrato   = 0x1A7A2000u;  // green

    static constexpr Fl_Color kSliderAmber     = 0xE69B1E00u;  // amber fill

    // Tab accent colours per page
    Fl_Color tabAccent(int page) const;
};

} // namespace em::ui
