// src/f3_debug/input/KeyBinding.h
// Subscribes to the low-level key input event so the F3 toggle fires
// reliably on every keypress.
//
// We previously used ll::input::KeyRegistry for this, but the registry
// only sees keys that are bound to actions in Bedrock's InputHandler.
// F3 is reserved as a modifier in Bedrock (it powers the F3+<letter>
// debug chords like F3+A reload chunks, F3+B show hitboxes, etc.)
// and is never bound to a bare action, so the registry's handler was
// never invoked. The user's toggle never fired.
//
// ll::event::input::KeyInputEvent is published from a hook on
// HIDControllerGameCoreDesktop::onKeyDown, which fires for every
// keypress BEFORE Bedrock's input pipeline processes it. That is
// the right level to intercept F3.

#pragma once

#include <expected>
#include <memory>
#include <string>

namespace ll::event {
class ListenerBase;
}

namespace f3_debug {

class F3Debug;

namespace input {

// Bedrock key code for F3. Bedrock's input pipeline uses Windows
// Virtual-Key codes, not GLFW key codes. VK_F3 == 0x72 == 114.
// (GLFW_KEY_F3 == 292 is the GLFW library's F3 code and is NOT
// what Bedrock dispatches when you press the F3 key on a real
// keyboard.)
constexpr int kF3KeyCode = 114;

// Register a listener for the F3 toggle key. The returned listener
// is owned by the caller and must be kept alive for the listener to
// remain active; pass it back to unregisterToggleKey() to remove.
[[nodiscard]] std::expected<std::shared_ptr<ll::event::ListenerBase>, std::string> registerToggleKey();

// Unregister a previously registered F3 toggle listener.
void unregisterToggleKey(std::shared_ptr<ll::event::ListenerBase> const& listener);

} // namespace f3_debug::input

} // namespace f3_debug
