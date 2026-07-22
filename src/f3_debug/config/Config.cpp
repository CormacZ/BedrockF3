// src/f3_debug/config/Config.cpp

#include "f3_debug/config/Config.h"

#include <ll/api/Config.h>

namespace f3_debug::config {

std::expected<void, std::string> load(F3Config& out, const std::filesystem::path& path) {
    if (!ll::config::loadConfig(out, path)) {
        if (auto saveResult = save(out, path); !saveResult) {
            return std::unexpected(std::format("failed to write default config: {}", saveResult.error()));
        }
    }
    return {};
}

std::expected<void, std::string> save(const F3Config& cfg, const std::filesystem::path& path) {
    if (!ll::config::saveConfig(cfg, path)) {
        return std::unexpected("failed to write config file");
    }
    return {};
}

} // namespace f3_debug::config
