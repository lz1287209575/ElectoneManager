#pragma once
#if defined(EM_MACOS) || defined(EM_IOS)
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <string>
#include <vector>
#include <atomic>
#include "common/IAudioEngine.h"

namespace em {

class CoreAudioEngine : public IAudioEngine {
public:
    CoreAudioEngine();
    ~CoreAudioEngine() override;

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
    static OSStatus renderCallback(void*                       inRefCon,
                                   AudioUnitRenderActionFlags* ioActionFlags,
                                   const AudioTimeStamp*       inTimeStamp,
                                   UInt32                      inBusNumber,
                                   UInt32                      inNumberFrames,
                                   AudioBufferList*            ioData);

    AudioUnit            _audioUnit    = nullptr;
    AudioRenderCallback  _renderCb;
    std::atomic<bool>    _running      {false};
    uint32_t             _bufferSize   = EM_BUFFER_SIZE;
    double               _sampleRate   = EM_SAMPLE_RATE;

#ifdef EM_MACOS
    AudioDeviceID        _deviceId     = kAudioObjectUnknown;
#endif
};

} // namespace em
#endif // EM_MACOS || EM_IOS
