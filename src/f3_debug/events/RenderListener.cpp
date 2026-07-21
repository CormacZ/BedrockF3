// src/f3_debug/events/RenderListener.cpp

#include "f3_debug/events/RenderListener.h"

#include "f3_debug/F3Debug.h"
#include "f3_debug/overlay/Overlay.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/ListenerBase.h>

namespace f3_debug::events {

std::shared_ptr<ll::event::ListenerBase> makeRenderListener() {
    auto& bus = ll::event::EventBus::getInstance();
    return bus.emplaceListener<ll::event::render::AfterUIRenderEvent>(
        [](ll::event::render::AfterUIRenderEvent& ev) {
            if (F3Debug::getInstance().isOverlayVisible()) {
                overlay::draw(ev.uiRenderContext());
            }
        });
}

} // namespace f3_debug::events
