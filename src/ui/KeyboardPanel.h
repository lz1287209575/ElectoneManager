#pragma once
#include <FL/Fl_Widget.H>
#include <functional>
#include <array>
#include <cstdint>

namespace em::ui {

using NoteCallback = std::function<void(uint8_t note, uint8_t velocity, bool isOn)>;

/// Virtual piano keyboard widget (49 keys, C2-C6).
/// - Mouse click: note on/off
/// - Computer keyboard (QWERTY row) mapped to white keys
/// - External highlight via setNoteActive()
class KeyboardPanel : public Fl_Widget {
public:
    static constexpr int  kFirstNote  = 36;  // C2
    static constexpr int  kNumKeys    = 49;
    static constexpr int  kLastNote   = kFirstNote + kNumKeys - 1;

    KeyboardPanel(int x, int y, int w, int h, const char* label = nullptr);

    void setNoteCallback(NoteCallback cb)          { _cb = std::move(cb); }
    void setNoteActive(uint8_t note, bool active);
    void setDefaultVelocity(uint8_t v)             { _defaultVelocity = v; }
    /// Accent color used to tint the keyboard label strip
    void setAccentColor(Fl_Color c)                { _accentColor = c; redraw(); }

    int  handle(int event) override;
    void draw()   override;

private:
    // Layout helpers
    bool  isBlackKey(int noteOffset) const noexcept;
    int   whiteKeyCount()            const noexcept;
    int   whiteKeyIndex(int noteOff) const noexcept; // -1 if black
    float whiteKeyX(int whiteIdx)    const noexcept;
    float whiteKeyW()                const noexcept;
    float blackKeyW()                const noexcept;
    float blackKeyH()                const noexcept;
    int   noteAtPos(int px, int py)  const noexcept; // -1 if none

    void  triggerNote(int note, bool on);

    std::array<bool, 128>  _active{};
    NoteCallback           _cb;
    int                    _pressedNote     = -1;
    uint8_t                _defaultVelocity = 100;
    Fl_Color               _accentColor     = fl_rgb_color(60, 220, 230);

    // Computer keyboard → note offset mapping (white keys, starting C)
    static constexpr int kKeyMap[14] = {
        // z x c v b n m  ,  .
        // C D E F G A B  C  D
          0,2,4,5,7,9,11,12,14
    };
};

} // namespace em::ui
