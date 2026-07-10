#include "Text/Assets/GlyphResolver.h"

#include "Text/Shaping/TextShaper.h"

#include <algorithm>

namespace we::runtime::text::assets {

FallbackResolver::FallbackResolver(IFontAssetManager& assetManager)
    : m_AssetManager(assetManager)
{
}

FontStack FallbackResolver::BuildStack(
    const std::string& family,
    const shaping::Script script) const
{
    FontStack stack;
    const auto& config = m_AssetManager.GetFontStackConfig();

    for (const FontHandle handle : m_AssetManager.GetLoadedFonts()) {
        const auto asset = m_AssetManager.GetAsset(handle);
        if (!asset) {
            continue;
        }
        if (asset->metrics.familyName == family) {
            stack.faces.push_back(handle);
            break;
        }
    }

    for (const auto& entry : config.fallbackChain) {
        for (const FontHandle handle : m_AssetManager.GetLoadedFonts()) {
            const auto asset = m_AssetManager.GetAsset(handle);
            if (!asset) {
                continue;
            }
            if (asset->metrics.familyName.find(entry.family) != std::string::npos) {
                if (std::find(stack.faces.begin(), stack.faces.end(), handle) == stack.faces.end()) {
                    stack.faces.push_back(handle);
                }
            }
        }
    }

    if (stack.faces.empty()) {
        const auto loaded = m_AssetManager.GetLoadedFonts();
        if (!loaded.empty()) {
            stack.faces.push_back(loaded.front());
        }
    }

    (void)script;
    return stack;
}

GlyphResolver::GlyphResolver(
    IFontAssetManager& assetManager,
    IFallbackResolver& fallbackResolver)
    : m_AssetManager(assetManager)
    , m_FallbackResolver(fallbackResolver)
{
}

ResolvedGlyph GlyphResolver::Resolve(
    const FontStack& stack,
    const Codepoint codepoint) const
{
    const uint64_t cacheKey = (static_cast<uint64_t>(codepoint) << 32)
        | (stack.faces.empty() ? 0ULL : static_cast<uint64_t>(stack.faces.front()));
    if (const auto cached = m_Cache.find(cacheKey); cached != m_Cache.end()) {
        return cached->second;
    }

    FontStack effectiveStack = stack;
    if (effectiveStack.faces.empty()) {
        effectiveStack = m_FallbackResolver.BuildStack("", shaping::DetectScript(codepoint));
    }

    for (const FontHandle handle : effectiveStack.faces) {
        const auto asset = m_AssetManager.GetAsset(handle);
        if (!asset) {
            continue;
        }
        if (const GlyphMetrics* glyph = asset->FindGlyph(codepoint)) {
            ResolvedGlyph resolved{handle, *glyph};
            m_Cache.emplace(cacheKey, resolved);
            return resolved;
        }
    }

    for (const FontHandle handle : m_AssetManager.GetLoadedFonts()) {
        const auto asset = m_AssetManager.GetAsset(handle);
        if (!asset) {
            continue;
        }
        if (const GlyphMetrics* glyph = asset->FindGlyph(codepoint)) {
            ResolvedGlyph resolved{handle, *glyph};
            m_Cache.emplace(cacheKey, resolved);
            return resolved;
        }
    }

    ResolvedGlyph missing{};
    missing.fontHandle = effectiveStack.faces.empty() ? kInvalidFontHandle : effectiveStack.faces.front();
    return missing;
}

void GlyphResolver::ClearCache()
{
    m_Cache.clear();
}

std::unique_ptr<IFallbackResolver> CreateFallbackResolver(IFontAssetManager& assetManager)
{
    return std::make_unique<FallbackResolver>(assetManager);
}

std::unique_ptr<IGlyphResolver> CreateGlyphResolver(
    IFontAssetManager& assetManager,
    IFallbackResolver& fallbackResolver)
{
    return std::make_unique<GlyphResolver>(assetManager, fallbackResolver);
}

} // namespace we::runtime::text::assets
