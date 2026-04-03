#pragma once
#include <FL/Fl_Widget.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Counter.H>
#include <string>
#include <functional>

namespace em::ui {

using ExpressionCallback = std::function<void(float value)>;
using FootpadCallback    = std::function<void(int pedalId, bool pressed)>;
using TempoCallback      = std::function<void(float bpm)>;

/// Bottom strip: expression fader, knee lever sim, foot switches, tempo, status LEDs.
class FootpadPanel : public Fl_Group {
public:
    FootpadPanel(int x, int y, int w, int h);

    void setExpressionCallback(ExpressionCallback cb) { _exprCb  = std::move(cb); }
    void setFootpadCallback   (FootpadCallback    cb) { _padCb   = std::move(cb); }
    void setTempoCallback     (TempoCallback      cb) { _tempoCb = std::move(cb); }

    // Called from UI event queue poll
    void setMidiConnected(bool v);
    void setAudioRunning (bool v);
    void setUnderrun     (bool v);
    void setLatencyMs    (double ms);
    void setExpression   (float v);   // 0..1, driven by MIDI CC11
    void setTempo        (float bpm); // Update the tempo counter display

    void draw() override;

private:
    static void exprSliderCb(Fl_Widget* w, void* data);
    static void footBtnCb   (Fl_Widget* w, void* data);
    static void tempoCb     (Fl_Widget* w, void* data);

    Fl_Slider*  _exprSlider   = nullptr;
    Fl_Slider*  _kneeSlider   = nullptr;
    Fl_Counter* _tempoCounter = nullptr;

    bool   _midiOk   = false;
    bool   _audioOk  = false;
    bool   _underrun = false;
    double _latencyMs = 0.0;

    ExpressionCallback _exprCb;
    FootpadCallback    _padCb;
    TempoCallback      _tempoCb;

    void drawLed(int x, int y, int r, bool lit, Fl_Color onCol, const char* label);
};

} // namespace em::ui
