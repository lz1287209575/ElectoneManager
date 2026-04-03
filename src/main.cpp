#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <cmath>

#include "common/IMidiDevice.h"
#include "common/IAudioEngine.h"
#include "common/MidiMessage.h"
#include "common/MidiToChineseInstrumentMapper.h"
#include "common/SamplerEngine.h"
#include "common/RingBuffer.h"

// ── Platform factory includes ──────────────────────────────────────────────
#if defined(EM_MACOS)
#  include "platform/apple/CoreMidiDevice.h"
#  include "platform/apple/CoreAudioEngine.h"
#elif defined(EM_IOS)
#  include "platform/apple/CoreMidiDevice.h"
#  include "platform/apple/CoreAudioEngine.h"
#elif defined(EM_WINDOWS)
#  include "platform/windows/WinMidiDevice.h"
#  include "platform/windows/WasapiAudioEngine.h"
#  include "platform/windows/AsioAudioEngine.h"
#elif defined(EM_LINUX)
#  include "platform/linux/AlsaMidiDevice.h"
#  include "platform/linux/AlsaAudioEngine.h"
#elif defined(EM_ANDROID)
#  include "platform/android/AndroidMidiDevice.h"
#  include "platform/android/OboeAudioEngine.h"
#endif

// ── UI layer (FLTK) ────────────────────────────────────────────────────────
#ifdef EM_UI_ENABLED
#  include <FL/Fl.H>
#  include "ui/ElectoneWindow.h"
#endif

// ──────────────────────────────────────────────────────────────────────────────
// Platform factory: creates the appropriate MIDI / audio objects
// ──────────────────────────────────────────────────────────────────────────────
static std::unique_ptr<em::IMidiDevice> makeMidi() {
#if defined(EM_MACOS) || defined(EM_IOS)
    return std::make_unique<em::CoreMidiDevice>();
#elif defined(EM_WINDOWS)
    return std::make_unique<em::WinMidiDevice>();
#elif defined(EM_LINUX)
    return std::make_unique<em::AlsaMidiDevice>();
#elif defined(EM_ANDROID)
    return std::make_unique<em::AndroidMidiDevice>();
#else
    return nullptr;
#endif
}

static std::unique_ptr<em::IAudioEngine> makeAudio() {
#if defined(EM_MACOS) || defined(EM_IOS)
    return std::make_unique<em::CoreAudioEngine>();
#elif defined(EM_WINDOWS)
    // Try ASIO first (lower latency), fall back to WASAPI
    return std::make_unique<em::WasapiAudioEngine>();
#elif defined(EM_LINUX)
    return std::make_unique<em::AlsaAudioEngine>();
#elif defined(EM_ANDROID)
    return std::make_unique<em::OboeAudioEngine>();
#else
    return nullptr;
#endif
}

// ──────────────────────────────────────────────────────────────────────────────
// Headless mode (no UI — original CLI behaviour)
// ──────────────────────────────────────────────────────────────────────────────
#ifndef EM_UI_ENABLED

static std::atomic<bool> g_quit{false};

void signalHandler(int) { g_quit = true; }

static std::vector<float> makeSineWave(double freq = 440.0,
                                        double sr   = 48000.0,
                                        double secs = 1.0) {
    size_t n = static_cast<size_t>(sr * secs);
    std::vector<float> buf(n);
    for (size_t i = 0; i < n; ++i)
        buf[i] = 0.5f * std::sin(2.0 * M_PI * freq * i / sr);
    return buf;
}

int main() {
    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "=== ElectoneManager v1.0 (headless) ===\n";

    auto midi  = makeMidi();
    auto audio = makeAudio();

    if (!midi && !audio) {
        std::cerr << "[ERROR] No platform defined. "
                     "Set EM_MACOS/EM_WINDOWS/EM_LINUX/EM_ANDROID.\n";
        return 1;
    }

    // MIDI ports
    if (midi) {
        auto ports = midi->getAvailablePorts();
        std::cout << "\nAvailable MIDI ports:\n";
        for (size_t i = 0; i < ports.size(); ++i)
            std::cout << "  [" << i << "] " << ports[i] << "\n";

        bool opened = !ports.empty() &&
                      (midi->openPortByName("Yamaha") ||
                       midi->openPortByName("Electone") ||
                       midi->openPort(0));
        if (opened)
            std::cout << "[MIDI] Opened: " << midi->getOpenPortName() << "\n";
    }

    em::MidiToChineseInstrumentMapper mapper;
    em::SamplerEngine sampler;

    auto wave = makeSineWave(440.0, EM_SAMPLE_RATE, 1.0);
    sampler.setWaveform(wave.data(), static_cast<uint32_t>(wave.size()), EM_SAMPLE_RATE);

    em::RingBuffer<em::MidiMessage, 256> midiQueue;

    if (midi) {
        midi->setMessageCallback([&](const em::MidiMessage& msg) {
            midiQueue.push(msg);
        });
        midi->startListening();
    }

    std::thread processingThread([&]() {
        em::MidiMessage msg;
        while (!g_quit) {
            while (midiQueue.pop(msg)) {
                mapper.process(msg);
                auto params = mapper.getParams();
                sampler.setPitchBend(params.pitchBend);
                sampler.setVibratoDepth(params.vibratoDepth);
                sampler.setFilterCutoff(params.filterCutoff);
                sampler.setExpressionGain(params.expressionGain);
                if (msg.isNoteOn()) {
                    std::cout << "[NOTE ON ] " << params.instrumentName
                              << " note=" << (int)msg.data1 << "\n";
                    sampler.noteOn(msg.data1, msg.data2);
                } else if (msg.isNoteOff()) {
                    sampler.noteOff(msg.data1);
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    });

    if (audio) {
        audio->setBufferSize(EM_BUFFER_SIZE);
        audio->setSampleRate(EM_SAMPLE_RATE);
        audio->selectDefaultYamahaDevice();
        audio->setRenderCallback([&](float* out, uint32_t n, double sr) {
            sampler.render(out, n, sr);
        });
        if (audio->startStream())
            std::cout << "[AUDIO] Latency: " << audio->getLatencyMs() << " ms\n";
    }

    std::cout << "Press Ctrl+C to quit.\n";
    while (!g_quit)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (midi)  { midi->stopListening(); midi->closePort(); }
    if (audio) { audio->stopStream(); }
    g_quit = true;
    processingThread.join();
    std::cout << "Bye.\n";
    return 0;
}

#else // EM_UI_ENABLED

// ──────────────────────────────────────────────────────────────────────────────
// GUI mode (FLTK window)
// ──────────────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    Fl::scheme("gtk+");

    // Dark theme tweaks
    Fl::background(22, 28, 42);
    Fl::background2(30, 38, 56);
    Fl::foreground(200, 210, 225);

    auto midi  = makeMidi();
    auto audio = makeAudio();

    auto* win = new em::ui::ElectoneWindow(std::move(midi), std::move(audio));
    win->show(argc, argv);
    win->startEngines();

    int ret = Fl::run();

    win->stopEngines();
    return ret;
}

#endif // EM_UI_ENABLED
