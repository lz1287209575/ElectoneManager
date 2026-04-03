#ifdef EM_LINUX
#include "AlsaMidiDevice.h"
#include <stdexcept>
#include <cstring>
#include <chrono>

namespace em {

AlsaMidiDevice::AlsaMidiDevice() {
    if (snd_seq_open(&_seq, "default", SND_SEQ_OPEN_INPUT, 0) < 0)
        throw std::runtime_error("AlsaMidi: snd_seq_open failed");
    snd_seq_set_client_name(_seq, "ElectoneManager");
    _clientId = snd_seq_client_id(_seq);

    _portId = snd_seq_create_simple_port(
        _seq, "EM Input",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION);
    if (_portId < 0)
        throw std::runtime_error("AlsaMidi: snd_seq_create_simple_port failed");
}

AlsaMidiDevice::~AlsaMidiDevice() {
    stopListening();
    closePort();
    if (_seq) { snd_seq_close(_seq); _seq = nullptr; }
}

// ── Port enumeration ──────────────────────────────────────────────────────────

std::vector<std::string> AlsaMidiDevice::getAvailablePorts() const {
    std::vector<std::string> names;
    snd_seq_client_info_t* cinfo = nullptr;
    snd_seq_port_info_t*   pinfo = nullptr;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);

    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(_seq, cinfo) >= 0) {
        int client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(_seq, pinfo) >= 0) {
            unsigned int caps = snd_seq_port_info_get_capability(pinfo);
            if ((caps & SND_SEQ_PORT_CAP_READ) &&
                (caps & SND_SEQ_PORT_CAP_SUBS_READ)) {
                std::string name = std::string(snd_seq_client_info_get_name(cinfo))
                    + ":" + snd_seq_port_info_get_name(pinfo);
                names.push_back(name);
            }
        }
    }
    return names;
}

bool AlsaMidiDevice::openPort(unsigned int portIndex) {
    if (_open) closePort();

    // Enumerate and connect to portIndex-th readable port
    snd_seq_client_info_t* cinfo = nullptr;
    snd_seq_port_info_t*   pinfo = nullptr;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);

    unsigned int idx = 0;
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(_seq, cinfo) >= 0) {
        int client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(_seq, pinfo) >= 0) {
            unsigned int caps = snd_seq_port_info_get_capability(pinfo);
            if ((caps & SND_SEQ_PORT_CAP_READ) &&
                (caps & SND_SEQ_PORT_CAP_SUBS_READ)) {
                if (idx == portIndex) {
                    snd_seq_addr_t sender;
                    sender.client = snd_seq_port_info_get_client(pinfo);
                    sender.port   = snd_seq_port_info_get_port(pinfo);
                    snd_seq_addr_t dest;
                    dest.client = _clientId;
                    dest.port   = _portId;
                    snd_seq_port_subscribe_t* sub = nullptr;
                    snd_seq_port_subscribe_malloc(&sub);
                    snd_seq_port_subscribe_set_sender(sub, &sender);
                    snd_seq_port_subscribe_set_dest(sub, &dest);
                    int r = snd_seq_subscribe_port(_seq, sub);
                    snd_seq_port_subscribe_free(sub);
                    if (r < 0) return false;
                    _portName = std::string(snd_seq_client_info_get_name(cinfo))
                                + ":" + snd_seq_port_info_get_name(pinfo);
                    _open = true;
                    return true;
                }
                ++idx;
            }
        }
    }
    return false;
}

bool AlsaMidiDevice::openPortByName(const std::string& name) {
    auto ports = getAvailablePorts();
    for (size_t i = 0; i < ports.size(); ++i) {
        if (ports[i].find(name) != std::string::npos)
            return openPort(static_cast<unsigned int>(i));
    }
    return false;
}

void AlsaMidiDevice::closePort() {
    _open = false;
    // Port subscriptions removed when seq is closed
}

void AlsaMidiDevice::setMessageCallback(MessageCallback cb) {
    std::lock_guard<std::mutex> lk(_cbMutex);
    _callback = std::move(cb);
}

bool AlsaMidiDevice::startListening() {
    if (_running) return true;
    _stopFlag = false;
    _running  = true;
    _thread   = std::thread(&AlsaMidiDevice::listenerThreadFunc, this);
    return true;
}

void AlsaMidiDevice::stopListening() {
    if (!_running) return;
    _stopFlag = true;
    _running  = false;
    if (_thread.joinable()) _thread.join();
}

bool AlsaMidiDevice::isOpen() const { return _open; }
std::string AlsaMidiDevice::getOpenPortName() const { return _portName; }

// ── Listener thread ───────────────────────────────────────────────────────────

void AlsaMidiDevice::listenerThreadFunc() {
    snd_seq_event_t* ev = nullptr;

    while (!_stopFlag) {
        // Poll with timeout (50 ms) so we can check _stopFlag
        int r = snd_seq_event_input(_seq, &ev);
        if (r < 0 || !ev) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        MidiMessage msg;
        auto ts = static_cast<uint32_t>(
            ev->time.time.tv_sec * 1000000 + ev->time.time.tv_nsec / 1000);

        switch (ev->type) {
        case SND_SEQ_EVENT_NOTEON:
            msg = MidiMessage::fromRaw(
                (const uint8_t[]){
                    (uint8_t)(0x90 | ev->data.note.channel),
                    ev->data.note.note, ev->data.note.velocity
                }, 3, ts);
            break;
        case SND_SEQ_EVENT_NOTEOFF:
            msg = MidiMessage::fromRaw(
                (const uint8_t[]){
                    (uint8_t)(0x80 | ev->data.note.channel),
                    ev->data.note.note, 0
                }, 3, ts);
            break;
        case SND_SEQ_EVENT_PITCHBEND: {
            int val = ev->data.control.value + 8192; // -8192..8191 → 0..16383
            uint8_t lsb = val & 0x7F;
            uint8_t msb = (val >> 7) & 0x7F;
            msg = MidiMessage::fromRaw(
                (const uint8_t[]){
                    (uint8_t)(0xE0 | ev->data.control.channel), lsb, msb
                }, 3, ts);
            break;
        }
        case SND_SEQ_EVENT_CONTROLLER:
            msg = MidiMessage::fromRaw(
                (const uint8_t[]){
                    (uint8_t)(0xB0 | ev->data.control.channel),
                    (uint8_t)ev->data.control.param,
                    (uint8_t)ev->data.control.value
                }, 3, ts);
            break;
        case SND_SEQ_EVENT_PGMCHANGE:
            msg = MidiMessage::fromRaw(
                (const uint8_t[]){
                    (uint8_t)(0xC0 | ev->data.control.channel),
                    (uint8_t)ev->data.control.value, 0
                }, 2, ts);
            break;
        default:
            continue;
        }

        std::lock_guard<std::mutex> lk(_cbMutex);
        if (_callback) _callback(msg);
    }
}

} // namespace em
#endif // EM_LINUX
