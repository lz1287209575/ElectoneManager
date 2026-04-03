#include "ScorePanel.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_File_Chooser.H>
#include <cstring>
#include <cstdio>
#include <algorithm>

#ifdef EM_LOMSE_AVAILABLE
#include <lomse_doorway.h>
#include <lomse_document.h>
#include <lomse_graphic_view.h>
#include <lomse_interactor.h>
#include <lomse_presenter.h>
using namespace lomse;
#endif

namespace em::ui {

static const Fl_Color kScoreBg        = fl_rgb_color(245, 240, 228);
static const Fl_Color kScoreBorder    = fl_rgb_color(40,  55, 100);
static const Fl_Color kPlayhead       = fl_rgb_color(220, 60,  60);
static const Fl_Color kPlaceholderBg  = fl_rgb_color(20,  26,  44);
static const Fl_Color kPlaceholderMid = fl_rgb_color(24,  32,  56);
static const Fl_Color kPlaceholderBot = fl_rgb_color(18,  22,  38);
static const Fl_Color kScoreHeaderBg  = fl_rgb_color(32,  42,  72);
static const Fl_Color kScoreHeaderHi  = fl_rgb_color(48,  62, 100);
static const Fl_Color kAccentBlue     = fl_rgb_color(60, 120, 220);
static const Fl_Color kStaffLine      = fl_rgb_color(140, 130, 110);
static const Fl_Color kStaffBg        = fl_rgb_color(250, 246, 236);
static const Fl_Color kTransportBg    = fl_rgb_color(28, 34, 52);
static const Fl_Color kBtnColor       = fl_rgb_color(45, 55, 85);
static const Fl_Color kBtnText        = fl_rgb_color(180, 200, 230);

static constexpr int kTransportH = 28;  // Height of transport bar

// ─────────────────────────────────────────────────────────────────────────────
#ifdef EM_LOMSE_AVAILABLE
struct ScorePanel::LomseImpl {
    LomseDoorway  doorway;
    Presenter*    presenter  = nullptr;
    Interactor*   interactor = nullptr;

    LomseImpl() {
        doorway.init_library(k_pix_format_rgba32, 96, false);
    }
    ~LomseImpl() {
        if (presenter) { delete presenter; presenter = nullptr; }
    }
};
#endif

// ─────────────────────────────────────────────────────────────────────────────
ScorePanel::ScorePanel(int x, int y, int ww, int hh)
    : Fl_Group(x, y, ww, hh) {
    box(FL_FLAT_BOX);
    color(kPlaceholderBg);

    // ── Transport bar at bottom of panel ────────────────────────────────────
    int barY = y + hh - kTransportH;
    int btnW = 32, btnH = 22, btnGap = 4;
    int bx = x + 4;

    _rewBtn = new Fl_Button(bx, barY + 3, btnW, btnH, "@|<");
    _rewBtn->box(FL_FLAT_BOX); _rewBtn->color(kBtnColor);
    _rewBtn->labelcolor(kBtnText); _rewBtn->labelsize(10);
    _rewBtn->callback(rewBtnCb, this);
    bx += btnW + btnGap;

    _playBtn = new Fl_Button(bx, barY + 3, btnW + 8, btnH, "@>");
    _playBtn->box(FL_FLAT_BOX); _playBtn->color(fl_rgb_color(28, 75, 35));
    _playBtn->labelcolor(fl_rgb_color(80, 230, 100)); _playBtn->labelsize(12);
    _playBtn->callback(playBtnCb, this);
    bx += btnW + 8 + btnGap;

    _stopBtn = new Fl_Button(bx, barY + 3, btnW, btnH, "@square");
    _stopBtn->box(FL_FLAT_BOX); _stopBtn->color(fl_rgb_color(85, 30, 28));
    _stopBtn->labelcolor(fl_rgb_color(230, 80, 80)); _stopBtn->labelsize(10);
    _stopBtn->callback(stopBtnCb, this);
    bx += btnW + btnGap;

    _ffBtn = new Fl_Button(bx, barY + 3, btnW, btnH, "@>|");
    _ffBtn->box(FL_FLAT_BOX); _ffBtn->color(kBtnColor);
    _ffBtn->labelcolor(kBtnText); _ffBtn->labelsize(10);
    _ffBtn->callback(ffBtnCb, this);
    bx += btnW + btnGap + 8;

    // Progress slider
    int sliderW = ww - bx - 8 + x;
    _progressSlider = new Fl_Slider(bx, barY + 5, sliderW, btnH - 4);
    _progressSlider->type(FL_HOR_NICE_SLIDER);
    _progressSlider->range(0.0, 1.0);
    _progressSlider->value(0.0);
    _progressSlider->color(fl_rgb_color(22, 28, 42));
    _progressSlider->selection_color(kAccentBlue);
    _progressSlider->callback(progressCb, this);

    end();

#ifdef EM_LOMSE_AVAILABLE
    _lomse = new LomseImpl();
#endif
}

ScorePanel::~ScorePanel() {
#ifdef EM_LOMSE_AVAILABLE
    delete _lomse;
#endif
}

bool ScorePanel::loadMusicXML(const std::string& path) {
    _currentFile  = path;
    _scoreLoaded  = false;
    _playbackBeat = 0.0;
    _progressFrac = 0.0f;

    // Check if it's a MIDI file
    _isMidiFile = false;
    if (path.size() >= 4) {
        std::string ext = path.substr(path.size() - 4);
        if (ext == ".mid" || ext == ".MID" || ext == ".Mid") {
            _isMidiFile = true;
            _scoreLoaded = true;
            redraw();
            return true;
        }
    }

#ifdef EM_LOMSE_AVAILABLE
    if (!_lomse) return false;
    if (_lomse->presenter) { delete _lomse->presenter; _lomse->presenter = nullptr; }

    _lomse->presenter = _lomse->doorway.open_document(
        k_view_horizontal_book,
        path.c_str());
    if (!_lomse->presenter) return false;

    _lomse->interactor = _lomse->presenter->get_interactor(0).lock().get();
    _scoreLoaded = (_lomse->interactor != nullptr);
    if (_scoreLoaded) renderToBuffer();
#else
    _scoreLoaded = true;
#endif
    redraw();
    return _scoreLoaded;
}

void ScorePanel::scrollToMeasure(int measure) {
    _scrollMeasure = measure;
    redraw();
}

void ScorePanel::setPlaybackPosition(double beat) {
    _playbackBeat = beat;
    redraw();
}

void ScorePanel::setProgress(float fraction) {
    _progressFrac = std::max(0.0f, std::min(1.0f, fraction));
    if (_progressSlider) {
        _progressSlider->value(static_cast<double>(_progressFrac));
        _progressSlider->redraw();
    }
}

void ScorePanel::openFileDialog() {
    const char* file = fl_file_chooser("Open Score / MIDI File",
                                        "Score Files (*.xml,*.mxl,*.mid)", "");
    if (file && *file) loadMusicXML(file);
}

// ─────────────────────────────────────────────────────────────────────────────
#ifdef EM_LOMSE_AVAILABLE
void ScorePanel::renderToBuffer() {
    if (!_lomse || !_lomse->interactor) return;
    _bitmapW = w();
    _bitmapH = h() - kTransportH;
    _bitmap.assign(_bitmapW * _bitmapH * 4, 255);
    RenderingBuffer rbuf(_bitmap.data(), _bitmapW, _bitmapH, _bitmapW * 4);
    _lomse->interactor->set_rendering_buffer(&rbuf);
    _lomse->interactor->force_redraw();
}
#endif

// ── Button callbacks ────────────────────────────────────────────────────────
void ScorePanel::playBtnCb(Fl_Widget*, void* data) {
    auto* self = static_cast<ScorePanel*>(data);
    if (self->_playCb) self->_playCb();
}

void ScorePanel::stopBtnCb(Fl_Widget*, void* data) {
    auto* self = static_cast<ScorePanel*>(data);
    if (self->_stopCb) self->_stopCb();
}

void ScorePanel::rewBtnCb(Fl_Widget*, void* data) {
    auto* self = static_cast<ScorePanel*>(data);
    if (self->_seekCb) self->_seekCb(0.0f);  // Rewind to start
}

void ScorePanel::ffBtnCb(Fl_Widget*, void* data) {
    auto* self = static_cast<ScorePanel*>(data);
    if (self->_seekCb) self->_seekCb(1.0f);  // Jump to end
}

void ScorePanel::progressCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<ScorePanel*>(data);
    auto* sl   = static_cast<Fl_Slider*>(w);
    if (self->_seekCb) self->_seekCb(static_cast<float>(sl->value()));
}

// ─────────────────────────────────────────────────────────────────────────────
void ScorePanel::draw() {
    // Outer bezel
    fl_color(fl_rgb_color(55, 70, 115));
    fl_line(x(), y(), x() + w() - 1, y());
    fl_line(x(), y(), x(), y() + h() - 1);
    fl_color(fl_rgb_color(18, 24, 50));
    fl_line(x() + w() - 1, y(),          x() + w() - 1, y() + h() - 1);
    fl_line(x(),            y() + h() - 1, x() + w() - 1, y() + h() - 1);

    // Inner frame
    fl_color(kScoreBorder);
    fl_rect(x() + 1, y() + 1, w() - 2, h() - 2);

    // Transport bar background
    int barY = y() + h() - kTransportH;
    fl_color(kTransportBg);
    fl_rectf(x() + 2, barY, w() - 4, kTransportH - 2);
    // Separator line
    fl_color(fl_rgb_color(45, 58, 95));
    fl_line(x() + 2, barY, x() + w() - 3, barY);

    // Draw children (buttons, slider)
    draw_children();

    if (!_scoreLoaded) {
        drawPlaceholder();
        return;
    }
    drawScore();
    drawPlayhead();
}

void ScorePanel::drawTransportBar() {
    // Already drawn in draw() above, buttons are child widgets
}

void ScorePanel::drawPlaceholder() {
    int ix = x() + 2, iy = y() + 2;
    int iw = w() - 4, ih = h() - 4 - kTransportH;

    // Three-band gradient
    int band = ih / 3;
    fl_color(kPlaceholderBg);
    fl_rectf(ix, iy, iw, band);
    fl_color(kPlaceholderMid);
    fl_rectf(ix, iy + band, iw, band);
    fl_color(kPlaceholderBot);
    fl_rectf(ix, iy + band * 2, iw, ih - band * 2);

    // Header strip
    const int hdrH = 20;
    fl_color(kScoreHeaderHi);
    fl_rectf(ix, iy, iw, 1);
    fl_color(kScoreHeaderBg);
    fl_rectf(ix, iy + 1, iw, hdrH - 1);

    // Accent square
    fl_color(kAccentBlue);
    fl_rectf(ix + 6, iy + (hdrH - 10) / 2, 10, 10);
    fl_color(fl_rgb_color(30, 80, 170));
    fl_rect(ix + 6, iy + (hdrH - 10) / 2, 10, 10);

    fl_font(FL_HELVETICA_BOLD, 10);
    fl_color(fl_rgb_color(160, 190, 230));
    fl_draw("SCORE VIEWER", ix + 22, iy, iw - 26, hdrH,
            FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    // Separator
    fl_color(fl_rgb_color(35, 50, 90));
    fl_line(ix, iy + hdrH, ix + iw - 1, iy + hdrH);

    // Music note icon
    int noteX = ix + iw / 2;
    int noteY = iy + hdrH + (ih - hdrH) / 2 - 8;
    fl_color(fl_rgb_color(30, 50, 100));
    fl_pie(noteX - 20, noteY - 20, 40, 40, 0, 360);
    fl_color(fl_rgb_color(45, 75, 140));
    fl_pie(noteX - 15, noteY - 15, 30, 30, 0, 360);

    fl_font(FL_HELVETICA, 28);
    fl_color(fl_rgb_color(90, 130, 200));
    fl_draw("\xe2\x99\xaa", ix, iy + hdrH + 2, iw, ih - hdrH - 24,
            FL_ALIGN_CENTER);

    // Hint
    fl_font(FL_HELVETICA, 11);
    fl_color(fl_rgb_color(100, 140, 200));
    fl_draw("\xe7\x82\xb9\xe5\x87\xbb\xe6\xad\xa4\xe5\xa4\x84\xe5\x8a\xa0\xe8\xbd\xbd\xe4\xb9\x90\xe8\xb0\xb1 (MusicXML / MIDI)",
            ix, iy + ih - 22, iw, 14, FL_ALIGN_CENTER);

    if (!_currentFile.empty()) {
        fl_color(fl_rgb_color(50, 70, 110));
        fl_rectf(ix, iy + ih - 16, iw, 14);
        fl_font(FL_HELVETICA, 9);
        fl_color(fl_rgb_color(80, 110, 160));
        fl_draw(_currentFile.c_str(),
                ix + 6, iy + ih - 15, iw - 12, 13,
                FL_ALIGN_LEFT | FL_ALIGN_CLIP);
    }
}

void ScorePanel::drawScore() {
#ifdef EM_LOMSE_AVAILABLE
    if (!_bitmap.empty() && _bitmapW > 0 && _bitmapH > 0) {
        fl_draw_image(_bitmap.data(), x() + 2, y() + 2,
                      _bitmapW, _bitmapH, 4, _bitmapW * 4);
        return;
    }
#endif
    int ix = x() + 2, iy = y() + 2;
    int iw = w() - 4, ih = h() - 4 - kTransportH;

    fl_color(kStaffBg);
    fl_rectf(ix, iy, iw, ih);

    // Header
    const int hdrH = 18;
    fl_color(kScoreHeaderBg);
    fl_rectf(ix, iy, iw, hdrH);
    fl_color(kAccentBlue);
    fl_rectf(ix + 4, iy + (hdrH - 8) / 2, 8, 8);
    fl_font(FL_HELVETICA_BOLD, 9);
    fl_color(fl_rgb_color(160, 190, 230));

    const char* typeLabel = _isMidiFile ? "MIDI FILE" : "SCORE VIEWER";
    fl_draw(typeLabel, ix + 18, iy, iw / 3, hdrH,
            FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    if (!_currentFile.empty()) {
        fl_font(FL_HELVETICA, 9);
        fl_color(fl_rgb_color(120, 155, 200));
        fl_draw(_currentFile.c_str(),
                ix + iw / 3, iy, iw - iw / 3 - 4, hdrH,
                FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    }

    fl_color(fl_rgb_color(18, 24, 50));
    fl_line(ix, iy + hdrH, ix + iw - 1, iy + hdrH);

    // Staff area
    int staffAreaY = iy + hdrH + 6;
    int staffAreaH = ih - hdrH - 8;

    int nBars = 8;
    int barSpacing = iw / nBars;
    fl_color(fl_rgb_color(175, 165, 145));
    for (int b = 0; b <= nBars; ++b) {
        int bx2 = ix + b * barSpacing;
        fl_line(bx2, staffAreaY, bx2, staffAreaY + staffAreaH);
    }

    auto drawStaff = [&](int sy) {
        fl_color(kStaffLine);
        fl_line_style(FL_SOLID, 1);
        for (int line = 0; line < 5; ++line) {
            int ly = sy + line * 7;
            fl_line(ix + 2, ly, ix + iw - 2, ly);
        }
        fl_line_style(FL_SOLID, 0);
        fl_color(fl_rgb_color(190, 185, 165));
        fl_rectf(ix + 2, sy - 2, 12, 5 * 7 + 4);
    };
    int staffGap  = staffAreaH / 3;
    drawStaff(staffAreaY + staffGap / 4);
    drawStaff(staffAreaY + staffGap / 4 + staffGap);
}

void ScorePanel::drawPlayhead() {
    if (_playbackBeat <= 0.0 && _progressFrac <= 0.0f) return;
    float progress = _progressFrac > 0.0f ? _progressFrac
                   : std::min(1.0f, std::max(0.0f, static_cast<float>(_playbackBeat / 32.0)));
    int scoreH = h() - kTransportH;
    int lineX = x() + 20 + static_cast<int>(progress * (w() - 40));

    fl_color(fl_color_average(kPlayhead, fl_rgb_color(20, 26, 44), 0.35f));
    fl_line_style(FL_SOLID, 4);
    fl_line(lineX, y() + 2, lineX, y() + scoreH - 2);

    fl_color(kPlayhead);
    fl_line_style(FL_SOLID, 2);
    fl_line(lineX, y() + 2, lineX, y() + scoreH - 2);
    fl_line_style(FL_SOLID, 0);

    fl_color(kPlayhead);
    fl_polygon(lineX - 5, y() + 2,
               lineX + 5, y() + 2,
               lineX,     y() + 10);
}

int ScorePanel::handle(int event) {
    // Let child widgets handle events first
    int result = Fl_Group::handle(event);
    if (result) return result;

    if (event == FL_PUSH) {
        if (!_scoreLoaded || _currentFile.empty()) {
            openFileDialog();
        }
        return 1;
    }
    return 0;
}

} // namespace em::ui
