#pragma once
#ifdef EM_WINDOWS
#include <Windows.h>
#include <mmsystem.h>
#include <string>
#include <vector>
#include "common/IMidiOutput.h"

namespace em {

class WinMidiOutput : public IMidiOutput {
public:
    WinMidiOutput() = default;
    ~WinMidiOutput() override;

    std::vector<std::string> getAvailablePorts() const override;
    bool openPort(unsigned int portIndex) override;
    bool openPortByName(const std::string& name) override;
    void closePort() override;
    void sendMessage(const MidiMessage& msg) override;
    void sendRaw(const uint8_t* data, int len) override;
    bool isOpen() const override;
    std::string getOpenPortName() const override;

private:
    HMIDIOUT    _handle   = nullptr;
    bool        _open     = false;
    std::string _portName;
};

} // namespace em
#endif // EM_WINDOWS
