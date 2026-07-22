// src/f3_debug/overlay/Overlay.cpp

#include "f3_debug/overlay/Overlay.h"

#include "f3_debug/F3Debug.h"
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
#include <mc/world/level/biome/Biome.h>
#include <mc/world/level/BlockPos.h>
#include <mc/world/level/BlockSource.h>
#include <mc/world/level/dimension/Dimension.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
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

// Build the F3 lines. The FPS counter is ticked with the delta passed
// in from the render listener so the value we render is the same
// sample we just recorded.
//
// Layout (Java Edition F3 style, with a few Bedrock-specific extras):
//
//   Minecraft Bedrock (BedrockF3)
//   FPS:  216   Frame: 4.41 ms
//
//   XYZ: 511.052 / 11.620 / 510.411
//   Block: 511 11 510
//   Chunk: 31 31 [15 02]
//   Facing: south (Towards positive Z) (0.5 / -1.4)
//   Biome: plains
//   Section-relative: 15 11 02
//
//   Dimension: Overworld
//   Uptime: 00:03:03
std::vector<Line> buildLines(double frameDeltaMs) {
    std::vector<Line> lines;

    auto& fps = frameState();
    // Tick the FPS counter with the actual frame delta. On the very
    // first frame after enable() the listener has no previous sample
    // and passes 0.0; in that case tick() returns 0 and we just
    // display "FPS:    0   Frame: 0.00 ms" for that single frame.
    const int   fpsVal = static_cast<int>(fps.tick(frameDeltaMs));
    // Compute the frame time from the smoothed FPS so the two values
    // always agree. lastDeltaMs() returns the raw delta between two
    // consecutive AfterUIRenderEvent calls, which Bedrock may fire
    // multiple times per frame, so it can disagree with the smoothed
    // FPS at high frame rates.
    const double frameMs = fpsVal > 0 ? 1000.0 / static_cast<double>(fpsVal) : 0.0;
    lines.push_back(makeLine("Minecraft Bedrock (BedrockF3)", kColorHeader));
    lines.push_back(makeLine(std::format("FPS: {:>4}   Frame: {:.2f} ms",
        fpsVal, frameMs), kColorBody));
    lines.push_back({}); // spacer

    auto& client = *ll::service::getClientInstance();
    LocalPlayer* player = client.getLocalPlayer();
    if (player != nullptr) {
        const Vec3 pos = player->getPosition();
        const Vec2 rot = player->getRotation();

        const int   blockX = static_cast<int>(std::floor(pos.x));
        const int   blockY = static_cast<int>(std::floor(pos.y));
        const int   blockZ = static_cast<int>(std::floor(pos.z));
        const int   chunkX = blockX >> 4;
        const int   chunkZ = blockZ >> 4;
        // Section-relative position is the offset within the 16x16x16
        // sub-chunk. C++ % on a negative integer can return a negative
        // result, so we normalize to [0, 16) explicitly.
        const int   inChunkX   = ((blockX & 15) + 16) % 16;
        const int   inChunkZ   = ((blockZ & 15) + 16) % 16;
        const int   inSectionX = inChunkX;
        const int   inSectionY = ((blockY & 15) + 16) % 16;
        const int   inSectionZ = inChunkZ;

        // Look up the biome at the player's block position. tryGetBiome
        // returns nullptr if the chunk is not loaded; fall back to
        // "unknown" in that case rather than crashing.
        std::string biomeName = "unknown";
        try {
            auto& blockSource = player->getDimensionBlockSource();
            BlockPos bp(pos.x, pos.y, pos.z);
            if (auto* biome = blockSource.tryGetBiome(bp); biome != nullptr) {
                // mHash is wrapped in ll::TypedStorage, so we need to
                // apply operator-> to get the underlying HashedString,
                // then call getString() on that.
                biomeName = biome->mHash->getString();
            }
        } catch (std::exception const& e) {
            // Some dimensions (e.g. older custom ones) can throw on
            // biome access. Log the message so it's not completely
            // hidden, then keep the fallback name for this frame.
            F3Debug::getInstance().getSelf().getLogger().warn(
                "biome lookup failed: {}", e.what());
        }

        lines.push_back(makeLine(std::format("XYZ: {:.3f} / {:.3f} / {:.3f}",
            pos.x, pos.y, pos.z), kColorOk));
        lines.push_back(makeLine(std::format("Block: {} {} {}",
            blockX, blockY, blockZ), kColorBody));
        lines.push_back(makeLine(std::format("Chunk: {} {} [{:02d} {:02d}]",
            chunkX, chunkZ, inChunkX, inChunkZ), kColorBody));
        // Java format: "Facing: <cardinal> (Towards <axis>) (<yaw> / <pitch>)"
        lines.push_back(makeLine(std::format("Facing: {} ({}) ({:.1f} / {:.1f})",
            util::cardinalDirection(rot.y),
            util::cardinalAxisName(rot.y),
            rot.y, rot.x), kColorBody));
        lines.push_back(makeLine(std::format("Biome: {}", biomeName), kColorBody));
        lines.push_back(makeLine(std::format("Section-relative: {:02d} {:02d} {:02d}",
            inSectionX, inSectionY, inSectionZ), kColorBody));
    } else {
        // The render listener already early-outs on null LocalPlayer
        // before calling us, so this branch is defensive only.
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

    // Bedrock-specific extras at the bottom. Java doesn't have a
    // session timer, but it's useful for tracking how long a debug
    // session has been running.
    lines.push_back(makeLine(std::format("Uptime: {}", sessionState().format()), kColorBody));

    return lines;
}

// Position the panel at the top-left of the screen.
constexpr int kPanelX     = 4;
constexpr int kPanelY     = 4;
constexpr int kPadding    = 6;
constexpr float kTextScale = 1.0f;

// Bedrock's default font (Mojangles) has a 9-pixel line height at scale 1.0.
// Font has no public line-height accessor, so we hardcode the value. If a
// non-default font is active the value may differ; in that case the panel
// background height will need to be recomputed.
constexpr int kLineHeightPx = 9;

void draw(MinecraftUIRenderContext& ctx, double deltaMs) {
    const auto lines = buildLines(deltaMs);

    // The Font used for in-game debug strings. MinecraftUIRenderContext
    // keeps the debug FontHandle in a private member (`mDebugTextFontHandle`).
    // There is no public getter for it, so we reach into the member via
    // offsetof. If your build of LeviLamina adds a public getter later,
    // replace this with the proper call.
    auto& fontHandle = ll::memory::dAccess<FontHandle>(
        &ctx,
        offsetof(MinecraftUIRenderContext, mDebugTextFontHandle));
    Font& font = fontHandle.getFont();

    // Measure each line so we can size the background rectangle. getLineLength
    // returns the rendered WIDTH of a string, not its height -- we use the
    // hardcoded kLineHeightPx for the vertical stride since Font has no
    // public line-height accessor.
    int maxPxW = 0;
    for (auto const& l : lines) {
        if (l.text.empty()) {
            continue;
        }
        maxPxW = std::max(maxPxW,
            ctx.getLineLength(font, l.text, kTextScale, /*showColorSymbol=*/false));
    }
    const int linePxH = static_cast<int>(static_cast<float>(kLineHeightPx) * kTextScale);
    const int boxW    = maxPxW + kPadding * 2;
    const int boxH    = linePxH * static_cast<int>(lines.size()) + kPadding * 2;
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
            // TextMeasureData / CaretMeasureData have no usable default
            // constructor in LeviLamina 26.20.4 (the header says
            // "prevent constructor by default"). Use the MCAPI ctor with
            // the defaults the game uses for HUD debug text:
            //   fontSize=kTextScale, no padding, shadow on, no color
            //   symbol, no hyphen hiding, alignment=Left.
            //   caret position 0, don't render the caret.
            TextMeasureData{
                kTextScale,
                0.0f,
                /*renderShadow=*/true,
                /*showColorSymbol=*/false,
                /*hideHyphen=*/false,
                ui::TextAlignment::Left
            },
            CaretMeasureData{
                /*position=*/0,
                /*shouldRender=*/false
            });
        y += static_cast<float>(linePxH);
    }

    ctx.flushText(0.0f, std::nullopt);
}

} // namespace f3_debug::overlay
