// src/f3_debug/overlay/Overlay.h
// Builds the F3 debug lines and draws them onto a MinecraftUIRenderContext.
//
// All methods are static because the overlay holds no per-instance state -
// every frame is independent, and the visible flag lives on F3Debug.

#pragma once

#include "f3_debug/util/FpsCounter.h"
#include "f3_debug/util/Uptime.h"

#include <string>
#include <vector>

class MinecraftUIRenderContext;

namespace f3_debug::overlay {

struct Line {
    std::string text;
    struct Rgba { float r, g, b, a; } color{1.0f, 1.0f, 1.0f, 1.0f};
};

// Draw the F3 overlay onto the supplied context. The caller is expected
// to have already verified that the overlay is enabled. `deltaMs` is the
// wall-clock time elapsed since the previous frame in milliseconds;
// pass 0.0 on the first frame (no previous sample) and the FpsCounter
// will simply skip that sample.
void draw(MinecraftUIRenderContext& ctx, double deltaMs);

} // namespace f3_debug::overlay
