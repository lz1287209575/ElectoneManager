#pragma once
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Counter.H>
#include <functional>
#include "common/DrumPattern.h"
#include "common/DrumSequencer.h"

namespace em::ui {

/// Drum step sequencer grid editor — opens as a separate window.
/// Displays a 12-row × 16/32-column grid where each cell can be toggled.
/// Supports:
///   - Click to toggle step on/off
///   - Shift+click to adjust velocity
///   - Playback cursor follows the current beat
///   - Clear / Randomize / Copy / Paste utilities
class StepSequencerWindow : public Fl_Window {
public:
    /// @param seq  Reference to the drum sequencer (for sending edits)
    StepSequencerWindow(em::DrumSequencer& seq);
    ~StepSequencerWindow() override;

    /// Update the playback cursor position (called from timer).
    void setCurrentStep(int step);

    /// Refresh the grid from the sequencer's current pattern.
    void refreshFromPattern();

    void draw() override;
    int  handle(int event) override;

private:
    em::DrumSequencer& _seq;
    em::DrumPattern    _pattern;   ///< Local copy for display
    int                _currentStep = -1;

    // ── Layout constants ─────────────────────────────────────────────────
    static constexpr int kTopBarH    = 40;
    static constexpr int kGridLeft   = 60;   ///< Left margin for instrument labels
    static constexpr int kGridTop    = 48;   ///< Top margin (below top bar)
    static constexpr int kCellSize   = 24;   ///< Cell width & height
    static constexpr int kCellGap    = 2;
    static constexpr int kBottomBarH = 36;

    // ── Top bar widgets ──────────────────────────────────────────────────
    Fl_Choice*  _patternChoice = nullptr;
    Fl_Counter* _stepsCounter  = nullptr;

    // ── Bottom bar widgets ───────────────────────────────────────────────
    Fl_Button*  _clearBtn     = nullptr;
    Fl_Button*  _randomBtn    = nullptr;

    // ── Callbacks ────────────────────────────────────────────────────────
    static void patternChoiceCb(Fl_Widget* w, void* data);
    static void stepsCounterCb(Fl_Widget* w, void* data);
    static void clearBtnCb    (Fl_Widget* w, void* data);
    static void randomBtnCb   (Fl_Widget* w, void* data);

    // ── Grid helpers ─────────────────────────────────────────────────────
    void drawGrid();
    void drawCell(int inst, int step, int cx, int cy, int cw, int ch);
    void drawCursor();
    void drawInstrumentLabels();

    /// Hit-test: returns (instrument, step) or (-1,-1) if outside grid.
    std::pair<int, int> cellAtPos(int px, int py) const;

    /// Compute grid area
    int gridRight()  const { return kGridLeft + _pattern.steps * (kCellSize + kCellGap); }
    int gridBottom() const { return kGridTop + em::kNumDrumInstruments * (kCellSize + kCellGap); }

    /// Calculate required window size
    static int calcWidth(int steps)  { return kGridLeft + steps * (kCellSize + kCellGap) + 20; }
    static int calcHeight()          { return kGridTop + em::kNumDrumInstruments * (kCellSize + kCellGap) + kBottomBarH + 10; }
};

} // namespace em::ui
