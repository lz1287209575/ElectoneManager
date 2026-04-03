#pragma once
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Slider.H>
#include <string>
#include <functional>
#include <vector>

namespace em::ui {

/// Playback transport callbacks from ScorePanel buttons.
using ScorePlayCallback    = std::function<void()>;            ///< Play/Pause
using ScoreStopCallback    = std::function<void()>;            ///< Stop
using ScoreSeekCallback    = std::function<void(float frac)>;  ///< Seek to fraction [0,1]

/// Score panel: loads and renders a MusicXML file, with playback transport bar.
/// Uses Lomse if available, otherwise shows a placeholder with file info.
class ScorePanel : public Fl_Group {
public:
    ScorePanel(int x, int y, int w, int h);
    ~ScorePanel() override;

    /// Load a MusicXML (.xml / .mxl) or MIDI (.mid) file. Returns true on success.
    bool loadMusicXML(const std::string& path);

    /// Scroll to a specific measure number (1-based).
    void scrollToMeasure(int measure);

    /// Highlight the current beat position (called from audio sync timer).
    /// beat is in beats from the beginning of the score.
    void setPlaybackPosition(double beatPosition);

    /// Set the progress bar position [0.0, 1.0].
    void setProgress(float fraction);

    /// Open a file dialog and load a score.
    void openFileDialog();

    const std::string& currentFile() const { return _currentFile; }
    bool isMidiFile() const { return _isMidiFile; }

    /// Transport callbacks
    void setPlayCallback(ScorePlayCallback cb)  { _playCb = std::move(cb); }
    void setStopCallback(ScoreStopCallback cb)  { _stopCb = std::move(cb); }
    void setSeekCallback(ScoreSeekCallback cb)   { _seekCb = std::move(cb); }

    void draw() override;
    int  handle(int event) override;

private:
    void drawPlaceholder();
    void drawScore();
    void drawPlayhead();
    void drawTransportBar();

    // Transport bar widgets
    Fl_Button* _playBtn  = nullptr;
    Fl_Button* _stopBtn  = nullptr;
    Fl_Button* _rewBtn   = nullptr;
    Fl_Button* _ffBtn    = nullptr;
    Fl_Slider* _progressSlider = nullptr;

    static void playBtnCb(Fl_Widget* w, void* data);
    static void stopBtnCb(Fl_Widget* w, void* data);
    static void rewBtnCb (Fl_Widget* w, void* data);
    static void ffBtnCb  (Fl_Widget* w, void* data);
    static void progressCb(Fl_Widget* w, void* data);

    std::string _currentFile;
    double      _playbackBeat    = 0.0;
    float       _progressFrac    = 0.0f;
    int         _scrollMeasure   = 1;
    bool        _scoreLoaded     = false;
    bool        _isMidiFile      = false;

    ScorePlayCallback _playCb;
    ScoreStopCallback _stopCb;
    ScoreSeekCallback _seekCb;

#ifdef EM_LOMSE_AVAILABLE
    void initLomse();
    void renderToBuffer();

    struct LomseImpl;
    LomseImpl* _lomse = nullptr;

    // Rendered bitmap (RGBA, width x height)
    std::vector<unsigned char> _bitmap;
    int _bitmapW = 0;
    int _bitmapH = 0;
#endif
};

} // namespace em::ui
