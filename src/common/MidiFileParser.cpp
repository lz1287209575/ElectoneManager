#include "MidiFileParser.h"
#include <fstream>
#include <algorithm>
#include <cstring>

namespace em {

// ── Helper: read big-endian integers from a byte stream ──────────────────────

static uint16_t readU16(const uint8_t* p) {
    return (static_cast<uint16_t>(p[0]) << 8) | p[1];
}

static uint32_t readU32(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) |
           (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8)  |
            p[3];
}

/// Read a variable-length quantity (VLQ) from data at offset.
/// Advances offset past the VLQ bytes.
static uint32_t readVLQ(const std::vector<uint8_t>& data, size_t& offset) {
    uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
        if (offset >= data.size()) break;
        uint8_t b = data[offset++];
        value = (value << 7) | (b & 0x7F);
        if ((b & 0x80) == 0) break;
    }
    return value;
}

/// Parse a single track chunk.
static bool parseTrack(const std::vector<uint8_t>& data, size_t& offset, MidiTrack& track) {
    // Track header: "MTrk" + 4-byte length
    if (offset + 8 > data.size()) return false;
    if (std::memcmp(&data[offset], "MTrk", 4) != 0) return false;

    uint32_t trackLen = readU32(&data[offset + 4]);
    offset += 8;

    size_t trackEnd = offset + trackLen;
    if (trackEnd > data.size()) trackEnd = data.size();

    uint64_t absTick = 0;
    uint8_t  runningStatus = 0;

    while (offset < trackEnd) {
        // Delta time (VLQ)
        uint32_t delta = readVLQ(data, offset);
        absTick += delta;

        if (offset >= trackEnd) break;
        uint8_t statusByte = data[offset];

        if (statusByte == 0xFF) {
            // ── Meta event ──────────────────────────────────────────────
            offset++; // skip 0xFF
            if (offset >= trackEnd) break;
            uint8_t metaType = data[offset++];
            uint32_t metaLen = readVLQ(data, offset);

            MidiFileEvent ev;
            ev.absTick = absTick;
            ev.isMeta  = true;
            ev.metaType = metaType;
            if (offset + metaLen <= trackEnd) {
                ev.metaData.assign(data.begin() + offset,
                                   data.begin() + offset + metaLen);
            }
            offset += metaLen;
            track.events.push_back(ev);

            // End of track meta event
            if (metaType == 0x2F) break;

        } else if (statusByte == 0xF0 || statusByte == 0xF7) {
            // ── SysEx event ─────────────────────────────────────────────
            offset++; // skip F0/F7
            uint32_t sysexLen = readVLQ(data, offset);
            offset += sysexLen; // skip sysex data

        } else {
            // ── Channel message ─────────────────────────────────────────
            if (statusByte & 0x80) {
                // New status byte
                runningStatus = statusByte;
                offset++;
            }
            // else: running status, data byte starts at current offset

            if (runningStatus == 0) { offset++; continue; }

            uint8_t type = runningStatus & 0xF0;
            uint8_t ch   = runningStatus & 0x0F;

            MidiFileEvent ev;
            ev.absTick    = absTick;
            ev.msg.type    = static_cast<MidiType>(type);
            ev.msg.channel = ch;

            // How many data bytes?
            if (type == 0xC0 || type == 0xD0) {
                // 1 data byte
                if (offset < trackEnd) ev.msg.data1 = data[offset++];
            } else {
                // 2 data bytes
                if (offset < trackEnd) ev.msg.data1 = data[offset++];
                if (offset < trackEnd) ev.msg.data2 = data[offset++];
            }

            track.events.push_back(ev);
        }
    }

    // Ensure we're at trackEnd
    offset = trackEnd;
    return true;
}

bool parseMidiFile(const std::string& path, MidiFile& outFile) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    // Read entire file into memory
    file.seekg(0, std::ios::end);
    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    if (fileSize < 14) return false; // Minimum: MThd(4) + len(4) + format(2) + tracks(2) + ppqn(2)

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    if (!file) return false;

    // Parse header: "MThd" + 4-byte length (always 6) + format + numTracks + division
    size_t offset = 0;
    if (std::memcmp(&data[offset], "MThd", 4) != 0) return false;

    uint32_t headerLen = readU32(&data[offset + 4]);
    offset += 8;

    if (headerLen < 6 || offset + headerLen > data.size()) return false;

    outFile.format    = readU16(&data[offset]);
    outFile.numTracks = readU16(&data[offset + 2]);
    uint16_t division = readU16(&data[offset + 4]);
    offset += headerLen;

    // Handle division: if bit 15 is 0, it's ticks per quarter note
    if (division & 0x8000) {
        // SMPTE-based timing — convert to approximate PPQN
        // Not commonly used; default to 480
        outFile.ppqn = 480;
    } else {
        outFile.ppqn = division;
    }

    // Parse tracks
    outFile.tracks.clear();
    for (int t = 0; t < outFile.numTracks && offset < data.size(); ++t) {
        MidiTrack track;
        if (!parseTrack(data, offset, track)) break;
        outFile.tracks.push_back(std::move(track));
    }

    outFile.numTracks = static_cast<int>(outFile.tracks.size());
    return outFile.numTracks > 0;
}

} // namespace em
