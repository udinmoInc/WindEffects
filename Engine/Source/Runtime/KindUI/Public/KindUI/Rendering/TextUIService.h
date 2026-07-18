#pragma once

#include "KindUI/Export.h"
#include "KindUI/Rendering/OverlayRenderer.h"
#include "KindUI/Core/PaintContext.h"
#include "Text/TextEngine.h"
#include "RHI/Types.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::kindui {

class OverlayRenderer;

struct TextDebugGlyphInfo {
    Rect bounds{};
    uint32_t atlasPage = 0;
    float geometryScale = 0.0f;
    float effectiveScale = 0.0f;
    float msdfRange = 0.0f;
    float planeW = 0.0f;
    float planeH = 0.0f;
    uint64_t atlasGeneration = 0;
};

class KINDUI_API TextUIService {
public:
    TextUIService();
    ~TextUIService();

    bool Initialize(OverlayRenderer* renderer);
    void Shutdown();

    [[nodiscard]] we::runtime::text::ITextEngine* GetEngine() const { return m_TextEngine.get(); }

    /// Enable with env WE_TEXT_DEBUG=1 (Development builds). Draws glyph bounds and dumps atlases.
    void SetDebugEnabled(bool enabled);
    [[nodiscard]] bool IsDebugEnabled() const { return m_DebugEnabled; }
    [[nodiscard]] const std::vector<TextDebugGlyphInfo>& LastDebugGlyphs() const { return m_LastDebugGlyphs; }
    void DumpAtlasPagesToDisk();

    [[nodiscard]] float MeasureText(const std::string& text, float fontSize, bool bold) const;
    bool GenerateTextGeometry(
        const DrawCommand& cmd,
        std::vector<UIVertex2>& vertices,
        std::vector<uint32_t>& indices,
        we::rhi::RHIDescriptorSetHandle& outTextureSet,
        UIRenderBatch* outBatchInfo = nullptr);

private:
    struct GpuAtlasPage {
        we::rhi::RHIDescriptorSetHandle descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        uint32_t width = 0;
        uint32_t height = 0;
        uint64_t version = 0;
    };

    [[nodiscard]] we::runtime::text::layout::TextStyle BuildStyle(const DrawCommand& cmd) const;
    [[nodiscard]] we::rhi::RHIDescriptorSetHandle EnsureAtlasPageUploaded(uint32_t pageIndex);
    [[nodiscard]] we::rhi::RHIDescriptorSetHandle GetDescriptorForFont(we::runtime::text::FontHandle handle);
    bool UploadFontAtlasFallback(we::runtime::text::FontHandle handle, GpuAtlasPage& gpuAtlas);
    void SyncDirtyAtlasPages();
    void MaybeLogScaleDiagnostics(const we::runtime::text::layout::LayoutResult& layout);

    OverlayRenderer* m_Renderer = nullptr;
    std::unique_ptr<we::runtime::text::ITextEngine> m_TextEngine;
    std::unordered_map<uint32_t, GpuAtlasPage> m_DynamicPages;
    std::unordered_map<we::runtime::text::FontHandle, GpuAtlasPage> m_FontAtlases;
    we::runtime::text::FontHandle m_RegularFont = we::runtime::text::kInvalidFontHandle;
    we::runtime::text::FontHandle m_SemiBoldFont = we::runtime::text::kInvalidFontHandle;

    bool m_DebugEnabled = false;
    bool m_LoggedScaleDiagnostics = false;
    bool m_DumpedAtlas = false;
    uint64_t m_LastSeenAtlasGeneration = 0;
    std::vector<TextDebugGlyphInfo> m_LastDebugGlyphs;
};

} // namespace we::runtime::kindui
