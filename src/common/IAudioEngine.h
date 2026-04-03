#pragma once
#include <functional>
#include <string>
#include <vector>
#include <cstdint>

namespace em {

/// Audio render callback signature.
/// @param outputBuffer  Interleaved stereo float samples [-1.0, 1.0]
/// @param numFrames     Number of sample frames to fill
/// @param sampleRate    Current sample rate in Hz
using AudioRenderCallback =
    std::function<void(float* outputBuffer, uint32_t numFrames, double sampleRate)>;

struct AudioDeviceInfo {
    std::string name;
    unsigned int index = 0;
    unsigned int maxOutputChannels = 0;
    double defaultSampleRate = 48000.0;
};

/// Abstract audio engine interface.
/// Each platform provides a concrete implementation.
class IAudioEngine {
public:
    virtual ~IAudioEngine() = default;

    /// Enumerate available output audio devices.
    virtual std::vector<AudioDeviceInfo> getAvailableDevices() const = 0;

    /// Select the output device by index.
    virtual bool selectDevice(unsigned int deviceIndex) = 0;

    /// Automatically find and select a Yamaha / Electone device.
    /// Returns true if found and selected, false if not found (uses default).
    virtual bool selectDefaultYamahaDevice() = 0;

    /// Set audio buffer size in frames (must be called before startStream).
    virtual void setBufferSize(uint32_t frames) = 0;

    /// Set sample rate (must be called before startStream).
    virtual void setSampleRate(double rate) = 0;

    /// Register the audio render callback.
    virtual void setRenderCallback(AudioRenderCallback cb) = 0;

    /// Open and start the audio stream.
    virtual bool startStream() = 0;

    /// Stop and close the audio stream.
    virtual void stopStream() = 0;

    /// True if the stream is currently running.
    virtual bool isRunning() const = 0;

    /// Actual buffer size in frames (may differ from requested).
    virtual uint32_t getActualBufferSize() const = 0;

    /// Actual sample rate (may differ from requested).
    virtual double getActualSampleRate() const = 0;

    /// Estimated round-trip latency in milliseconds.
    virtual double getLatencyMs() const = 0;
};

} // namespace em
