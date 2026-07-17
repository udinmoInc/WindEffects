#include "Text/TextEngine.h"

#include "Core/AssetRegistry.h"

#include <cmath>
#include <filesystem>
#include <functional>

namespace we::runtime::text {
namespace {

uint64_t HashMix(uint64_t h, uint64_t v) {
    h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    return h;
}

uint64_t HashLayoutKey(
    const std::string_view text,
    const layout::TextStyle& style,
    const layout::LayoutConstraints& constraints,
    const FontHandle font)
{
    uint64_t h = 14695981039346656037ULL;
    for (const unsigned char c : text) {
        h = HashMix(h, c);
    }
    h = HashMix(h, static_cast<uint64_t>(style.weight));
    h = HashMix(h, static_cast<uint64_t>(style.italic ? 1 : 0));
    h = HashMix(h, static_cast<uint64_t>(style.sizePx * 1000.0f));
    h = HashMix(h, static_cast<uint64_t>(constraints.maxWidth * 1000.0f));
    h = HashMix(h, static_cast<uint64_t>(constraints.dpiScale * 1000.0f));
    h = HashMix(h, static_cast<uint64_t>(constraints.wordWrap ? 1 : 0));
    h = HashMix(h, static_cast<uint64_t>(font));
    h = HashMix(h, std::hash<std::string>{}(style.family));
    return h;
}

} // namespace

TextEngine::TextEngine(const TextEngineConfig& config)
{
    m_AssetManager = assets::CreateFontAssetManager();
    m_Diagnostics = diagnostics::CreateFontDiagnostics();
    m_AtlasManager = atlas::CreateFontAtlasManager();
    m_FallbackResolver = assets::CreateFallbackResolver(*m_AssetManager);
    m_GlyphResolver = assets::CreateGlyphResolver(*m_AssetManager, *m_FallbackResolver, m_AtlasManager.get());
    m_Shaper = shaping::CreateTextShaper(*m_AssetManager);
    m_BidiProcessor = shaping::CreateBidiProcessor();
    m_LayoutEngine = layout::CreateTextLayoutEngine(
        *m_Shaper, *m_GlyphResolver, *m_FallbackResolver, m_BidiProcessor.get());
    m_Batcher = rendering::CreateTextBatcher();

    if (!config.fontStackConfig.empty()) {
        const std::vector<std::string> configCandidates = {
            config.fontStackConfig.string(),
            "Engine/Config/Fonts/DefaultFontStack.json",
            "Config/Fonts/DefaultFontStack.json",
            "../Engine/Config/Fonts/DefaultFontStack.json",
            "../Config/Fonts/DefaultFontStack.json",
        };
        const auto resolvedConfig = we::core::AssetRegistry::ResolveAssetPath(configCandidates);
        if (!resolvedConfig.empty()) {
            (void)m_AssetManager->LoadFontStackConfig(resolvedConfig);
        }
    }
}

TextEngine::~TextEngine() = default;

TextResult<FontHandle> TextEngine::LoadFont(const std::filesystem::path& assetPath)
{
    const auto result = m_AssetManager->LoadSync(assetPath);
    if (result.ok) {
        if (m_DefaultFont == kInvalidFontHandle) {
            m_DefaultFont = result.value;
        }
        if (const auto asset = m_AssetManager->GetAsset(result.value)) {
            if (m_AtlasManager) {
                m_AtlasManager->SeedFromFontAsset(result.value, *asset);
                InvalidateLayoutCache();
            }
            diagnostics::FontDebuggerEntry entry;
            entry.handle = result.value;
            entry.family = asset->metrics.familyName;
            entry.assetPath = assetPath.string();
            entry.glyphCount = asset->glyphs.size();
            if (!asset->atlasPages.empty()) {
                entry.atlasWidth = asset->atlasPages.front().width;
                entry.atlasHeight = asset->atlasPages.front().height;
            }
            m_Diagnostics->Log({"FontLoad", "Loaded font " + entry.family, assetPath.string()});
        }
    }
    return result;
}

layout::LayoutResult TextEngine::LayoutCached(
    const std::string_view utf8Text,
    const layout::TextStyle& style,
    const layout::LayoutConstraints& constraints,
    const FontHandle font)
{
    const uint64_t atlasGen = AtlasGeneration();
    const uint64_t key = HashLayoutKey(utf8Text, style, constraints, font);
    for (const auto& entry : m_LayoutCache) {
        if (entry.key == key && entry.atlasGeneration == atlasGen) {
            return entry.result;
        }
    }

    unicode::UnicodeDecoder decoder;
    const auto bytes = std::span(
        reinterpret_cast<const std::byte*>(utf8Text.data()),
        utf8Text.size());
    const auto decoded = decoder.Decode(bytes, unicode::Encoding::Utf8);

    if (!decoded.errors.empty()) {
        for (const auto& error : decoded.errors) {
            m_Diagnostics->Log(
                {"Unicode",
                    "Decode error at byte " + std::to_string(error.byteOffset),
                    "reason=" + std::to_string(static_cast<int>(error.reason))});
        }
    }

    const FontHandle layoutFont = font != kInvalidFontHandle ? font : m_DefaultFont;
    if (layoutFont == kInvalidFontHandle) {
        m_Diagnostics->Log({"Layout", "No default font loaded", ""});
        return {};
    }

    layout::LayoutResult result =
        m_LayoutEngine->Layout(decoded.codepoints, style, constraints, layoutFont);

    LayoutCacheEntry entry;
    entry.key = key;
    entry.atlasGeneration = atlasGen;
    entry.result = result;
    if (m_LayoutCache.size() >= kMaxLayoutCacheEntries) {
        m_LayoutCache.erase(m_LayoutCache.begin());
    }
    m_LayoutCache.push_back(std::move(entry));
    return result;
}

layout::LayoutResult TextEngine::Layout(
    const std::string_view utf8Text,
    const layout::TextStyle& style,
    const layout::LayoutConstraints& constraints,
    const FontHandle font)
{
    return LayoutCached(utf8Text, style, constraints, font);
}

MeasureResult TextEngine::Measure(
    const std::string_view utf8Text,
    const layout::TextStyle& style,
    const layout::LayoutConstraints& constraints,
    const FontHandle font)
{
    layout::LayoutConstraints measureConstraints = constraints;
    if (measureConstraints.maxWidth <= 0.0f) {
        measureConstraints.maxWidth = 1.0e9f;
        measureConstraints.wordWrap = false;
    }
    const auto layout = LayoutCached(utf8Text, style, measureConstraints, font);
    MeasureResult result;
    result.width = layout.bounds.width;
    result.height = layout.bounds.height;
    result.lineCount = layout.lines.size();
    return result;
}

MeasureResult TextEngine::MeasureLines(
    const std::string_view utf8Text,
    const layout::TextStyle& style,
    const float maxWidth,
    const FontHandle font)
{
    layout::LayoutConstraints constraints;
    constraints.maxWidth = maxWidth > 0.0f ? maxWidth : 1.0e9f;
    constraints.wordWrap = true;
    constraints.dpiScale = 1.0f;
    return Measure(utf8Text, style, constraints, font);
}

HitTestResult TextEngine::HitTest(
    const std::string_view utf8Text,
    const layout::TextStyle& style,
    const layout::LayoutConstraints& constraints,
    const float localX,
    const float localY,
    const FontHandle font)
{
    const auto layout = LayoutCached(utf8Text, style, constraints, font);
    HitTestResult hit;
    if (layout.lines.empty()) {
        return hit;
    }

    uint32_t lineIndex = 0;
    for (size_t i = 0; i < layout.lines.size(); ++i) {
        const auto& line = layout.lines[i];
        const float top = line.baselineY - line.height;
        const float bottom = line.baselineY;
        if (localY >= top && localY <= bottom) {
            lineIndex = static_cast<uint32_t>(i);
            hit.inside = true;
            break;
        }
        if (localY > bottom) {
            lineIndex = static_cast<uint32_t>(i);
        }
    }

    const auto& line = layout.lines[lineIndex];
    hit.lineIndex = lineIndex;
    hit.caretY = line.baselineY - line.height;
    hit.codepointIndex = line.endIndex;
    hit.caretX = line.width;

    float bestDist = 1.0e9f;
    for (const auto& caret : layout.caretMap) {
        if (caret.lineIndex != lineIndex) {
            continue;
        }
        const float dist = std::abs(caret.x - localX);
        if (dist < bestDist) {
            bestDist = dist;
            hit.codepointIndex = caret.codepointIndex;
            hit.caretX = caret.x;
            hit.caretY = caret.y;
        }
    }
    return hit;
}

layout::CaretPosition TextEngine::CaretFromOffset(
    const std::string_view utf8Text,
    const layout::TextStyle& style,
    const layout::LayoutConstraints& constraints,
    const size_t codepointIndex,
    const FontHandle font)
{
    const auto layout = LayoutCached(utf8Text, style, constraints, font);
    for (const auto& caret : layout.caretMap) {
        if (caret.codepointIndex == codepointIndex) {
            return caret;
        }
    }
    if (!layout.caretMap.empty()) {
        if (codepointIndex == 0) {
            layout::CaretPosition start;
            start.codepointIndex = 0;
            return start;
        }
        return layout.caretMap.back();
    }
    return {};
}

void TextEngine::InvalidateLayoutCache()
{
    m_LayoutCache.clear();
}

uint64_t TextEngine::AtlasGeneration() const
{
    return m_AtlasManager ? m_AtlasManager->Generation() : 0;
}

void TextEngine::SubmitFrame(
    rendering::ITextGpuBackend& backend,
    const std::span<const layout::LayoutResult> layouts)
{
    if (m_AtlasManager) {
        m_AtlasManager->BeginFrame(AtlasGeneration());
        for (const uint32_t pageIndex : m_AtlasManager->TakeDirtyPages()) {
            if (const auto page = m_AtlasManager->CopyPage(pageIndex)) {
                (void)backend.UploadAtlas(page->page);
            }
        }
    }

    m_Batcher->BeginFrame();
    for (const layout::LayoutResult& layout : layouts) {
        m_Batcher->AddLayout(layout);
    }
    const auto batches = m_Batcher->BuildBatches();
    backend.DrawBatches(nullptr, batches, 1920.0f, 1080.0f);
}

diagnostics::IFontDiagnostics& TextEngine::Diagnostics()
{
    return *m_Diagnostics;
}

assets::IFontAssetManager& TextEngine::Assets()
{
    return *m_AssetManager;
}

std::unique_ptr<ITextEngine> CreateTextEngine(const TextEngineConfig& config)
{
    return std::make_unique<TextEngine>(config);
}

} // namespace we::runtime::text
