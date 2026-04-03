#pragma once
#if defined(EM_MACOS) || defined(EM_IOS)
#include <CoreMIDI/CoreMIDI.h>
#include <string>
#include <vector>
#include "common/IMidiOutput.h"

namespace em {

class CoreMidiOutput : public IMidiOutput {
public:
    CoreMidiOutput();
    ~CoreMidiOutput() override;

    std::vector<std::string> getAvailablePorts() const override;
    bool openPort(unsigned int portIndex) override;
    bool openPortByName(const std::string& name) override;
    void closePort() override;
    void sendMessage(const MidiMessage& msg) override;
    void sendRaw(const uint8_t* data, int len) override;
    bool isOpen() const override;
    std::string getOpenPortName() const override;

private:
    MIDIClientRef   _client   = 0;
    MIDIPortRef     _outPort  = 0;
    MIDIEndpointRef _endpoint = 0;
    bool            _open     = false;
    std::string     _portName;
};

} // namespace em
#endif // EM_MACOS || EM_IOS
