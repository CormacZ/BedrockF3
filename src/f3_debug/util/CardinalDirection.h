// src/f3_debug/util/CardinalDirection.h
// Bedrock Edition uses 0 = +Z (south) and increases clockwise. This helper
// converts a yaw value to a human-readable cardinal direction.

#pragma once

#include <array>
#include <cmath>
#include <string_view>

namespace f3_debug::util {

[[nodiscard]] constexpr std::string_view cardinalDirection(float yaw) noexcept {
    constexpr std::array<std::string_view, 4> dirs = {
        "South (+Z)", "West (-X)", "North (-Z)", "East (+X)"
    };
    const int idx = static_cast<int>(std::floor(((yaw + 180.0f) / 90.0f) + 0.5f)) % 4;
    return dirs[idx];
}

} // namespace f3_debug::util
