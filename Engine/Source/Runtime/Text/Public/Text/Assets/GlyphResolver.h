#pragma once

#include "Text/Assets/FontAsset.h"
#include "Text/Atlas/FontAtlasManager.h"
#include "Text/Core/Types.h"
#include "Text/Export.h"
#include "Text/Shaping/TextShaper.h"

#include <algorithm>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace we::runtime::text::assets {

struct FontStack {
    std::vector<FontHandle> faces;
};

struct ResolvedGlyph {
    FontHandle fontHandle = kInvalidFontHandle;
    GlyphMetrics metrics{};
    float bakeSizePx = 18.0f;
    float msdfPixelRange = 4.0f;
    float geometryScale = 18.0f;
    float ascender = 0.0f;
    float descender = 0.0f;
    float lineHeight = 0.0f;
    uint64_t atlasPageVersion = 0;

    /// Scale that converts stored plane bounds / advances into layout pixels.
    /// Detects em-normalized vs bake-pixel assets so mis-tagged geometryScale cannot
    /// collapse glyph quads to sub-pixel "dots" while HarfBuzz advances stay correct.
    [[nodiscard]] float EffectiveGeometryScale() const {
        const float sample = std::max(
            std::abs(metrics.advance),
            std::max(std::abs(metrics.bounds.width), std::abs(metrics.bounds.height)));
        const bool looksLikeEm = sample > 0.0f && sample < 2.0f;
        const bool looksLikeBakePx = sample >= 2.0f;

        if (looksLikeEm) {
            return 1.0f;
        }
        if (geometryScale > 1.5f) {
            return geometryScale;
        }
        if (looksLikeBakePx) {
            return std::max(bakeSizePx, 1.0f);
        }
        return geometryScale > 0.0f ? geometryScale : std::max(bakeSizePx, 1.0f);
    }
};

class TEXT_API IFallbackResolver {
public:
    virtual ~IFallbackResolver() = default;
    [[nodiscard]] virtual FontStack BuildStack(
        const std::string& family,
        shaping::Script script) const = 0;
};

class TEXT_API IGlyphResolver {
public:
    virtual ~IGlyphResolver() = default;
    [[nodiscard]] virtual ResolvedGlyph Resolve(
        const FontStack& stack,
        Codepoint codepoint) const = 0;
    virtual void ClearCache() = 0;
};

class FallbackResolver final : public IFallbackResolver {
public:
    explicit FallbackResolver(IFontAssetManager& assetManager);
    [[nodiscard]] FontStack BuildStack(
        const std::string& family,
        shaping::Script script) const override;

private:
    IFontAssetManager& m_AssetManager;
};

class GlyphResolver final : public IGlyphResolver {
public:
    GlyphResolver(
        IFontAssetManager& assetManager,
        IFallbackResolver& fallbackResolver,
        atlas::IFontAtlasManager* atlasManager = nullptr);

    [[nodiscard]] ResolvedGlyph Resolve(
        const FontStack& stack,
        Codepoint codepoint) const override;
    void ClearCache() override;
    void SetAtlasManager(atlas::IFontAtlasManager* atlasManager) { m_AtlasManager = atlasManager; }

private:
    IFontAssetManager& m_AssetManager;
    IFallbackResolver& m_FallbackResolver;
    atlas::IFontAtlasManager* m_AtlasManager = nullptr;
    mutable std::unordered_map<uint64_t, ResolvedGlyph> m_Cache;
};

[[nodiscard]] TEXT_API std::unique_ptr<IFallbackResolver> CreateFallbackResolver(IFontAssetManager& assetManager);
[[nodiscard]] TEXT_API std::unique_ptr<IGlyphResolver> CreateGlyphResolver(
    IFontAssetManager& assetManager,
    IFallbackResolver& fallbackResolver,
    atlas::IFontAtlasManager* atlasManager = nullptr);

} // namespace we::runtime::text::assets
