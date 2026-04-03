#pragma once
#if defined(EM_MACOS) || defined(EM_IOS)
#include <CoreMIDI/CoreMIDI.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include "common/IMidiDevice.h"

namespace em {

class CoreMidiDevice : public IMidiDevice {
public:
    CoreMidiDevice();
    ~CoreMidiDevice() override;

    std::vector<std::string> getAvailablePorts() const override;
    bool openPort(unsigned int portIndex) override;
    bool openPortByName(const std::string& name) override;
    void closePort() override;
    void setMessageCallback(MessageCallback cb) override;
    bool startListening() override;
    void stopListening() override;
    bool isOpen() const override;
    std::string getOpenPortName() const override;

private:
    static void midiReadProc(const MIDIPacketList* pktList,
                             void* readProcRefCon,
                             void* srcConnRefCon);

    MIDIClientRef   _client   = 0;
    MIDIPortRef     _inPort   = 0;
    MIDIEndpointRef _endpoint = 0;

    bool            _open     = false;
    std::string     _portName;
    MessageCallback _callback;
    mutable std::mutex _cbMutex;
};

} // namespace em
#endif // EM_MACOS || EM_IOS
