// src/f3_debug/F3Debug.cpp

#include "f3_debug/F3Debug.h"

#include "f3_debug/config/Config.h"
#include "f3_debug/events/RenderListener.h"
#include "f3_debug/input/KeyBinding.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/mod/RegisterHelper.h>

namespace f3_debug {

F3Debug& F3Debug::getInstance() {
    static F3Debug instance;
    return instance;
}

bool F3Debug::load() {
    auto& logger = getSelf().getLogger();
    logger.info("BedrockF3 loading");

    if (auto result = config::load(mConfig, getSelf().getConfigDir() / "config.json"); !result) {
        logger.warn("Could not load config ({}); using defaults", result.error());
    }

    mOverlayVisible = mConfig.visibleOnStartup;
    return true;
}

bool F3Debug::enable() {
    auto& logger = getSelf().getLogger();
    logger.info("BedrockF3 enabled");

    mRenderListener = events::makeRenderListener();

    if (auto result = input::registerToggleKey(); !result) {
        logger.error("Failed to register F3 key: {}", result.error());
        disable();
        return false;
    }
    mKeyListener = *result;
    logger.info("F3 toggle active (low-level KeyInputEvent listener)");

    return true;
}

bool F3Debug::disable() {
    auto& logger = getSelf().getLogger();
    logger.info("BedrockF3 disabled");

    if (mRenderListener) {
        ll::event::EventBus::getInstance().removeListener(mRenderListener);
        mRenderListener.reset();
    }

    if (mKeyListener) {
        input::unregisterToggleKey(mKeyListener);
        mKeyListener.reset();
    }

    return true;
}

bool F3Debug::unload() {
    auto& logger = getSelf().getLogger();
    logger.info("BedrockF3 unloading");
    return true;
}

} // namespace f3_debug

LL_REGISTER_MOD(f3_debug::F3Debug, f3_debug::F3Debug::getInstance());
