#ifdef EM_ANDROID
#include "AndroidMidiDevice.h"
#include <chrono>
#include <cstring>

namespace em {

AndroidMidiDevice::AndroidMidiDevice() {}

AndroidMidiDevice::~AndroidMidiDevice() {
    stopListening();
    closePort();
}

void AndroidMidiDevice::setNativeDevice(AMidiDevice* device) {
    _midiDevice = device;
}

std::vector<std::string> AndroidMidiDevice::getAvailablePorts() const {
    // On Android, port discovery happens via Java MidiManager.
    // Return placeholder — real listing is done in Java and injected via setNativeDevice.
    return {"Android MIDI (via MidiManager)"};
}

bool AndroidMidiDevice::openPort(unsigned int /*portIndex*/) {
    if (!_midiDevice) return false;
    if (AMidiDevice_getNumInputPorts(_midiDevice) < 1) return false;

    media_status_t st = AMidiInputPort_open(_midiDevice, 0, &_inputPort);
    if (st != AMEDIA_OK || !_inputPort) return false;

    _open = true;
    return true;
}

bool AndroidMidiDevice::openPortByName(const std::string& /*name*/) {
    return openPort(0);
}

void AndroidMidiDevice::closePort() {
    if (_inputPort) {
        AMidiInputPort_close(_inputPort);
        _inputPort = nullptr;
    }
    _open = false;
}

void AndroidMidiDevice::setMessageCallback(MessageCallback cb) {
    std::lock_guard<std::mutex> lk(_cbMutex);
    _callback = std::move(cb);
}

bool AndroidMidiDevice::startListening() {
    if (_running || !_open) return false;
    _stopFlag = false;
    _running  = true;
    _thread   = std::thread(&AndroidMidiDevice::listenerThreadFunc, this);
    return true;
}

void AndroidMidiDevice::stopListening() {
    if (!_running) return;
    _stopFlag = true;
    _running  = false;
    if (_thread.joinable()) _thread.join();
}

bool AndroidMidiDevice::isOpen()            const { return _open; }
std::string AndroidMidiDevice::getOpenPortName() const { return _portName; }

// ── Listener thread ───────────────────────────────────────────────────────────

void AndroidMidiDevice::listenerThreadFunc() {
    constexpr size_t kBufSize = 128;
    uint8_t  buf[kBufSize];
    int32_t  opCode;
    size_t   numBytesReceived;
    int64_t  timestamp;

    while (!_stopFlag) {
        ssize_t n = AMidiInputPort_receive(_inputPort, &opCode,
                                           buf, kBufSize,
                                           &numBytesReceived, &timestamp);
        if (n <= 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(500));
            continue;
        }
        if (opCode != AMIDI_OPCODE_DATA) continue;

        // timestamp is nanoseconds → convert to microseconds
        uint32_t ts = static_cast<uint32_t>(timestamp / 1000);
        size_t offset = 0;
        while (offset < numBytesReceived) {
            size_t remaining = numBytesReceived - offset;
            size_t msgLen = (remaining >= 3) ? 3 :
                            (remaining >= 2) ? 2 : 1;

            MidiMessage msg = MidiMessage::fromRaw(buf + offset, msgLen, ts);
            {
                std::lock_guard<std::mutex> lk(_cbMutex);
                if (_callback) _callback(msg);
            }
            offset += msgLen;
        }
    }
}

} // namespace em
#endif // EM_ANDROID
