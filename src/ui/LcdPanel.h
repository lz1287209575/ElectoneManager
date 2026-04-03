#pragma once
#include <FL/Fl_Widget.H>
#include <string>
#include <cstdint>
#include <functional>

namespace em::ui {

/// One voice slot on the LCD (e.g. Upper Voice 1)
struct VoiceSlot {
    std::string label;      ///< e.g. "UPPER KEYBOARD\nVOICE 1"
    std::string voiceName;  ///< e.g. "Strings 1"
    bool        active = false;
};

/// Full LCD state — mirrors what the real ELS-02C 7-inch screen shows
struct LcdState {
    // ── Voice slots (8 total, matching image 2 layout) ──────────────────
    // Right column (top to bottom in image): Upper KB V1, Upper KB V2,
    //   Lower KB V1, Lower KB V2, Lead V1, Lead V2, Pedal V1, Pedal V2
    VoiceSlot voices[8] = {
        {"UPPER KEYBOARD\nVOICE 1", "---", false},
        {"UPPER KEYBOARD\nVOICE 2", "---", false},
        {"LOWER KEYBOARD\nVOICE 1", "---", false},
        {"LOWER KEYBOARD\nVOICE 2", "---", false},
        {"LEAD VOICE 1",            "---", false},
        {"LEAD VOICE 2",            "---", false},
        {"PEDAL VOICE 1",           "---", false},
        {"PEDAL VOICE 2",           "---", false},
    };

    // ── Rhythm / transport (left column) ────────────────────────────────
    std::string rhythmName  = "Simple 8Beat Pop";
    int         tempo       = 120;
    int         bar         = 1;
    int         beat        = 1;
    int         beatsPerBar = 4;    ///< Time signature numerator
    bool        playing     = false;

    // ── Registration ────────────────────────────────────────────────────
    int         regBank     = 1;    ///< A=1..E=5
    int         regSlot     = 1;    ///< 1..16

    // ── LCD page tabs ─────────────────────────────────────────────────────
    int         activePage  = 1;    ///< 1=音色, 2=乐谱, 3=节奏

    // ── Page 2: 乐谱 / MIDI 回放状态 ─────────────────────────────────────
    std::string midiFileName;       ///< 当前加载的 .mid 文件名
    float       midiProgress = 0.f; ///< 0.0-1.0 进度
    bool        midiFileLoaded = false;

    // ── Page 3: 节奏状态 (简化显示) ───────────────────────────────────────
    std::string patternName;        ///< 当前 pattern 名
    int         patternSteps = 16;
    int         currentStep  = -1;  ///< 播放光标位置
    uint16_t    patternBits[12] = {}; ///< 位图: 每 bit = 1 step, 最多 16 步

    // ── System status ────────────────────────────────────────────────────
    bool        midiConnected = false;
    bool        audioRunning  = false;
    bool        underrun      = false;
    double      latencyMs     = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
/// ELS-02C LCD panel simulation.
/// Pure display widget — no interactive controls.
/// Layout mirrors the real instrument (see product photo).
class LcdPanel : public Fl_Widget {
public:
    LcdPanel(int x, int y, int w, int h);

    void setState(const LcdState& s) { _state = s; redraw(); }
    const LcdState& state() const    { return _state; }

    // Convenience setters
    void setVoiceName(int slot, const std::string& name) {
        if (slot >= 0 && slot < 8) { _state.voices[slot].voiceName = name; redraw(); }
    }
    void setVoiceActive(int slot, bool a) {
        if (slot >= 0 && slot < 8) { _state.voices[slot].active = a; redraw(); }
    }
    void setRhythm(const std::string& name, int bpm = 120) {
        _state.rhythmName = name; _state.tempo = bpm; redraw();
    }
    void setPlaying(bool p)       { _state.playing = p; redraw(); }
    void setBarBeat(int b, int bt){ _state.bar = b; _state.beat = bt; redraw(); }
    void setRegBank(int b)        { _state.regBank = b; redraw(); }
    void setRegSlot(int s)        { _state.regSlot = s; redraw(); }
    void setMidiConnected(bool v) { _state.midiConnected = v; redraw(); }
    void setAudioRunning(bool v)  { _state.audioRunning = v; redraw(); }
    void setUnderrun(bool v)      { _state.underrun = v; redraw(); }
    void setLatency(double ms)    { _state.latencyMs = ms; redraw(); }

    // Legacy shims so ElectoneWindow compiles unchanged
    void setUpperVoice(const std::string& name, int = -1) { setVoiceName(0, name); }
    void setLowerVoice(const std::string& name, int = -1) { setVoiceName(2, name); }

    void draw()   override;
    int  handle(int event) override;

    /// Called with slot index (0–7) when user double-clicks a voice card.
    using EditCallback = std::function<void(int slot)>;
    void setEditCallback(EditCallback cb) { _editCb = std::move(cb); }

    /// Called when the user clicks a page tab (1, 2, or 3).
    using PageChangeCallback = std::function<void(int page)>;
    void setPageChangeCallback(PageChangeCallback cb) { _pageChangeCb = std::move(cb); }

    /// Called when the user interacts with page 2 transport controls.
    /// action: "play", "stop", "rewind", or "seek" (frac in [0,1] for seek).
    using PageActionCallback = std::function<void(const std::string& action, float frac)>;
    void setPageActionCallback(PageActionCallback cb) { _pageActionCb = std::move(cb); }

private:
    LcdState _state;
    EditCallback       _editCb;
    PageChangeCallback _pageChangeCb;
    PageActionCallback _pageActionCb;

    // Draw helpers
    void drawHeader    (int x, int y, int w, int h);
    void drawVoiceGrid (int x, int y, int w, int h);
    void drawVoiceCard (int x, int y, int w, int h, int slot);
    void drawScorePage (int x, int y, int w, int h);
    void drawRhythmPage(int x, int y, int w, int h);
    void drawInfoRow   (int x, int y, int w, int h);
    void drawBottomRow (int x, int y, int w, int h);

    /// Returns voice-slot index (0-7) if (px,py) is inside a card, else -1.
    int voiceCardAtPos(int px, int py) const noexcept;

    /// Returns page-tab index (1-3) if (px,py) is inside a tab, else 0.
    int pageTabAtPos(int px, int py) const noexcept;

    /// Returns transport button index if (px,py) hits page-2 controls, else -1.
    /// 0=rewind, 1=play/pause, 2=stop.  Sets outFrac for progress-bar clicks.
    int scoreButtonAtPos(int px, int py, float& outFrac) const noexcept;

    // Cached screen-inset geometry (set in draw, read in handle)
    mutable int _sx = 0, _sy = 0, _sw = 0, _sh = 0;
    mutable int _kHdr = 22;
    mutable int _gridH = 0;
};

} // namespace em::ui
