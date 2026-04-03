#pragma once
#ifdef EM_ANDROID
#include <string>
#include <vector>
#include "common/IMidiOutput.h"

namespace em {

/// Android MIDI output using the NDK AMidi API.
/// Requires API level 29+.
class AndroidMidiOutput : public IMidiOutput {
public:
    AndroidMidiOutput() = default;
    ~AndroidMidiOutput() override;

    std::vector<std::string> getAvailablePorts() const override;
    bool openPort(unsigned int portIndex) override;
    bool openPortByName(const std::string& name) override;
    void closePort() override;
    void sendMessage(const MidiMessage& msg) override;
    void sendRaw(const uint8_t* data, int len) override;
    bool isOpen() const override;
    std::string getOpenPortName() const override;

private:
    void* _device = nullptr;  // AMidiDevice*
    void* _port   = nullptr;  // AMidiOutputPort*
    bool  _open   = false;
    std::string _portName;
};

} // namespace em
#endif // EM_ANDROID
