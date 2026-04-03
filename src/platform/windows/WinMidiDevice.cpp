#ifdef EM_WINDOWS
#include "WinMidiDevice.h"
#include <stdexcept>

#pragma comment(lib, "winmm.lib")

namespace em {

WinMidiDevice::WinMidiDevice() {}

WinMidiDevice::~WinMidiDevice() {
    closePort();
}

std::vector<std::string> WinMidiDevice::getAvailablePorts() const {
    std::vector<std::string> names;
    UINT count = midiInGetNumDevs();
    for (UINT i = 0; i < count; ++i) {
        MIDIINCAPS caps = {};
        if (midiInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
            names.emplace_back(caps.szPname);
        else
            names.emplace_back("MIDI Input " + std::to_string(i));
    }
    return names;
}

bool WinMidiDevice::openPort(unsigned int portIndex) {
    if (_open) closePort();
    MMRESULT res = midiInOpen(&_hMidiIn, portIndex,
                              reinterpret_cast<DWORD_PTR>(midiInProc),
                              reinterpret_cast<DWORD_PTR>(this),
                              CALLBACK_FUNCTION);
    if (res != MMSYSERR_NOERROR) return false;
    midiInStart(_hMidiIn);

    MIDIINCAPS caps = {};
    midiInGetDevCaps(portIndex, &caps, sizeof(caps));
    _portName = caps.szPname;
    _open = true;
    return true;
}

bool WinMidiDevice::openPortByName(const std::string& name) {
    auto ports = getAvailablePorts();
    for (size_t i = 0; i < ports.size(); ++i) {
        if (ports[i].find(name) != std::string::npos)
            return openPort(static_cast<unsigned int>(i));
    }
    return false;
}

void WinMidiDevice::closePort() {
    if (_hMidiIn) {
        midiInStop(_hMidiIn);
        midiInClose(_hMidiIn);
        _hMidiIn = nullptr;
    }
    _open = false;
}

void WinMidiDevice::setMessageCallback(MessageCallback cb) {
    std::lock_guard<std::mutex> lk(_cbMutex);
    _callback = std::move(cb);
}

bool WinMidiDevice::startListening() { return _open; }
void WinMidiDevice::stopListening()  { /* WinMM uses callbacks */ }
bool WinMidiDevice::isOpen()         const { return _open; }
std::string WinMidiDevice::getOpenPortName() const { return _portName; }

void CALLBACK WinMidiDevice::midiInProc(HMIDIIN /*hMidiIn*/, UINT wMsg,
                                         DWORD_PTR dwInstance,
                                         DWORD_PTR dwParam1,
                                         DWORD_PTR dwParam2) {
    if (wMsg != MIM_DATA) return;
    auto* self = reinterpret_cast<WinMidiDevice*>(dwInstance);
    if (!self) return;

    uint8_t bytes[3] = {
        static_cast<uint8_t>(dwParam1 & 0xFF),
        static_cast<uint8_t>((dwParam1 >> 8) & 0xFF),
        static_cast<uint8_t>((dwParam1 >> 16) & 0xFF),
    };
    uint32_t ts = static_cast<uint32_t>(dwParam2 * 1000); // ms → µs
    MidiMessage msg = MidiMessage::fromRaw(bytes, 3, ts);

    std::lock_guard<std::mutex> lk(self->_cbMutex);
    if (self->_callback) self->_callback(msg);
}

} // namespace em
#endif // EM_WINDOWS
