#pragma once
#ifdef EM_WINDOWS
#include <windows.h>
#include <mmsystem.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include "common/IMidiDevice.h"

namespace em {

class WinMidiDevice : public IMidiDevice {
public:
    WinMidiDevice();
    ~WinMidiDevice() override;

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
    static void CALLBACK midiInProc(HMIDIIN hMidiIn, UINT wMsg,
                                    DWORD_PTR dwInstance,
                                    DWORD_PTR dwParam1, DWORD_PTR dwParam2);

    HMIDIIN          _hMidiIn   = nullptr;
    bool             _open      = false;
    std::string      _portName;
    MessageCallback  _callback;
    mutable std::mutex _cbMutex;
};

} // namespace em
#endif // EM_WINDOWS
