#include "LcdPanel.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace em::ui {

// ── Palette (dark bezel + LCD screen colours) ────────────────────────────────
static const Fl_Color kScreenBg   = fl_rgb_color(210, 212, 205); // warm grey LCD bg
static const Fl_Color kBezelBg    = fl_rgb_color( 52,  54,  56); // outer bezel
static const Fl_Color kHeaderBg   = fl_rgb_color(172, 174, 168); // "VOICE DISPLAY" bar
static const Fl_Color kInfoBg     = fl_rgb_color(185, 187, 180); // middle info row
static const Fl_Color kBottomBg   = fl_rgb_color(200, 202, 195); // bottom row
static const Fl_Color kDark       = fl_rgb_color( 30,  30,  30);
static const Fl_Color kMidGray    = fl_rgb_color(110, 112, 108);
static const Fl_Color kLabelGray  = fl_rgb_color( 80,  82,  78);

// Page 2/3 dark theme colours
static const Fl_Color kPageDarkBg   = fl_rgb_color( 24,  28,  38);
static const Fl_Color kPageDarkMid  = fl_rgb_color( 32,  38,  52);
static const Fl_Color kAccentBlue   = fl_rgb_color( 60, 120, 220);
static const Fl_Color kAccentGreen  = fl_rgb_color( 50, 200,  80);
static const Fl_Color kAccentYellow = fl_rgb_color(255, 200,  40);
static const Fl_Color kDimText      = fl_rgb_color(120, 130, 150);

// Voice-card accent colours — matching the photograph exactly
static Fl_Color cardBg(int slot) {
    switch (slot) {
    case 0: return fl_rgb_color(180,  65,  40);  // Upper V1  deep red
    case 1: return fl_rgb_color(142, 148, 138);  // Upper V2  warm grey
    case 2: return fl_rgb_color( 52, 112,  52);  // Lead  V1  green
    case 3: return fl_rgb_color( 52, 112,  52);  // Lead  V2  green
    case 4: return fl_rgb_color(175,  62,  38);  // Lower V1  deep red
    case 5: return fl_rgb_color(138, 144, 134);  // Lower V2  warm grey
    case 6: return fl_rgb_color( 48, 108,  48);  // Pedal V1  green
    case 7: return fl_rgb_color( 48, 108,  48);  // Pedal V2  green
    default: return fl_rgb_color(100, 100, 100);
    }
}

// The 8 role labels in the grid
static const char* kRoleLabel[8] = {
    "UPPER KEYBOARD\nVOICE 1",
    "UPPER KEYBOARD\nVOICE 2",
    "LEAD VOICE 1",
    "LEAD VOICE 2",
    "LOWER KEYBOARD\nVOICE 1",
    "LOWER KEYBOARD\nVOICE 2",
    "PEDAL VOICE 1",
    "PEDAL VOICE 2",
};

// Tab labels for pages 1-3
static const char* kTabLabels[3] = {
    "\xe9\x9f\xb3\xe8\x89\xb2",   // 音色
    "\xe4\xb9\x90\xe8\xb0\xb1",   // 乐谱
    "\xe8\x8a\x82\xe5\xa5\x8f",   // 节奏
};

// Tab geometry constants (must match drawHeader)
static constexpr int kTabW = 32, kTabH = 16;

// Drum instrument labels for rhythm page (12 rows)
static const char* kDrumLabels[12] = {
    "BD", "SD", "CH", "OH", "LT", "MT",
    "HT", "CR", "RD", "CB", "CL", "RS",
};

// ─────────────────────────────────────────────────────────────────────────────
LcdPanel::LcdPanel(int x, int y, int w, int h)
    : Fl_Widget(x, y, w, h) {
    box(FL_FLAT_BOX);
    // Initialise with sensible defaults matching the photo
    _state.voices[0] = { kRoleLabel[0], "Strings 1",   true  };
    _state.voices[1] = { kRoleLabel[1], "Brass 1",     false };
    _state.voices[2] = { kRoleLabel[2], "Violin 1",    false };
    _state.voices[3] = { kRoleLabel[3], "vFlute 1",    false };
    _state.voices[4] = { kRoleLabel[4], "Piano 1",     false };
    _state.voices[5] = { kRoleLabel[5], "Strings 1",   false };
    _state.voices[6] = { kRoleLabel[6], "FingrBass1",  false };
    _state.voices[7] = { kRoleLabel[7], "Contbass 1",  false };
}

// ── draw() ────────────────────────────────────────────────────────────────────
void LcdPanel::draw() {
    // Outer bezel
    fl_color(kBezelBg);
    fl_rectf(x(), y(), w(), h());

    // Bezel bevel: lighter top-left, darker bottom-right edges
    fl_color(fl_rgb_color(72, 74, 70));
    fl_line(x(), y(), x() + w() - 1, y());
    fl_line(x(), y(), x(), y() + h() - 1);
    fl_color(fl_rgb_color(30, 31, 29));
    fl_line(x() + w() - 1, y(),          x() + w() - 1, y() + h() - 1);
    fl_line(x(),            y() + h() - 1, x() + w() - 1, y() + h() - 1);

    // Screen inset (6 px bezel each side)
    const int bz  = 6;
    _sx  = x() + bz;
    _sy  = y() + bz;
    _sw  = w() - bz * 2;
    _sh  = h() - bz * 2;

    // Screen background
    fl_color(kScreenBg);
    fl_rectf(_sx, _sy, _sw, _sh);

    // Screen inner bevel
    fl_color(fl_rgb_color(228, 230, 222));
    fl_line(_sx, _sy, _sx + _sw - 1, _sy);
    fl_line(_sx, _sy, _sx,          _sy + _sh - 1);
    fl_color(fl_rgb_color(180, 182, 175));
    fl_line(_sx + _sw - 1, _sy,          _sx + _sw - 1, _sy + _sh - 1);
    fl_line(_sx,          _sy + _sh - 1, _sx + _sw - 1, _sy + _sh - 1);

    // Row heights (absolute)
    _kHdr = 22;
    const int kInfo = 28;
    const int kBot  = 44;
    _gridH = _sh - _kHdr - kInfo - kBot;

    drawHeader(_sx, _sy, _sw, _kHdr);

    // ── Page content based on activePage ──────────────────────────────────
    switch (_state.activePage) {
    case 2:
        drawScorePage(_sx, _sy + _kHdr, _sw, _gridH);
        break;
    case 3:
        drawRhythmPage(_sx, _sy + _kHdr, _sw, _gridH);
        break;
    default: // page 1
        drawVoiceGrid(_sx, _sy + _kHdr, _sw, _gridH);
        break;
    }

    drawInfoRow  (_sx, _sy + _kHdr + _gridH, _sw, kInfo);
    drawBottomRow(_sx, _sy + _kHdr + _gridH + kInfo, _sw, kBot);
}

// ── Header: title + page tabs ────────────────────────────────────────────────
void LcdPanel::drawHeader(int hx, int hy, int hw, int hh) {
    // Two-tone header
    fl_color(fl_rgb_color(158, 160, 154));
    fl_rectf(hx, hy, hw, hh / 2);
    fl_color(kHeaderBg);
    fl_rectf(hx, hy + hh / 2, hw, hh - hh / 2);

    // Decorative accent square
    fl_color(fl_rgb_color(220, 130, 30));
    fl_rectf(hx + 6, hy + (hh - 10) / 2, 10, 10);
    fl_color(fl_rgb_color(180, 100, 20));
    fl_rect(hx + 6, hy + (hh - 10) / 2, 10, 10);

    // Title changes with page
    const char* titles[] = { "VOICE DISPLAY", "SCORE / MIDI", "RHYTHM PATTERN" };
    int titleIdx = std::max(0, std::min(2, _state.activePage - 1));
    fl_font(FL_HELVETICA_BOLD, 10);
    fl_color(kDark);
    fl_draw(titles[titleIdx], hx + 22, hy, hw - 22, hh, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    // Page tabs (right side): wider to fit Chinese labels
    const int tabY = hy + (hh - kTabH) / 2;
    int tx = hx + hw - 3 * (kTabW + 3) - 4;
    for (int i = 1; i <= 3; ++i) {
        bool active = (i == _state.activePage);
        fl_color(active ? fl_rgb_color(230, 130, 30) : fl_rgb_color(155, 158, 152));
        fl_rectf(tx, tabY, kTabW, kTabH);
        fl_font(FL_HELVETICA_BOLD, 9);
        fl_color(active ? FL_WHITE : kDark);
        fl_draw(kTabLabels[i - 1], tx, tabY, kTabW, kTabH, FL_ALIGN_CENTER);
        // subtle border
        fl_color(fl_rgb_color(80, 82, 78));
        fl_rect(tx, tabY, kTabW, kTabH);
        tx += kTabW + 3;
    }

    // Separator line
    fl_color(kMidGray);
    fl_line(hx, hy + hh - 1, hx + hw - 1, hy + hh - 1);
}

// ── Voice grid: 4 columns x 2 rows ──────────────────────────────────────────
void LcdPanel::drawVoiceGrid(int gx, int gy, int gw, int gh) {
    const int cols = 4, rows = 2;
    const int gap  = 2;
    int cardW = (gw - gap * (cols + 1)) / cols;
    int cardH = (gh - gap * (rows + 1)) / rows;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            int slot = row * cols + col;
            int cx = gx + gap + col * (cardW + gap);
            int cy = gy + gap + row * (cardH + gap);
            drawVoiceCard(cx, cy, cardW, cardH, slot);
        }
    }
}

// ── Single voice card ────────────────────────────────────────────────────────
void LcdPanel::drawVoiceCard(int cx, int cy, int cw, int ch, int slot) {
    const VoiceSlot& vs    = _state.voices[slot];
    Fl_Color         bg    = cardBg(slot);
    bool             isGrey = (slot == 1 || slot == 5);
    bool             disabled = !vs.active;

    Fl_Color fill = vs.active ? fl_color_average(bg, FL_WHITE, 0.85f) : bg;

    Fl_Color fillHi = fl_color_average(fill, FL_WHITE, 0.15f);
    fl_color(fillHi);
    fl_rectf(cx, cy, cw, 8);
    fl_color(fill);
    fl_rectf(cx, cy + 8, cw, ch - 8);

    fl_color(fl_color_average(fill, kDark, 0.80f));
    fl_rectf(cx, cy + ch - 4, cw, 4);

    fl_color(isGrey ? kLabelGray : fl_color_average(bg, FL_WHITE, 0.52f));
    fl_font(FL_HELVETICA, 12);

    const char* lbl = vs.label.c_str();
    const char* nl  = strchr(lbl, '\n');
    if (nl) {
        int len1 = static_cast<int>(nl - lbl);
        char line1[40] = {};
        if (len1 > 38) len1 = 38;
        strncpy(line1, lbl, len1);
        fl_draw(line1,  cx + 5, cy + 9,  cw - 8, 15, FL_ALIGN_LEFT | FL_ALIGN_CLIP);
        fl_draw(nl + 1, cx + 5, cy + 24, cw - 8, 15, FL_ALIGN_LEFT | FL_ALIGN_CLIP);
    } else {
        fl_draw(lbl, cx + 5, cy + 9, cw - 8, 15, FL_ALIGN_LEFT | FL_ALIGN_CLIP);
    }

    fl_font(FL_HELVETICA_BOLD, 19);
    fl_color(isGrey ? kDark : FL_WHITE);
    fl_draw(vs.voiceName.c_str(),
            cx + 5, cy + ch / 2,
            cw - 10, ch / 2 - 4,
            FL_ALIGN_LEFT | FL_ALIGN_BOTTOM | FL_ALIGN_CLIP);

    if (vs.active) {
        fl_color(fl_color_average(bg, FL_WHITE, 0.70f));
        fl_rectf(cx, cy, 4, ch);
    }

    if (disabled) {
        fl_color(fl_color_average(fl_rgb_color(20, 20, 20), bg, 0.55f));
        fl_rectf(cx + 1, cy + 1, cw - 2, ch - 2);
        fl_color(fl_color_average(bg, FL_BLACK, 0.75f));
        fl_font(FL_HELVETICA, 12);
        if (nl) {
            int len1 = static_cast<int>(nl - lbl);
            char line1[40] = {};
            if (len1 > 38) len1 = 38;
            strncpy(line1, lbl, len1);
            fl_draw(line1,  cx + 5, cy + 9,  cw - 8, 15, FL_ALIGN_LEFT | FL_ALIGN_CLIP);
            fl_draw(nl + 1, cx + 5, cy + 24, cw - 8, 15, FL_ALIGN_LEFT | FL_ALIGN_CLIP);
        } else {
            fl_draw(lbl, cx + 5, cy + 9, cw - 8, 15, FL_ALIGN_LEFT | FL_ALIGN_CLIP);
        }
        fl_font(FL_HELVETICA_BOLD, 19);
        fl_color(fl_color_average(fl_rgb_color(90, 90, 90), bg, 0.60f));
        fl_draw(vs.voiceName.c_str(),
                cx + 5, cy + ch / 2,
                cw - 10, ch / 2 - 4,
                FL_ALIGN_LEFT | FL_ALIGN_BOTTOM | FL_ALIGN_CLIP);
        fl_color(fl_color_average(fl_rgb_color(40, 40, 40), bg, 0.80f));
        int badgeW = cw - 16, badgeH = 14;
        int badgeX = cx + 8, badgeY = cy + ch / 2 - badgeH / 2 - 4;
        fl_rectf(badgeX, badgeY, badgeW, badgeH);
        fl_font(FL_HELVETICA_BOLD, 9);
        fl_color(fl_rgb_color(160, 162, 155));
        fl_draw("DISABLED", badgeX, badgeY, badgeW, badgeH, FL_ALIGN_CENTER | FL_ALIGN_CLIP);
    }

    fl_color(fl_color_average(bg, kDark, 0.6f));
    fl_rect(cx, cy, cw, ch);
}

// ── Page 2: Score / MIDI playback UI ─────────────────────────────────────────
void LcdPanel::drawScorePage(int gx, int gy, int gw, int gh) {
    // Dark background
    fl_color(kPageDarkBg);
    fl_rectf(gx, gy, gw, gh);

    const int pad = 12;
    const int innerW = gw - pad * 2;

    if (!_state.midiFileLoaded) {
        // ── No file loaded: show placeholder ──────────────────────────────
        // Music note icon
        int iconCx = gx + gw / 2;
        int iconCy = gy + gh / 2 - 20;
        fl_color(fl_rgb_color(30, 50, 100));
        fl_pie(iconCx - 25, iconCy - 25, 50, 50, 0, 360);
        fl_color(fl_rgb_color(45, 75, 140));
        fl_pie(iconCx - 18, iconCy - 18, 36, 36, 0, 360);
        fl_font(FL_HELVETICA, 24);
        fl_color(fl_rgb_color(90, 130, 200));
        fl_draw("\xe2\x99\xaa", gx, gy + 10, gw, gh - 50, FL_ALIGN_CENTER);

        fl_font(FL_HELVETICA, 12);
        fl_color(kDimText);
        // 未加载乐谱 — 请通过菜单加载 MIDI 文件
        fl_draw("\xe6\x9c\xaa\xe5\x8a\xa0\xe8\xbd\xbd\xe4\xb9\x90\xe8\xb0\xb1",
                gx, gy + gh - 50, gw, 20, FL_ALIGN_CENTER);
        fl_font(FL_HELVETICA, 10);
        fl_color(fl_rgb_color(80, 95, 120));
        fl_draw("\xe8\xaf\xb7\xe9\x80\x9a\xe8\xbf\x87\xe8\x8f\x9c\xe5\x8d\x95\xe5\x8a\xa0\xe8\xbd\xbd MIDI \xe6\x96\x87\xe4\xbb\xb6",
                gx, gy + gh - 30, gw, 20, FL_ALIGN_CENTER);
        return;
    }

    // ── File loaded: show file name + progress + controls ─────────────────

    // Header: file name
    int hdrY = gy + 8;
    fl_color(kPageDarkMid);
    fl_rectf(gx + pad, hdrY, innerW, 22);
    fl_font(FL_HELVETICA_BOLD, 11);
    fl_color(fl_rgb_color(160, 190, 230));
    fl_draw("MIDI FILE", gx + pad + 6, hdrY, 80, 22, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    fl_font(FL_HELVETICA, 11);
    fl_color(fl_rgb_color(200, 210, 230));
    fl_draw(_state.midiFileName.c_str(),
            gx + pad + 90, hdrY, innerW - 96, 22,
            FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    // Progress bar
    int barY = gy + gh / 2 - 8;
    int barH = 16;
    // Track background
    fl_color(fl_rgb_color(18, 22, 32));
    fl_rectf(gx + pad, barY, innerW, barH);
    fl_color(fl_rgb_color(28, 34, 48));
    fl_rectf(gx + pad + 1, barY + 1, innerW - 2, barH - 2);
    // Filled portion
    int fillW = static_cast<int>(_state.midiProgress * (innerW - 4));
    if (fillW > 0) {
        fl_color(kAccentBlue);
        fl_rectf(gx + pad + 2, barY + 2, fillW, barH - 4);
        // Bright leading edge
        fl_color(fl_rgb_color(100, 170, 255));
        fl_rectf(gx + pad + 2 + fillW - 2, barY + 2, 2, barH - 4);
    }
    // Border
    fl_color(fl_rgb_color(50, 65, 100));
    fl_rect(gx + pad, barY, innerW, barH);

    // Percentage text
    char pctBuf[8];
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", static_cast<int>(_state.midiProgress * 100));
    fl_font(FL_COURIER_BOLD, 12);
    fl_color(fl_rgb_color(180, 200, 240));
    fl_draw(pctBuf, gx + pad, barY + barH + 4, innerW, 16, FL_ALIGN_CENTER);

    // ── Transport buttons: ⏪  ▶/⏸  ⏹ ──────────────────────────────────
    int btnW = 48, btnH = 28, btnGap = 12;
    int totalBtnW = btnW * 3 + btnGap * 2;
    int btnX = gx + (gw - totalBtnW) / 2;
    int btnY = gy + gh - btnH - 16;

    // Rewind button
    fl_color(fl_rgb_color(40, 50, 75));
    fl_rectf(btnX, btnY, btnW, btnH);
    fl_color(fl_rgb_color(55, 70, 100));
    fl_rect(btnX, btnY, btnW, btnH);
    fl_font(FL_HELVETICA_BOLD, 14);
    fl_color(fl_rgb_color(160, 180, 220));
    fl_draw("@|<", btnX, btnY, btnW, btnH, FL_ALIGN_CENTER);

    // Play/Pause button
    int playX = btnX + btnW + btnGap;
    Fl_Color playBg = _state.playing ? fl_rgb_color(28, 75, 35) : fl_rgb_color(35, 80, 40);
    fl_color(playBg);
    fl_rectf(playX, btnY, btnW, btnH);
    fl_color(fl_rgb_color(40, 100, 50));
    fl_rect(playX, btnY, btnW, btnH);
    fl_font(FL_HELVETICA_BOLD, 14);
    fl_color(_state.playing ? fl_rgb_color(80, 230, 100) : fl_rgb_color(100, 200, 120));
    fl_draw(_state.playing ? "@||" : "@>", playX, btnY, btnW, btnH, FL_ALIGN_CENTER);

    // Stop button
    int stopX = playX + btnW + btnGap;
    fl_color(fl_rgb_color(80, 30, 28));
    fl_rectf(stopX, btnY, btnW, btnH);
    fl_color(fl_rgb_color(100, 40, 38));
    fl_rect(stopX, btnY, btnW, btnH);
    fl_font(FL_HELVETICA_BOLD, 14);
    fl_color(fl_rgb_color(220, 80, 80));
    fl_draw("@square", stopX, btnY, btnW, btnH, FL_ALIGN_CENTER);
}

// ── Page 3: Rhythm pattern visualisation ─────────────────────────────────────
void LcdPanel::drawRhythmPage(int gx, int gy, int gw, int gh) {
    // Dark background
    fl_color(kPageDarkBg);
    fl_rectf(gx, gy, gw, gh);

    const int pad = 8;
    int steps = std::max(1, std::min(32, _state.patternSteps));

    // Header: pattern name
    int hdrY = gy + 4;
    fl_font(FL_HELVETICA_BOLD, 11);
    fl_color(fl_rgb_color(160, 190, 230));

    const char* pName = _state.patternName.empty()
        ? "\xe6\x97\xa0 Pattern"  // 无 Pattern
        : _state.patternName.c_str();
    fl_draw(pName, gx + pad, hdrY, gw / 2, 18, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    char stepsBuf[16];
    snprintf(stepsBuf, sizeof(stepsBuf), "%d steps", steps);
    fl_font(FL_HELVETICA, 10);
    fl_color(kDimText);
    fl_draw(stepsBuf, gx + gw / 2, hdrY, gw / 2 - pad, 18,
            FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

    // ── Mini step grid: 12 rows x N columns ──────────────────────────────
    const int gridRows = 12;
    const int labelW   = 24;    // space for drum label
    int gridX = gx + pad + labelW;
    int gridY = gy + 26;
    int gridW = gw - pad * 2 - labelW;
    int gridH = gh - 26 - 30;   // leave room at bottom for "edit" button area

    int cellW = std::max(2, gridW / steps);
    int cellH = std::max(2, gridH / gridRows);
    int actualGridW = cellW * steps;
    int actualGridH = cellH * gridRows;

    // Draw drum labels
    fl_font(FL_HELVETICA, 8);
    for (int r = 0; r < gridRows; ++r) {
        int cy = gridY + r * cellH;
        fl_color(kDimText);
        fl_draw(kDrumLabels[r], gx + pad, cy, labelW - 2, cellH,
                FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    }

    // Draw grid background
    fl_color(fl_rgb_color(20, 24, 34));
    fl_rectf(gridX, gridY, actualGridW, actualGridH);

    // Draw cells
    for (int r = 0; r < gridRows; ++r) {
        uint16_t bits = _state.patternBits[r];
        for (int s = 0; s < steps; ++s) {
            int cx = gridX + s * cellW;
            int cy = gridY + r * cellH;
            bool hit = (bits >> s) & 1;

            if (hit) {
                // Bright cell for active step
                fl_color(fl_rgb_color(60, 180, 100));
                fl_rectf(cx + 1, cy + 1, cellW - 2, cellH - 2);
            } else {
                // Dim cell
                fl_color(fl_rgb_color(30, 36, 48));
                fl_rectf(cx + 1, cy + 1, cellW - 2, cellH - 2);
            }

            // Grid lines
            fl_color(fl_rgb_color(40, 46, 60));
            fl_rect(cx, cy, cellW, cellH);
        }
    }

    // Beat division lines (every 4 steps)
    fl_color(fl_rgb_color(70, 80, 100));
    for (int s = 4; s < steps; s += 4) {
        int lx = gridX + s * cellW;
        fl_line(lx, gridY, lx, gridY + actualGridH - 1);
    }

    // ── Current step highlight (yellow vertical line) ────────────────────
    if (_state.currentStep >= 0 && _state.currentStep < steps) {
        int hlX = gridX + _state.currentStep * cellW;
        fl_color(fl_color_average(kAccentYellow, fl_rgb_color(0, 0, 0), 0.25f));
        fl_rectf(hlX, gridY, cellW, actualGridH);
        // Re-draw active cells in the highlighted column brighter
        for (int r = 0; r < gridRows; ++r) {
            uint16_t bits = _state.patternBits[r];
            if ((bits >> _state.currentStep) & 1) {
                int cy = gridY + r * cellH;
                fl_color(kAccentYellow);
                fl_rectf(hlX + 1, cy + 1, cellW - 2, cellH - 2);
            }
        }
    }

    // Grid border
    fl_color(fl_rgb_color(50, 60, 80));
    fl_rect(gridX, gridY, actualGridW, actualGridH);

    // ── Bottom: step counter ─────────────────────────────────────────────
    int botY = gridY + actualGridH + 4;
    fl_font(FL_HELVETICA, 9);
    fl_color(kDimText);
    if (_state.currentStep >= 0) {
        char curBuf[32];
        snprintf(curBuf, sizeof(curBuf), "Step %d / %d", _state.currentStep + 1, steps);
        fl_draw(curBuf, gx + pad, botY, gw - pad * 2, 16, FL_ALIGN_CENTER);
    } else {
        fl_draw("STOP", gx + pad, botY, gw - pad * 2, 16, FL_ALIGN_CENTER);
    }
}

// ── Hit tests ────────────────────────────────────────────────────────────────

int LcdPanel::pageTabAtPos(int px, int py) const noexcept {
    const int tabY = _sy + (_kHdr - kTabH) / 2;
    int tx = _sx + _sw - 3 * (kTabW + 3) - 4;
    for (int i = 1; i <= 3; ++i) {
        if (px >= tx && px < tx + kTabW && py >= tabY && py < tabY + kTabH)
            return i;
        tx += kTabW + 3;
    }
    return 0;
}

int LcdPanel::voiceCardAtPos(int px, int py) const noexcept {
    if (_state.activePage != 1) return -1;

    int gx = _sx,      gy = _sy + _kHdr;
    int gw = _sw,      gh = _gridH;

    const int cols = 4, rows = 2, gap = 2;
    int cardW = (gw - gap * (cols + 1)) / cols;
    int cardH = (gh - gap * (rows + 1)) / rows;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            int cx = gx + gap + col * (cardW + gap);
            int cy = gy + gap + row * (cardH + gap);
            if (px >= cx && px < cx + cardW && py >= cy && py < cy + cardH)
                return row * cols + col;
        }
    }
    return -1;
}

int LcdPanel::scoreButtonAtPos(int px, int py, float& outFrac) const noexcept {
    if (_state.activePage != 2) return -1;

    int gx = _sx, gy = _sy + _kHdr;
    int gw = _sw, gh = _gridH;
    const int pad = 12;
    const int innerW = gw - pad * 2;

    // Progress bar hit test
    int barY = gy + gh / 2 - 8;
    int barH = 16;
    int barX = gx + pad;
    if (px >= barX && px < barX + innerW && py >= barY && py < barY + barH) {
        outFrac = static_cast<float>(px - barX) / static_cast<float>(innerW);
        outFrac = std::max(0.0f, std::min(1.0f, outFrac));
        return 3; // seek
    }

    // Transport buttons
    int btnW = 48, btnH = 28, btnGap = 12;
    int totalBtnW = btnW * 3 + btnGap * 2;
    int btnX = gx + (gw - totalBtnW) / 2;
    int btnY = gy + gh - btnH - 16;

    if (py >= btnY && py < btnY + btnH) {
        if (px >= btnX && px < btnX + btnW) return 0;                      // rewind
        int playX = btnX + btnW + btnGap;
        if (px >= playX && px < playX + btnW) return 1;                     // play/pause
        int stopX = playX + btnW + btnGap;
        if (px >= stopX && px < stopX + btnW) return 2;                     // stop
    }

    outFrac = 0.f;
    return -1;
}

// ── Mouse click handler ──────────────────────────────────────────────────────
int LcdPanel::handle(int event) {
    switch (event) {
    case FL_PUSH: {
        int mx = Fl::event_x(), my = Fl::event_y();

        // ── Tab click ─────────────────────────────────────────────────────
        int tab = pageTabAtPos(mx, my);
        if (tab > 0 && tab != _state.activePage) {
            _state.activePage = tab;
            if (_pageChangeCb) _pageChangeCb(tab);
            redraw();
            return 1;
        }

        // ── Page 1: voice card click ──────────────────────────────────────
        if (_state.activePage == 1) {
            int slot = voiceCardAtPos(mx, my);
            if (slot >= 0) {
                if (Fl::event_clicks() >= 1) {
                    if (_editCb) _editCb(slot);
                } else {
                    _state.voices[slot].active = !_state.voices[slot].active;
                    redraw();
                }
                return 1;
            }
        }

        // ── Page 2: score/MIDI transport buttons ──────────────────────────
        if (_state.activePage == 2) {
            float seekFrac = 0.f;
            int btn = scoreButtonAtPos(mx, my, seekFrac);
            if (btn >= 0 && _pageActionCb) {
                switch (btn) {
                case 0: _pageActionCb("rewind", 0.f); break;
                case 1: _pageActionCb("play",   0.f); break;
                case 2: _pageActionCb("stop",   0.f); break;
                case 3: _pageActionCb("seek",   seekFrac); break;
                }
                return 1;
            }
        }

        return 0;
    }
    case FL_RELEASE:
        return 1;
    case FL_ENTER:
    case FL_LEAVE:
        return 1;
    default:
        return Fl_Widget::handle(event);
    }
}

// ── Info row: RHYTHM | REGISTRATION | SHIFT OFF ───────────────────────────────
void LcdPanel::drawInfoRow(int rx, int ry, int rw, int rh) {
    fl_color(kInfoBg);
    fl_rectf(rx, ry, rw, rh);

    fl_color(kMidGray);
    fl_line(rx, ry, rx + rw - 1, ry);

    int secW = rw / 3;

    // RHYTHM section
    fl_color(fl_rgb_color(178, 180, 173));
    fl_rectf(rx, ry, secW, rh);

    fl_font(FL_HELVETICA, 8);
    fl_color(kLabelGray);
    fl_draw("RHYTHM", rx + 4, ry + 2, secW - 8, 12, FL_ALIGN_LEFT);
    fl_font(FL_HELVETICA_BOLD, 11);
    fl_color(kDark);
    fl_draw(_state.rhythmName.c_str(),
            rx + 4, ry + 12, secW - 8, rh - 14,
            FL_ALIGN_LEFT | FL_ALIGN_CLIP);

    fl_color(kMidGray);
    fl_line(rx + secW, ry + 3, rx + secW, ry + rh - 3);

    // REGISTRATION section
    fl_font(FL_HELVETICA, 8);
    fl_color(kLabelGray);
    fl_draw("REGISTRATION", rx + secW + 4, ry + 2, secW - 8, 12, FL_ALIGN_LEFT);

    fl_color(kMidGray);
    fl_line(rx + secW * 2, ry + 3, rx + secW * 2, ry + rh - 3);

    // SHIFT OFF section
    fl_font(FL_HELVETICA, 8);
    fl_color(kLabelGray);
    fl_draw("SHIFT OFF", rx + secW * 2 + 4, ry + 2, secW - 8, 12, FL_ALIGN_LEFT);
}

// ── Bottom row: TEMPO | BAR/BEAT | REG slot ──────────────────────────────────
void LcdPanel::drawBottomRow(int bx, int by, int bw, int bh) {
    fl_color(kBottomBg);
    fl_rectf(bx, by, bw, bh);

    fl_color(fl_rgb_color(170, 172, 165));
    fl_line(bx, by, bx + bw - 1, by);
    fl_color(kMidGray);
    fl_line(bx, by + 1, bx + bw - 1, by + 1);

    int pad   = 6;
    int cy    = by + 2;
    int tempoW = bw * 25 / 100;
    int bbW    = bw * 45 / 100;
    int boxH = bh - 6;

    // TEMPO box
    fl_color(fl_rgb_color(18, 20, 18));
    fl_rectf(bx + pad, cy, tempoW - pad, boxH);
    fl_color(fl_rgb_color(28, 30, 28));
    fl_rectf(bx + pad + 1, cy + 1, tempoW - pad - 2, boxH - 2);
    fl_font(FL_HELVETICA, 8);
    fl_color(fl_rgb_color(140, 145, 138));
    fl_draw("TEMPO", bx + pad + 2, cy + 2, tempoW - pad - 4, 11, FL_ALIGN_LEFT);
    char tbuf[8];
    snprintf(tbuf, sizeof(tbuf), "%d", _state.tempo);
    fl_font(FL_COURIER_BOLD, 20);
    fl_color(fl_rgb_color(255, 215, 60));
    fl_draw(tbuf, bx + pad, cy + 12, tempoW - pad, boxH - 14, FL_ALIGN_CENTER);

    // BAR/BEAT box
    int bbX = bx + tempoW + pad;
    fl_color(fl_rgb_color(18, 20, 18));
    fl_rectf(bbX, cy, bbW - pad * 2, boxH);
    fl_color(fl_rgb_color(28, 30, 28));
    fl_rectf(bbX + 1, cy + 1, bbW - pad * 2 - 2, boxH - 2);
    int bbInnerW = bbW - pad * 2;

    // Beat indicator dots
    {
        int bpb = _state.beatsPerBar;
        if (bpb < 1) bpb = 4;
        if (bpb > 8) bpb = 8;

        int dotR   = 7;
        int dotGap = 8;
        int dotsW  = bpb * (dotR * 2 + dotGap) - dotGap;
        int dotsX  = bbX + (bbInnerW - dotsW) / 2;
        int dotsY  = cy + 4;

        for (int d = 0; d < bpb; ++d) {
            int dx = dotsX + d * (dotR * 2 + dotGap) + dotR;
            int dy = dotsY + dotR;
            bool isCurrentBeat = _state.playing && (d + 1 == _state.beat);

            fl_color(fl_rgb_color(18, 20, 18));
            fl_pie(dx - dotR - 1, dy - dotR - 1, (dotR + 1) * 2, (dotR + 1) * 2, 0, 360);

            if (isCurrentBeat) {
                fl_color(fl_rgb_color(50, 230, 80));
                fl_pie(dx - dotR, dy - dotR, dotR * 2, dotR * 2, 0, 360);
                fl_color(fl_rgb_color(140, 255, 160));
                fl_pie(dx - dotR / 2, dy - dotR / 2, dotR, dotR, 0, 360);
                fl_color(fl_rgb_color(30, 120, 50));
                fl_arc(dx - dotR - 3, dy - dotR - 3, (dotR + 3) * 2, (dotR + 3) * 2, 0, 360);
            } else {
                fl_color(fl_rgb_color(50, 52, 48));
                fl_pie(dx - dotR, dy - dotR, dotR * 2, dotR * 2, 0, 360);
            }
        }
    }

    // Bar : Beat number
    Fl_Color playCol = _state.playing ? fl_rgb_color(60, 230, 100) : fl_rgb_color(160, 162, 155);
    fl_color(playCol);
    if (_state.playing) {
        char bbBuf[16];
        snprintf(bbBuf, sizeof(bbBuf), "%d : %d", _state.bar, _state.beat);
        fl_font(FL_COURIER_BOLD, 16);
        fl_draw(bbBuf, bbX, cy + 22, bbInnerW, boxH - 24, FL_ALIGN_CENTER);
    } else {
        fl_font(FL_HELVETICA_BOLD, 14);
        fl_draw("STOP", bbX, cy + 22, bbInnerW, boxH - 24, FL_ALIGN_CENTER);
    }

    // REGISTRATION slot button
    int regX = bx + tempoW + bbW;
    int regW = bw - tempoW - bbW - 4;

    static const char kBankLetters[] = "ABCDE";
    char bankCh[2] = { (_state.regBank >= 1 && _state.regBank <= 5)
                       ? kBankLetters[_state.regBank - 1] : 'A', '\0' };

    int btnSz = std::min(regW - 8, bh - 12);
    int btnX  = regX + (regW - btnSz) / 2;
    int btnY  = by + (bh - btnSz) / 2;

    fl_font(FL_HELVETICA, 8);
    fl_color(kLabelGray);
    fl_draw(bankCh, btnX, btnY - 11, btnSz, 10, FL_ALIGN_CENTER);

    fl_color(fl_rgb_color(255, 160, 40));
    fl_rectf(btnX - 2, btnY - 2, btnSz + 4, btnSz + 4);

    fl_color(fl_rgb_color(235, 135, 35));
    fl_rectf(btnX, btnY, btnSz, btnSz);
    fl_color(fl_rgb_color(255, 155, 45));
    fl_rectf(btnX, btnY, btnSz, btnSz / 3);

    fl_color(fl_rgb_color(180, 100, 20));
    fl_rect(btnX, btnY, btnSz, btnSz);

    char slotBuf[4];
    snprintf(slotBuf, sizeof(slotBuf), "%d", _state.regSlot);
    fl_font(FL_HELVETICA_BOLD, 16);
    fl_color(FL_WHITE);
    fl_draw(slotBuf, btnX, btnY, btnSz, btnSz, FL_ALIGN_CENTER);
}

} // namespace em::ui
