#pragma once

#include "Text/Assets/FontAsset.h"
#include "Text/Core/FontTypes.h"
#include "Text/Core/Types.h"
#include "Text/Export.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace we::runtime::text::atlas {

struct GlyphAtlasKey {
    FontHandle fontHandle = kInvalidFontHandle;
    Codepoint codepoint = 0;
    uint16_t sizeBucket = 0;

    [[nodiscard]] bool operator==(const GlyphAtlasKey& other) const noexcept {
        return fontHandle == other.fontHandle
            && codepoint == other.codepoint
            && sizeBucket == other.sizeBucket;
    }
};

struct GlyphAtlasKeyHash {
    [[nodiscard]] size_t operator()(const GlyphAtlasKey& key) const noexcept {
        return (static_cast<size_t>(key.fontHandle) * 1315423911u)
            ^ (static_cast<size_t>(key.codepoint) * 2654435761u)
            ^ (static_cast<size_t>(key.sizeBucket) * 97531u);
    }
};

struct AtlasGlyphEntry {
    GlyphMetrics metrics{};
    float bakeSizePx = 18.0f;
    float msdfPixelRange = 4.0f;
    float geometryScale = 1.0f;
    uint32_t pageIndex = 0;
    uint64_t pageVersion = 0;
    uint64_t lastUsedFrame = 0;
};

struct AtlasPageRuntime {
    AtlasPage page{};
    uint64_t version = 1;
    uint32_t packX = 0;
    uint32_t packY = 0;
    uint32_t rowHeight = 0;
    bool dirty = true;
};

class TEXT_API IFontAtlasManager {
public:
    virtual ~IFontAtlasManager() = default;

    /// Seed dynamic atlas from a prebaked .wefont page set (startup cache).
    virtual void SeedFromFontAsset(FontHandle fontHandle, const assets::FontAsset& asset) = 0;

    /// Pack a glyph bitmap into the atlas (RGBA MSDF). Returns entry with UVs.
    [[nodiscard]] virtual std::optional<AtlasGlyphEntry> PackGlyph(
        const GlyphAtlasKey& key,
        const GlyphMetrics& sourceMetrics,
        std::span<const uint8_t> rgba,
        uint32_t width,
        uint32_t height,
        float bakeSizePx,
        float msdfPixelRange,
        float geometryScale) = 0;

    [[nodiscard]] virtual std::optional<AtlasGlyphEntry> Find(const GlyphAtlasKey& key) const = 0;
    virtual void Touch(const GlyphAtlasKey& key) = 0;
    virtual void BeginFrame(uint64_t frameIndex) = 0;
    virtual void EvictUnused(size_t maxEntries) = 0;

    [[nodiscard]] virtual const AtlasPageRuntime* GetPage(uint32_t pageIndex) const = 0;
    [[nodiscard]] virtual std::optional<AtlasPageRuntime> CopyPage(uint32_t pageIndex) const = 0;
    [[nodiscard]] virtual size_t PageCount() const = 0;
    [[nodiscard]] virtual uint64_t Generation() const = 0;
    [[nodiscard]] virtual std::vector<uint32_t> TakeDirtyPages() = 0;

    virtual void SetPageSize(uint32_t width, uint32_t height) = 0;
};

[[nodiscard]] TEXT_API std::unique_ptr<IFontAtlasManager> CreateFontAtlasManager(
    uint32_t pageWidth = 2048,
    uint32_t pageHeight = 2048);

} // namespace we::runtime::text::atlas
