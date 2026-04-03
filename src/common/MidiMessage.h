#pragma once
#include <cstdint>
#include <cstddef>
#include <chrono>

namespace em {

/// MIDI message type (status byte high nibble)
enum class MidiType : uint8_t {
    NoteOff         = 0x80,
    NoteOn          = 0x90,
    PolyAftertouch  = 0xA0,
    ControlChange   = 0xB0,
    ProgramChange   = 0xC0,
    ChannelPressure = 0xD0,
    PitchBend       = 0xE0,
    SysEx           = 0xF0,
    Unknown         = 0x00,
};

/// MIDI Realtime status bytes (single-byte, no channel)
static constexpr uint8_t kMidiClock    = 0xF8;  ///< Timing Clock
static constexpr uint8_t kMidiStart    = 0xFA;  ///< Start
static constexpr uint8_t kMidiContinue = 0xFB;  ///< Continue
static constexpr uint8_t kMidiStop     = 0xFC;  ///< Stop

/// A timestamped MIDI message (up to 3 data bytes).
/// Kept small (16 bytes) so it fits in a cache line alongside other data.
struct MidiMessage {
    MidiType type      = MidiType::Unknown;
    uint8_t  channel   = 0;   ///< 0-based channel (0-15)
    uint8_t  data1     = 0;   ///< Note / CC number / MSB
    uint8_t  data2     = 0;   ///< Velocity / CC value / LSB
    uint32_t timestamp = 0;   ///< Microseconds (wraps ~71 min)

    /// Construct from raw MIDI bytes
    static MidiMessage fromRaw(const uint8_t* bytes, size_t len,
                               uint32_t ts = 0) noexcept {
        MidiMessage m;
        m.timestamp = ts;
        if (len < 1) return m;
        m.type    = static_cast<MidiType>(bytes[0] & 0xF0);
        m.channel = bytes[0] & 0x0F;
        if (len >= 2) m.data1 = bytes[1];
        if (len >= 3) m.data2 = bytes[2];
        return m;
    }

    /// Serialize back to raw MIDI bytes. Returns number of bytes written (1-3).
    int toRaw(uint8_t* buf) const noexcept {
        uint8_t status = static_cast<uint8_t>(type) | (channel & 0x0F);
        buf[0] = status;
        switch (type) {
        case MidiType::ProgramChange:
        case MidiType::ChannelPressure:
            buf[1] = data1;
            return 2;
        default:
            buf[1] = data1;
            buf[2] = data2;
            return 3;
        }
    }

    /// Factory: Note On message
    static MidiMessage noteOn(uint8_t ch, uint8_t note, uint8_t vel) noexcept {
        MidiMessage m;
        m.type    = MidiType::NoteOn;
        m.channel = ch;
        m.data1   = note;
        m.data2   = vel;
        return m;
    }

    /// Factory: Note Off message
    static MidiMessage noteOff(uint8_t ch, uint8_t note, uint8_t vel = 0) noexcept {
        MidiMessage m;
        m.type    = MidiType::NoteOff;
        m.channel = ch;
        m.data1   = note;
        m.data2   = vel;
        return m;
    }

    /// Factory: Control Change message
    static MidiMessage cc(uint8_t ch, uint8_t ccNum, uint8_t value) noexcept {
        MidiMessage m;
        m.type    = MidiType::ControlChange;
        m.channel = ch;
        m.data1   = ccNum;
        m.data2   = value;
        return m;
    }

    /// Factory: Program Change message
    static MidiMessage programChange(uint8_t ch, uint8_t program) noexcept {
        MidiMessage m;
        m.type    = MidiType::ProgramChange;
        m.channel = ch;
        m.data1   = program;
        m.data2   = 0;
        return m;
    }

    /// Decode 14-bit Pitch Bend value to [-1.0, 1.0]
    /// data1 = LSB (7-bit), data2 = MSB (7-bit)
    float pitchBendFloat() const noexcept {
        int raw = (static_cast<int>(data2) << 7) | static_cast<int>(data1);
        return (raw - 8192) / 8192.0f;
    }

    bool isNoteOn()  const noexcept { return type == MidiType::NoteOn  && data2 > 0; }
    bool isNoteOff() const noexcept {
        return type == MidiType::NoteOff ||
               (type == MidiType::NoteOn && data2 == 0);
    }
    bool isPitchBend()     const noexcept { return type == MidiType::PitchBend; }
    bool isProgramChange() const noexcept { return type == MidiType::ProgramChange; }
    bool isControlChange() const noexcept { return type == MidiType::ControlChange; }
};

} // namespace em
