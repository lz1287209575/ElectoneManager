#include "ControlPanel.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace em::ui {

// ── Color palette ─────────────────────────────────────────────────────────────
static const Fl_Color kPanelBg       = fl_rgb_color(68, 70, 66);
static const Fl_Color kSectionHeader = fl_rgb_color(182, 182, 175); // warm silver
static const Fl_Color kDark          = fl_rgb_color(26, 26, 24);

// Card base colors per category (matching LcdPanel spirit)
static const Fl_Color kCardUpper     = fl_rgb_color(155, 55, 35);   // deep red
static const Fl_Color kCardLower     = fl_rgb_color(145, 85, 15);   // warm amber
static const Fl_Color kCardRhythm    = fl_rgb_color(42,  98, 42);   // forest green
static const Fl_Color kCardRegBank   = fl_rgb_color(155, 68, 20);   // orange-red
static const Fl_Color kCardRegSlot   = fl_rgb_color(52,  54, 50);   // neutral dark

// Tab accent colors (same as card colors)
static const Fl_Color kAccent[4] = { kCardUpper, kCardLower, kCardRhythm, kCardRegBank };

// Tab labels (English \n Chinese)
static const char* kTabLabel[4] = {
    "UPPER VOICE\n\xe4\xb8\x8a\xe9\x94\xae\xe7\x9b\x98\xe9\x9f\xb3\xe8\x89\xb2",
    "LOWER VOICE\n\xe4\xb8\x8b\xe9\x94\xae\xe7\x9b\x98\xe9\x9f\xb3\xe8\x89\xb2",
    "RHYTHM\n\xe8\x8a\x82\xe5\xa5\x8f",
    "REGISTRATION\n\xe6\xb3\xa8\xe5\x86\x8c"
};

// ── Encode helpers ────────────────────────────────────────────────────────────
// user_data layout:  bits[31:24]=kind  bits[23:16]=extra  bits[15:0]=id
static constexpr int kKindUpper  = 0;
static constexpr int kKindLower  = 1;
static constexpr int kKindRhythm = 2;
static constexpr int kKindReg    = 3;
static constexpr int kKindTab    = 4;

static intptr_t encode(int kind, int extra, int id) {
    return (kind << 24) | (extra << 16) | (id & 0xFFFF);
}
static int decodeKind (intptr_t v) { return (v >> 24) & 0xFF; }
static int decodeExtra(intptr_t v) { return (v >> 16) & 0xFF; }
static int decodeId   (intptr_t v) { return v & 0xFFFF; }

// ── Draw a single LCD-style card button ───────────────────────────────────────
// label format:  "Chinese\nEnglish"  (top-left small, bottom large bold)
static void drawCardButton(Fl_Button* btn, Fl_Color cardBg, bool active) {
    int bx = btn->x(), by = btn->y(), bw = btn->w(), bh = btn->h();

    // Card fill (two-tone: lighter top stripe → main fill)
    Fl_Color fill = active ? fl_color_average(cardBg, FL_WHITE, 0.82f) : cardBg;
    Fl_Color fillHi = fl_color_average(fill, FL_WHITE, 0.14f);
    fl_color(fillHi);
    fl_rectf(bx, by, bw, 5);
    fl_color(fill);
    fl_rectf(bx, by + 5, bw, bh - 5);

    // Bottom shadow strip
    fl_color(fl_color_average(fill, kDark, 0.82f));
    fl_rectf(bx, by + bh - 3, bw, 3);

    // Border
    Fl_Color border = fl_color_average(cardBg, kDark, 0.6f);
    fl_color(border);
    fl_rect(bx, by, bw, bh);

    // Active left accent stripe
    if (active) {
        fl_color(fl_color_average(cardBg, FL_WHITE, 0.80f));
        fl_rectf(bx, by, 3, bh);
        fl_color(fl_color_average(cardBg, FL_WHITE, 0.92f));
        fl_rectf(bx, by, 2, bh);
    }

    const char* lbl = btn->label();
    if (!lbl) return;

    // Split label on '\n'
    const char* nl = strchr(lbl, '\n');

    if (!nl) {
        // ── Single-line pill label (e.g. bank selectors "B1"–"B5") ───────
        fl_font(FL_HELVETICA_BOLD, 13);
        fl_color(active ? fl_rgb_color(30, 30, 28) : fl_color_average(fill, FL_WHITE, 0.70f));
        fl_draw(lbl, bx + 1, by + 1, bw - 2, bh - 2, FL_ALIGN_CENTER | FL_ALIGN_CLIP);
        return;
    }

    // ── Role label: small text, top-left ──────────────────────────────────
    {
        int line1Len = static_cast<int>(nl - lbl);
        char line1[48] = {};
        snprintf(line1, std::min(line1Len + 1, 47), "%s", lbl);

        fl_font(FL_HELVETICA, 8);
        fl_color(fl_color_average(fill, FL_WHITE, 0.55f));
        fl_draw(line1, bx + 4, by + 3, bw - 18, 12, FL_ALIGN_LEFT | FL_ALIGN_CLIP);
    }

    // ── Voice name: large bold, bottom half ───────────────────────────────
    const char* namePart = nl + 1;
    fl_font(FL_HELVETICA_BOLD, 14);
    fl_color(FL_WHITE);
    // Bottom half of card
    int nameY = by + bh / 2;
    int nameH = bh - bh / 2 - 4;
    fl_draw(namePart, bx + 2, nameY, bw - 4, nameH, FL_ALIGN_CENTER | FL_ALIGN_CLIP);

    // ── LED dot: top-right ─────────────────────────────────────────────────
    int ldx = bx + bw - 8;
    int ldy = by + 6;
    Fl_Color ledOff = fl_color_average(cardBg, kDark, 0.4f);
    fl_color(active ? fl_color_average(cardBg, FL_WHITE, 0.75f) : ledOff);
    fl_pie(ldx - 4, ldy - 4, 8, 8, 0, 360);
    if (active) {
        // Bright inner highlight on active LED
        fl_color(fl_color_average(cardBg, FL_WHITE, 0.95f));
        fl_pie(ldx - 2, ldy - 2, 4, 4, 0, 360);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
ControlPanel::ControlPanel(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h) {
    box(FL_FLAT_BOX);
    color(kPanelBg);

    const int tabH  = h / 4;
    const int tabW  = kTabStripW;
    const int contX = x + tabW;
    const int contW = w - tabW;

    // ── Create 4 invisible tab hit-area buttons (left strip) ──────────────
    for (int t = 0; t < 4; ++t) {
        auto* tbtn = new Fl_Button(x, y + t * tabH, tabW, tabH, nullptr);
        tbtn->box(FL_NO_BOX);
        tbtn->color(kPanelBg);
        // IMPORTANT: use single-arg callback() so user_data_ is NOT overwritten.
        tbtn->user_data(reinterpret_cast<void*>(encode(kKindTab, 0, t)));
        tbtn->callback(tabBtnCb);   // single-arg: preserves user_data_
        _tabBtns.push_back(tbtn);
    }

    // ── Create 4 content groups (right area) ──────────────────────────────
    for (int t = 0; t < 4; ++t) {
        auto* grp = new Fl_Group(contX, y, contW, h);
        grp->box(FL_NO_BOX);
        buildTabContent(t, contX, y, contW, h);
        grp->end();
        _tabGroups[t] = grp;
    }

    end();

    // Show only the first tab
    for (int t = 1; t < 4; ++t)
        _tabGroups[t]->hide();
    _tabGroups[0]->show();
}

// ── Build content for a given tab ─────────────────────────────────────────────
void ControlPanel::buildTabContent(int tab, int x, int y, int w, int h) {
    const int pad = 6;

    if (tab == 0 || tab == 1) {
        // ── 4×3 voice grid ────────────────────────────────────────────────
        struct VDef { const char* label; int prog; };
        static const VDef voices[] = {
            {"\xe5\xbc\xa6\xe4\xb9\x90\nStrings",   40},
            {"\xe9\x93\x9c\xe7\xae\xa1\nBrass",      56},
            {"\xe6\x9c\xa8\xe7\xae\xa1\nWoodwind",   72},
            {"\xe5\x90\x88\xe5\xa5\x8f\nEnsemble",   48},
            {"\xe9\x95\xbf\xe9\x9f\xb3\nSustain",    92},
            {"\xe5\x90\x88\xe6\x88\x90\nSynth",      80},
            {"\xe9\x92\xa2\xe7\x90\xb4\nPiano",       0},
            {"\xe9\xa3\x8e\xe7\x90\xb4\nOrgan",      16},
            {"\xe6\x89\x93\xe5\x87\xbb\nPerc",      112},
            {"\xe5\x90\x89\xe4\xbb\x96\nGuitar",     24},
            {"\xe5\x90\x88\xe5\x94\xb1\nChoir",      52},
            {"\xe6\xb0\x91\xe6\x97\x8f\nEthnic",    104},
        };

        const int cols = 4, rows = 3;
        int btnW = (w - pad * (cols + 1)) / cols;
        int btnH = (h - pad * (rows + 1)) / rows;

        for (int i = 0; i < 12; ++i) {
            int col = i % cols, row = i / cols;
            int bx = x + pad + col * (btnW + pad);
            int by = y + pad + row * (btnH + pad);
            auto* btn = makeVoiceBtn(bx, by, btnW, btnH,
                                      voices[i].label, voices[i].prog,
                                      tab == 0);
            if (tab == 0) _upperVoiceBtns.push_back(btn);
            else          _lowerVoiceBtns.push_back(btn);
        }

    } else if (tab == 2) {
        // ── 2×4 rhythm grid (7 buttons) ───────────────────────────────────
        struct RDef { const char* label; int bpm; };
        static const RDef rhythms[] = {
            {"\xe8\xbf\x9b\xe8\xa1\x8c\xe6\x9b\xb2\nMarch",  120},
            {"\xe5\x8d\x8e\xe5\xb0\x94\xe5\x85\xb9\nWaltz",  160},
            {"\xe6\x91\x87\xe6\x91\x86\nSwing",               130},
            {"\xe6\xb5\x81\xe8\xa1\x8c\nPop",                 120},
            {"R&B\nR&B",                                       100},
            {"\xe6\x8b\x89\xe4\xb8\x81\nLatin",               128},
            {"\xe6\xb0\x91\xe4\xb9\x90\nEthnic",              108},
        };

        const int cols = 2, rows = 4;
        int btnW = (w - pad * (cols + 1)) / cols;
        int btnH = (h - pad * (rows + 1)) / rows;

        for (int i = 0; i < 7; ++i) {
            int col = i % cols, row = i / cols;
            int bx = x + pad + col * (btnW + pad);
            int by = y + pad + row * (btnH + pad);
            auto* btn = makeRhythmBtn(bx, by, btnW, btnH,
                                       rhythms[i].label, i, rhythms[i].bpm);
            _rhythmBtns.push_back(btn);
        }

        // "Edit Pattern" button in the remaining cell (index 7 = row 3, col 1)
        {
            int col = 1, row = 3;
            int bx = x + pad + col * (btnW + pad);
            int by = y + pad + row * (btnH + pad);
            auto* btn = new Fl_Button(bx, by, btnW, btnH, nullptr);
            btn->copy_label("\xe7\xbc\x96\xe8\xbe\x91\nEdit");
            btn->box(FL_NO_BOX);
            btn->labeltype(FL_NO_LABEL);
            btn->labelsize(11);
            btn->user_data(reinterpret_cast<void*>(encode(kKindRhythm, 0, 99)));
            btn->callback(editBtnCb);
            _rhythmBtns.push_back(btn);
        }

    } else { // tab == 3
        // ── Registration: 5 bank tabs + 2×8 slot grid ─────────────────────
        const int bankTabH = 32;
        const int bankTabW = (w - pad * 6) / 5;
        const int slotAreaY = y + pad + bankTabH + pad;
        const int slotAreaH = h - bankTabH - pad * 3;

        // Bank selector tabs
        for (int b = 0; b < 5; ++b) {
            char lbl[6]; snprintf(lbl, sizeof(lbl), "B%d", b + 1);
            auto* btn = new Fl_Button(x + pad + b * (bankTabW + pad),
                                       y + pad,
                                       bankTabW, bankTabH, nullptr);
            btn->copy_label(lbl);
            btn->box(FL_NO_BOX);
            btn->labeltype(FL_NO_LABEL);  // we paint manually
            btn->labelsize(12);
            btn->labelfont(FL_HELVETICA_BOLD);
            btn->user_data(reinterpret_cast<void*>(encode(kKindReg, 255, b)));
            btn->callback(regBtnCb);   // single-arg
            _regBtns.push_back(btn);
        }

        // Slot grid: 2 rows × 8 cols
        const int slotCols = 8, slotRows = 2;
        int slotW = (w - pad * (slotCols + 1)) / slotCols;
        int slotH = (slotAreaH - pad * (slotRows + 1)) / slotRows;

        for (int r = 0; r < slotRows; ++r) {
            for (int c = 0; c < slotCols; ++c) {
                int slot = r * slotCols + c;
                char lbl[4]; snprintf(lbl, sizeof(lbl), "%d", slot + 1);
                auto* btn = new Fl_Button(
                    x + pad + c * (slotW + pad),
                    slotAreaY + pad + r * (slotH + pad),
                    slotW, slotH, nullptr);
                btn->copy_label(lbl);
                btn->box(FL_NO_BOX);
                btn->labeltype(FL_NO_LABEL);  // we paint manually
                btn->labelsize(12);
                btn->labelfont(FL_HELVETICA_BOLD);
                btn->user_data(reinterpret_cast<void*>(encode(kKindReg, 0, slot)));
                btn->callback(regBtnCb);   // single-arg
                _regBtns.push_back(btn);
            }
        }
    }
}

// ── Factory helpers ────────────────────────────────────────────────────────────
// NOTE: All factory helpers use the single-arg callback() overload so that
// user_data_ (which holds the encoded kind/extra/id) is NOT overwritten.
// The `self` pointer is recovered by walking w->parent()->parent() (button →
// Fl_Group content pane → ControlPanel).

Fl_Button* ControlPanel::makeVoiceBtn(int x, int y, int w, int h,
                                       const char* label, int programBase,
                                       bool isUpper) {
    auto* btn = new Fl_Button(x, y, w, h, nullptr);
    btn->copy_label(label);
    btn->box(FL_NO_BOX);
    btn->labeltype(FL_NO_LABEL);  // suppress FLTK's auto-label draw; we paint manually
    btn->labelsize(11);
    btn->user_data(reinterpret_cast<void*>(
        encode(isUpper ? kKindUpper : kKindLower, 0, programBase)));
    btn->callback(voiceBtnCb);   // single-arg
    return btn;
}

Fl_Button* ControlPanel::makeRhythmBtn(int x, int y, int w, int h,
                                        const char* label, int idx, int bpm) {
    auto* btn = new Fl_Button(x, y, w, h, nullptr);
    btn->copy_label(label);
    btn->box(FL_NO_BOX);
    btn->labeltype(FL_NO_LABEL);  // suppress FLTK's auto-label draw; we paint manually
    btn->labelsize(11);
    btn->user_data(reinterpret_cast<void*>(encode(kKindRhythm, bpm, idx)));
    btn->callback(rhythmBtnCb);  // single-arg
    return btn;
}

Fl_Button* ControlPanel::makeRegBtn(int x, int y, int w, int h,
                                     const char* label, int bank, int slot) {
    auto* btn = new Fl_Button(x, y, w, h, nullptr);
    btn->copy_label(label);
    btn->box(FL_NO_BOX);
    btn->labeltype(FL_NO_LABEL);  // suppress FLTK's auto-label draw; we paint manually
    btn->labelsize(11);
    btn->user_data(reinterpret_cast<void*>(encode(kKindReg, bank, slot)));
    btn->callback(regBtnCb);     // single-arg
    return btn;
}

// ── draw() ────────────────────────────────────────────────────────────────────
void ControlPanel::draw() {
    int px = x(), py = y(), pw = w(), ph = h();
    const int tabW = kTabStripW;

    // Panel background
    fl_color(kPanelBg);
    fl_rectf(px, py, pw, ph);

    // Top accent bar (1px highlight at very top)
    fl_color(fl_rgb_color(98, 100, 95));
    fl_line(px, py, px + pw - 1, py);

    // Content area: slightly lighter tint
    fl_color(fl_rgb_color(74, 76, 72));
    fl_rectf(px + tabW, py, pw - tabW, ph);

    // Vertical divider between tab strip and content (double line for depth)
    fl_color(fl_rgb_color(38, 39, 36));
    fl_line(px + tabW,     py + 2, px + tabW,     py + ph - 2);
    fl_color(fl_rgb_color(105, 107, 102));
    fl_line(px + tabW + 1, py + 2, px + tabW + 1, py + ph - 2);

    // ── Draw tab buttons (left strip) ─────────────────────────────────────
    const int tabH = ph / 4;
    for (int t = 0; t < 4; ++t) {
        int tx = px;
        int ty = py + t * tabH;
        int tw = tabW;
        int th = (t == 3) ? (ph - t * tabH) : tabH; // last tab takes remainder

        bool active = (t == _activeTab);

        // Tab background
        if (active) {
            // Active: two-tone silver header look
            fl_color(fl_color_average(kSectionHeader, FL_WHITE, 0.10f));
            fl_rectf(tx, ty, tw, th / 3);
            fl_color(kSectionHeader);
            fl_rectf(tx, ty + th / 3, tw, th - th / 3);
        } else {
            fl_color(kPanelBg);
            fl_rectf(tx, ty, tw, th);
        }

        // Left accent stripe (5px, category color, gradient effect: brighter top)
        fl_color(fl_color_average(kAccent[t], FL_WHITE, active ? 0.25f : 0.08f));
        fl_rectf(tx, ty, 5, th);
        fl_color(kAccent[t]);
        fl_rectf(tx + 1, ty + 2, 3, th - 4);

        // Bottom divider (between tabs)
        fl_color(fl_rgb_color(42, 43, 40));
        fl_line(tx + 4, ty + th - 1, tx + tw - 1, ty + th - 1);
        if (!active) {
            fl_color(fl_rgb_color(82, 84, 80));
            fl_line(tx + 4, ty + th - 2, tx + tw - 1, ty + th - 2);
        }

        // Tab text — two lines (English + Chinese)
        const char* tabLbl = kTabLabel[t];
        const char* nl2    = strchr(tabLbl, '\n');
        char eng[32] = {}, chn[32] = {};
        if (nl2) {
            snprintf(eng, std::min((int)(nl2 - tabLbl) + 1, 31), "%s", tabLbl);
            snprintf(chn, 31, "%s", nl2 + 1);
        } else {
            snprintf(eng, 31, "%s", tabLbl);
        }

        Fl_Color textCol = active ? fl_rgb_color(30, 30, 28)
                                  : fl_rgb_color(148, 148, 140);

        // English line (bold)
        fl_font(FL_HELVETICA_BOLD, active ? 11 : 10);
        fl_color(textCol);
        fl_draw(eng, tx + 10, ty + 6, tw - 14, 14,
                FL_ALIGN_LEFT | FL_ALIGN_CLIP);

        // Chinese line (smaller, below)
        if (nl2) {
            fl_font(FL_HELVETICA, 9);
            fl_color(active ? fl_rgb_color(55, 55, 52)
                            : fl_rgb_color(112, 112, 106));
            fl_draw(chn, tx + 10, ty + 20, tw - 14, 12,
                    FL_ALIGN_LEFT | FL_ALIGN_CLIP);
        }

        // Active tab: right-edge highlight (bridges into content area)
        if (active) {
            fl_color(kSectionHeader);
            fl_line(px + tabW + 1, ty + 1, px + tabW + 1, ty + th - 1);
        }
    }

    // ── Draw active tab's card buttons ────────────────────────────────────
    if (_activeTab == 0) {
        for (auto* btn : _upperVoiceBtns) {
            int prog = decodeId(reinterpret_cast<intptr_t>(btn->user_data()));
            drawCardButton(btn, kCardUpper, prog == _activeUpperVoice);
        }
    } else if (_activeTab == 1) {
        for (auto* btn : _lowerVoiceBtns) {
            int prog = decodeId(reinterpret_cast<intptr_t>(btn->user_data()));
            drawCardButton(btn, kCardLower, prog == _activeLowerVoice);
        }
    } else if (_activeTab == 2) {
        for (auto* btn : _rhythmBtns) {
            intptr_t ud = reinterpret_cast<intptr_t>(btn->user_data());
            int idx = decodeId(ud);
            if (idx == 99) {
                // Edit button — draw with blue accent
                drawCardButton(btn, fl_rgb_color(40, 65, 110), false);
            } else {
                drawCardButton(btn, kCardRhythm, idx == _activeRhythm);
            }
        }
    } else {
        // Registration: bank tabs (indices 0-4) + slots (5+)
        for (size_t i = 0; i < _regBtns.size(); ++i) {
            auto* btn   = _regBtns[i];
            intptr_t ud = reinterpret_cast<intptr_t>(btn->user_data());
            int extra   = decodeExtra(ud);
            int id      = decodeId(ud);
            if (extra == 255) {
                // Bank tab button
                bool active = (id == _activeRegBank);
                drawCardButton(btn, kCardRegBank, active);
            } else {
                // Slot button — active if this slot is selected in current bank
                bool active = (id == _activeRegSlot);
                Fl_Color slotBg = (id % 2 == 0)
                    ? fl_color_average(kCardRegSlot, FL_WHITE, 0.92f)
                    : kCardRegSlot;
                drawCardButton(btn, slotBg, active);
            }
        }
    }

    draw_children();
}

// ── Tab callback ──────────────────────────────────────────────────────────────
// Single-arg callback: user_data_ holds the encoded tab index.
// We recover `self` by walking up to the ControlPanel parent.
void ControlPanel::tabBtnCb(Fl_Widget* w, void*) {
    intptr_t ud = reinterpret_cast<intptr_t>(w->user_data());
    int tab = decodeId(ud);

    // Walk up the widget tree to find our ControlPanel
    auto* self = static_cast<ControlPanel*>(w->parent());
    if (!self) return;
    if (tab < 0 || tab >= 4 || !self->_tabGroups[tab]) return;
    if (tab == self->_activeTab) return;

    // Hide all, show selected
    for (int t = 0; t < 4; ++t)
        if (self->_tabGroups[t]) self->_tabGroups[t]->hide();
    self->_tabGroups[tab]->show();
    self->_activeTab = tab;
    self->redraw();
}

// ── Voice / Rhythm / Reg callbacks ───────────────────────────────────────────
// All use the single-arg form: recover self via parent chain.
// Tab buttons are direct children of ControlPanel (depth 1).
// Voice/Rhythm/Reg buttons are children of an Fl_Group content pane (depth 2).
static ControlPanel* selfFromChild(Fl_Widget* w) {
    // Try depth-1 first (tab buttons), then depth-2 (content group children)
    Fl_Widget* p1 = w->parent();
    if (!p1) return nullptr;
    if (auto* cp = dynamic_cast<ControlPanel*>(p1)) return cp;
    Fl_Widget* p2 = p1->parent();
    if (!p2) return nullptr;
    return dynamic_cast<ControlPanel*>(p2);
}

void ControlPanel::voiceBtnCb(Fl_Widget* w, void*) {
    auto* self = selfFromChild(w);
    if (!self) return;
    intptr_t ud  = reinterpret_cast<intptr_t>(w->user_data());
    int kind = decodeKind(ud);
    int prog = decodeId(ud);

    if (kind == kKindUpper) {
        self->_activeUpperVoice = prog;
        if (self->_upperVoiceCb) self->_upperVoiceCb(prog);
    } else {
        self->_activeLowerVoice = prog;
        if (self->_lowerVoiceCb) self->_lowerVoiceCb(prog);
    }
    self->redraw();
}

void ControlPanel::rhythmBtnCb(Fl_Widget* w, void*) {
    auto* self = selfFromChild(w);
    if (!self) return;
    intptr_t ud = reinterpret_cast<intptr_t>(w->user_data());
    int idx = decodeId(ud);
    int bpm = decodeExtra(ud);

    self->_activeRhythm = idx;

    static const char* kNames[] = {
        "\xe8\xbf\x9b\xe8\xa1\x8c\xe6\x9b\xb2",
        "\xe5\x8d\x8e\xe5\xb0\x94\xe5\x85\xb9",
        "\xe6\x91\x87\xe6\x91\x86",
        "\xe6\xb5\x81\xe8\xa1\x8c",
        "R&B",
        "\xe6\x8b\x89\xe4\xb8\x81",
        "\xe6\xb0\x91\xe4\xb9\x90"
    };
    if (self->_rhythmCb && idx >= 0 && idx < 7)
        self->_rhythmCb(kNames[idx], bpm);
    self->redraw();
}

void ControlPanel::regBtnCb(Fl_Widget* w, void*) {
    auto* self = selfFromChild(w);
    if (!self) return;
    intptr_t ud   = reinterpret_cast<intptr_t>(w->user_data());
    int extra = decodeExtra(ud);
    int id    = decodeId(ud);

    if (extra == 255) {
        // Bank tab clicked
        self->_activeRegBank = id;
        self->_activeRegSlot = -1;  // clear slot selection when bank changes
        // Update slot user_data bank field (slots start at index 5)
        for (size_t i = 5; i < self->_regBtns.size(); ++i) {
            intptr_t sud = reinterpret_cast<intptr_t>(self->_regBtns[i]->user_data());
            int slot = decodeId(sud);
            self->_regBtns[i]->user_data(
                reinterpret_cast<void*>(encode(kKindReg, id, slot)));
        }
    } else {
        int bank = extra;
        self->_activeRegSlot = id;
        if (self->_regCb) self->_regCb(bank, id);
    }
    self->redraw();
}

void ControlPanel::editBtnCb(Fl_Widget* w, void*) {
    auto* self = selfFromChild(w);
    if (!self) return;
    if (self->_rhythmEditCb) self->_rhythmEditCb();
}

// ── Public setters ─────────────────────────────────────────────────────────────
void ControlPanel::setActiveUpperVoice(int prog) {
    _activeUpperVoice = prog; redraw();
}
void ControlPanel::setActiveLowerVoice(int prog) {
    _activeLowerVoice = prog; redraw();
}
void ControlPanel::setActiveRhythm(int idx) {
    _activeRhythm = idx; redraw();
}

} // namespace em::ui
