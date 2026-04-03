#include "KeyboardPanel.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <cmath>
#include <algorithm>

namespace em::ui {

// ── Color constants ───────────────────────────────────────────────────────────
static const Fl_Color kWhiteKey      = fl_rgb_color(244, 244, 240);
static const Fl_Color kWhiteKeyHi    = fl_rgb_color(255, 255, 255);
static const Fl_Color kWhiteKeySh    = fl_rgb_color(195, 195, 188);
static const Fl_Color kWhiteKeyPress = fl_rgb_color(165, 205, 255);
static const Fl_Color kWhiteKeyPressHi = fl_rgb_color(200, 225, 255);
static const Fl_Color kBlackKey      = fl_rgb_color(28,  28,  33);
static const Fl_Color kBlackKeyTop   = fl_rgb_color(48,  48,  55);
static const Fl_Color kBlackKeyPress = fl_rgb_color(55, 115, 200);
static const Fl_Color kKeyBorder     = fl_rgb_color(88,  88,  98);
static const Fl_Color kKeyboardBg    = fl_rgb_color(16,  20,  33);

// chromatic offset within octave → black key?
static const bool kIsBlack[12] = {
    false,true,false,true,false,false,true,false,true,false,true,false
};

// ──────────────────────────────────────────────────────────────────────────────
KeyboardPanel::KeyboardPanel(int x, int y, int w, int h, const char* label)
    : Fl_Widget(x, y, w, h, label) {
    _active.fill(false);
}

bool KeyboardPanel::isBlackKey(int noteOffset) const noexcept {
    return kIsBlack[(kFirstNote + noteOffset) % 12];
}

int KeyboardPanel::whiteKeyCount() const noexcept {
    int count = 0;
    for (int i = 0; i < kNumKeys; ++i)
        if (!isBlackKey(i)) ++count;
    return count;
}

int KeyboardPanel::whiteKeyIndex(int noteOff) const noexcept {
    if (isBlackKey(noteOff)) return -1;
    int idx = 0;
    for (int i = 0; i < noteOff; ++i)
        if (!isBlackKey(i)) ++idx;
    return idx;
}

float KeyboardPanel::whiteKeyW() const noexcept {
    return static_cast<float>(w()) / whiteKeyCount();
}

float KeyboardPanel::blackKeyW() const noexcept {
    return whiteKeyW() * 0.62f;
}

float KeyboardPanel::blackKeyH() const noexcept {
    return h() * 0.60f;
}

float KeyboardPanel::whiteKeyX(int whiteIdx) const noexcept {
    return x() + whiteIdx * whiteKeyW();
}

// Returns the MIDI note at pixel (px, py), or -1 if none.
int KeyboardPanel::noteAtPos(int px, int py) const noexcept {
    const float wkW = whiteKeyW();
    const float bkW = blackKeyW();
    const float bkH = blackKeyH();
    const int   ky  = y();

    // Check black keys first (they are on top)
    if (py - ky < static_cast<int>(bkH)) {
        for (int i = 0; i < kNumKeys; ++i) {
            if (!isBlackKey(i)) continue;
            // Black key x = previous white key x + (whiteKeyW - bkW/2)
            // Find the white key just before this black key
            int prevWhiteIdx = -1;
            for (int j = i - 1; j >= 0; --j) {
                if (!isBlackKey(j)) { prevWhiteIdx = whiteKeyIndex(j); break; }
            }
            if (prevWhiteIdx < 0) continue;
            float bx = x() + (prevWhiteIdx + 1) * wkW - bkW * 0.5f;
            if (px >= static_cast<int>(bx) &&
                px <  static_cast<int>(bx + bkW))
                return kFirstNote + i;
        }
    }

    // White keys
    for (int i = 0; i < kNumKeys; ++i) {
        if (isBlackKey(i)) continue;
        int wIdx = whiteKeyIndex(i);
        float kx = whiteKeyX(wIdx);
        if (px >= static_cast<int>(kx) &&
            px <  static_cast<int>(kx + wkW))
            return kFirstNote + i;
    }
    return -1;
}

void KeyboardPanel::setNoteActive(uint8_t note, bool active) {
    if (note < 128) {
        _active[note] = active;
        redraw();
    }
}

void KeyboardPanel::triggerNote(int note, bool on) {
    if (note < 0 || note > 127) return;
    _active[note] = on;
    if (_cb) _cb(static_cast<uint8_t>(note), _defaultVelocity, on);
    redraw();
}

// ── Draw ──────────────────────────────────────────────────────────────────────
void KeyboardPanel::draw() {
    const float wkW = whiteKeyW();
    const float wkH = static_cast<float>(h());
    const float bkW = blackKeyW();
    const float bkH = blackKeyH();

    // Background
    fl_color(kKeyboardBg);
    fl_rectf(x(), y(), w(), h());

    // Accent strip at top (category color)
    fl_color(_accentColor);
    fl_rectf(x(), y(), w(), 4);

    // Subtle top-highlight just below accent strip
    fl_color(fl_color_average(_accentColor, FL_WHITE, 0.20f));
    fl_line(x(), y() + 4, x() + w() - 1, y() + 4);

    // ── White keys ────────────────────────────────────────────────────────
    for (int i = 0; i < kNumKeys; ++i) {
        if (isBlackKey(i)) continue;
        int wIdx = whiteKeyIndex(i);
        int note = kFirstNote + i;
        int kx = static_cast<int>(whiteKeyX(wIdx));
        int kw = static_cast<int>(wkW) - 1;
        int ky = y() + 4;
        int kh = static_cast<int>(wkH) - 5;

        bool lit = _active[note] || (note == _pressedNote);

        Fl_Color fill = lit ? kWhiteKeyPress : kWhiteKey;
        Fl_Color fillHi = lit ? kWhiteKeyPressHi : kWhiteKeyHi;

        // Main fill
        fl_color(fill);
        fl_rectf(kx + 1, ky, kw - 1, kh);

        // Top highlight strip (2px)
        fl_color(fillHi);
        fl_rectf(kx + 2, ky, kw - 3, 3);

        // Left highlight edge
        fl_color(fillHi);
        fl_line(kx + 1, ky, kx + 1, ky + kh - 1);

        // Bottom shadow strip
        fl_color(kWhiteKeySh);
        fl_rectf(kx + 1, ky + kh - 5, kw - 1, 5);

        // Key border
        fl_color(kKeyBorder);
        fl_rect(kx, ky, kw + 1, kh);

        // Pressed glow: thin colored bottom bar
        if (lit) {
            fl_color(fl_color_average(kWhiteKeyPress, kBlackKeyPress, 0.3f));
            fl_rectf(kx + 2, ky + kh - 7, kw - 3, 5);
        }

        // Note name on C keys
        if (note % 12 == 0) {
            fl_font(FL_HELVETICA, 8);
            fl_color(lit ? fl_rgb_color(40, 80, 160) : fl_rgb_color(140, 140, 148));
            char buf[4];
            snprintf(buf, sizeof(buf), "C%d", note / 12 - 1);
            fl_draw(buf, kx + 2, ky + kh - 13, kw - 4, 12, FL_ALIGN_CENTER);
        }
    }

    // ── Black keys (drawn on top) ─────────────────────────────────────────
    for (int i = 0; i < kNumKeys; ++i) {
        if (!isBlackKey(i)) continue;
        int note = kFirstNote + i;

        int prevWhiteIdx = -1;
        for (int j = i - 1; j >= 0; --j) {
            if (!isBlackKey(j)) { prevWhiteIdx = whiteKeyIndex(j); break; }
        }
        if (prevWhiteIdx < 0) continue;

        int bx = static_cast<int>(x() + (prevWhiteIdx + 1) * wkW - bkW * 0.5f);
        int bw = static_cast<int>(bkW);
        int bh = static_cast<int>(bkH);

        bool lit = _active[note] || (note == _pressedNote);

        // Main body
        fl_color(lit ? kBlackKeyPress : kBlackKey);
        fl_rectf(bx, y() + 4, bw, bh);

        // Top sheen (lighter top edge — 3px)
        fl_color(lit ? fl_color_average(kBlackKeyPress, FL_WHITE, 0.35f) : kBlackKeyTop);
        fl_rectf(bx + 1, y() + 4, bw - 2, 3);

        // Side highlights (left edge)
        fl_color(lit ? fl_color_average(kBlackKeyPress, FL_WHITE, 0.22f)
                     : fl_rgb_color(50, 50, 58));
        fl_line(bx + 1, y() + 4, bx + 1, y() + 4 + bh - 8);

        // Border
        fl_color(kKeyBorder);
        fl_rect(bx, y() + 4, bw, bh);

        // Bottom accent strip
        if (lit) {
            fl_color(fl_rgb_color(120, 200, 255));
            fl_rectf(bx + 2, y() + 4 + bh - 6, bw - 4, 4);
        } else {
            fl_color(fl_rgb_color(45, 45, 55));
            fl_rectf(bx + 2, y() + 4 + bh - 5, bw - 4, 3);
        }
    }
}

// ── Event handling ────────────────────────────────────────────────────────────
int KeyboardPanel::handle(int event) {
    switch (event) {
    case FL_PUSH: {
        int note = noteAtPos(Fl::event_x(), Fl::event_y());
        if (note >= 0) {
            _pressedNote = note;
            triggerNote(note, true);
            return 1;
        }
        break;
    }
    case FL_RELEASE: {
        if (_pressedNote >= 0) {
            triggerNote(_pressedNote, false);
            _pressedNote = -1;
            return 1;
        }
        break;
    }
    case FL_DRAG: {
        int note = noteAtPos(Fl::event_x(), Fl::event_y());
        if (note != _pressedNote) {
            if (_pressedNote >= 0) triggerNote(_pressedNote, false);
            if (note >= 0)         triggerNote(note, true);
            _pressedNote = note;
        }
        return 1;
    }
    case FL_KEYDOWN: {
        // Map QWERTY bottom row to white keys starting from C
        static const char kKeys[] = "zsxdcvgbhnjm,.";
        int key = Fl::event_key();
        for (int i = 0; kKeys[i]; ++i) {
            if (key == kKeys[i]) {
                // Find i-th white key
                int wCount = 0;
                for (int n = 0; n < kNumKeys; ++n) {
                    if (!isBlackKey(n)) {
                        if (wCount == i) {
                            triggerNote(kFirstNote + n, true);
                            return 1;
                        }
                        ++wCount;
                    }
                }
            }
        }
        break;
    }
    case FL_KEYUP: {
        static const char kKeys[] = "zsxdcvgbhnjm,.";
        int key = Fl::event_key();
        for (int i = 0; kKeys[i]; ++i) {
            if (key == kKeys[i]) {
                int wCount = 0;
                for (int n = 0; n < kNumKeys; ++n) {
                    if (!isBlackKey(n)) {
                        if (wCount == i) {
                            triggerNote(kFirstNote + n, false);
                            return 1;
                        }
                        ++wCount;
                    }
                }
            }
        }
        break;
    }
    case FL_FOCUS:
    case FL_UNFOCUS:
        return 1;
    default:
        break;
    }
    return Fl_Widget::handle(event);
}

constexpr int KeyboardPanel::kKeyMap[14];

} // namespace em::ui
