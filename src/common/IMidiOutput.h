#pragma once
#include "MidiMessage.h"
#include <string>
#include <vector>
#include <cstdint>

namespace em {

/// Abstract MIDI output interface.
/// Each platform provides a concrete implementation for sending MIDI data
/// to external instruments (e.g., Yamaha ELS-02C rhythm module).
class IMidiOutput {
public:
    virtual ~IMidiOutput() = default;

    /// Enumerate available MIDI output port names.
    virtual std::vector<std::string> getAvailablePorts() const = 0;

    /// Open a MIDI output port by index (0-based).
    virtual bool openPort(unsigned int portIndex) = 0;

    /// Open a MIDI output port by name (partial match allowed).
    virtual bool openPortByName(const std::string& name) = 0;

    /// Close the currently open output port.
    virtual void closePort() = 0;

    /// Send a structured MidiMessage.
    virtual void sendMessage(const MidiMessage& msg) = 0;

    /// Send raw MIDI bytes (1-3 bytes for channel messages, 1 byte for realtime).
    virtual void sendRaw(const uint8_t* data, int len) = 0;

    /// True if a port is currently open.
    virtual bool isOpen() const = 0;

    /// Human-readable name of the open port, or empty string.
    virtual std::string getOpenPortName() const = 0;
};

} // namespace em
