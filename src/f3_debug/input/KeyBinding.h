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

// GLFW key code for F3. This matches the GDK client's underlying windowing
// layer. (GLFW_KEY_F3 == 292)
constexpr int kF3KeyCode = 292;

constexpr const char* kToggleKeyName = "bedrockf3.toggle";

// Register the F3 toggle key against the KeyRegistry and wire its handler
// to F3Debug::toggleOverlay.
std::expected<void, std::string> registerToggleKey();

// Unregister the F3 toggle key.
void unregisterToggleKey();

} // namespace f3_debug::input

} // namespace f3_debug
