#ifdef EM_ANDROID
#include "AndroidMidiOutput.h"
#include <android/log.h>

// AMidi API is available from API 29+
#if __ANDROID_API__ >= 29
#include <amidi/AMidi.h>
#endif

#define LOG_TAG "ElectoneManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace em {

AndroidMidiOutput::~AndroidMidiOutput() {
    closePort();
}

std::vector<std::string> AndroidMidiOutput::getAvailablePorts() const {
    // Android MIDI enumeration requires JNI calls to MidiManager.
    // The NDK AMidi API doesn't enumerate devices — that's done in Java.
    // Return empty for now; the Java layer should call openPort() with index.
    LOGW("AndroidMidiOutput::getAvailablePorts() — use Java MidiManager for enumeration");
    return {};
}

bool AndroidMidiOutput::openPort(unsigned int /*portIndex*/) {
    // Must be called from Java side with an AMidiDevice pointer
    LOGW("AndroidMidiOutput::openPort() — requires AMidiDevice from Java layer");
    return false;
}

bool AndroidMidiOutput::openPortByName(const std::string& /*name*/) {
    LOGW("AndroidMidiOutput::openPortByName() — requires AMidiDevice from Java layer");
    return false;
}

void AndroidMidiOutput::closePort() {
#if __ANDROID_API__ >= 29
    if (_port) {
        AMidiOutputPort_close(static_cast<AMidiOutputPort*>(_port));
        _port = nullptr;
    }
    if (_device) {
        AMidiDevice_release(static_cast<AMidiDevice*>(_device));
        _device = nullptr;
    }
#endif
    _open = false;
    _portName.clear();
}

void AndroidMidiOutput::sendMessage(const MidiMessage& msg) {
    uint8_t buf[3];
    int len = msg.toRaw(buf);
    sendRaw(buf, len);
}

void AndroidMidiOutput::sendRaw(const uint8_t* data, int len) {
#if __ANDROID_API__ >= 29
    if (!_open || !_port || len <= 0) return;
    AMidiOutputPort_send(static_cast<AMidiOutputPort*>(_port), data, len);
#else
    (void)data; (void)len;
#endif
}

bool AndroidMidiOutput::isOpen() const { return _open; }
std::string AndroidMidiOutput::getOpenPortName() const { return _portName; }

} // namespace em
#endif // EM_ANDROID
