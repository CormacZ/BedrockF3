// src/f3_debug/events/RenderListener.h
// Subscribes to ll::event::render::AfterUIRenderEvent so we can draw the
// F3 panel on top of the in-game HUD every frame.
//
// LeviLamina's event bus is the supported way for client-side mods to
// hook into the render flow. We do not need to hook the engine binary
// directly.

#pragma once

#include <ll/api/event/render/UIRenderEvent.h>

#include <functional>
#include <memory>

namespace f3_debug::events {

// Build and return a listener for AfterUIRenderEvent. The caller owns the
// returned shared_ptr; dropping it unregisters the listener.
[[nodiscard]] std::shared_ptr<ll::event::ListenerBase> makeRenderListener();

} // namespace f3_debug::events
