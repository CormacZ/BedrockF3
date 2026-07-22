// src/f3_debug/util/CardinalDirection.h
// Bedrock Edition uses 0 = +Z (south) and increases clockwise. This helper
// converts a yaw value to a human-readable cardinal direction.
//
// Minecraft yaw convention:
//   yaw = 0   -> South (+Z)
//   yaw = 90  -> West  (-X)
//   yaw = 180 -> North (-Z)
//   yaw = -90 -> East  (+X)
//
// To map an arbitrary yaw to a direction index [0..3], we shift to a
// positive range first (handles negative yaws), center each direction on
// a multiple of 90 with a +45 offset (so the boundaries fall between
// directions), and floor-divide by 90. The +4 / %4 wrap makes the
// negative-yaw case land on a valid index without an out-of-range read.

#pragma once

#include <array>
#include <cmath>
#include <string_view>

namespace f3_debug::util {

[[nodiscard]] constexpr std::string_view cardinalDirection(float yaw) noexcept {
    constexpr std::array<std::string_view, 4> dirs = {"South (+Z)", "West (-X)", "North (-Z)", "East (+X)"};
    // Normalize yaw to [0, 360) so we can use a single closed-form
    // mapping without separate branches for negative yaws.
    const float normalized = std::fmod(yaw + 360.0f, 360.0f);
    const int idx = static_cast<int>(std::floor((normalized + 45.0f) / 90.0f)) % 4;
    return dirs[idx];
}

// Java-style axis name for a yaw, e.g. "Towards positive Z".
// Used to fill the parenthetical in the Facing line:
//   Facing: south (Towards positive Z) (-17.7 / 12.5)
[[nodiscard]] constexpr std::string_view cardinalAxisName(float yaw) noexcept {
    constexpr std::array<std::string_view, 4> axes = {
        "Towards positive Z",
        "Towards negative X",
        "Towards negative Z",
        "Towards positive X",
    };
    const float normalized = std::fmod(yaw + 360.0f, 360.0f);
    const int idx = static_cast<int>(std::floor((normalized + 45.0f) / 90.0f)) % 4;
    return axes[idx];
}

} // namespace f3_debug::util
