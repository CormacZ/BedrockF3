// src/f3_debug/input/KeyBinding.cpp

#include "f3_debug/input/KeyBinding.h"

#include "f3_debug/F3Debug.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/ListenerBase.h>
#include <ll/api/event/input/KeyInputEvent.h>

#include <memory>

namespace f3_debug::input {

std::expected<std::shared_ptr<ll::event::ListenerBase>, std::string>
registerToggleKey() {
    auto& bus = ll::event::EventBus::getInstance();
    auto listener = bus.emplaceListener<ll::event::input::KeyInputEvent>(
        [](ll::event::input::KeyInputEvent& ev) {
            // Only act on the down event so a single press is one
            // toggle, not one toggle per key repeat.
            if (!ev.isDown()) {
                return;
            }
            // Match F3 only. Other F-keys are left alone so they
            // can be bound by the user (or by other mods) without
            // colliding with this mod.
            if (ev.keyCode() != kF3KeyCode) {
                return;
            }
            F3Debug::getInstance().toggleOverlay();
        });
    return listener;
}

void unregisterToggleKey(std::shared_ptr<ll::event::ListenerBase> const& listener) {
    if (!listener) {
        return;
    }
    ll::event::EventBus::getInstance().removeListener(listener);
}

} // namespace f3_debug::input
