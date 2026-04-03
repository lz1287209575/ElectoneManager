#if defined(EM_MACOS) || defined(EM_IOS)
#include "CoreMidiDevice.h"
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_time.h>

namespace em {

// ──────────────────────────────────────────────────────────────────────────────
CoreMidiDevice::CoreMidiDevice() {
    OSStatus st = MIDIClientCreate(CFSTR("ElectoneManager"), nullptr, nullptr,
                                   &_client);
    if (st != noErr) {
        // Don't throw — caller checks _client == 0 before using
        _client = 0;
    }
}

CoreMidiDevice::~CoreMidiDevice() {
    closePort();
    if (_client) MIDIClientDispose(_client);
}

// ── Port enumeration ──────────────────────────────────────────────────────────

std::vector<std::string> CoreMidiDevice::getAvailablePorts() const {
    if (!_client) return {};
    std::vector<std::string> names;
    ItemCount count = MIDIGetNumberOfSources();
    for (ItemCount i = 0; i < count; ++i) {
        MIDIEndpointRef ep = MIDIGetSource(i);
        CFStringRef cfName = nullptr;
        MIDIObjectGetStringProperty(ep, kMIDIPropertyDisplayName, &cfName);
        if (cfName) {
            char buf[256] = {};
            CFStringGetCString(cfName, buf, sizeof(buf), kCFStringEncodingUTF8);
            names.emplace_back(buf);
            CFRelease(cfName);
        } else {
            names.emplace_back("MIDI Source " + std::to_string(i));
        }
    }
    return names;
}

// ── Open / close ──────────────────────────────────────────────────────────────

bool CoreMidiDevice::openPort(unsigned int portIndex) {
    if (!_client) return false;
    if (_open) closePort();

    ItemCount count = MIDIGetNumberOfSources();
    if (portIndex >= static_cast<unsigned int>(count)) return false;

    _endpoint = MIDIGetSource(portIndex);

    OSStatus st = MIDIInputPortCreate(_client, CFSTR("EM Input"),
                                      midiReadProc, this, &_inPort);
    if (st != noErr) return false;

    st = MIDIPortConnectSource(_inPort, _endpoint, nullptr);
    if (st != noErr) {
        MIDIPortDispose(_inPort);
        _inPort = 0;
        return false;
    }

    // Get port name
    CFStringRef cfName = nullptr;
    MIDIObjectGetStringProperty(_endpoint, kMIDIPropertyDisplayName, &cfName);
    if (cfName) {
        char buf[256] = {};
        CFStringGetCString(cfName, buf, sizeof(buf), kCFStringEncodingUTF8);
        _portName = buf;
        CFRelease(cfName);
    }

    _open = true;
    return true;
}

bool CoreMidiDevice::openPortByName(const std::string& name) {
    auto ports = getAvailablePorts();
    for (size_t i = 0; i < ports.size(); ++i) {
        if (ports[i].find(name) != std::string::npos)
            return openPort(static_cast<unsigned int>(i));
    }
    return false;
}

void CoreMidiDevice::closePort() {
    if (_inPort) {
        MIDIPortDisconnectSource(_inPort, _endpoint);
        MIDIPortDispose(_inPort);
        _inPort = 0;
    }
    _endpoint = 0;
    _open = false;
}

void CoreMidiDevice::setMessageCallback(MessageCallback cb) {
    std::lock_guard<std::mutex> lock(_cbMutex);
    _callback = std::move(cb);
}

bool CoreMidiDevice::startListening() {
    // Core MIDI is interrupt-driven; no additional thread needed.
    return _open;
}

void CoreMidiDevice::stopListening() {
    // No-op for Core MIDI (driven by system RunLoop).
}

bool CoreMidiDevice::isOpen() const { return _open; }

std::string CoreMidiDevice::getOpenPortName() const { return _portName; }

// ── MIDI read callback (called on Core MIDI thread) ───────────────────────────

void CoreMidiDevice::midiReadProc(const MIDIPacketList* pktList,
                                   void* readProcRefCon,
                                   void* /*srcConnRefCon*/) {
    auto* self = static_cast<CoreMidiDevice*>(readProcRefCon);
    if (!self) return;

    const MIDIPacket* pkt = &pktList->packet[0];
    for (UInt32 i = 0; i < pktList->numPackets; ++i) {
        if (pkt->length > 0) {
            // Convert Mach absolute time to microseconds
            static mach_timebase_info_data_t tb = {};
            if (tb.denom == 0) mach_timebase_info(&tb);
            uint32_t ts = static_cast<uint32_t>(
                (pkt->timeStamp * tb.numer / tb.denom) / 1000);

            MidiMessage msg = MidiMessage::fromRaw(pkt->data,
                                                   pkt->length, ts);
            std::lock_guard<std::mutex> lock(self->_cbMutex);
            if (self->_callback) self->_callback(msg);
        }
        pkt = MIDIPacketNext(pkt);
    }
}

} // namespace em
#endif // EM_MACOS
