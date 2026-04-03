#ifdef EM_WINDOWS
#include "WinMidiOutput.h"
#include <cstring>

namespace em {

WinMidiOutput::~WinMidiOutput() {
    closePort();
}

std::vector<std::string> WinMidiOutput::getAvailablePorts() const {
    std::vector<std::string> ports;
    UINT n = midiOutGetNumDevs();
    for (UINT i = 0; i < n; ++i) {
        MIDIOUTCAPS caps;
        if (midiOutGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            ports.push_back(caps.szPname);
        }
    }
    return ports;
}

bool WinMidiOutput::openPort(unsigned int portIndex) {
    closePort();
    MMRESULT result = midiOutOpen(&_handle, portIndex, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) return false;

    MIDIOUTCAPS caps;
    if (midiOutGetDevCaps(portIndex, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
        _portName = caps.szPname;
    }
    _open = true;
    return true;
}

bool WinMidiOutput::openPortByName(const std::string& name) {
    auto ports = getAvailablePorts();
    for (unsigned int i = 0; i < ports.size(); ++i) {
        if (ports[i].find(name) != std::string::npos) {
            return openPort(i);
        }
    }
    return false;
}

void WinMidiOutput::closePort() {
    if (_handle) {
        midiOutClose(_handle);
        _handle = nullptr;
    }
    _open = false;
    _portName.clear();
}

void WinMidiOutput::sendMessage(const MidiMessage& msg) {
    uint8_t buf[3];
    int len = msg.toRaw(buf);
    sendRaw(buf, len);
}

void WinMidiOutput::sendRaw(const uint8_t* data, int len) {
    if (!_open || !_handle || len <= 0) return;

    // Pack into DWORD for midiOutShortMsg
    DWORD msg = 0;
    for (int i = 0; i < len && i < 4; ++i) {
        msg |= static_cast<DWORD>(data[i]) << (i * 8);
    }
    midiOutShortMsg(_handle, msg);
}

bool WinMidiOutput::isOpen() const { return _open; }
std::string WinMidiOutput::getOpenPortName() const { return _portName; }

} // namespace em
#endif // EM_WINDOWS
