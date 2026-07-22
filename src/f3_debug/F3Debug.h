// src/f3_debug/F3Debug.h
// Entry-point class for the BedrockF3 mod.
//
// The mod follows the standard LeviLamina NativeMod lifecycle: load() runs
// once on load, enable() runs every time the mod is enabled, disable() runs
// when the mod is disabled, unload() runs when the mod is being unloaded.

#pragma once

#include <ll/api/mod/NativeMod.h>

#include "f3_debug/config/Config.h"

namespace ll::event {
class ListenerBase;
}

namespace f3_debug {

class F3Debug {
public:
    static F3Debug& getInstance();

    F3Debug() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();
    bool enable();
    bool disable();
    bool unload();

    // Toggle the overlay at runtime. Called from the F3 key handler.
    void toggleOverlay() noexcept { mOverlayVisible = !mOverlayVisible; }

    [[nodiscard]] bool isOverlayVisible() const noexcept { return mOverlayVisible; }

private:
    ll::mod::NativeMod& mSelf;
    config::F3Config     mConfig{};
    bool mOverlayVisible = true;

    // Owned by the EventBus. Reset to unregister the render listener.
    // The F3 toggle key is handled by an ll::event::input::KeyInputEvent
    // listener; storing the shared_ptr here lets disable() unregister it.
    std::shared_ptr<ll::event::ListenerBase> mRenderListener;
    std::shared_ptr<ll::event::ListenerBase> mKeyListener;
};

} // namespace f3_debug
