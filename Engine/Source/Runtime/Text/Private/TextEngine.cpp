#include "Text/TextEngine.h"

#include "AssetRegistry.h"

#include <filesystem>

namespace we::runtime::text {

TextEngine::TextEngine(const TextEngineConfig& config)
{
    m_AssetManager = assets::CreateFontAssetManager();
    m_Diagnostics = diagnostics::CreateFontDiagnostics();
    m_FallbackResolver = assets::CreateFallbackResolver(*m_AssetManager);
    m_GlyphResolver = assets::CreateGlyphResolver(*m_AssetManager, *m_FallbackResolver);
    m_Shaper = shaping::CreateTextShaper(*m_AssetManager);
    m_BidiProcessor = shaping::CreateBidiProcessor();
    m_LayoutEngine = layout::CreateTextLayoutEngine(*m_Shaper, *m_GlyphResolver, *m_FallbackResolver);
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

layout::LayoutResult TextEngine::Layout(
    const std::string_view utf8Text,
    const layout::TextStyle& style,
    const layout::LayoutConstraints& constraints,
    const FontHandle font)
{
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

    return m_LayoutEngine->Layout(decoded.codepoints, style, constraints, layoutFont);
}

void TextEngine::SubmitFrame(
    rendering::ITextGpuBackend& backend,
    const std::span<const layout::LayoutResult> layouts)
{
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
