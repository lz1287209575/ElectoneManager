#pragma once
#include <functional>
#include <string>
#include <vector>
#include "MidiMessage.h"

namespace em {

/// Abstract MIDI device interface.
/// Each platform provides a concrete implementation.
class IMidiDevice {
public:
    using MessageCallback = std::function<void(const MidiMessage&)>;

    virtual ~IMidiDevice() = default;

    /// Enumerate available MIDI input port names.
    virtual std::vector<std::string> getAvailablePorts() const = 0;

    /// Open a MIDI input port by index (0-based).
    /// Returns true on success.
    virtual bool openPort(unsigned int portIndex) = 0;

    /// Open a MIDI input port by name (partial match allowed).
    virtual bool openPortByName(const std::string& name) = 0;

    /// Close the currently open port.
    virtual void closePort() = 0;

    /// Register a callback that fires on every incoming MIDI message.
    /// Called from the MIDI listener thread — keep it fast.
    virtual void setMessageCallback(MessageCallback cb) = 0;

    /// Start the background listener thread.
    virtual bool startListening() = 0;

    /// Stop the listener thread (blocks until joined).
    virtual void stopListening() = 0;

    /// True if a port is currently open and listening.
    virtual bool isOpen() const = 0;

    /// Human-readable name of the open port, or empty string.
    virtual std::string getOpenPortName() const = 0;
};

} // namespace em
