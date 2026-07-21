// src/f3_debug/config/Config.h
// Runtime configuration for the F3 overlay.

#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace ll {
class Logger;
}

namespace f3_debug::config {

struct F3Config {
    // Schema version. Bumped on any breaking field change (added, removed,
    // type-changed, or renamed). Required by ll::config::loadConfig /
    // saveConfig, which compare this against the on-disk "version" key
    // and run defaultConfigUpdater to migrate older files.
    int version = 1;

    // Show the overlay when the mod first loads.
    bool visibleOnStartup = true;

    // Which key toggles the overlay. The default follows Java Edition.
    std::string toggleKey = "F3";

    // Position of the panel on screen, in pixels from the top-left.
    unsigned int panelX = 4;
    unsigned int panelY = 4;

    // Text scale (1.0 == Minecraft default).
    float textScale = 1.0f;

    // Background opacity, 0.0 (transparent) .. 1.0 (opaque).
    float backgroundAlpha = 0.55f;

    // Pixels between the panel border and the text.
    unsigned int padding = 6;
};

// Read a JSON file into the given struct. On missing file, writes defaults.
// Returns the load result or an error message.
std::expected<void, std::string> load(F3Config& out, const std::filesystem::path& path);

// Write the given config to a JSON file.
std::expected<void, std::string> save(const F3Config& cfg, const std::filesystem::path& path);

} // namespace f3_debug::config
