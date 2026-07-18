#pragma once

#include "Text/Assets/FontAssetManager.h"
#include "Text/Assets/GlyphResolver.h"
#include "Text/Atlas/FontAtlasManager.h"
#include "Text/Diagnostics/FontDiagnostics.h"
#include "Text/Export.h"
#include "Text/Layout/TextLayoutEngine.h"
#include "Text/Rendering/TextBatcher.h"
#include "Text/Rendering/TextGpuBackend.h"
#include "Text/Shaping/BidiProcessor.h"
#include "Text/Shaping/TextShaper.h"
#include "Text/Unicode/UnicodeDecoder.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace we::runtime::text {

struct MeasureResult {
    float width = 0.0f;
    float height = 0.0f;
    size_t lineCount = 0;
};

struct HitTestResult {
    size_t codepointIndex = 0;
    uint32_t lineIndex = 0;
    float caretX = 0.0f;
    float caretY = 0.0f;
    bool inside = false;
};

class TEXT_API ITextEngine {
public:
    virtual ~ITextEngine() = default;

    [[nodiscard]] virtual TextResult<FontHandle> LoadFont(const std::filesystem::path& assetPath) = 0;
    [[nodiscard]] virtual layout::LayoutResult Layout(
        std::string_view utf8Text,
        const layout::TextStyle& style,
        const layout::LayoutConstraints& constraints,
        FontHandle font = kInvalidFontHandle) = 0;
    [[nodiscard]] virtual MeasureResult Measure(
        std::string_view utf8Text,
        const layout::TextStyle& style,
        const layout::LayoutConstraints& constraints = {},
        FontHandle font = kInvalidFontHandle) = 0;
    [[nodiscard]] virtual MeasureResult MeasureLines(
        std::string_view utf8Text,
        const layout::TextStyle& style,
        float maxWidth,
        FontHandle font = kInvalidFontHandle) = 0;
    [[nodiscard]] virtual HitTestResult HitTest(
        std::string_view utf8Text,
        const layout::TextStyle& style,
        const layout::LayoutConstraints& constraints,
        float localX,
        float localY,
        FontHandle font = kInvalidFontHandle) = 0;
    [[nodiscard]] virtual layout::CaretPosition CaretFromOffset(
        std::string_view utf8Text,
        const layout::TextStyle& style,
        const layout::LayoutConstraints& constraints,
        size_t codepointIndex,
        FontHandle font = kInvalidFontHandle) = 0;
    virtual void InvalidateLayoutCache() = 0;
    [[nodiscard]] virtual uint64_t AtlasGeneration() const = 0;
    virtual void SubmitFrame(
        rendering::ITextGpuBackend& backend,
        std::span<const layout::LayoutResult> layouts) = 0;
    [[nodiscard]] virtual diagnostics::IFontDiagnostics& Diagnostics() = 0;
    [[nodiscard]] virtual assets::IFontAssetManager& Assets() = 0;
    [[nodiscard]] virtual atlas::IFontAtlasManager* AtlasManager() = 0;
};

struct TextEngineConfig {
    std::filesystem::path fontStackConfig = std::filesystem::path("Engine") / "Config" / "Fonts" / "DefaultFontStack.json";
    GraphicsApi graphicsApi = GraphicsApi::Unknown;
};

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

class TEXT_API TextEngine final : public ITextEngine {
public:
    explicit TextEngine(const TextEngineConfig& config);
    ~TextEngine() override;

    [[nodiscard]] TextResult<FontHandle> LoadFont(const std::filesystem::path& assetPath) override;
    [[nodiscard]] layout::LayoutResult Layout(
        std::string_view utf8Text,
        const layout::TextStyle& style,
        const layout::LayoutConstraints& constraints,
        FontHandle font = kInvalidFontHandle) override;
    [[nodiscard]] MeasureResult Measure(
        std::string_view utf8Text,
        const layout::TextStyle& style,
        const layout::LayoutConstraints& constraints = {},
        FontHandle font = kInvalidFontHandle) override;
    [[nodiscard]] MeasureResult MeasureLines(
        std::string_view utf8Text,
        const layout::TextStyle& style,
        float maxWidth,
        FontHandle font = kInvalidFontHandle) override;
    [[nodiscard]] HitTestResult HitTest(
        std::string_view utf8Text,
        const layout::TextStyle& style,
        const layout::LayoutConstraints& constraints,
        float localX,
        float localY,
        FontHandle font = kInvalidFontHandle) override;
    [[nodiscard]] layout::CaretPosition CaretFromOffset(
        std::string_view utf8Text,
        const layout::TextStyle& style,
        const layout::LayoutConstraints& constraints,
        size_t codepointIndex,
        FontHandle font = kInvalidFontHandle) override;
    void InvalidateLayoutCache() override;
    [[nodiscard]] uint64_t AtlasGeneration() const override;
    void SubmitFrame(
        rendering::ITextGpuBackend& backend,
        std::span<const layout::LayoutResult> layouts) override;
    [[nodiscard]] diagnostics::IFontDiagnostics& Diagnostics() override;
    [[nodiscard]] assets::IFontAssetManager& Assets() override;
    [[nodiscard]] atlas::IFontAtlasManager* AtlasManager() override { return m_AtlasManager.get(); }

private:
    [[nodiscard]] layout::LayoutResult LayoutCached(
        std::string_view utf8Text,
        const layout::TextStyle& style,
        const layout::LayoutConstraints& constraints,
        FontHandle font);

    std::unique_ptr<assets::IFontAssetManager> m_AssetManager;
    std::unique_ptr<assets::IFallbackResolver> m_FallbackResolver;
    std::unique_ptr<atlas::IFontAtlasManager> m_AtlasManager;
    std::unique_ptr<assets::IGlyphResolver> m_GlyphResolver;
    std::unique_ptr<shaping::ITextShaper> m_Shaper;
    std::unique_ptr<shaping::IBidiProcessor> m_BidiProcessor;
    std::unique_ptr<layout::ITextLayoutEngine> m_LayoutEngine;
    std::unique_ptr<rendering::ITextBatcher> m_Batcher;
    std::unique_ptr<diagnostics::IFontDiagnostics> m_Diagnostics;
    FontHandle m_DefaultFont = kInvalidFontHandle;

    struct LayoutCacheEntry {
        uint64_t key = 0;
        uint64_t atlasGeneration = 0;
        layout::LayoutResult result;
    };
    std::vector<LayoutCacheEntry> m_LayoutCache;
    static constexpr size_t kMaxLayoutCacheEntries = 256;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

[[nodiscard]] TEXT_API std::unique_ptr<ITextEngine> CreateTextEngine(const TextEngineConfig& config = {});

} // namespace we::runtime::text
