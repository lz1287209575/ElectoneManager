#pragma once
#ifdef EM_ANDROID
#include <amidi/AMidi.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include "common/IMidiDevice.h"

namespace em {

/// Android MIDI device using AMidi NDK API (API level 29+).
/// Requires ACCESS_MIDI and android.hardware.midi features in AndroidManifest.xml.
class AndroidMidiDevice : public IMidiDevice {
public:
    AndroidMidiDevice();
    ~AndroidMidiDevice() override;

    // Called by Java/Kotlin layer to inject the AMidiDevice*
    void setNativeDevice(AMidiDevice* device);

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
    void listenerThreadFunc();

    AMidiDevice*      _midiDevice  = nullptr;
    AMidiInputPort*   _inputPort   = nullptr;

    std::thread       _thread;
    std::atomic<bool> _running     {false};
    std::atomic<bool> _stopFlag    {false};

    bool              _open        = false;
    std::string       _portName    = "Android MIDI";
    MessageCallback   _callback;
    mutable std::mutex _cbMutex;
};

} // namespace em
#endif // EM_ANDROID
