#if defined(EM_MACOS) || defined(EM_IOS)
#include "CoreAudioEngine.h"
#include "common/AudioDeviceScanner.h"
#include <stdexcept>
#include <cstring>
#include <vector>

#ifdef EM_IOS
#include <AVFoundation/AVFoundation.h>
#endif

namespace em {

// ──────────────────────────────────────────────────────────────────────────────
CoreAudioEngine::CoreAudioEngine() {}

CoreAudioEngine::~CoreAudioEngine() {
    stopStream();
}

// ── Device enumeration (macOS only; iOS has single hardware device) ───────────

std::vector<AudioDeviceInfo> CoreAudioEngine::getAvailableDevices() const {
    std::vector<AudioDeviceInfo> result;

#ifdef EM_MACOS
    AudioObjectPropertyAddress addr = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    UInt32 dataSize = 0;
    AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &addr,
                                   0, nullptr, &dataSize);
    uint32_t count = dataSize / sizeof(AudioDeviceID);
    std::vector<AudioDeviceID> ids(count);
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr,
                               0, nullptr, &dataSize, ids.data());

    for (uint32_t i = 0; i < count; ++i) {
        // Check output channels
        AudioObjectPropertyAddress outAddr = {
            kAudioDevicePropertyStreams,
            kAudioDevicePropertyScopeOutput,
            kAudioObjectPropertyElementMain
        };
        UInt32 streamSize = 0;
        AudioObjectGetPropertyDataSize(ids[i], &outAddr, 0, nullptr, &streamSize);
        if (streamSize == 0) continue; // input-only device

        // Get name
        CFStringRef cfName = nullptr;
        AudioObjectPropertyAddress nameAddr = {
            kAudioObjectPropertyName,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        UInt32 nameSize = sizeof(CFStringRef);
        AudioObjectGetPropertyData(ids[i], &nameAddr, 0, nullptr,
                                   &nameSize, &cfName);
        std::string name = "Unknown Device";
        if (cfName) {
            char buf[256] = {};
            CFStringGetCString(cfName, buf, sizeof(buf), kCFStringEncodingUTF8);
            name = buf;
            CFRelease(cfName);
        }

        AudioDeviceInfo info;
        info.name  = name;
        info.index = i;
        info.defaultSampleRate = _sampleRate;
        result.push_back(info);
    }
#else
    // iOS: single output device
    AudioDeviceInfo info;
    info.name  = "iOS Hardware Output";
    info.index = 0;
    info.defaultSampleRate = 48000.0;
    result.push_back(info);
#endif

    return result;
}

bool CoreAudioEngine::selectDevice(unsigned int deviceIndex) {
#ifdef EM_MACOS
    auto devices = getAvailableDevices();
    if (deviceIndex >= devices.size()) return false;
    // Re-enumerate raw IDs to get AudioDeviceID
    AudioObjectPropertyAddress addr = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    UInt32 dataSize = 0;
    AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &addr,
                                   0, nullptr, &dataSize);
    uint32_t count = dataSize / sizeof(AudioDeviceID);
    std::vector<AudioDeviceID> ids(count);
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr,
                               0, nullptr, &dataSize, ids.data());
    if (deviceIndex < count) {
        _deviceId = ids[deviceIndex];
        return true;
    }
#endif
    return false;
}

bool CoreAudioEngine::selectDefaultYamahaDevice() {
    auto devices = getAvailableDevices();
    int idx = AudioDeviceScanner::findYamahaDevice(devices);
    if (idx >= 0) return selectDevice(static_cast<unsigned int>(idx));
    return false; // use system default
}

// ── Stream configuration ──────────────────────────────────────────────────────

void CoreAudioEngine::setBufferSize(uint32_t frames) { _bufferSize = frames; }
void CoreAudioEngine::setSampleRate(double rate)      { _sampleRate = rate; }
void CoreAudioEngine::setRenderCallback(AudioRenderCallback cb) {
    _renderCb = std::move(cb);
}

// ── Start / stop ──────────────────────────────────────────────────────────────

bool CoreAudioEngine::startStream() {
    if (_running) return true;

#ifdef EM_IOS
    // Configure AVAudioSession for low-latency I/O
    [[AVAudioSession sharedInstance]
        setCategory:AVAudioSessionCategoryPlayback
        mode:AVAudioSessionModeMeasurement
        options:AVAudioSessionCategoryOptionMixWithOthers
        error:nil];
    [[AVAudioSession sharedInstance]
        setPreferredIOBufferDuration:_bufferSize / _sampleRate
        error:nil];
    [[AVAudioSession sharedInstance] setActive:YES error:nil];
#endif

    // Describe the RemoteIO / DefaultOutput Audio Unit
    AudioComponentDescription desc = {};
    desc.componentType    = kAudioUnitType_Output;
#ifdef EM_IOS
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
#else
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
#endif
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
    if (!comp) return false;

    OSStatus st = AudioComponentInstanceNew(comp, &_audioUnit);
    if (st != noErr) return false;

#ifdef EM_MACOS
    // Assign selected device (if any)
    if (_deviceId != kAudioObjectUnknown) {
        AudioUnitSetProperty(_audioUnit,
                             kAudioOutputUnitProperty_CurrentDevice,
                             kAudioUnitScope_Global, 0,
                             &_deviceId, sizeof(_deviceId));
    }
    // Set buffer size
    AudioUnitSetProperty(_audioUnit,
                         kAudioDevicePropertyBufferFrameSize,
                         kAudioUnitScope_Global, 0,
                         &_bufferSize, sizeof(_bufferSize));
#endif

    // Set stream format: 32-bit float, stereo, non-interleaved
    AudioStreamBasicDescription fmt = {};
    fmt.mSampleRate       = _sampleRate;
    fmt.mFormatID         = kAudioFormatLinearPCM;
    fmt.mFormatFlags      = kAudioFormatFlagIsFloat |
                            kAudioFormatFlagIsPacked |
                            kAudioFormatFlagIsNonInterleaved;
    fmt.mBitsPerChannel   = 32;
    fmt.mChannelsPerFrame = 2;
    fmt.mFramesPerPacket  = 1;
    fmt.mBytesPerFrame    = sizeof(float);
    fmt.mBytesPerPacket   = sizeof(float);
    AudioUnitSetProperty(_audioUnit, kAudioUnitProperty_StreamFormat,
                         kAudioUnitScope_Input, 0, &fmt, sizeof(fmt));

    // Register render callback
    AURenderCallbackStruct cbStruct = { renderCallback, this };
    AudioUnitSetProperty(_audioUnit, kAudioUnitProperty_SetRenderCallback,
                         kAudioUnitScope_Input, 0, &cbStruct, sizeof(cbStruct));

    AudioUnitInitialize(_audioUnit);
    st = AudioOutputUnitStart(_audioUnit);
    if (st != noErr) return false;

    _running = true;
    return true;
}

void CoreAudioEngine::stopStream() {
    if (!_running) return;
    AudioOutputUnitStop(_audioUnit);
    AudioUnitUninitialize(_audioUnit);
    AudioComponentInstanceDispose(_audioUnit);
    _audioUnit = nullptr;
    _running   = false;
}

bool     CoreAudioEngine::isRunning()           const { return _running; }
uint32_t CoreAudioEngine::getActualBufferSize()  const { return _bufferSize; }
double   CoreAudioEngine::getActualSampleRate()  const { return _sampleRate; }
double   CoreAudioEngine::getLatencyMs()         const {
    return (_bufferSize / _sampleRate) * 1000.0;
}

// ── Render callback ───────────────────────────────────────────────────────────

OSStatus CoreAudioEngine::renderCallback(
        void*                       inRefCon,
        AudioUnitRenderActionFlags* /*ioActionFlags*/,
        const AudioTimeStamp*       /*inTimeStamp*/,
        UInt32                      /*inBusNumber*/,
        UInt32                      inNumberFrames,
        AudioBufferList*            ioData) {
    auto* self = static_cast<CoreAudioEngine*>(inRefCon);
    if (!self || !self->_renderCb) {
        // Silence
        for (UInt32 ch = 0; ch < ioData->mNumberBuffers; ++ch)
            memset(ioData->mBuffers[ch].mData, 0,
                   ioData->mBuffers[ch].mDataByteSize);
        return noErr;
    }

    // Provide interleaved stereo scratch buffer, then de-interleave into CoreAudio's
    // non-interleaved buffers.
    thread_local std::vector<float> scratch;
    scratch.assign(inNumberFrames * 2, 0.0f);

    self->_renderCb(scratch.data(), inNumberFrames, self->_sampleRate);

    // De-interleave L/R
    if (ioData->mNumberBuffers >= 2) {
        auto* L = static_cast<float*>(ioData->mBuffers[0].mData);
        auto* R = static_cast<float*>(ioData->mBuffers[1].mData);
        for (UInt32 f = 0; f < inNumberFrames; ++f) {
            L[f] = scratch[f * 2 + 0];
            R[f] = scratch[f * 2 + 1];
        }
    } else if (ioData->mNumberBuffers == 1) {
        // Mono fallback: copy interleaved directly
        memcpy(ioData->mBuffers[0].mData, scratch.data(),
               inNumberFrames * 2 * sizeof(float));
    }

    return noErr;
}

} // namespace em
#endif // EM_MACOS || EM_IOS
