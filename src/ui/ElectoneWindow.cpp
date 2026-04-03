#include "ElectoneWindow.h"
#include "StepSequencerWindow.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_File_Chooser.H>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace em::ui {

// Window dimensions
static constexpr int kWinW = 1100;
static constexpr int kWinH = 820;

// Row heights
static constexpr int kMenuH   = 22;
static constexpr int kLcdH    = 420;   // enlarged VOICE DISPLAY (absorbed ScorePanel's 120px)
static constexpr int kCtrlH   = 310;
static constexpr int kFootH   = 68;

// ──────────────────────────────────────────────────────────────────────────────
ElectoneWindow::ElectoneWindow(std::unique_ptr<em::IMidiDevice>  midi,
                                std::unique_ptr<em::IAudioEngine> audio)
    : Fl_Window(kWinW, kWinH, "ElectoneManager — Yamaha ELS-02C")
    , _midi(std::move(midi))
    , _audio(std::move(audio))
{
    color(fl_rgb_color(52, 53, 50));
    buildLayout();
    buildMenuBar();
    Fl::add_timeout(kTimerInterval, timerCb, this);
}

ElectoneWindow::~ElectoneWindow() {
    stopEngines();
    Fl::remove_timeout(timerCb, this);
    if (_voiceEditor) { delete _voiceEditor; _voiceEditor = nullptr; }
    if (_stepSeqWin) { delete _stepSeqWin; _stepSeqWin = nullptr; }
}

// ── Layout ────────────────────────────────────────────────────────────────────
void ElectoneWindow::buildLayout() {
    int curY = kMenuH;

    // LCD panel
    _lcd = new LcdPanel(0, curY, kWinW, kLcdH);
    // Double-click a voice card → open the voice editor for that slot
    _lcd->setEditCallback([this](int slot) {
        if (_voiceEditor) { _voiceEditor->hide(); delete _voiceEditor; _voiceEditor = nullptr; }
        _voiceEditor = new VoiceEditorWindow(slot, _lcdState.voices[slot].voiceName, &_slots[slot]);
        _voiceEditor->show();
    });
    // Page 2 transport actions (play/stop/rewind/seek)
    _lcd->setPageActionCallback([this](const std::string& action, float frac) {
        if (action == "play") {
            if (_lcdState.playing) {
                // Pause: stop transport
                _transport.stop();
                _lcdState.playing = false;
            } else {
                _transport.play();
                _lcdState.playing = true;
            }
            _lcd->setState(_lcdState);
        } else if (action == "stop") {
            _transport.stop();
            _lcdState.playing = false;
            _lcdState.bar  = 1;
            _lcdState.beat = 1;
            _lcd->setState(_lcdState);
            _midiPlayer.rewind();
        } else if (action == "rewind") {
            _midiPlayer.rewind();
            if (_midiPlayer.isLoaded()) {
                _transport.setPosition(0.f);
            }
            _lcdState.midiProgress = 0.f;
            _lcd->setState(_lcdState);
        } else if (action == "seek") {
            if (_midiPlayer.isLoaded()) {
                double totalBeats = _midiPlayer.totalBeats();
                _transport.setPosition(static_cast<float>(totalBeats * frac));
                _lcdState.midiProgress = frac;
                _lcd->setState(_lcdState);
            }
        }
    });
    curY += kLcdH;

    // Control panel (voice / rhythm / registration buttons)
    _ctrl = new ControlPanel(0, curY, kWinW, kCtrlH);
    // Upper voice selected -> update slot 0 on LCD
    _ctrl->setUpperVoiceCallback([this](int programBase) {
        _slots[0].allNotesOff();
        _lcdState.voices[0].voiceName =
            em::MidiToChineseInstrumentMapper::programToChineseInstrument(programBase);
        _lcd->setState(_lcdState);
    });
    // Lower voice selected -> update slot 2 on LCD
    _ctrl->setLowerVoiceCallback([this](int programBase) {
        _lcdState.voices[2].voiceName =
            em::MidiToChineseInstrumentMapper::programToChineseInstrument(programBase);
        _lcd->setState(_lcdState);
    });
    _ctrl->setRhythmCallback([this](const std::string& name, int bpm) {
        _lcdState.rhythmName = name;
        _lcdState.tempo      = bpm;
        _lcd->setState(_lcdState);

        // Load the corresponding drum pattern
        // Map rhythm names to pattern indices
        static const char* kNames[] = {
            "\xe8\xbf\x9b\xe8\xa1\x8c\xe6\x9b\xb2",  // March
            "\xe5\x8d\x8e\xe5\xb0\x94\xe5\x85\xb9",  // Waltz
            "\xe6\x91\x87\xe6\x91\x86",                // Swing
            "\xe6\xb5\x81\xe8\xa1\x8c",                // Pop
            "R&B",                                       // R&B
            "\xe6\x8b\x89\xe4\xb8\x81",                // Latin
            "\xe6\xb0\x91\xe4\xb9\x90"                 // Ethnic
        };
        int patIdx = 3; // default to Pop
        for (int i = 0; i < 7; ++i) {
            if (name == kNames[i]) { patIdx = i; break; }
        }

        auto pattern = em::DrumPatternBank::getPreset(patIdx);
        _drumSeq.setPattern(pattern);
        _transport.setTempo(static_cast<float>(bpm));
        _transport.setBeatsPerBar(pattern.beatsPerBar);

        // Update LCD page 3 rhythm state
        _lcdState.patternName  = pattern.name;
        _lcdState.patternSteps = pattern.steps;
        for (int r = 0; r < 12; ++r) {
            uint16_t bits = 0;
            for (int s = 0; s < std::min(16, pattern.steps); ++s) {
                if (pattern.grid[r][s] > 0) bits |= (1u << s);
            }
            _lcdState.patternBits[r] = bits;
        }
    });
    _ctrl->setRegCallback([this](int bank, int slot) {
        _lcdState.regBank = bank + 1;
        _lcdState.regSlot = slot + 1;
        _lcd->setState(_lcdState);
    });
    // Edit button → open StepSequencerWindow
    _ctrl->setRhythmEditCallback([this]() {
        if (_stepSeqWin) { _stepSeqWin->hide(); delete _stepSeqWin; _stepSeqWin = nullptr; }
        _stepSeqWin = new StepSequencerWindow(_drumSeq);
        _stepSeqWin->show();
    });
    curY += kCtrlH;

    // Footpad panel
    _footpad = new FootpadPanel(0, curY, kWinW, kFootH);
    _footpad->setExpressionCallback([this](float v) {
        for (int s = 0; s < kNumSlots; ++s)
            _slots[s].setExpressionGain(v);
        _lcd->setState(_lcdState);
    });
    // Connect foot switches: PLAY (id=3) and STOP (id=2)
    _footpad->setFootpadCallback([this](int pedalId, bool pressed) {
        if (!pressed) return;
        switch (pedalId) {
        case 3: // PLAY
            _transport.play();
            _lcdState.playing = true;
            _lcd->setState(_lcdState);
            break;
        case 2: // STOP
            _transport.stop();
            _lcdState.playing = false;
            _lcdState.bar  = 1;
            _lcdState.beat = 1;
            _lcd->setState(_lcdState);
            break;
        default:
            break;
        }
    });
    // Tempo counter callback
    _footpad->setTempoCallback([this](float bpm) {
        _transport.setTempo(bpm);
        _lcdState.tempo = static_cast<int>(bpm);
        _lcd->setState(_lcdState);
    });

    end();
    resizable(nullptr);
}

void ElectoneWindow::buildMenuBar() {
    _menuBar = new Fl_Menu_Bar(0, 0, kWinW, kMenuH);
    _menuBar->color(fl_rgb_color(18, 24, 42));
    _menuBar->selection_color(fl_rgb_color(45, 60, 110));
    _menuBar->textcolor(fl_rgb_color(195, 210, 235));
    _menuBar->textsize(11);
    _menuBar->add("文件/加载乐谱...",  FL_CTRL + 'o', menuCb, this);
    _menuBar->add("文件/退出",         FL_CTRL + 'q', menuCb, this);
    _menuBar->add("引擎/重新扫描 MIDI", 0,            menuCb, this);
    _menuBar->add("引擎/重启音频",      0,            menuCb, this);
    _menuBar->add("帮助/关于",          0,            menuCb, this);
}

// ── Start / stop engines ──────────────────────────────────────────────────────
bool ElectoneWindow::startEngines() {
    // Initialise slot 0 with one layer + default Strings voice
    _slots[0].addLayer();
    {
        auto& info = _slots[0].layerInfo(0);
        // Strings 1: 5 odd harmonics
        em::VoiceSlotEngine::generateWaveformWithHarmonics(
            5, info.waveformData, EM_SAMPLE_RATE, 440.0f);
        _slots[0].layer(0)->setWaveform(info.waveformData.data(),
                                         static_cast<uint32_t>(info.waveformData.size()),
                                         EM_SAMPLE_RATE);
        info.instrumentName = "Strings 1";
    }

    // ── Transport / Drum engine setup ─────────────────────────────────────
    // Load default drum pattern (Pop)
    auto defaultPattern = em::DrumPatternBank::getPreset(3);
    _drumSeq.setPattern(defaultPattern);
    _transport.setTempo(120.0f);
    _transport.setBeatsPerBar(4);

    // Populate LCD page 3 rhythm state from default pattern
    _lcdState.patternName  = defaultPattern.name;
    _lcdState.patternSteps = defaultPattern.steps;
    for (int r = 0; r < 12; ++r) {
        uint16_t bits = 0;
        for (int s = 0; s < std::min(16, defaultPattern.steps); ++s) {
            if (defaultPattern.grid[r][s] > 0) bits |= (1u << s);
        }
        _lcdState.patternBits[r] = bits;
    }

    // MIDI file player callbacks
    _midiPlayer.setNoteCallback([this](const em::MidiMessage& msg) {
        // Route MIDI file notes to slot 0 (upper keyboard)
        if (msg.isNoteOn()) {
            _slots[0].noteOn(msg.data1, msg.data2);
        } else if (msg.isNoteOff()) {
            _slots[0].noteOff(msg.data1);
        }
    });
    _midiPlayer.setTempoCallback([this](float bpm) {
        _transport.setTempo(bpm);
    });

    // Connect transport tick → drum sequencer → drum kit + MIDI file player
    _transport.setTickCallback([this](uint64_t tick) {
        // Drum sequencer
        em::TriggerBuffer triggers;
        triggers.clear();
        _drumSeq.onTick(tick, em::TransportEngine::kPPQN, triggers);
        for (int i = 0; i < triggers.count; ++i) {
            _drumKit.trigger(triggers.triggers[i].instrument,
                             triggers.triggers[i].velocity);
        }
        // MIDI file player
        _midiPlayer.onTick(tick);
    });

    // MIDI
    if (_midi) {
        auto ports = _midi->getAvailablePorts();
        bool opened = !ports.empty() &&
                      (_midi->openPortByName("Yamaha")  ||
                       _midi->openPortByName("Electone")||
                       _midi->openPort(0));

        _midi->setMessageCallback([this](const em::MidiMessage& msg) {
            _midiQueue.push(msg);
            _queue.postMidiMessage(msg);
        });
        if (opened) _midi->startListening();

        _lcdState.midiConnected = opened;
        _lcd->setState(_lcdState);
    }

    // Audio
    if (_audio) {
        _audio->setBufferSize(EM_BUFFER_SIZE);
        _audio->setSampleRate(EM_SAMPLE_RATE);
        _audio->selectDefaultYamahaDevice();
        _audio->setRenderCallback([this](float* out, uint32_t n, double sr) {
            std::memset(out, 0, n * sizeof(float));
            // Advance transport clock + trigger drums
            _transport.processAudioBlock(n, sr);
            // Render drum kit
            _drumKit.render(out, n, sr);
            // Render voice slots
            for (int s = 0; s < kNumSlots; ++s)
                _slots[s].render(out, n, sr);
        });
        bool ok = _audio->startStream();
        _lcdState.audioRunning = ok;
        _lcdState.latencyMs    = ok ? _audio->getLatencyMs() : 0.0;

        AudioState as;
        as.running    = ok;
        as.latencyMs  = _lcdState.latencyMs;
        as.sampleRate = ok ? _audio->getActualSampleRate() : 0.0;
        as.bufferSize = ok ? _audio->getActualBufferSize() : 0;
        _queue.postAudioState(as);
    }

    // Processing thread
    _stopProcessing = false;
    _processingThread = std::thread(&ElectoneWindow::processingThreadFunc, this);

    _lcd->setState(_lcdState);
    return true;
}

void ElectoneWindow::stopEngines() {
    // Stop transport first
    _transport.stop();

    _stopProcessing = true;
    if (_processingThread.joinable()) _processingThread.join();
    if (_midi)  { _midi->stopListening(); _midi->closePort(); }
    if (_audio) { _audio->stopStream(); }
}

// ── Processing thread ─────────────────────────────────────────────────────────
void ElectoneWindow::processingThreadFunc() {
    em::MidiMessage msg;
    while (!_stopProcessing) {
        while (_midiQueue.pop(msg)) {
            _mapper.process(msg);
            auto params = _mapper.getParams();
            _slots[0].setPitchBend(params.pitchBend);
            _slots[0].setVibratoDepth(params.vibratoDepth);
            _slots[0].setFilterCutoff(params.filterCutoff);
            _slots[0].setExpressionGain(params.expressionGain);

            if (msg.isNoteOn()) {
                _slots[0].noteOn(msg.data1, msg.data2);
                _queue.postNote(msg.data1, msg.data2, true);
            } else if (msg.isNoteOff()) {
                _slots[0].noteOff(msg.data1);
                _queue.postNote(msg.data1, 0, false);
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
}

// ── Timer (60 Hz UI refresh) ──────────────────────────────────────────────────
void ElectoneWindow::timerCb(void* data) {
    static_cast<ElectoneWindow*>(data)->onTimer();
    Fl::repeat_timeout(kTimerInterval, timerCb, data);
}

void ElectoneWindow::onTimer() {
    // Note events (keyboard panels removed; still drain queue)
    std::vector<NoteEvent> notes;
    _queue.pollNotes(notes);
    (void)notes;

    // MIDI messages for LCD
    std::vector<em::MidiMessage> msgs;
    _queue.pollMidiMessages(msgs);
    bool lcdDirty = false;
    for (auto& m : msgs) {
        auto params = _mapper.getParams();
        if (m.isProgramChange()) {
            // MIDI program change -> update Upper Voice 1 slot
            _lcdState.voices[0].voiceName = params.instrumentName;
            lcdDirty = true;
        }
    }

    // Audio state
    AudioState as;
    if (_queue.pollAudioState(as)) {
        _lcdState.audioRunning = as.running;
        _lcdState.latencyMs    = as.latencyMs;
        _lcdState.underrun     = as.underrun;
        if (_footpad) {
            _footpad->setAudioRunning(as.running);
            _footpad->setLatencyMs(as.latencyMs);
            _footpad->setUnderrun(as.underrun);
        }
        lcdDirty = true;
    }

    // Poll transport beat info
    em::BeatInfo beatInfo;
    if (_transport.pollBeatInfo(beatInfo)) {
        _lcdState.bar         = beatInfo.bar;
        _lcdState.beat        = beatInfo.beat;
        _lcdState.beatsPerBar = beatInfo.beatsPerBar;
        _lcdState.playing     = beatInfo.playing;
        _lcdState.tempo       = static_cast<int>(beatInfo.bpm);
        if (_footpad) _footpad->setTempo(beatInfo.bpm);

        // Update LCD page 2 progress
        if (_midiPlayer.isLoaded()) {
            _lcdState.midiProgress = _midiPlayer.getProgressFraction();
        }

        // Update LCD page 3 rhythm step cursor
        _lcdState.currentStep = _drumSeq.currentStep();

        // Update step sequencer cursor
        if (_stepSeqWin && _stepSeqWin->visible()) {
            _stepSeqWin->setCurrentStep(_drumSeq.currentStep());
        }

        lcdDirty = true;
    }

    if (lcdDirty && _lcd) _lcd->setState(_lcdState);
}

// ── Menu ──────────────────────────────────────────────────────────────────────
void ElectoneWindow::menuCb(Fl_Widget* w, void* data) {
    auto* self = static_cast<ElectoneWindow*>(data);
    auto* bar  = static_cast<Fl_Menu_Bar*>(w);
    const Fl_Menu_Item* item = bar->mvalue();
    if (item && item->label()) self->handleMenu(item->label());
}

void ElectoneWindow::handleMenu(const char* label) {
    if (!label) return;
    std::string l(label);
    if (l == "加载乐谱...") {
        const char* file = fl_file_chooser(
            "Open Score / MIDI File",
            "MIDI Files (*.mid)", "");
        if (file && *file) {
            if (_midiPlayer.loadFile(file)) {
                _midiPlayer.rewind();
                // Extract filename from path
                std::string path(file);
                auto pos = path.find_last_of("/\\");
                _lcdState.midiFileName   = (pos != std::string::npos) ? path.substr(pos + 1) : path;
                _lcdState.midiFileLoaded = true;
                _lcdState.midiProgress   = 0.f;
                _lcdState.activePage     = 2; // switch to score page
                _lcd->setState(_lcdState);
            }
        }
    } else if (l == "退出") {
        stopEngines();
        hide();
    } else if (l == "重新扫描 MIDI") {
        if (_midi) {
            auto ports = _midi->getAvailablePorts();
            (void)ports;
        }
    } else if (l == "重启音频") {
        if (_audio) {
            _audio->stopStream();
            _audio->startStream();
        }
    } else if (l == "关于") {
        fl_message("ElectoneManager v1.0\n"
                   "Yamaha ELS-02C 跨平台 C++ 音乐框架\n"
                   "FLTK UI + 原生 MIDI/Audio 引擎");
    }
}

} // namespace em::ui
