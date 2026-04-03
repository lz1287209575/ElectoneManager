#pragma once
#ifdef EM_WINDOWS
// ─────────────────────────────────────────────────────────────────────────────
// ASIO Audio Engine (Yamaha Steinberg USB / generic ASIO driver)
//
// Requires the Steinberg ASIO SDK headers.  Place them at:
//   third_party/asiosdk/common/asiosys.h
//   third_party/asiosdk/common/asio.h
//   third_party/asiosdk/host/asiodrivers.h
//   third_party/asiosdk/host/pc/asiolist.h
//
// Download from: https://www.steinberg.net/developers/
// ─────────────────────────────────────────────────────────────────────────────
#include <windows.h>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include "common/IAudioEngine.h"

// Only compile if ASIO SDK is present
#if __has_include(<asiosys.h>)
#include <asiosys.h>
#include <asio.h>
#include <asiodrivers.h>
#define EM_ASIO_AVAILABLE 1
#endif

namespace em {

class AsioAudioEngine : public IAudioEngine {
public:
    AsioAudioEngine();
    ~AsioAudioEngine() override;

    std::vector<AudioDeviceInfo> getAvailableDevices() const override;
    bool selectDevice(unsigned int deviceIndex) override;
    bool selectDefaultYamahaDevice() override;
    void setBufferSize(uint32_t frames) override;
    void setSampleRate(double rate) override;
    void setRenderCallback(AudioRenderCallback cb) override;
    bool startStream() override;
    void stopStream() override;
    bool isRunning() const override;
    uint32_t getActualBufferSize() const override;
    double   getActualSampleRate() const override;
    double   getLatencyMs() const override;

private:
#ifdef EM_ASIO_AVAILABLE
    static void asioBufferSwitch(long doubleBufferIndex, ASIOBool directProcess);
    static ASIOTime* asioBufferSwitchTimeInfo(ASIOTime* timeInfo,
                                              long index, ASIOBool directProcess);
    static void asioSampleRateDidChange(ASIOSampleRate sRate);
    static long asioMessage(long selector, long value,
                            void* message, double* opt);

    IASIO*               _asio         = nullptr;
    ASIOBufferInfo       _bufferInfos[4] {};
    ASIOChannelInfo      _channelInfos[4] {};
    ASIOCallbacks        _callbacks    {};
    long                 _numChannels  = 2;
    static AsioAudioEngine* s_instance;
#endif

    std::atomic<bool>    _running      {false};
    AudioRenderCallback  _renderCb;
    uint32_t             _bufferSize   = EM_BUFFER_SIZE;
    double               _sampleRate   = EM_SAMPLE_RATE;
    uint32_t             _actualBufferSize = 0;
    double               _actualSampleRate = 0.0;
    int                  _selectedDriver   = -1;
};

} // namespace em
#endif // EM_WINDOWS
