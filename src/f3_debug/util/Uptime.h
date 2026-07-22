// src/f3_debug/util/Uptime.h
// Track elapsed wall-clock time and format it as HH:MM:SS.

#pragma once

#include <chrono>
#include <format>
#include <string>

namespace f3_debug::util {

class Uptime {
public:
    Uptime() : mStart(std::chrono::steady_clock::now()) {}

    [[nodiscard]] std::string format() const {
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - mStart).count();
        return std::format("{:02}:{:02}:{:02}", elapsed / 3600, (elapsed % 3600) / 60, elapsed % 60);
    }

    void reset() noexcept { mStart = std::chrono::steady_clock::now(); }

private:
    std::chrono::steady_clock::time_point mStart;
};

} // namespace f3_debug::util
