#pragma once
#ifdef EM_ANDROID
#include <oboe/Oboe.h>
#include <string>
#include <vector>
#include <atomic>
#include "common/IAudioEngine.h"

namespace em {

class OboeAudioEngine : public IAudioEngine,
                        public oboe::AudioStreamDataCallback {
public:
    OboeAudioEngine();
    ~OboeAudioEngine() override;

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

    // oboe::AudioStreamDataCallback
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* stream,
                                          void* audioData,
                                          int32_t numFrames) override;

private:
    std::shared_ptr<oboe::AudioStream> _stream;
    std::atomic<bool>    _running         {false};
    AudioRenderCallback  _renderCb;
    uint32_t             _bufferSize       = EM_BUFFER_SIZE;
    double               _sampleRate       = EM_SAMPLE_RATE;
    int32_t              _deviceId         = oboe::kUnspecified;
};

} // namespace em
#endif // EM_ANDROID
