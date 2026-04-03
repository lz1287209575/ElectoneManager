#ifdef EM_WINDOWS
#include "AsioAudioEngine.h"
#include "common/AudioDeviceScanner.h"
#include <stdexcept>
#include <cstring>
#include <vector>

namespace em {

#ifdef EM_ASIO_AVAILABLE
AsioAudioEngine* AsioAudioEngine::s_instance = nullptr;
#endif

// ──────────────────────────────────────────────────────────────────────────────
AsioAudioEngine::AsioAudioEngine() {
    CoInitialize(nullptr);
#ifdef EM_ASIO_AVAILABLE
    s_instance = this;
#endif
}

AsioAudioEngine::~AsioAudioEngine() {
    stopStream();
    CoUninitialize();
}

// ── Device enumeration ────────────────────────────────────────────────────────

std::vector<AudioDeviceInfo> AsioAudioEngine::getAvailableDevices() const {
    std::vector<AudioDeviceInfo> result;
#ifdef EM_ASIO_AVAILABLE
    // AsioDrivers enumerates installed ASIO drivers from the registry
    AsioDrivers drivers;
    char names[32][32] = {};
    long count = drivers.getDriverNames(reinterpret_cast<char**>(names), 32);
    for (long i = 0; i < count; ++i) {
        AudioDeviceInfo info;
        info.name  = names[i];
        info.index = static_cast<unsigned int>(i);
        info.defaultSampleRate = _sampleRate;
        result.push_back(info);
    }
#else
    // ASIO SDK not available — return placeholder
    AudioDeviceInfo info;
    info.name  = "[ASIO SDK not found — download from steinberg.net]";
    info.index = 0;
    result.push_back(info);
#endif
    return result;
}

bool AsioAudioEngine::selectDevice(unsigned int deviceIndex) {
    _selectedDriver = static_cast<int>(deviceIndex);
    return true;
}

bool AsioAudioEngine::selectDefaultYamahaDevice() {
    auto devices = getAvailableDevices();
    int idx = AudioDeviceScanner::findYamahaDevice(devices);
    if (idx >= 0) { _selectedDriver = idx; return true; }
    return false;
}

void AsioAudioEngine::setBufferSize(uint32_t frames) { _bufferSize = frames; }
void AsioAudioEngine::setSampleRate(double rate)      { _sampleRate = rate; }
void AsioAudioEngine::setRenderCallback(AudioRenderCallback cb) {
    _renderCb = std::move(cb);
}

// ── Start / stop ──────────────────────────────────────────────────────────────

bool AsioAudioEngine::startStream() {
    if (_running) return true;

#ifdef EM_ASIO_AVAILABLE
    AsioDrivers drivers;
    char names[32][32] = {};
    long count = drivers.getDriverNames(reinterpret_cast<char**>(names), 32);

    int driverIdx = (_selectedDriver >= 0 &&
                     _selectedDriver < count) ? _selectedDriver : 0;
    if (count == 0) return false;

    if (!drivers.loadDriver(names[driverIdx])) return false;
    if (ASIOInit(nullptr) != ASE_OK) return false;

    ASIOSetSampleRate(_sampleRate);

    long minBuf, maxBuf, prefBuf, gran;
    ASIOGetBufferSize(&minBuf, &maxBuf, &prefBuf, &gran);
    long chosenBuf = static_cast<long>(_bufferSize);
    if (chosenBuf < minBuf) chosenBuf = minBuf;
    if (chosenBuf > maxBuf) chosenBuf = maxBuf;
    _actualBufferSize = static_cast<uint32_t>(chosenBuf);

    // Set up L/R output buffer infos
    _bufferInfos[0].isInput    = ASIOFalse;
    _bufferInfos[0].channelNum = 0;
    _bufferInfos[1].isInput    = ASIOFalse;
    _bufferInfos[1].channelNum = 1;
    _numChannels = 2;

    _callbacks.bufferSwitch         = &AsioAudioEngine::asioBufferSwitch;
    _callbacks.sampleRateDidChange  = &AsioAudioEngine::asioSampleRateDidChange;
    _callbacks.asioMessage          = &AsioAudioEngine::asioMessage;
    _callbacks.bufferSwitchTimeInfo = &AsioAudioEngine::asioBufferSwitchTimeInfo;

    if (ASIOCreateBuffers(_bufferInfos, _numChannels, chosenBuf,
                          &_callbacks) != ASE_OK)
        return false;

    // Query channel format
    for (int i = 0; i < _numChannels; ++i) {
        _channelInfos[i].channel  = _bufferInfos[i].channelNum;
        _channelInfos[i].isInput  = ASIOFalse;
        ASIOGetChannelInfo(&_channelInfos[i]);
    }

    ASIOGetSampleRate(&_actualSampleRate);
    ASIOStart();
    _running = true;
    return true;
#else
    return false; // ASIO SDK not present
#endif
}

void AsioAudioEngine::stopStream() {
    if (!_running) return;
#ifdef EM_ASIO_AVAILABLE
    ASIOStop();
    ASIODisposeBuffers();
    ASIOExit();
#endif
    _running = false;
}

bool     AsioAudioEngine::isRunning()           const { return _running; }
uint32_t AsioAudioEngine::getActualBufferSize()  const { return _actualBufferSize; }
double   AsioAudioEngine::getActualSampleRate()  const { return _actualSampleRate; }
double   AsioAudioEngine::getLatencyMs()         const {
    if (_actualSampleRate <= 0) return 0.0;
    return (_actualBufferSize / _actualSampleRate) * 1000.0;
}

// ── ASIO callbacks ────────────────────────────────────────────────────────────

#ifdef EM_ASIO_AVAILABLE
void AsioAudioEngine::asioBufferSwitch(long index, ASIOBool /*directProcess*/) {
    asioBufferSwitchTimeInfo(nullptr, index, ASIOFalse);
}

ASIOTime* AsioAudioEngine::asioBufferSwitchTimeInfo(ASIOTime* /*timeInfo*/,
                                                     long index,
                                                     ASIOBool /*directProcess*/) {
    if (!s_instance || !s_instance->_renderCb) return nullptr;

    const uint32_t n = s_instance->_actualBufferSize;
    thread_local std::vector<float> scratch;
    scratch.assign(n * 2, 0.0f);
    s_instance->_renderCb(scratch.data(), n, s_instance->_actualSampleRate);

    // Write float data to ASIO 32-bit float buffers (ASIOSTFloat32LSB)
    for (int ch = 0; ch < s_instance->_numChannels && ch < 2; ++ch) {
        float* dst = static_cast<float*>(s_instance->_bufferInfos[ch].buffers[index]);
        for (uint32_t f = 0; f < n; ++f)
            dst[f] = scratch[f * 2 + ch];
    }

    ASIOOutputReady();
    return nullptr;
}

void AsioAudioEngine::asioSampleRateDidChange(ASIOSampleRate sRate) {
    if (s_instance) s_instance->_actualSampleRate = sRate;
}

long AsioAudioEngine::asioMessage(long /*selector*/, long /*value*/,
                                   void* /*message*/, double* /*opt*/) {
    return 0;
}
#endif

} // namespace em
#endif // EM_WINDOWS
