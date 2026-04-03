#pragma once
#ifdef EM_LINUX
#include <alsa/asoundlib.h>
#include <string>
#include <vector>
#include "common/IMidiOutput.h"

namespace em {

class AlsaMidiOutput : public IMidiOutput {
public:
    AlsaMidiOutput() = default;
    ~AlsaMidiOutput() override;

    std::vector<std::string> getAvailablePorts() const override;
    bool openPort(unsigned int portIndex) override;
    bool openPortByName(const std::string& name) override;
    void closePort() override;
    void sendMessage(const MidiMessage& msg) override;
    void sendRaw(const uint8_t* data, int len) override;
    bool isOpen() const override;
    std::string getOpenPortName() const override;

private:
    snd_rawmidi_t* _handle = nullptr;
    bool           _open   = false;
    std::string    _portName;

    // Cache port names -> hw device strings
    struct PortInfo { std::string name; std::string hwId; };
    mutable std::vector<PortInfo> _portCache;
    void refreshPorts() const;
};

} // namespace em
#endif // EM_LINUX
