// src/f3_debug/overlay/Overlay.cpp

#include "f3_debug/overlay/Overlay.h"

#include "f3_debug/util/CardinalDirection.h"

#include <ll/api/memory/Memory.h>
#include <ll/api/service/TargetedBedrock.h>

#include <mc/client/game/ClientInstance.h>
#include <mc/client/game/IClientInstance.h>
#include <mc/client/renderer/screen/MinecraftUIRenderContext.h>
#include <mc/client/gui/FontHandle.h>
#include <mc/client/gui/TextAlignment.h>
#include <mc/client/gui/controls/UIRenderContext.h>
#include <mc/client/player/LocalPlayer.h>
#include <mc/deps/core/math/Color.h>
#include <mc/deps/core/utility/NonOwnerPointer.h>
#include <mc/deps/input/RectangleArea.h>
#include <mc/client/gui/CaretMeasureData.h>
#include <mc/client/gui/TextMeasureData.h>
#include <mc/world/level/dimension/Dimension.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <format>
#include <span>
#include <string_view>
#include <utility>

namespace f3_debug::overlay {

namespace {

// Per-line colors. Indexed into by a small enum below.
constexpr std::array<float, 4> kColorHeader = {0.40f, 0.70f, 1.00f, 1.0f};
constexpr std::array<float, 4> kColorBody   = {1.00f, 1.00f, 1.00f, 1.0f};
constexpr std::array<float, 4> kColorOk     = {0.55f, 1.00f, 0.55f, 1.0f};
constexpr std::array<float, 4> kColorWarn   = {1.00f, 0.85f, 0.30f, 1.0f};
constexpr std::array<float, 4> kColorBg     = {0.00f, 0.00f, 0.00f, 0.55f};

Line makeLine(std::string text, std::span<const float, 4> color) {
    return Line{
        std::move(text),
        {color[0], color[1], color[2], color[3]}
    };
}

// Frame-local state. A function-local static lives for the process
// lifetime and is thread-safe to construct under C++11.
util::FpsCounter& frameState() {
    static util::FpsCounter fps;
    return fps;
}

util::Uptime& sessionState() {
    static util::Uptime up;
    return up;
}

} // namespace

// Build the F3 lines. The FPS counter is ticked here so the value
// we render is consistent with the one we just sampled.
std::vector<Line> buildLines() {
    std::vector<Line> lines;

    auto& fps  = frameState();
    const double ms = fps.lastDeltaMs();
    lines.push_back(makeLine("Minecraft Bedrock (BedrockF3)", kColorHeader));
    lines.push_back(makeLine(std::format("FPS: {:>4}   Frame: {:.2f} ms",
        static_cast<int>(fps.tick(ms > 0.0 ? ms : 0.0)), ms), kColorBody));
    lines.push_back(makeLine(std::format("Uptime: {}", sessionState().format()), kColorBody));
    lines.push_back({}); // spacer

    auto& client = *ll::service::getClientInstance();
    LocalPlayer* player = client.getLocalPlayer();
    if (player != nullptr) {
        const Vec3 pos = player->getPosition();
        const int chunkX = static_cast<int>(std::floor(pos.x)) >> 4;
        const int chunkZ = static_cast<int>(std::floor(pos.z)) >> 4;
        lines.push_back(makeLine(std::format("XYZ: {:.3f} / {:.3f} / {:.3f}",
            pos.x, pos.y, pos.z), kColorOk));
        lines.push_back(makeLine(std::format("Chunk: {} / {}  (in chunk: {:.2f}, {:.2f})",
            chunkX, chunkZ, pos.x - chunkX * 16.0, pos.z - chunkZ * 16.0), kColorBody));

        const Vec2 rot = player->getRotation();
        lines.push_back(makeLine(std::format("Facing: {} ({:.1f} deg)",
            util::cardinalDirection(rot.y), rot.y), kColorBody));
        lines.push_back(makeLine(std::format("Pitch: {:.1f} deg", rot.x), kColorBody));
    } else {
        lines.push_back(makeLine("Player unavailable (join a world to populate)", kColorWarn));
    }

    lines.push_back({}); // spacer
    if (player != nullptr) {
        // getLevel() returns Level& (always non-null while player is alive).
        const int dim = static_cast<int>(player->getDimensionId());
        std::string name = "Unknown";
        switch (dim) {
            case 0: name = "Overworld"; break;
            case 1: name = "Nether";    break;
            case 2: name = "The End";   break;
            default: name = std::format("Dimension {}", dim); break;
        }
        lines.push_back(makeLine(std::format("Dimension: {}", name), kColorBody));
    }

    lines.push_back({}); // spacer
    lines.push_back(makeLine("Press F3 to toggle this overlay", kColorHeader));

    return lines;
}

// Position the panel at the top-left of the screen.
constexpr int kPanelX     = 4;
constexpr int kPanelY     = 4;
constexpr int kPadding    = 6;
constexpr float kTextScale = 1.0f;

void draw(MinecraftUIRenderContext& ctx) {
    const auto lines = buildLines();

    // The Font used for in-game debug strings. MinecraftUIRenderContext
    // keeps the debug FontHandle in a private member (`mDebugTextFontHandle`).
    // There is no public getter for it, so we reach into the member via
    // offsetof. If your build of LeviLamina adds a public getter later,
    // replace this with the proper call.
    auto& fontHandle = ll::memory::dAccess<FontHandle>(
        &ctx,
        offsetof(MinecraftUIRenderContext, mDebugTextFontHandle));
    Font& font = fontHandle.getFont();

    // Measure each line so we can size the background rectangle.
    int   maxPxW = 0;
    int   linePxH = ctx.getLineLength(font, "M", kTextScale, /*showColorSymbol=*/false);
    for (auto const& l : lines) {
        if (l.text.empty()) {
            continue;
        }
        maxPxW = std::max(maxPxW,
            ctx.getLineLength(font, l.text, kTextScale, /*showColorSymbol=*/false));
    }
    const int boxW = maxPxW + kPadding * 2;
    const int boxH = linePxH * static_cast<int>(lines.size()) + kPadding * 2;
    const float x0 = static_cast<float>(kPanelX);
    const float y0 = static_cast<float>(kPanelY);

    // Background. fillRectangle is the supported way to draw a solid
    // rectangle underneath HUD text.
    {
        // RectangleArea's 4-float ctor requires the bool checkForValidity
        // 5th arg. Pass true to opt into the bounds check.
        RectangleArea bg{
            x0, y0,
            x0 + static_cast<float>(boxW),
            y0 + static_cast<float>(boxH),
            /*checkForValidity=*/true
        };
        mce::Color bgColor{0.0f, 0.0f, 0.0f, kColorBg[3]};
        ctx.fillRectangle(bg, bgColor, 1.0f);
    }

    // Each line is queued via drawText; flushText commits all queued text
    // in a single batch.
    float y = y0 + static_cast<float>(kPadding);
    for (auto const& l : lines) {
        if (l.text.empty()) {
            continue;
        }
        RectangleArea lineRect{
            x0 + static_cast<float>(kPadding),
            y,
            x0 + static_cast<float>(kPadding) + static_cast<float>(maxPxW),
            y + static_cast<float>(linePxH),
            /*checkForValidity=*/true
        };
        mce::Color lineColor{l.color.r, l.color.g, l.color.b, l.color.a};
        // ui::TextAlignment has only Left, Right, Center. Use Left for
        // top-left anchored text rendering.
        ctx.drawText(
            font,
            lineRect,
            std::string{l.text},
            lineColor,
            1.0f,
            ui::TextAlignment::Left,
            TextMeasureData{},
            CaretMeasureData{});
        y += static_cast<float>(linePxH);
    }

    ctx.flushText(0.0f, std::nullopt);
}

} // namespace f3_debug::overlay
