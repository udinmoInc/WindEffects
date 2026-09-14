// ==============================================================================
// WindEffects — KindUI — TextUIService
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Host/OverlayRenderer.h"
#include "KindUI/Core/PaintContext.h"
#include "Text/TextEngine.h"
#include "RHI/Types.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <string_view>
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

/// Canonical KindUI text path stats (reset at BeginFrame).
struct TextUIServiceFrameStats {
    uint32_t measureCalls = 0;
    uint32_t layoutHits = 0;
    uint32_t layoutMisses = 0;
    uint32_t geometryCacheHits = 0;
    uint32_t geometryCacheMisses = 0;
    uint32_t glyphsEmitted = 0;
    uint32_t atlasUploads = 0;
    uint32_t textDraws = 0;
    uint64_t layoutCacheBytes = 0;
    uint32_t layoutCacheEntries = 0;
    uint32_t geometryCacheEntries = 0;
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

    /// Call once per UI drawgen frame before emitting text (resets frame stats, begins engine stats).
    void BeginFrame();
    [[nodiscard]] const TextUIServiceFrameStats& CurrentFrameStats() const { return m_FrameStats; }

    [[nodiscard]] float MeasureText(std::string_view text, float fontSize, bool bold) const;
    [[nodiscard]] float MeasureText(
        std::string_view text,
        float fontSize,
        we::runtime::text::layout::FontWeight weight) const;
    bool GenerateTextGeometry(
        const DrawCommand& cmd,
        std::vector<UIVertex2>& vertices,
        std::vector<uint32_t>& indices,
        we::rhi::RHIDescriptorSetHandle& outTextureSet,
        UIRenderBatch* outBatchInfo = nullptr);

    [[nodiscard]] size_t MeasureCacheEntryCount() const;
    [[nodiscard]] uint64_t EstimateMeasureCacheBytes() const;
    [[nodiscard]] uint32_t FontAtlasPageCount() const;
    [[nodiscard]] uint64_t EstimateFontAtlasCpuBytes() const;
    [[nodiscard]] size_t GeometryCacheEntryCount() const;
    [[nodiscard]] uint64_t EstimateGeometryCacheBytes() const;

    /// Apply UIResourceResidency budgets to geometry + glyph caches (bounded).
    void OnResidencyTick();

private:
    struct GpuAtlasPage {
        we::rhi::RHIDescriptorSetHandle descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        uint32_t width = 0;
        uint32_t height = 0;
        uint64_t version = 0;
    };

    /// Local-space glyph quads reused across draws with the same layout key.
    struct GlyphQuad {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        float u0 = 0.0f;
        float v0 = 0.0f;
        float u1 = 0.0f;
        float v1 = 0.0f;
        float msdf = 4.0f;
        uint32_t atlasPage = 0;
    };

    struct GeometryCacheEntry {
        uint64_t atlasGeneration = 0;
        uint32_t atlasPage = 0;
        float msdfRange = 4.0f;
        uint64_t lastUsedFrame = 0;
        std::vector<GlyphQuad> quads;
    };

    [[nodiscard]] we::runtime::text::layout::TextStyle BuildStyle(const DrawCommand& cmd) const;
    [[nodiscard]] we::runtime::text::FontHandle ResolveFont(
        we::runtime::text::layout::FontWeight weight) const;
    [[nodiscard]] we::rhi::RHIDescriptorSetHandle EnsureAtlasPageUploaded(uint32_t pageIndex);
    [[nodiscard]] we::rhi::RHIDescriptorSetHandle GetDescriptorForFont(we::runtime::text::FontHandle handle);
    bool UploadFontAtlasFallback(we::runtime::text::FontHandle handle, GpuAtlasPage& gpuAtlas);
    void SyncDirtyAtlasPages();
    void MaybeLogScaleDiagnostics(const we::runtime::text::layout::LayoutResult& layout);
    [[nodiscard]] uint64_t HashGeometryKey(
        std::string_view text,
        float fontSize,
        we::runtime::text::layout::FontWeight weight,
        bool italic) const;
    const GeometryCacheEntry* GetOrBuildGeometry(
        std::string_view text,
        const we::runtime::text::layout::TextStyle& style,
        we::runtime::text::FontHandle font,
        uint64_t geomKey);

    OverlayRenderer* m_Renderer = nullptr;
    std::unique_ptr<we::runtime::text::ITextEngine> m_TextEngine;
    std::unordered_map<uint32_t, GpuAtlasPage> m_DynamicPages;
    std::unordered_map<we::runtime::text::FontHandle, GpuAtlasPage> m_FontAtlases;
    we::runtime::text::FontHandle m_RegularFont = we::runtime::text::kInvalidFontHandle;
    we::runtime::text::FontHandle m_MediumFont = we::runtime::text::kInvalidFontHandle;
    we::runtime::text::FontHandle m_SemiBoldFont = we::runtime::text::kInvalidFontHandle;

    std::unordered_map<uint64_t, GeometryCacheEntry> m_GeometryCache;
    std::vector<uint64_t> m_GeometryCacheOrder;
    static constexpr size_t kMaxGeometryCacheEntries = 512;

    mutable TextUIServiceFrameStats m_FrameStats{};

    bool m_DebugEnabled = false;
    bool m_LoggedScaleDiagnostics = false;
    bool m_DumpedAtlas = false;
    uint64_t m_LastSeenAtlasGeneration = 0;
    std::vector<TextDebugGlyphInfo> m_LastDebugGlyphs;
};

} // namespace we::runtime::kindui
