#pragma once
#ifdef EM_LINUX
#include <alsa/asoundlib.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include "common/IAudioEngine.h"

namespace em {

class AlsaAudioEngine : public IAudioEngine {
public:
    AlsaAudioEngine();
    ~AlsaAudioEngine() override;

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

    snd_pcm_t*        _pcm            = nullptr;
    std::string       _deviceName     = "default";

    std::thread       _thread;
    std::atomic<bool> _running        {false};
    std::atomic<bool> _stopFlag       {false};

    AudioRenderCallback _renderCb;
    uint32_t          _bufferSize     = EM_BUFFER_SIZE;
    double            _sampleRate     = EM_SAMPLE_RATE;
    uint32_t          _actualBufferSize = 0;
    double            _actualSampleRate = 0.0;
};

} // namespace em
#endif // EM_LINUX
