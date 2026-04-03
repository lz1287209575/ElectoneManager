#ifdef EM_LINUX
#include "AlsaMidiOutput.h"
#include <cstring>

namespace em {

AlsaMidiOutput::~AlsaMidiOutput() {
    closePort();
}

void AlsaMidiOutput::refreshPorts() const {
    _portCache.clear();
    int card = -1;
    while (snd_card_next(&card) >= 0 && card >= 0) {
        snd_ctl_t* ctl = nullptr;
        char name[32];
        snprintf(name, sizeof(name), "hw:%d", card);
        if (snd_ctl_open(&ctl, name, 0) < 0) continue;

        int device = -1;
        while (snd_ctl_rawmidi_next_device(ctl, &device) >= 0 && device >= 0) {
            snd_rawmidi_info_t* info;
            snd_rawmidi_info_alloca(&info);
            snd_rawmidi_info_set_device(info, device);
            snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
            snd_rawmidi_info_set_subdevice(info, 0);

            if (snd_ctl_rawmidi_info(ctl, info) >= 0) {
                PortInfo pi;
                pi.name = snd_rawmidi_info_get_name(info);
                char hw[64];
                snprintf(hw, sizeof(hw), "hw:%d,%d", card, device);
                pi.hwId = hw;
                _portCache.push_back(pi);
            }
        }
        snd_ctl_close(ctl);
    }
}

std::vector<std::string> AlsaMidiOutput::getAvailablePorts() const {
    refreshPorts();
    std::vector<std::string> ports;
    for (auto& p : _portCache) ports.push_back(p.name);
    return ports;
}

bool AlsaMidiOutput::openPort(unsigned int portIndex) {
    closePort();
    refreshPorts();
    if (portIndex >= _portCache.size()) return false;

    int err = snd_rawmidi_open(nullptr, &_handle,
                                _portCache[portIndex].hwId.c_str(), 0);
    if (err < 0) return false;

    _portName = _portCache[portIndex].name;
    _open = true;
    return true;
}

bool AlsaMidiOutput::openPortByName(const std::string& name) {
    refreshPorts();
    for (unsigned int i = 0; i < _portCache.size(); ++i) {
        if (_portCache[i].name.find(name) != std::string::npos) {
            return openPort(i);
        }
    }
    return false;
}

void AlsaMidiOutput::closePort() {
    if (_handle) {
        snd_rawmidi_close(_handle);
        _handle = nullptr;
    }
    _open = false;
    _portName.clear();
}

void AlsaMidiOutput::sendMessage(const MidiMessage& msg) {
    uint8_t buf[3];
    int len = msg.toRaw(buf);
    sendRaw(buf, len);
}

void AlsaMidiOutput::sendRaw(const uint8_t* data, int len) {
    if (!_open || !_handle || len <= 0) return;
    snd_rawmidi_write(_handle, data, len);
}

bool AlsaMidiOutput::isOpen() const { return _open; }
std::string AlsaMidiOutput::getOpenPortName() const { return _portName; }

} // namespace em
#endif // EM_LINUX
