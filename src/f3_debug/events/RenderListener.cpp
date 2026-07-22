// src/f3_debug/events/RenderListener.cpp

#include "f3_debug/events/RenderListener.h"

#include "f3_debug/F3Debug.h"
#include "f3_debug/overlay/Overlay.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/ListenerBase.h>

#include <chrono>

namespace f3_debug::events {

std::shared_ptr<ll::event::ListenerBase> makeRenderListener() {
    auto& bus = ll::event::EventBus::getInstance();

    // Steady_clock timestamp of the previous AfterUIRenderEvent. Lives for
    // the lifetime of the listener (so the closure outlives all calls).
    // First call uses an "unset" sentinel (zero) which the overlay treats
    // as "no delta yet" and skips the FPS sample.
    auto lastTick = std::make_shared<std::chrono::steady_clock::time_point>();

    return bus.emplaceListener<ll::event::render::AfterUIRenderEvent>(
        [lastTick](ll::event::render::AfterUIRenderEvent& ev) {
            if (!F3Debug::getInstance().isOverlayVisible()) {
                return;
            }

            const auto now = std::chrono::steady_clock::now();
            double deltaMs = 0.0;
            if (lastTick->time_since_epoch().count() != 0) {
                deltaMs = std::chrono::duration<double, std::milli>(
                              now - *lastTick)
                              .count();
            }
            *lastTick = now;

            overlay::draw(ev.uiRenderContext(), deltaMs);
        });
}

} // namespace f3_debug::events
