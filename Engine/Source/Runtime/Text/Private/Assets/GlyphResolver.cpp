#include "Text/Assets/GlyphResolver.h"
#include "Text/Assets/FontAssetManager.h"

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

    // Prefer fallbacks whose scripts match the requested script, then append the rest.
    auto appendMatching = [&](bool requireScriptMatch) {
        for (const auto& entry : config.fallbackChain) {
            bool scriptOk = !requireScriptMatch;
            if (requireScriptMatch) {
                for (const auto& scriptName : entry.scripts) {
                    const shaping::Script mapped = shaping::DetectScriptFromName(scriptName);
                    if (mapped == script || mapped == shaping::Script::Unknown) {
                        scriptOk = true;
                        break;
                    }
                }
            }
            if (!scriptOk) {
                continue;
            }
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
    };
    appendMatching(true);
    appendMatching(false);

    if (stack.faces.empty()) {
        const auto loaded = m_AssetManager.GetLoadedFonts();
        if (!loaded.empty()) {
            stack.faces.push_back(loaded.front());
        }
    }

    return stack;
}

GlyphResolver::GlyphResolver(
    IFontAssetManager& assetManager,
    IFallbackResolver& fallbackResolver,
    atlas::IFontAtlasManager* atlasManager)
    : m_AssetManager(assetManager)
    , m_FallbackResolver(fallbackResolver)
    , m_AtlasManager(atlasManager)
{
}

ResolvedGlyph GlyphResolver::Resolve(
    const FontStack& stack,
    const Codepoint codepoint) const
{
    const uint64_t cacheKey = (static_cast<uint64_t>(codepoint) << 32)
        | (stack.faces.empty() ? 0ULL : static_cast<uint64_t>(stack.faces.front()));
    if (const auto cached = m_Cache.find(cacheKey); cached != m_Cache.end()) {
        if (m_AtlasManager) {
            m_AtlasManager->Touch({cached->second.fontHandle, codepoint, 0});
        }
        return cached->second;
    }

    FontStack effectiveStack = stack;
    if (effectiveStack.faces.empty()) {
        effectiveStack = m_FallbackResolver.BuildStack("", shaping::DetectScript(codepoint));
    }

    auto makeResolved = [&](FontHandle handle, const FontAsset& asset, const GlyphMetrics& glyph) {
        ResolvedGlyph resolved{handle, glyph};
        resolved.bakeSizePx = asset.metrics.bakeSizePx > 0.0f ? asset.metrics.bakeSizePx : 18.0f;
        resolved.msdfPixelRange = asset.metrics.msdfPixelRange > 0.0f ? asset.metrics.msdfPixelRange : 4.0f;
        resolved.geometryScale = asset.metrics.geometryScale > 0.0f
            ? asset.metrics.geometryScale
            : resolved.bakeSizePx;
        resolved.ascender = asset.metrics.ascender;
        resolved.descender = asset.metrics.descender;
        resolved.lineHeight = asset.metrics.lineHeight > 0.0f
            ? asset.metrics.lineHeight
            : (std::abs(asset.metrics.ascender) + std::abs(asset.metrics.descender));

        if (m_AtlasManager) {
            const atlas::GlyphAtlasKey key{handle, codepoint, 0};
            if (const auto atlasGlyph = m_AtlasManager->Find(key)) {
                resolved.metrics = atlasGlyph->metrics;
                resolved.bakeSizePx = atlasGlyph->bakeSizePx > 0.0f
                    ? atlasGlyph->bakeSizePx
                    : resolved.bakeSizePx;
                resolved.msdfPixelRange = atlasGlyph->msdfPixelRange > 0.0f
                    ? atlasGlyph->msdfPixelRange
                    : resolved.msdfPixelRange;
                resolved.geometryScale = atlasGlyph->geometryScale > 0.0f
                    ? atlasGlyph->geometryScale
                    : resolved.geometryScale;
                resolved.atlasPageVersion = atlasGlyph->pageVersion;
                m_AtlasManager->Touch(key);
            }
        }
        m_Cache.emplace(cacheKey, resolved);
        return resolved;
    };

    for (const FontHandle handle : effectiveStack.faces) {
        const auto asset = m_AssetManager.GetAsset(handle);
        if (!asset) {
            continue;
        }
        if (const GlyphMetrics* glyph = asset->FindGlyph(codepoint)) {
            return makeResolved(handle, *asset, *glyph);
        }
    }

    for (const FontHandle handle : m_AssetManager.GetLoadedFonts()) {
        const auto asset = m_AssetManager.GetAsset(handle);
        if (!asset) {
            continue;
        }
        if (const GlyphMetrics* glyph = asset->FindGlyph(codepoint)) {
            return makeResolved(handle, *asset, *glyph);
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
    IFallbackResolver& fallbackResolver,
    atlas::IFontAtlasManager* atlasManager)
{
    return std::make_unique<GlyphResolver>(assetManager, fallbackResolver, atlasManager);
}

} // namespace we::runtime::text::assets
