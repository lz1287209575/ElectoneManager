#pragma once
#include <string>
#include <vector>
#include "IAudioEngine.h"

namespace em {

/// Platform-independent helper to find Yamaha / Electone audio devices.
/// Each platform's IAudioEngine implementation calls into this.
struct AudioDeviceScanner {
    static constexpr const char* kYamahaKeyword  = "Yamaha";
    static constexpr const char* kElectoneKeyword = "Electone";

    /// Returns true if a device name matches Yamaha or Electone.
    static bool isYamahaDevice(const std::string& name) noexcept {
        auto ci_find = [](const std::string& haystack, const char* needle) {
            std::string h = haystack, n = needle;
            for (auto& c : h) c = static_cast<char>(tolower(c));
            for (auto& c : n) c = static_cast<char>(tolower(c));
            return h.find(n) != std::string::npos;
        };
        return ci_find(name, kYamahaKeyword) || ci_find(name, kElectoneKeyword);
    }

    /// Among a list of devices, return index of the first Yamaha/Electone match.
    /// Returns -1 if none found.
    static int findYamahaDevice(const std::vector<AudioDeviceInfo>& devices) noexcept {
        for (size_t i = 0; i < devices.size(); ++i) {
            if (isYamahaDevice(devices[i].name))
                return static_cast<int>(i);
        }
        return -1;
    }
};

} // namespace em
