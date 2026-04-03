#ifdef EM_ANDROID
#include "OboeAudioEngine.h"
#include "common/AudioDeviceScanner.h"
#include <vector>
#include <cstring>

namespace em {

OboeAudioEngine::OboeAudioEngine() {}

OboeAudioEngine::~OboeAudioEngine() {
    stopStream();
}

// ── Device enumeration ────────────────────────────────────────────────────────
// On Android, AAudio/Oboe doesn't expose a rich device list via the NDK.
// Device enumeration is typically done in Java (AudioManager.getDevices).
// We return a single placeholder here; real selection uses setDeviceId from Java.

std::vector<AudioDeviceInfo> OboeAudioEngine::getAvailableDevices() const {
    return {{"Android Default Output", 0, 2, _sampleRate}};
}

bool OboeAudioEngine::selectDevice(unsigned int deviceIndex) {
    _deviceId = static_cast<int32_t>(deviceIndex);
    return true;
}

bool OboeAudioEngine::selectDefaultYamahaDevice() {
    // Cannot enumerate by name from NDK; rely on Java to set deviceId.
    return false;
}

void OboeAudioEngine::setBufferSize(uint32_t frames) { _bufferSize = frames; }
void OboeAudioEngine::setSampleRate(double rate)      { _sampleRate = rate; }
void OboeAudioEngine::setRenderCallback(AudioRenderCallback cb) {
    _renderCb = std::move(cb);
}

// ── Start / stop ──────────────────────────────────────────────────────────────

bool OboeAudioEngine::startStream() {
    if (_running) return true;

    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output)
           .setPerformanceMode(oboe::PerformanceMode::LowLatency)
           .setSharingMode(oboe::SharingMode::Exclusive)
           .setFormat(oboe::AudioFormat::Float)
           .setChannelCount(oboe::ChannelCount::Stereo)
           .setSampleRate(static_cast<int32_t>(_sampleRate))
           .setFramesPerDataCallback(static_cast<int32_t>(_bufferSize))
           .setDataCallback(this)
           .setSampleRateConversionQuality(
               oboe::SampleRateConversionQuality::Medium);

    if (_deviceId != oboe::kUnspecified)
        builder.setDeviceId(_deviceId);

    oboe::Result r = builder.openStream(_stream);
    if (r != oboe::Result::OK || !_stream) return false;

    r = _stream->start();
    if (r != oboe::Result::OK) {
        _stream->close();
        _stream.reset();
        return false;
    }

    _running = true;
    return true;
}

void OboeAudioEngine::stopStream() {
    if (!_running || !_stream) return;
    _stream->stop();
    _stream->close();
    _stream.reset();
    _running = false;
}

bool OboeAudioEngine::isRunning() const { return _running; }

uint32_t OboeAudioEngine::getActualBufferSize() const {
    if (_stream) return static_cast<uint32_t>(_stream->getFramesPerDataCallback());
    return _bufferSize;
}

double OboeAudioEngine::getActualSampleRate() const {
    if (_stream) return static_cast<double>(_stream->getSampleRate());
    return _sampleRate;
}

double OboeAudioEngine::getLatencyMs() const {
    if (!_stream) return 0.0;
    auto lat = _stream->calculateLatencyMillis();
    return (lat) ? lat.value() : (getActualBufferSize() / getActualSampleRate() * 1000.0);
}

// ── Oboe data callback (audio RT thread) ──────────────────────────────────────

oboe::DataCallbackResult OboeAudioEngine::onAudioReady(
        oboe::AudioStream* /*stream*/,
        void* audioData,
        int32_t numFrames) {
    auto* out = static_cast<float*>(audioData);
    const uint32_t n = static_cast<uint32_t>(numFrames);
    // Zero first
    memset(out, 0, n * 2 * sizeof(float));
    if (_renderCb)
        _renderCb(out, n, getActualSampleRate());
    return oboe::DataCallbackResult::Continue;
}

} // namespace em
#endif // EM_ANDROID
