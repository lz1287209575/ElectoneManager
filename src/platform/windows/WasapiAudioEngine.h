#pragma once
#ifdef EM_WINDOWS
// ─────────────────────────────────────────────────────────────────────────────
// WASAPI Exclusive-Mode Audio Engine
// Provides the lowest-latency path on Windows without requiring ASIO drivers.
// ─────────────────────────────────────────────────────────────────────────────
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include "common/IAudioEngine.h"

namespace em {

class WasapiAudioEngine : public IAudioEngine {
public:
    WasapiAudioEngine();
    ~WasapiAudioEngine() override;

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
    void audioThreadFunc();

    IMMDeviceEnumerator* _enumerator  = nullptr;
    IMMDevice*           _device      = nullptr;
    IAudioClient*        _audioClient = nullptr;
    IAudioRenderClient*  _renderClient= nullptr;
    HANDLE               _hEvent      = nullptr;

    std::thread          _audioThread;
    std::atomic<bool>    _running     {false};
    std::atomic<bool>    _stopFlag    {false};

    AudioRenderCallback  _renderCb;
    uint32_t             _bufferSize  = EM_BUFFER_SIZE;
    double               _sampleRate  = EM_SAMPLE_RATE;
    uint32_t             _actualBufferSize = 0;
    double               _actualSampleRate = 0.0;

    std::wstring         _deviceId;   // selected device ID (empty = default)
};

} // namespace em
#endif // EM_WINDOWS
