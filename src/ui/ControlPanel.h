#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <string>
#include <vector>
#include <functional>

namespace em::ui {

using VoiceCallback    = std::function<void(int programBase)>;
using RhythmCallback   = std::function<void(const std::string& name, int bpm)>;
using RegisterCallback = std::function<void(int bank, int slot)>;
using RhythmEditCallback = std::function<void()>;  ///< Called when "Edit" button is pressed

// ─────────────────────────────────────────────────────────────────────────────
// ELS-02C Style Control Panel  (tabbed layout)
//
// Layout:
//   [TAB STRIP 130px] | [CONTENT AREA: remaining width]
//
// Four tabs: UPPER VOICE / LOWER VOICE / RHYTHM / REGISTRATION
// Each tab shows its content as a grid of LCD-style card buttons.
// ─────────────────────────────────────────────────────────────────────────────
class ControlPanel : public Fl_Group {
public:
    ControlPanel(int x, int y, int w, int h);

    /// Called when an Upper Voice button is clicked
    void setUpperVoiceCallback(VoiceCallback cb) { _upperVoiceCb = std::move(cb); }
    /// Called when a Lower Voice button is clicked
    void setLowerVoiceCallback(VoiceCallback cb) { _lowerVoiceCb = std::move(cb); }
    /// Legacy: forwards to upper voice callback
    void setVoiceCallback   (VoiceCallback    cb) { _upperVoiceCb = std::move(cb); }
    void setRhythmCallback  (RhythmCallback   cb) { _rhythmCb  = std::move(cb); }
    void setRegCallback     (RegisterCallback cb) { _regCb     = std::move(cb); }
    void setRhythmEditCallback(RhythmEditCallback cb) { _rhythmEditCb = std::move(cb); }

    void setActiveUpperVoice(int programBase);
    void setActiveLowerVoice(int programBase);
    void setActiveRhythm    (int idx);

    void draw() override;

private:
    // ── Tab content builders ──────────────────────────────────────────────
    void buildTabContent(int tab, int x, int y, int w, int h);

    // ── Internal helpers ──────────────────────────────────────────────────
    Fl_Button* makeVoiceBtn (int x, int y, int w, int h,
                              const char* label, int programBase, bool isUpper);
    Fl_Button* makeRhythmBtn(int x, int y, int w, int h,
                              const char* label, int idx, int bpm);
    Fl_Button* makeRegBtn   (int x, int y, int w, int h,
                              const char* label, int bank, int slot);

    static void voiceBtnCb (Fl_Widget* w, void* data);
    static void rhythmBtnCb(Fl_Widget* w, void* data);
    static void regBtnCb   (Fl_Widget* w, void* data);
    static void tabBtnCb   (Fl_Widget* w, void* data);
    static void editBtnCb  (Fl_Widget* w, void* data);

    // ── Callbacks ─────────────────────────────────────────────────────────
    VoiceCallback    _upperVoiceCb;
    VoiceCallback    _lowerVoiceCb;
    RhythmCallback   _rhythmCb;
    RegisterCallback _regCb;
    RhythmEditCallback _rhythmEditCb;

    // ── Tab state ─────────────────────────────────────────────────────────
    int          _activeTab  = 0;
    Fl_Group*    _tabGroups[4] = {};
    std::vector<Fl_Button*> _tabBtns;   // 4 invisible hit-area buttons for tab strip

    // ── Button tracking ───────────────────────────────────────────────────
    std::vector<Fl_Button*> _upperVoiceBtns;
    std::vector<Fl_Button*> _lowerVoiceBtns;
    std::vector<Fl_Button*> _rhythmBtns;
    std::vector<Fl_Button*> _regBtns;   // [0..4] = bank selectors, [5..] = slots

    int _activeUpperVoice = -1;
    int _activeLowerVoice = -1;
    int _activeRhythm     = -1;
    int _activeRegBank    = 0;
    int _activeRegSlot    = -1;

    // ── Layout constants ──────────────────────────────────────────────────
    static constexpr int kTabStripW = 130;
};

} // namespace em::ui
