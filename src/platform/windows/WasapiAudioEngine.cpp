#ifdef EM_WINDOWS
#include "WasapiAudioEngine.h"
#include "common/AudioDeviceScanner.h"
#include <stdexcept>
#include <cstring>
#include <vector>
#include <Functiondiscoverykeys_devpkey.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

namespace em {

// ──────────────────────────────────────────────────────────────────────────────
WasapiAudioEngine::WasapiAudioEngine() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                     CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                     reinterpret_cast<void**>(&_enumerator));
}

WasapiAudioEngine::~WasapiAudioEngine() {
    stopStream();
    if (_enumerator) { _enumerator->Release(); _enumerator = nullptr; }
    CoUninitialize();
}

// ── Device enumeration ────────────────────────────────────────────────────────

std::vector<AudioDeviceInfo> WasapiAudioEngine::getAvailableDevices() const {
    std::vector<AudioDeviceInfo> result;
    if (!_enumerator) return result;

    IMMDeviceCollection* col = nullptr;
    _enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &col);
    if (!col) return result;

    UINT count = 0;
    col->GetCount(&count);
    for (UINT i = 0; i < count; ++i) {
        IMMDevice* dev = nullptr;
        col->Item(i, &dev);
        if (!dev) continue;

        IPropertyStore* store = nullptr;
        dev->OpenPropertyStore(STGM_READ, &store);
        std::string name = "Device " + std::to_string(i);
        if (store) {
            PROPVARIANT pv;
            PropVariantInit(&pv);
            if (SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &pv)) &&
                pv.vt == VT_LPWSTR) {
                int sz = WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1,
                                             nullptr, 0, nullptr, nullptr);
                std::string s(sz - 1, '\0');
                WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1,
                                    &s[0], sz, nullptr, nullptr);
                name = s;
            }
            PropVariantClear(&pv);
            store->Release();
        }

        AudioDeviceInfo info;
        info.name  = name;
        info.index = i;
        info.defaultSampleRate = _sampleRate;
        result.push_back(info);
        dev->Release();
    }
    col->Release();
    return result;
}

bool WasapiAudioEngine::selectDevice(unsigned int deviceIndex) {
    if (!_enumerator) return false;
    IMMDeviceCollection* col = nullptr;
    _enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &col);
    if (!col) return false;
    IMMDevice* dev = nullptr;
    col->Item(deviceIndex, &dev);
    col->Release();
    if (!dev) return false;

    LPWSTR id = nullptr;
    dev->GetId(&id);
    if (id) { _deviceId = id; CoTaskMemFree(id); }
    dev->Release();
    return true;
}

bool WasapiAudioEngine::selectDefaultYamahaDevice() {
    auto devices = getAvailableDevices();
    int idx = AudioDeviceScanner::findYamahaDevice(devices);
    if (idx >= 0) return selectDevice(static_cast<unsigned int>(idx));
    return false;
}

void WasapiAudioEngine::setBufferSize(uint32_t frames) { _bufferSize = frames; }
void WasapiAudioEngine::setSampleRate(double rate)      { _sampleRate = rate; }
void WasapiAudioEngine::setRenderCallback(AudioRenderCallback cb) {
    _renderCb = std::move(cb);
}

// ── Start / stop ──────────────────────────────────────────────────────────────

bool WasapiAudioEngine::startStream() {
    if (_running || !_enumerator) return false;

    // Get device
    if (!_deviceId.empty()) {
        _enumerator->GetDevice(_deviceId.c_str(), &_device);
    } else {
        _enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &_device);
    }
    if (!_device) return false;

    _device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                      nullptr, reinterpret_cast<void**>(&_audioClient));
    if (!_audioClient) return false;

    // Find exclusive-mode format (32-bit float preferred)
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag      = WAVE_FORMAT_IEEE_FLOAT;
    wfx.nChannels       = 2;
    wfx.nSamplesPerSec  = static_cast<DWORD>(_sampleRate);
    wfx.wBitsPerSample  = 32;
    wfx.nBlockAlign     = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    REFERENCE_TIME hnsReq =
        static_cast<REFERENCE_TIME>(10000000.0 * _bufferSize / _sampleRate);

    HRESULT hr = _audioClient->Initialize(
        AUDCLNT_SHAREMODE_EXCLUSIVE,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        hnsReq, hnsReq, &wfx, nullptr);

    if (FAILED(hr)) {
        // Fall back to shared mode if exclusive fails
        hr = _audioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            hnsReq, 0, &wfx, nullptr);
        if (FAILED(hr)) return false;
    }

    _hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    _audioClient->SetEventHandle(_hEvent);

    UINT32 bufFrames = 0;
    _audioClient->GetBufferSize(&bufFrames);
    _actualBufferSize = bufFrames;
    _actualSampleRate = _sampleRate;

    _audioClient->GetService(__uuidof(IAudioRenderClient),
                              reinterpret_cast<void**>(&_renderClient));
    if (!_renderClient) return false;

    _audioClient->Start();
    _running  = true;
    _stopFlag = false;
    _audioThread = std::thread(&WasapiAudioEngine::audioThreadFunc, this);
    return true;
}

void WasapiAudioEngine::stopStream() {
    if (!_running) return;
    _stopFlag = true;
    if (_hEvent) SetEvent(_hEvent);
    if (_audioThread.joinable()) _audioThread.join();
    if (_audioClient) { _audioClient->Stop(); _audioClient->Release(); _audioClient = nullptr; }
    if (_renderClient) { _renderClient->Release(); _renderClient = nullptr; }
    if (_device) { _device->Release(); _device = nullptr; }
    if (_hEvent) { CloseHandle(_hEvent); _hEvent = nullptr; }
    _running = false;
}

bool     WasapiAudioEngine::isRunning()           const { return _running; }
uint32_t WasapiAudioEngine::getActualBufferSize()  const { return _actualBufferSize; }
double   WasapiAudioEngine::getActualSampleRate()  const { return _actualSampleRate; }
double   WasapiAudioEngine::getLatencyMs()         const {
    if (_actualSampleRate <= 0) return 0.0;
    return (_actualBufferSize / _actualSampleRate) * 1000.0;
}

// ── Audio thread ──────────────────────────────────────────────────────────────

void WasapiAudioEngine::audioThreadFunc() {
    // Raise thread priority to real-time
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    thread_local std::vector<float> scratch;

    while (!_stopFlag) {
        WaitForSingleObject(_hEvent, 2000);
        if (_stopFlag) break;

        UINT32 padding = 0;
        _audioClient->GetCurrentPadding(&padding);
        UINT32 available = _actualBufferSize - padding;
        if (available == 0) continue;

        BYTE* data = nullptr;
        _renderClient->GetBuffer(available, &data);

        scratch.assign(available * 2, 0.0f);
        if (_renderCb)
            _renderCb(scratch.data(), available, _actualSampleRate);

        // Convert interleaved float to BYTE* directly (already float)
        memcpy(data, scratch.data(), available * 2 * sizeof(float));
        _renderClient->ReleaseBuffer(available, 0);
    }
}

} // namespace em
#endif // EM_WINDOWS
