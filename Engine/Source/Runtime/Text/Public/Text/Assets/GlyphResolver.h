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

    /// Bake-pixel geometry scale used by MSDF plane bounds / advances.
    [[nodiscard]] float EffectiveGeometryScale() const {
        if (geometryScale > 1.5f) {
            return geometryScale;
        }
        // Legacy .wefont assets stored geometryScale=1 while plane bounds are bake-pixel sized.
        return std::max(bakeSizePx, 1.0f);
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
