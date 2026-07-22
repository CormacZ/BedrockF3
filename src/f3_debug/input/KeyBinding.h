// src/f3_debug/input/KeyBinding.h
// Registers the F3 toggle key with the LeviLamina KeyRegistry.
//
// KeyRegistry is the supported way to add a rebindable key binding to a
// client-side mod. The binding is automatically surfaced in the in-game
// options menu, so users can rebind it without the mod author doing any
// extra work.

#pragma once

#include <expected>
#include <string>

namespace f3_debug {

class F3Debug;

namespace input {

// Bedrock key code for F3. Bedrock's input pipeline uses Windows Virtual-Key
// codes, not GLFW key codes. VK_F3 == 0x72 == 114. (GLFW_KEY_F3 == 292 is
// the GLFW library's F3 code and is NOT what Bedrock dispatches when you
// press the F3 key on a real keyboard.)
constexpr int kF3KeyCode = 114;

constexpr const char* kToggleKeyName = "bedrockf3.toggle";

// Register the F3 toggle key against the KeyRegistry and wire its handler
// to F3Debug::toggleOverlay.
std::expected<void, std::string> registerToggleKey();

// Unregister the F3 toggle key.
void unregisterToggleKey();

} // namespace f3_debug::input

} // namespace f3_debug
