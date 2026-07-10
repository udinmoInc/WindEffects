#pragma once

#include "Text/Assets/FontAsset.h"
#include "Text/Core/Types.h"
#include "Text/Export.h"

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

class TEXT_API FallbackResolver final : public IFallbackResolver {
public:
    explicit FallbackResolver(IFontAssetManager& assetManager);
    [[nodiscard]] FontStack BuildStack(
        const std::string& family,
        shaping::Script script) const override;

private:
    IFontAssetManager& m_AssetManager;
};

class TEXT_API GlyphResolver final : public IGlyphResolver {
public:
    GlyphResolver(IFontAssetManager& assetManager, IFallbackResolver& fallbackResolver);

    [[nodiscard]] ResolvedGlyph Resolve(
        const FontStack& stack,
        Codepoint codepoint) const override;
    void ClearCache() override;

private:
    IFontAssetManager& m_AssetManager;
    IFallbackResolver& m_FallbackResolver;
    mutable std::unordered_map<uint64_t, ResolvedGlyph> m_Cache;
};

[[nodiscard]] TEXT_API std::unique_ptr<IFallbackResolver> CreateFallbackResolver(IFontAssetManager& assetManager);
[[nodiscard]] TEXT_API std::unique_ptr<IGlyphResolver> CreateGlyphResolver(
    IFontAssetManager& assetManager,
    IFallbackResolver& fallbackResolver);

} // namespace we::runtime::text::assets
