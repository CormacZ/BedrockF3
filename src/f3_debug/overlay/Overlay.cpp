// src/f3_debug/overlay/Overlay.cpp

#include "f3_debug/overlay/Overlay.h"

#include "f3_debug/F3Debug.h"
#include "f3_debug/util/CardinalDirection.h"

#include <ll/api/Versions.h>
#include <ll/api/memory/Memory.h>
#include <ll/api/service/TargetedBedrock.h>

#include <mc/client/game/ClientInstance.h>
#include <mc/client/game/IClientInstance.h>
#include <mc/client/gui/GuiData.h>
#include <mc/client/gui/ScreenSizeData.h>
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
// Translucent gray for the per-line background boxes. Java Edition's
// F3 panel uses a similar dark-gray, very-transparent background
// (we go 20% alpha to match the subtle, ghostly look).
constexpr std::array<float, 4> kColorBg     = {0.15f, 0.15f, 0.15f, 0.2f};

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
// Returns the LEFT column (player/world info) and the RIGHT column
// (system info) as separate vectors. The render function draws the
// left column flush-left at (kPanelX, kPanelY) and the right column
// flush-right at (screenW - kBoxRightPad, kPanelY).
//
// Layout (Java Edition F3 style, with a few Bedrock-specific extras):
//
//   LEFT                          RIGHT
//   -----                         -----
//   Minecraft Bedrock (BedrockF3) Minecraft 1.21.11 (1.21.11/<build>)
//   FPS:  216   Frame: 4.41 ms    Display: 1920x1080
//
//   XYZ: 511.052 / 11.620 / ...
//   Block: 511 11 510
//   Chunk: 31 31 [15 02]
//   Facing: south (Towards positive Z) (0.5 / -1.4)
//   Biome: plains
//   Section-relative: 15 11 02
//
//   Dimension: Overworld
//   Uptime: 00:03:03
struct PanelLines {
    std::vector<Line> left;
    std::vector<Line> right;
};

PanelLines buildLines(double frameDeltaMs) {
    PanelLines lines;

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
    lines.left.push_back(makeLine("Minecraft Bedrock (BedrockF3)", kColorHeader));
    lines.left.push_back(makeLine(std::format("FPS: {:>4}   Frame: {:.2f} ms",
        fpsVal, frameMs), kColorBody));
    lines.left.push_back({}); // spacer

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

        lines.left.push_back(makeLine(std::format("XYZ: {:.3f} / {:.3f} / {:.3f}",
            pos.x, pos.y, pos.z), kColorOk));
        lines.left.push_back(makeLine(std::format("Block: {} {} {}",
            blockX, blockY, blockZ), kColorBody));
        lines.left.push_back(makeLine(std::format("Chunk: {} {} [{:02d} {:02d}]",
            chunkX, chunkZ, inChunkX, inChunkZ), kColorBody));
        // Java format: "Facing: <cardinal> (Towards <axis>) (<yaw> / <pitch>)"
        lines.left.push_back(makeLine(std::format("Facing: {} ({}) ({:.1f} / {:.1f})",
            util::cardinalDirection(rot.y),
            util::cardinalAxisName(rot.y),
            rot.y, rot.x), kColorBody));
        lines.left.push_back(makeLine(std::format("Biome: {}", biomeName), kColorBody));
        lines.left.push_back(makeLine(std::format("Section-relative: {:02d} {:02d} {:02d}",
            inSectionX, inSectionY, inSectionZ), kColorBody));
    } else {
        // The render listener already early-outs on null LocalPlayer
        // before calling us, so this branch is defensive only.
        lines.left.push_back(makeLine("Player unavailable (join a world to populate)", kColorWarn));
    }

    lines.left.push_back({}); // spacer
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
        lines.left.push_back(makeLine(std::format("Dimension: {}", name), kColorBody));
    }

    // Bedrock-specific extras at the bottom. Java doesn't have a
    // session timer, but it's useful for tracking how long a debug
    // session has been running.
    lines.left.push_back(makeLine(std::format("Uptime: {}", sessionState().format()), kColorBody));

    return lines;
}

// Position the panel at the top-left of the screen.
constexpr int kPanelX      = 4;
constexpr int kPanelY      = 4;
// Per-line padding: 2px of slack between the text and the left edge
// of its background box, 4px on the right. Matches Java Edition's
// F3 panel where the text is nearly flush-left with a small right
// pad.
constexpr int kBoxLeftPad  = 2;
constexpr int kBoxRightPad = 4;
constexpr float kTextScale  = 1.0f;

// Bedrock's default font (Mojangles) has a 9-pixel line height at scale 1.0.
// Font has no public line-height accessor, so we hardcode the value. If a
// non-default font is active the value may differ; in that case the panel
// background height will need to be recomputed.
constexpr int kLineHeightPx = 9;

// Build the right column. The right column shows system info, similar
// to Java Edition's F3 panel:
//
//   Minecraft 1.21.11 (1.21.11/<commit>)
//   Display: 1920x1080
//
// GPU name, CPU, OpenGL version, and the rest of Java's right column
// are deliberately deferred to a follow-up -- they need Windows API
// (DXGI for GPU, __cpuid for CPU brand, GetSystemMetrics for the
// display device name) which is a chunkier change.
std::vector<Line> buildRightLines() {
    std::vector<Line> lines;

    // Game version. ll::getGameVersion() returns a data::Version parsed
    // from Common::getBuildInfo(), including the build/commit suffix.
    // We use the bare to_string() (e.g. "1.21.11+abc1234") and wrap it
    // in Java's "Minecraft <ver> (<ver>)" format.
    const auto ver        = ll::getGameVersion();
    const auto versionStr = ver.to_string();
    lines.push_back(makeLine(std::format("Minecraft {}", versionStr), kColorBody));

    // Display resolution. The totalScreenSize is the full window size
    // reported by the platform layer (in pixels at the current
    // monitor's scale). For a 1080p monitor at 100% scale, this is
    // 1920x1080. For a 4K monitor at 200% scale, Bedrock reports
    // 3840x2160 (the OS handles the DPI scaling for the render
    // target, so the number you see here matches the panel you see).
    int screenW = 0;
    int screenH = 0;
    try {
        auto& client   = *ll::service::getClientInstance();
        auto  guiData  = client.getGuiData();
        const auto& ss = guiData->getScreenSizeData().totalScreenSize.get();
        screenW       = static_cast<int>(ss.x);
        screenH       = static_cast<int>(ss.y);
    } catch (std::exception const& e) {
        // ClientInstance / GuiData can be null during early boot or
        // shutdown. Log the message so it's not completely hidden,
        // then fall through with screenW=0 so we render a placeholder.
        F3Debug::getInstance().getSelf().getLogger().warn(
            "right-column screen-size lookup failed: {}", e.what());
    }
    if (screenW > 0 && screenH > 0) {
        lines.push_back(makeLine(std::format("Display: {}x{}", screenW, screenH), kColorBody));
    } else {
        lines.push_back(makeLine("Display: unknown", kColorBody));
    }

    return lines;
}

void draw(MinecraftUIRenderContext& ctx, double deltaMs) {
    const auto panel = buildLines(deltaMs);
    const auto right = buildRightLines();

    // The Font used for in-game debug strings. MinecraftUIRenderContext
    // keeps the debug FontHandle in a private member (`mDebugTextFontHandle`).
    // There is no public getter for it, so we reach into the member via
    // offsetof. If your build of LeviLamina adds a public getter later,
    // replace this with the proper call.
    auto& fontHandle = ll::memory::dAccess<FontHandle>(
        &ctx,
        offsetof(MinecraftUIRenderContext, mDebugTextFontHandle));
    Font& font = fontHandle.getFont();

    const int   linePxH = static_cast<int>(static_cast<float>(kLineHeightPx) * kTextScale);
    const float x0      = static_cast<float>(kPanelX);
    const float y0      = static_cast<float>(kPanelY);

    // Screen width for right-column anchoring. If the screen size
    // isn't available (very early/late frame) we still want to draw
    // the left column; the right column will just be skipped.
    int screenW = 0;
    try {
        auto& client   = *ll::service::getClientInstance();
        auto  guiData  = client.getGuiData();
        const auto& ss = guiData->getScreenSizeData().totalScreenSize.get();
        screenW       = static_cast<int>(ss.x);
    } catch (std::exception const& e) {
        F3Debug::getInstance().getSelf().getLogger().warn(
            "draw() screen-size lookup failed: {}", e.what());
    }

    // Java Edition F3 style: one small translucent gray box per
    // non-empty line, sized to that line's text width. Spacer lines
    // (empty text) get no background, so the boxes don't bleed into
    // the gaps between logical sections.
    auto drawColumn = [&](std::vector<Line> const& col, bool isRight) {
        float y = y0;
        for (auto const& l : col) {
            if (l.text.empty()) {
                y += static_cast<float>(linePxH);
                continue;
            }

            const int textW = ctx.getLineLength(
                font, l.text, kTextScale, /*showColorSymbol=*/false);
            const float boxY0 = y;
            const float boxY1 = y + static_cast<float>(linePxH);
            // For the right column we anchor the box's RIGHT edge to
            // the screen edge (minus the right pad). For the left
            // column we anchor the box's LEFT edge to the panel x.
            const float boxX0 = isRight
                ? static_cast<float>(screenW) - static_cast<float>(textW) - static_cast<float>(kBoxRightPad)
                : x0;
            const float boxX1 = isRight
                ? static_cast<float>(screenW)
                : x0 + static_cast<float>(textW) + static_cast<float>(kBoxRightPad);

            // Draw the per-line translucent background box. RectangleArea's
            // 4-float ctor requires the bool checkForValidity 5th arg; pass
            // true to opt into the bounds check.
            RectangleArea bg{
                boxX0, boxY0, boxX1, boxY1,
                /*checkForValidity=*/true
            };
            mce::Color bgColor{kColorBg[0], kColorBg[1], kColorBg[2], kColorBg[3]};
            ctx.fillRectangle(bg, bgColor, 1.0f);

            // Draw the text inside the box. Left column is anchored
            // at the box's left + kBoxLeftPad; right column is right-
            // aligned to the box's right edge.
            const float textX0 = isRight
                ? boxX0
                : x0 + static_cast<float>(kBoxLeftPad);
            const float textX1 = isRight
                ? boxX0 + static_cast<float>(textW)
                : x0 + static_cast<float>(kBoxLeftPad) + static_cast<float>(textW);
            RectangleArea lineRect{
                textX0, y, textX1, y + static_cast<float>(linePxH),
                /*checkForValidity=*/true
            };
            mce::Color lineColor{l.color.r, l.color.g, l.color.b, l.color.a};
            // ui::TextAlignment has only Left, Right, Center.
            ctx.drawText(
                font,
                lineRect,
                std::string{l.text},
                lineColor,
                1.0f,
                isRight ? ui::TextAlignment::Right : ui::TextAlignment::Left,
                // TextMeasureData / CaretMeasureData have no usable default
                // constructor in LeviLamina 26.20.4. Use the MCAPI ctor.
                TextMeasureData{
                    kTextScale,
                    0.0f,
                    /*renderShadow=*/true,
                    /*showColorSymbol=*/false,
                    /*hideHyphen=*/false,
                    isRight ? ui::TextAlignment::Right : ui::TextAlignment::Left
                },
                CaretMeasureData{
                    /*position=*/0,
                    /*shouldRender=*/false
                });
            y += static_cast<float>(linePxH);
        }
    };

    drawColumn(panel.left, /*isRight=*/false);
    if (screenW > 0) {
        drawColumn(right, /*isRight=*/true);
    }

    ctx.flushText(0.0f, std::nullopt);
}

} // namespace f3_debug::overlay
