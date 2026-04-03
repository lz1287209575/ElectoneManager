#ifdef EM_LINUX
#include "AlsaAudioEngine.h"
#include "common/AudioDeviceScanner.h"
#include <stdexcept>
#include <cstring>
#include <vector>
#include <pthread.h>
#include <sched.h>

namespace em {

AlsaAudioEngine::AlsaAudioEngine() {}

AlsaAudioEngine::~AlsaAudioEngine() {
    stopStream();
}

// ── Device enumeration ────────────────────────────────────────────────────────

std::vector<AudioDeviceInfo> AlsaAudioEngine::getAvailableDevices() const {
    std::vector<AudioDeviceInfo> result;
    void** hints = nullptr;
    if (snd_device_name_hint(-1, "pcm", &hints) < 0) return result;

    for (void** h = hints; *h != nullptr; ++h) {
        char* name = snd_device_name_get_hint(*h, "NAME");
        char* desc = snd_device_name_get_hint(*h, "DESC");
        char* ioid = snd_device_name_get_hint(*h, "IOID");

        // Only output (or bidirectional) devices
        if (!ioid || strcmp(ioid, "Input") != 0) {
            AudioDeviceInfo info;
            info.name  = name ? name : "unknown";
            info.index = static_cast<unsigned int>(result.size());
            info.defaultSampleRate = _sampleRate;
            result.push_back(info);
        }
        if (name) free(name);
        if (desc) free(desc);
        if (ioid) free(ioid);
    }
    snd_device_name_free_hint(hints);
    return result;
}

bool AlsaAudioEngine::selectDevice(unsigned int deviceIndex) {
    auto devices = getAvailableDevices();
    if (deviceIndex >= devices.size()) return false;
    _deviceName = devices[deviceIndex].name;
    return true;
}

bool AlsaAudioEngine::selectDefaultYamahaDevice() {
    auto devices = getAvailableDevices();
    int idx = AudioDeviceScanner::findYamahaDevice(devices);
    if (idx >= 0) { _deviceName = devices[idx].name; return true; }
    return false;
}

void AlsaAudioEngine::setBufferSize(uint32_t frames) { _bufferSize = frames; }
void AlsaAudioEngine::setSampleRate(double rate)      { _sampleRate = rate; }
void AlsaAudioEngine::setRenderCallback(AudioRenderCallback cb) {
    _renderCb = std::move(cb);
}

// ── Start / stop ──────────────────────────────────────────────────────────────

bool AlsaAudioEngine::startStream() {
    if (_running) return true;

    if (snd_pcm_open(&_pcm, _deviceName.c_str(),
                     SND_PCM_STREAM_PLAYBACK, 0) < 0)
        return false;

    snd_pcm_hw_params_t* hw = nullptr;
    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_hw_params_any(_pcm, hw);
    snd_pcm_hw_params_set_access(_pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(_pcm, hw, SND_PCM_FORMAT_FLOAT_LE);
    snd_pcm_hw_params_set_channels(_pcm, hw, 2);

    unsigned int rate = static_cast<unsigned int>(_sampleRate);
    snd_pcm_hw_params_set_rate_near(_pcm, hw, &rate, nullptr);
    _actualSampleRate = rate;

    snd_pcm_uframes_t period = _bufferSize;
    snd_pcm_hw_params_set_period_size_near(_pcm, hw, &period, nullptr);
    snd_pcm_uframes_t bufSize = period * 2; // double buffer
    snd_pcm_hw_params_set_buffer_size_near(_pcm, hw, &bufSize);

    if (snd_pcm_hw_params(_pcm, hw) < 0) {
        snd_pcm_close(_pcm);
        _pcm = nullptr;
        return false;
    }

    snd_pcm_uframes_t actualPeriod = 0;
    snd_pcm_get_params(_pcm, &bufSize, &actualPeriod);
    _actualBufferSize = static_cast<uint32_t>(actualPeriod);

    snd_pcm_sw_params_t* sw = nullptr;
    snd_pcm_sw_params_alloca(&sw);
    snd_pcm_sw_params_current(_pcm, sw);
    snd_pcm_sw_params_set_start_threshold(_pcm, sw, actualPeriod);
    snd_pcm_sw_params(_pcm, sw);

    snd_pcm_prepare(_pcm);

    _running  = true;
    _stopFlag = false;
    _thread   = std::thread(&AlsaAudioEngine::audioThreadFunc, this);
    return true;
}

void AlsaAudioEngine::stopStream() {
    if (!_running) return;
    _stopFlag = true;
    _running  = false;
    if (_thread.joinable()) _thread.join();
    if (_pcm) { snd_pcm_drain(_pcm); snd_pcm_close(_pcm); _pcm = nullptr; }
}

bool     AlsaAudioEngine::isRunning()           const { return _running; }
uint32_t AlsaAudioEngine::getActualBufferSize()  const { return _actualBufferSize; }
double   AlsaAudioEngine::getActualSampleRate()  const { return _actualSampleRate; }
double   AlsaAudioEngine::getLatencyMs()         const {
    if (_actualSampleRate <= 0) return 0.0;
    return (_actualBufferSize / _actualSampleRate) * 1000.0;
}

// ── Audio thread ──────────────────────────────────────────────────────────────

void AlsaAudioEngine::audioThreadFunc() {
    // Request SCHED_FIFO real-time priority
    sched_param sp {};
    sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);

    const uint32_t n = _actualBufferSize;
    std::vector<float> buf(n * 2, 0.0f);

    while (!_stopFlag) {
        std::fill(buf.begin(), buf.end(), 0.0f);
        if (_renderCb)
            _renderCb(buf.data(), n, _actualSampleRate);

        int ret = snd_pcm_writei(_pcm, buf.data(),
                                 static_cast<snd_pcm_uframes_t>(n));
        if (ret == -EPIPE) {
            // Underrun recovery
            snd_pcm_recover(_pcm, ret, 0);
        } else if (ret < 0) {
            snd_pcm_recover(_pcm, ret, 0);
        }
    }
}

} // namespace em
#endif // EM_LINUX
