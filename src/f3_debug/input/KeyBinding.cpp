// src/f3_debug/input/KeyBinding.cpp

#include "f3_debug/input/KeyBinding.h"

#include "f3_debug/F3Debug.h"

#include <ll/api/input/KeyRegistry.h>
#include <ll/api/mod/NativeMod.h>

#include <mc/client/game/IClientInstance.h>
#include <mc/deps/input/enums/FocusImpact.h>

namespace f3_debug::input {

std::expected<void, std::string> registerToggleKey(F3Debug& self) {
    auto& registry = ll::input::KeyRegistry::getInstance();
    auto& handle   = registry.getOrCreateKey(
        kToggleKeyName,
        std::vector<int>{kF3KeyCode},
        /*allowRemap=*/true,
        std::weak_ptr<ll::mod::Mod>{self.getSelf().shared_from_this()});

    handle.registerButtonDownHandler(
        [](FocusImpact /*focus*/, IClientInstance& /*client*/) {
            F3Debug::getInstance().toggleOverlay();
        });

    return {};
}

void unregisterToggleKey(F3Debug& self) {
    // KeyHandle destructor unregisters handlers. The registry releases the
    // entry when the last reference to the handle goes out of scope.
    // The handle is owned by F3Debug; nothing extra to do here.
    (void)self;
}

} // namespace f3_debug::input
