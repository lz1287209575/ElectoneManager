#include "FootpadPanel.h"
#include <FL/fl_draw.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Counter.H>
#include <cstdio>
#include <cmath>

namespace em::ui {

static const Fl_Color kFpBg      = fl_rgb_color(58, 60, 56);
static const Fl_Color kFpBorder  = fl_rgb_color(38, 39, 36);
static const Fl_Color kFpTop     = fl_rgb_color(78, 80, 76);
static const Fl_Color kLedGreen  = fl_rgb_color(50, 210, 80);
static const Fl_Color kLedCyan   = fl_rgb_color(60, 200, 230);
static const Fl_Color kLedRed    = fl_rgb_color(220, 60, 60);
static const Fl_Color kLedOff    = fl_rgb_color(35, 37, 33);
static const Fl_Color kSliderFg  = fl_rgb_color(50, 160, 220);
static const Fl_Color kSliderBg  = fl_rgb_color(40, 42, 38);
static const Fl_Color kTextColor = fl_rgb_color(195, 196, 188);

FootpadPanel::FootpadPanel(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h) {
    box(FL_FLAT_BOX);
    color(kFpBg);

    int sliderX = x + 14;
    int sliderH = h - 24;
    int sliderY = y + 13;

    // Expression fader (vertical)
    _exprSlider = new Fl_Slider(sliderX, sliderY, 18, sliderH, "EXPR");
    _exprSlider->type(FL_VERT_NICE_SLIDER);
    _exprSlider->range(0.0, 1.0);
    _exprSlider->value(1.0);
    _exprSlider->color(kSliderBg);
    _exprSlider->selection_color(kSliderFg);
    _exprSlider->labelsize(8);
    _exprSlider->labelcolor(kTextColor);
    _exprSlider->align(FL_ALIGN_BOTTOM);
    _exprSlider->callback(exprSliderCb, this);

    // Knee lever fader
    _kneeSlider = new Fl_Slider(sliderX + 28, sliderY, 18, sliderH, "KNEE");
    _kneeSlider->type(FL_VERT_NICE_SLIDER);
    _kneeSlider->range(0.0, 1.0);
    _kneeSlider->value(0.0);
    _kneeSlider->color(kSliderBg);
    _kneeSlider->selection_color(fl_rgb_color(255, 175, 25));
    _kneeSlider->labelsize(8);
    _kneeSlider->labelcolor(kTextColor);
    _kneeSlider->align(FL_ALIGN_BOTTOM);

    // Foot switch buttons — taller, centred vertically
    const char* padLabels[] = {"SW.L", "SW.R", "STOP", "PLAY"};
    static const Fl_Color padColors[] = {
        fl_rgb_color(48, 50, 46),
        fl_rgb_color(48, 50, 46),
        fl_rgb_color(90, 35, 30),   // STOP — dark red
        fl_rgb_color(28, 70, 30),   // PLAY — dark green
    };
    int btnW = 52, btnH = h - 18;
    int btnStartX = x + 82;
    int btnY      = y + 9;
    for (int i = 0; i < 4; ++i) {
        auto* btn = new Fl_Button(btnStartX + i * (btnW + 5),
                                   btnY, btnW, btnH, padLabels[i]);
        btn->box(FL_FLAT_BOX);
        btn->color(padColors[i]);
        btn->labelcolor(kTextColor);
        btn->labelsize(10);
        btn->labelfont(FL_HELVETICA_BOLD);
        btn->user_data(reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        btn->callback(footBtnCb, this);
    }

    // Tempo counter (between foot switches and status LEDs)
    int tempoX = btnStartX + 4 * (btnW + 5) + 10;
    int tempoW = 120;
    _tempoCounter = new Fl_Counter(tempoX, y + 10, tempoW, h - 20, "BPM");
    _tempoCounter->type(FL_SIMPLE_COUNTER);
    _tempoCounter->range(40.0, 240.0);
    _tempoCounter->step(1.0);
    _tempoCounter->lstep(10.0);
    _tempoCounter->value(120.0);
    _tempoCounter->color(fl_rgb_color(28, 30, 28));
    _tempoCounter->selection_color(fl_rgb_color(255, 215, 60));
    _tempoCounter->textcolor(fl_rgb_color(255, 215, 60));
    _tempoCounter->textsize(14);
    _tempoCounter->textfont(FL_COURIER_BOLD);
    _tempoCounter->labelsize(8);
    _tempoCounter->labelcolor(kTextColor);
    _tempoCounter->align(FL_ALIGN_TOP);
    _tempoCounter->callback(tempoCb, this);

    end();
}

void FootpadPanel::setMidiConnected(bool v) { _midiOk  = v; redraw(); }
void FootpadPanel::setAudioRunning (bool v) { _audioOk = v; redraw(); }
void FootpadPanel::setUnderrun     (bool v) { _underrun = v; redraw(); }
void FootpadPanel::setLatencyMs    (double ms) { _latencyMs = ms; redraw(); }

void FootpadPanel::setExpression(float v) {
    if (_exprSlider) {
        _exprSlider->value(static_cast<double>(v));
        _exprSlider->redraw();
    }
}

void FootpadPanel::setTempo(float bpm) {
    if (_tempoCounter) {
        _tempoCounter->value(static_cast<double>(bpm));
        _tempoCounter->redraw();
    }
}

void FootpadPanel::drawLed(int lx, int ly, int r, bool lit,
                            Fl_Color onCol, const char* label) {
    // Outer ring (dark halo)
    fl_color(fl_color_average(kFpBg, FL_BLACK, 0.7f));
    fl_pie(lx - r - 1, ly - r - 1, (r + 1) * 2, (r + 1) * 2, 0, 360);

    fl_color(lit ? onCol : kLedOff);
    fl_pie(lx - r, ly - r, r * 2, r * 2, 0, 360);

    if (lit) {
        // Bright specular highlight
        fl_color(fl_color_average(onCol, FL_WHITE, 0.35f));
        fl_pie(lx - r / 2, ly - r / 2, r, r, 0, 360);
        // Outer glow ring
        fl_color(fl_color_average(onCol, kFpBg, 0.55f));
        fl_arc(lx - r - 2, ly - r - 2, (r + 2) * 2, (r + 2) * 2, 0, 360);
    }

    fl_font(FL_HELVETICA, 8);
    fl_color(kTextColor);
    fl_draw(label, lx - 14, ly + r + 2, 28, 10, FL_ALIGN_CENTER);
}

void FootpadPanel::draw() {
    // Background with subtle top highlight
    fl_color(kFpTop);
    fl_rectf(x(), y(), w(), 2);
    fl_color(kFpBg);
    fl_rectf(x(), y() + 2, w(), h() - 2);

    // Top border (dark inset line)
    fl_color(kFpBorder);
    fl_line(x(), y(), x() + w() - 1, y());
    // Bottom border
    fl_color(fl_rgb_color(28, 29, 27));
    fl_line(x(), y() + h() - 1, x() + w() - 1, y() + h() - 1);

    Fl_Group::draw();

    // Section title
    fl_font(FL_HELVETICA_BOLD, 9);
    fl_color(fl_rgb_color(140, 142, 136));
    fl_draw("PEDAL / EXPRESSION", x() + 4, y() + 3, 150, 10, FL_ALIGN_LEFT);

    // Divider between sliders and foot buttons
    int divX = x() + 75;
    fl_color(fl_rgb_color(42, 44, 40));
    fl_line(divX, y() + 4, divX, y() + h() - 4);
    fl_color(fl_rgb_color(88, 90, 85));
    fl_line(divX + 1, y() + 4, divX + 1, y() + h() - 4);

    // Divider between foot buttons and status area
    int div2X = x() + w() - 120;
    fl_color(fl_rgb_color(42, 44, 40));
    fl_line(div2X, y() + 4, div2X, y() + h() - 4);
    fl_color(fl_rgb_color(88, 90, 85));
    fl_line(div2X + 1, y() + 4, div2X + 1, y() + h() - 4);

    // Status LEDs (right side) — better spacing
    int ledAreaX = x() + w() - 112;
    int ledY = y() + h() / 2 - 4;
    drawLed(ledAreaX + 12,  ledY, 7, _midiOk,   kLedGreen, "MIDI");
    drawLed(ledAreaX + 46,  ledY, 7, _audioOk,  kLedCyan,  "AUDIO");
    drawLed(ledAreaX + 82,  ledY, 7, _underrun, kLedRed,   "ERR");

    // Latency display — styled dark box
    char buf[24];
    snprintf(buf, sizeof(buf), "%.1f ms", _latencyMs);
    int latX = x() + w() - 68;
    int latY = y() + h() - 16;
    fl_color(fl_rgb_color(28, 30, 28));
    fl_rectf(latX, latY, 62, 13);
    fl_color(fl_rgb_color(80, 82, 78));
    fl_rect(latX, latY, 62, 13);
    fl_font(FL_COURIER, 9);
    fl_color(fl_rgb_color(180, 215, 180));
    fl_draw(buf, latX + 2, latY + 1, 58, 11, FL_ALIGN_CENTER);
}

void FootpadPanel::exprSliderCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<FootpadPanel*>(data);
    auto* sl   = static_cast<Fl_Slider*>(w);
    if (self->_exprCb)
        self->_exprCb(static_cast<float>(sl->value()));
}

void FootpadPanel::footBtnCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<FootpadPanel*>(data);
    int id = static_cast<int>(reinterpret_cast<intptr_t>(w->user_data()));
    if (self->_padCb) self->_padCb(id, true);
}

void FootpadPanel::tempoCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<FootpadPanel*>(data);
    auto* ctr  = static_cast<Fl_Counter*>(w);
    if (self->_tempoCb)
        self->_tempoCb(static_cast<float>(ctr->value()));
}

} // namespace em::ui
