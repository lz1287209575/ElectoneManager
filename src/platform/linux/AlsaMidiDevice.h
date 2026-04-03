#pragma once
#ifdef EM_LINUX
#include <alsa/asoundlib.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include "common/IMidiDevice.h"

namespace em {

class AlsaMidiDevice : public IMidiDevice {
public:
    AlsaMidiDevice();
    ~AlsaMidiDevice() override;

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

    snd_seq_t*         _seq        = nullptr;
    int                _clientId   = -1;
    int                _portId     = -1;

    std::thread        _thread;
    std::atomic<bool>  _running    {false};
    std::atomic<bool>  _stopFlag   {false};

    bool               _open       = false;
    std::string        _portName;
    MessageCallback    _callback;
    mutable std::mutex _cbMutex;
};

} // namespace em
#endif // EM_LINUX
