#include "Icons/Compile/IconCompileDetail.h"
#include "Icons/Core/IconTypes.h"
#include "Icons/Core/KindIconsManifest.h"

#include <algorithm>
#include <filesystem>
#include <string_view>

namespace we::runtime::icons::compiling::detail {
namespace {

void BlitRgbaOntoAtlas(
    std::vector<uint8_t>& atlasRgba,
    uint32_t atlasWidth,
    uint32_t atlasHeight,
    const std::vector<uint8_t>& srcRgba,
    uint32_t srcWidth,
    uint32_t srcHeight,
    uint32_t dstX,
    uint32_t dstY,
    uint32_t dstWidth,
    uint32_t dstHeight)
{
    if (atlasRgba.empty() || srcRgba.empty() || atlasWidth == 0 || atlasHeight == 0) {
        return;
    }

    // Number of pixels to actually copy: constrained by both src and dst dimensions and
    // atlas bounds. This never up-scales or down-scales — it is a 1:1 pixel copy.
    const uint32_t copyW = std::min(srcWidth, std::min(dstWidth, atlasWidth > dstX ? atlasWidth - dstX : 0u));
    const uint32_t copyH = std::min(srcHeight, std::min(dstHeight, atlasHeight > dstY ? atlasHeight - dstY : 0u));
    if (copyW == 0 || copyH == 0) {
        return;
    }

    // Source crop offset: if the PNG is wider/taller than the slot, read from the center.
    const uint32_t srcOffsetX = srcWidth  > copyW ? (srcWidth  - copyW) / 2u : 0u;
    const uint32_t srcOffsetY = srcHeight > copyH ? (srcHeight - copyH) / 2u : 0u;

    // Destination centering offset: if the slot is wider/taller than the PNG,
    // write into the center of the slot so the icon art is pixel-perfectly centered.
    // Without this offset the icon lands at the top-left corner of the slot, causing
    // the runtime UV rect (which covers the whole slot) to sample blank atlas pixels
    // next to the icon and produce blurry output.
    const uint32_t dstOffsetX = dstWidth  > copyW ? (dstWidth  - copyW) / 2u : 0u;
    const uint32_t dstOffsetY = dstHeight > copyH ? (dstHeight - copyH) / 2u : 0u;

    for (uint32_t y = 0; y < copyH; ++y) {
        for (uint32_t x = 0; x < copyW; ++x) {
            const size_t srcIndex = (static_cast<size_t>(srcOffsetY + y) * srcWidth  + (srcOffsetX + x)) * 4u;
            const size_t dstIndex = (static_cast<size_t>(dstY + dstOffsetY + y) * atlasWidth + (dstX + dstOffsetX + x)) * 4u;
            if (srcIndex + 3 >= srcRgba.size() || dstIndex + 3 >= atlasRgba.size()) {
                continue;
            }
            atlasRgba[dstIndex + 0] = srcRgba[srcIndex + 0];
            atlasRgba[dstIndex + 1] = srcRgba[srcIndex + 1];
            atlasRgba[dstIndex + 2] = srcRgba[srcIndex + 2];
            atlasRgba[dstIndex + 3] = srcRgba[srcIndex + 3];
        }
    }
}

std::filesystem::path KindIconPngPath(
    const std::filesystem::path& kindIconsDir,
    std::string_view sourceStem,
    uint32_t tierPx)
{
    return kindIconsDir / ("icon_" + std::string(sourceStem) + "__" + std::to_string(tierPx) + ".png");
}

} // namespace

std::filesystem::path ResolveKindIconsDirectory(const std::filesystem::path& atlasInputDir)
{
    const std::filesystem::path candidate = atlasInputDir.parent_path() / "kindicons" / "icons";
    std::error_code ec;
    if (std::filesystem::is_directory(candidate, ec)) {
        return candidate;
    }
    return {};
}

uint32_t ApplyKindIconsOverlay(
    std::vector<uint8_t>& atlasRgba,
    uint32_t atlasWidth,
    uint32_t atlasHeight,
    uint32_t tierPx,
    const std::filesystem::path& kindIconsDir,
    const std::vector<ParsedAtlasRegion>& regions)
{
    if (kindIconsDir.empty() || atlasRgba.empty() || atlasWidth == 0 || atlasHeight == 0) {
        return 0;
    }
    if (!IsEssentialUiAtlasTier(tierPx)) {
        return 0;
    }

    uint32_t applied = 0;
    for (const auto& binding : kindicons::kBindings) {
        const auto pngPath = KindIconPngPath(kindIconsDir, binding.sourceStem, tierPx);
        if (!std::filesystem::exists(pngPath)) {
            continue;
        }

        const ParsedAtlasRegion* targetRegion = nullptr;
        for (const auto& region : regions) {
            if (ResolveRuntimeIconName(region.sourceName) == binding.runtimeName) {
                targetRegion = &region;
                break;
            }
        }
        if (targetRegion == nullptr) {
            continue;
        }

        std::vector<uint8_t> srcRgba;
        uint32_t srcWidth = 0;
        uint32_t srcHeight = 0;
        if (!LoadPngRgbaNative(pngPath, srcRgba, srcWidth, srcHeight)) {
            continue;
        }

        // The kindicon filename encodes the native tier size (e.g. icon_settings__24.png = 24×24).
        // If the decoded PNG dimensions don't match the tier we are compiling, the art was
        // authored at the wrong resolution. Skip it rather than blit a mis-sized glyph.
        if (srcWidth != tierPx || srcHeight != tierPx) {
            continue;
        }

        BlitRgbaOntoAtlas(
            atlasRgba,
            atlasWidth,
            atlasHeight,
            srcRgba,
            srcWidth,
            srcHeight,
            targetRegion->x,
            targetRegion->y,
            targetRegion->width,
            targetRegion->height);
        ++applied;
    }

    return applied;
}

} // namespace we::runtime::icons::compiling::detail
