#if defined(EM_MACOS) || defined(EM_IOS)
#include "CoreMidiOutput.h"
#include <CoreFoundation/CoreFoundation.h>
#include <cstring>
#include <vector>

namespace em {

CoreMidiOutput::CoreMidiOutput() {
    // Reuse existing MIDIClient if possible, otherwise create one
    OSStatus status = MIDIClientCreate(
        CFSTR("ElectoneManager-Out"), nullptr, nullptr, &_client);
    if (status != noErr) _client = 0;

    if (_client) {
        status = MIDIOutputPortCreate(_client, CFSTR("EM-Output"), &_outPort);
        if (status != noErr) _outPort = 0;
    }
}

CoreMidiOutput::~CoreMidiOutput() {
    closePort();
    if (_outPort) MIDIPortDispose(_outPort);
    if (_client)  MIDIClientDispose(_client);
}

std::vector<std::string> CoreMidiOutput::getAvailablePorts() const {
    std::vector<std::string> ports;
    ItemCount n = MIDIGetNumberOfDestinations();
    for (ItemCount i = 0; i < n; ++i) {
        MIDIEndpointRef ep = MIDIGetDestination(i);
        CFStringRef name = nullptr;
        MIDIObjectGetStringProperty(ep, kMIDIPropertyName, &name);
        if (name) {
            char buf[256] = {};
            CFStringGetCString(name, buf, sizeof(buf), kCFStringEncodingUTF8);
            ports.push_back(buf);
            CFRelease(name);
        }
    }
    return ports;
}

bool CoreMidiOutput::openPort(unsigned int portIndex) {
    closePort();
    ItemCount n = MIDIGetNumberOfDestinations();
    if (portIndex >= n) return false;

    _endpoint = MIDIGetDestination(portIndex);
    if (!_endpoint) return false;

    CFStringRef name = nullptr;
    MIDIObjectGetStringProperty(_endpoint, kMIDIPropertyName, &name);
    if (name) {
        char buf[256] = {};
        CFStringGetCString(name, buf, sizeof(buf), kCFStringEncodingUTF8);
        _portName = buf;
        CFRelease(name);
    }

    _open = true;
    return true;
}

bool CoreMidiOutput::openPortByName(const std::string& name) {
    auto ports = getAvailablePorts();
    for (unsigned int i = 0; i < ports.size(); ++i) {
        if (ports[i].find(name) != std::string::npos) {
            return openPort(i);
        }
    }
    return false;
}

void CoreMidiOutput::closePort() {
    _open     = false;
    _endpoint = 0;
    _portName.clear();
}

void CoreMidiOutput::sendMessage(const MidiMessage& msg) {
    uint8_t buf[3];
    int len = msg.toRaw(buf);
    sendRaw(buf, len);
}

void CoreMidiOutput::sendRaw(const uint8_t* data, int len) {
    if (!_open || !_outPort || !_endpoint || len <= 0) return;

    // For small messages, use fixed buffer; for SysEx, allocate dynamic buffer
    const int kMinBufferSize = 256;
    const int kPacketDataSize = (len < 3) ? 3 : len;
    const int kBufferSize = sizeof(MIDIPacketList) + 
                           sizeof(MIDIPacket) + 
                           kPacketDataSize + 64;  // Extra space for safety
    
    std::vector<uint8_t> dynamicBuffer(std::max(kMinBufferSize, kBufferSize));
    MIDIPacketList* pktList = reinterpret_cast<MIDIPacketList*>(dynamicBuffer.data());
    MIDIPacket* pkt = MIDIPacketListInit(pktList);
    pkt = MIDIPacketListAdd(pktList, dynamicBuffer.size(), pkt, 0,
                            static_cast<ByteCount>(len), data);
    if (pkt) {
        MIDISend(_outPort, _endpoint, pktList);
    }
}

bool CoreMidiOutput::isOpen() const { return _open; }
std::string CoreMidiOutput::getOpenPortName() const { return _portName; }

} // namespace em
#endif // EM_MACOS
