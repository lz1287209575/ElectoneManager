#include "StepSequencerWindow.h"
#include "common/DrumPatternBank.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>

namespace em::ui {

// ── Colors ──────────────────────────────────────────────────────────────────
static const Fl_Color kSeqBg       = fl_rgb_color(22, 26, 36);
static const Fl_Color kSeqGrid     = fl_rgb_color(32, 36, 48);
static const Fl_Color kSeqCellOff  = fl_rgb_color(38, 42, 55);
static const Fl_Color kSeqCellOn   = fl_rgb_color(60, 180, 100);
static const Fl_Color kSeqCellHi   = fl_rgb_color(80, 220, 130);
static const Fl_Color kSeqCursor   = fl_rgb_color(255, 200, 40);
static const Fl_Color kSeqBeatLine = fl_rgb_color(55, 60, 75);
static const Fl_Color kSeqLabel    = fl_rgb_color(160, 170, 190);
static const Fl_Color kSeqTopBar   = fl_rgb_color(28, 32, 48);
static const Fl_Color kSeqBtnBg    = fl_rgb_color(45, 50, 68);
static const Fl_Color kSeqBtnText  = fl_rgb_color(180, 190, 210);

StepSequencerWindow::StepSequencerWindow(em::DrumSequencer& seq)
    : Fl_Window(calcWidth(16), calcHeight(),
                "Step Sequencer — Drum Pattern Editor")
    , _seq(seq)
{
    color(kSeqBg);

    // Copy pattern from sequencer
    _pattern = _seq.currentPattern();

    // ── Top bar ──────────────────────────────────────────────────────────
    int bx = 10;

    // Pattern preset choice
    _patternChoice = new Fl_Choice(bx + 60, 8, 130, 26, "Pattern:");
    _patternChoice->labelcolor(kSeqLabel);
    _patternChoice->labelsize(10);
    _patternChoice->textsize(11);
    _patternChoice->color(kSeqBtnBg);
    _patternChoice->textcolor(kSeqBtnText);
    _patternChoice->add("March");
    _patternChoice->add("Waltz");
    _patternChoice->add("Swing");
    _patternChoice->add("Pop");
    _patternChoice->add("R&&B");  // & must be escaped in FLTK
    _patternChoice->add("Latin");
    _patternChoice->add("Ethnic");
    _patternChoice->value(3); // Pop
    _patternChoice->callback(patternChoiceCb, this);

    // Steps counter
    _stepsCounter = new Fl_Counter(bx + 260, 8, 90, 26, "Steps:");
    _stepsCounter->type(FL_SIMPLE_COUNTER);
    _stepsCounter->range(8.0, 32.0);
    _stepsCounter->step(1.0);
    _stepsCounter->lstep(8.0);
    _stepsCounter->value(_pattern.steps);
    _stepsCounter->color(kSeqBtnBg);
    _stepsCounter->textcolor(kSeqBtnText);
    _stepsCounter->textsize(12);
    _stepsCounter->labelcolor(kSeqLabel);
    _stepsCounter->labelsize(10);
    _stepsCounter->callback(stepsCounterCb, this);

    // ── Bottom bar ───────────────────────────────────────────────────────
    int bbY = calcHeight() - kBottomBarH - 2;

    _clearBtn = new Fl_Button(10, bbY, 80, 28, "Clear");
    _clearBtn->box(FL_FLAT_BOX);
    _clearBtn->color(fl_rgb_color(85, 30, 28));
    _clearBtn->labelcolor(fl_rgb_color(220, 180, 170));
    _clearBtn->labelsize(11);
    _clearBtn->labelfont(FL_HELVETICA_BOLD);
    _clearBtn->callback(clearBtnCb, this);

    _randomBtn = new Fl_Button(100, bbY, 80, 28, "Random");
    _randomBtn->box(FL_FLAT_BOX);
    _randomBtn->color(fl_rgb_color(30, 55, 85));
    _randomBtn->labelcolor(fl_rgb_color(170, 200, 230));
    _randomBtn->labelsize(11);
    _randomBtn->labelfont(FL_HELVETICA_BOLD);
    _randomBtn->callback(randomBtnCb, this);

    end();
}

StepSequencerWindow::~StepSequencerWindow() = default;

void StepSequencerWindow::setCurrentStep(int step) {
    if (_currentStep != step) {
        _currentStep = step;
        redraw();
    }
}

void StepSequencerWindow::refreshFromPattern() {
    _pattern = _seq.currentPattern();
    redraw();
}

// ── Callbacks ──────────────────────────────────────────────────────────────
void StepSequencerWindow::patternChoiceCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<StepSequencerWindow*>(data);
    auto* ch   = static_cast<Fl_Choice*>(w);
    int idx = ch->value();
    auto pat = em::DrumPatternBank::getPreset(idx);
    self->_seq.setPattern(pat);
    self->_pattern = pat;
    if (self->_stepsCounter)
        self->_stepsCounter->value(pat.steps);
    // Resize window for new step count
    self->size(calcWidth(pat.steps), calcHeight());
    self->redraw();
}

void StepSequencerWindow::stepsCounterCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<StepSequencerWindow*>(data);
    auto* ctr  = static_cast<Fl_Counter*>(w);
    int newSteps = static_cast<int>(ctr->value());
    if (newSteps < 8)  newSteps = 8;
    if (newSteps > 32) newSteps = 32;
    self->_pattern.steps = newSteps;
    self->_seq.setPattern(self->_pattern);
    self->size(calcWidth(newSteps), calcHeight());
    self->redraw();
}

void StepSequencerWindow::clearBtnCb(Fl_Widget*, void* data) {
    auto* self = static_cast<StepSequencerWindow*>(data);
    self->_pattern.clear();
    self->_seq.setPattern(self->_pattern);
    self->redraw();
}

void StepSequencerWindow::randomBtnCb(Fl_Widget*, void* data) {
    auto* self = static_cast<StepSequencerWindow*>(data);
    self->_pattern.clear();

    // Seed from time
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Generate a simple random pattern
    for (int step = 0; step < self->_pattern.steps; ++step) {
        // Always have some kick/snare structure
        if (step % (self->_pattern.steps / self->_pattern.beatsPerBar) == 0) {
            self->_pattern.set(0, step, 100 + std::rand() % 28); // BD on downbeats
        }
        if ((step * self->_pattern.beatsPerBar / self->_pattern.steps) % 2 == 1 &&
            step % (self->_pattern.steps / self->_pattern.beatsPerBar) == 0) {
            self->_pattern.set(1, step, 90 + std::rand() % 38);  // SD on backbeats
        }
        // Random hi-hat
        if (std::rand() % 3 > 0) {
            self->_pattern.set(3, step, 60 + std::rand() % 40);
        }
        // Sparse random other instruments
        for (int inst = 4; inst < em::kNumDrumInstruments; ++inst) {
            if (std::rand() % 8 == 0) {
                self->_pattern.set(inst, step, 50 + std::rand() % 60);
            }
        }
    }

    self->_seq.setPattern(self->_pattern);
    self->redraw();
}

// ── Draw ──────────────────────────────────────────────────────────────────
void StepSequencerWindow::draw() {
    // Background
    fl_color(kSeqBg);
    fl_rectf(x(), y(), w(), h());

    // Top bar background
    fl_color(kSeqTopBar);
    fl_rectf(x(), y(), w(), kTopBarH);
    fl_color(fl_rgb_color(40, 45, 62));
    fl_line(x(), y() + kTopBarH - 1, x() + w() - 1, y() + kTopBarH - 1);

    // Draw children first (buttons, counters)
    draw_children();

    // Then draw grid overlay
    drawInstrumentLabels();
    drawGrid();
    drawCursor();
}

void StepSequencerWindow::drawInstrumentLabels() {
    fl_font(FL_HELVETICA, 10);
    for (int inst = 0; inst < em::kNumDrumInstruments; ++inst) {
        int cy = kGridTop + inst * (kCellSize + kCellGap);

        // Short name
        fl_color(kSeqLabel);
        fl_draw(em::kDrumInstruments[inst].shortName,
                4, cy, kGridLeft - 8, kCellSize,
                FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    }
}

void StepSequencerWindow::drawGrid() {
    // Beat lines (vertical, every stepsPerBeat steps)
    int spb = _pattern.stepsPerBeat;
    if (spb < 1) spb = 4;

    for (int step = 0; step < _pattern.steps; ++step) {
        int cx = kGridLeft + step * (kCellSize + kCellGap);

        // Draw vertical beat separator line
        if (step > 0 && (step % spb) == 0) {
            fl_color(kSeqBeatLine);
            fl_line(cx - 1, kGridTop, cx - 1, gridBottom());
        }

        for (int inst = 0; inst < em::kNumDrumInstruments; ++inst) {
            int cy = kGridTop + inst * (kCellSize + kCellGap);
            drawCell(inst, step, cx, cy, kCellSize, kCellSize);
        }
    }

    // Step numbers along top
    fl_font(FL_HELVETICA, 8);
    fl_color(fl_rgb_color(90, 95, 110));
    for (int step = 0; step < _pattern.steps; ++step) {
        int cx = kGridLeft + step * (kCellSize + kCellGap);
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", step + 1);
        fl_draw(buf, cx, kGridTop - 12, kCellSize, 10, FL_ALIGN_CENTER);
    }
}

void StepSequencerWindow::drawCell(int inst, int step, int cx, int cy, int cw, int ch) {
    uint8_t vel = _pattern.get(inst, step);

    if (vel > 0) {
        // On: color intensity based on velocity
        float bright = 0.4f + 0.6f * (vel / 127.0f);
        Fl_Color col = fl_color_average(kSeqCellOn, kSeqCellHi, bright);
        fl_color(col);
        fl_rectf(cx, cy, cw, ch);

        // Velocity bar at bottom (1-3px)
        int barH = 1 + static_cast<int>(vel / 50);
        fl_color(fl_color_average(col, FL_WHITE, 0.6f));
        fl_rectf(cx + 2, cy + ch - barH - 1, cw - 4, barH);
    } else {
        // Off: dark cell
        fl_color(kSeqCellOff);
        fl_rectf(cx, cy, cw, ch);
    }

    // Cell border
    fl_color(fl_rgb_color(30, 34, 46));
    fl_rect(cx, cy, cw, ch);
}

void StepSequencerWindow::drawCursor() {
    if (_currentStep < 0 || _currentStep >= _pattern.steps) return;

    int cx = kGridLeft + _currentStep * (kCellSize + kCellGap);
    int gridH = em::kNumDrumInstruments * (kCellSize + kCellGap);

    // Highlight column
    fl_color(fl_color_average(kSeqCursor, kSeqBg, 0.20f));
    fl_rectf(cx, kGridTop, kCellSize, gridH);

    // Re-draw the cells in this column on top of the highlight
    for (int inst = 0; inst < em::kNumDrumInstruments; ++inst) {
        int cy = kGridTop + inst * (kCellSize + kCellGap);
        drawCell(inst, _currentStep, cx, cy, kCellSize, kCellSize);
    }

    // Cursor line at bottom
    fl_color(kSeqCursor);
    fl_rectf(cx, kGridTop + gridH, kCellSize, 3);

    // Top triangle marker
    fl_color(kSeqCursor);
    fl_polygon(cx + kCellSize / 2 - 4, kGridTop - 2,
               cx + kCellSize / 2 + 4, kGridTop - 2,
               cx + kCellSize / 2,     kGridTop + 4);
}

// ── Hit test ─────────────────────────────────────────────────────────────
std::pair<int, int> StepSequencerWindow::cellAtPos(int px, int py) const {
    if (px < kGridLeft || py < kGridTop) return {-1, -1};

    int step = (px - kGridLeft) / (kCellSize + kCellGap);
    int inst = (py - kGridTop)  / (kCellSize + kCellGap);

    if (step < 0 || step >= _pattern.steps) return {-1, -1};
    if (inst < 0 || inst >= em::kNumDrumInstruments) return {-1, -1};

    // Verify we're inside the cell and not in the gap
    int cellX = kGridLeft + step * (kCellSize + kCellGap);
    int cellY = kGridTop  + inst  * (kCellSize + kCellGap);
    if (px >= cellX + kCellSize || py >= cellY + kCellSize) return {-1, -1};

    return {inst, step};
}

int StepSequencerWindow::handle(int event) {
    switch (event) {
    case FL_PUSH: {
        auto [inst, step] = cellAtPos(Fl::event_x(), Fl::event_y());
        if (inst >= 0 && step >= 0) {
            if (Fl::event_state() & FL_SHIFT) {
                // Shift+click: cycle velocity (off → 60 → 90 → 127 → off)
                uint8_t vel = _pattern.get(inst, step);
                if (vel == 0) vel = 60;
                else if (vel < 80) vel = 90;
                else if (vel < 120) vel = 127;
                else vel = 0;
                _pattern.set(inst, step, vel);
                _seq.setStep(inst, step, vel);
            } else {
                // Normal click: toggle
                uint8_t newVel = _pattern.toggle(inst, step);
                if (newVel > 0)
                    _seq.setStep(inst, step, newVel);
                else
                    _seq.setStep(inst, step, 0);
            }
            redraw();
            return 1;
        }
        break;
    }
    case FL_DRAG: {
        // Allow dragging to paint cells
        auto [inst, step] = cellAtPos(Fl::event_x(), Fl::event_y());
        if (inst >= 0 && step >= 0) {
            if (_pattern.get(inst, step) == 0) {
                _pattern.set(inst, step, _pattern.defaultVelocity);
                _seq.setStep(inst, step, _pattern.defaultVelocity);
                redraw();
            }
            return 1;
        }
        break;
    }
    case FL_RELEASE:
        return 1;
    default:
        break;
    }
    return Fl_Window::handle(event);
}

} // namespace em::ui
