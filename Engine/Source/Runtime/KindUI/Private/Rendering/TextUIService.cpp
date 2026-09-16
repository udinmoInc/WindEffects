// ==============================================================================
// WindEffects — KindUI — TextUIService
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Rendering/TextUIService.h"

#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Host/FontImportService.h"
#include "KindUI/Core/UIResourceResidency.h"
#include "KindUI/Host/OverlayRenderer.h"
#include "Rendering/UiDebugImageWriter.h"
#include "Text/Assets/FontAsset.h"
#include "Core/AssetRegistry.h"
#include "Core/Logger.h"
#include "Core/Paths.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <functional>

namespace we::runtime::kindui {

namespace {

constexpr float kFontBakeSizePx = 24.0f;

inline float SnapPx(const float v) {
    return std::floor(v + 0.5f);
}

std::filesystem::path ResolveFontPath(const std::string& baseName, const char* extension) {
    const auto candidates = we::core::PathService::Get().FontCandidates(baseName + extension);
    if (const auto found = we::core::PathService::FindExisting(candidates)) {
        return *found;
    }
    return {};
}

std::filesystem::path ResolveWeFontPath(const std::string& baseName) {
    return ResolveFontPath(baseName, ".wefont");
}

std::filesystem::path ResolveTtfPath(const std::string& baseName) {
    return ResolveFontPath(baseName, ".ttf");
}

bool NeedsFontRebake(const std::filesystem::path& wefontPath) {
    const auto loaded = we::runtime::text::assets::FontAssetReader::LoadFromFile(wefontPath);
    if (!loaded.ok) {
        return true;
    }
    return loaded.value.metrics.bakeSizePx + 0.5f < kFontBakeSizePx;
}

std::filesystem::path EnsureWeFontAsset(const std::string& baseName) {
    const auto ttfPath = ResolveTtfPath(baseName);
    if (const auto existing = ResolveWeFontPath(baseName); !existing.empty()) {
        if (!NeedsFontRebake(existing) || ttfPath.empty()) {
            return existing;
        }
        // Rebake at higher resolution for sharper small-size text.
        try {
            if (!FontImportService::ImportFontFile(ttfPath, ttfPath.parent_path(), kFontBakeSizePx, "basic")) {
                WE_LOG_WARN("TextUIService", "Font rebake failed for " + baseName + "; using existing asset.");
                return existing;
            }
        } catch (const std::exception& ex) {
            WE_LOG_WARN("TextUIService", std::string("Font rebake exception: ") + ex.what());
            return existing;
        }
        return ResolveWeFontPath(baseName);
    }

    if (ttfPath.empty()) {
        return {};
    }

    const auto outputDir = ttfPath.parent_path();
    try {
        if (!FontImportService::ImportFontFile(ttfPath, outputDir, kFontBakeSizePx, "basic")) {
            WE_LOG_ERROR("TextUIService", "Failed to import .wefont from " + ttfPath.string());
            return {};
        }
    } catch (const std::exception& ex) {
        WE_LOG_ERROR("TextUIService", std::string("Font import exception: ") + ex.what());
        return {};
    }

    return ResolveWeFontPath(baseName);
}

template <size_t N>
std::filesystem::path ResolveWeightedWeFontPath(const char* const (&names)[N]) {
    for (const char* name : names) {
        if (const auto path = EnsureWeFontAsset(name); !path.empty()) {
            return path;
        }
    }
    return {};
}

std::filesystem::path ResolveSemiBoldWeFontPath() {
    static constexpr const char* kNames[] = {"Roboto-Bold", "Roboto-Medium"};
    return ResolveWeightedWeFontPath(kNames);
}

std::filesystem::path ResolveMediumWeFontPath() {
    static constexpr const char* kNames[] = {"Roboto-Medium", "Roboto-Bold"};
    return ResolveWeightedWeFontPath(kNames);
}

we::runtime::text::layout::FontWeight EffectiveWeight(const DrawCommand& cmd) {
    if (cmd.textBold || cmd.textWeight >= static_cast<uint16_t>(we::runtime::text::layout::FontWeight::SemiBold)) {
        return we::runtime::text::layout::FontWeight::SemiBold;
    }
    if (cmd.textWeight >= static_cast<uint16_t>(we::runtime::text::layout::FontWeight::Medium)) {
        return we::runtime::text::layout::FontWeight::Medium;
    }
    return we::runtime::text::layout::FontWeight::Regular;
}

} // namespace

TextUIService::TextUIService() = default;
TextUIService::~TextUIService() { Shutdown(); }

bool TextUIService::Initialize(OverlayRenderer* renderer) {
    m_Renderer = renderer;
    m_TextEngine = we::runtime::text::CreateTextEngine();
    if (!m_TextEngine) {
        WE_LOG_ERROR("TextUIService", "Failed to create text engine");
        return false;
    }

#if !defined(NDEBUG) || defined(WE_DEVELOPMENT)
    if (const char* env = std::getenv("WE_TEXT_DEBUG")) {
        m_DebugEnabled = env[0] != '\0' && env[0] != '0';
    }
#endif

    const auto regularPath = EnsureWeFontAsset("Roboto-Regular");
    const auto mediumPath = ResolveMediumWeFontPath();
    const auto semiBoldPath = ResolveSemiBoldWeFontPath();
    if (!regularPath.empty()) {
        const auto loaded = m_TextEngine->LoadFont(regularPath);
        if (loaded.ok) {
            m_RegularFont = loaded.value;
        }
    }
    if (!mediumPath.empty()) {
        const auto loaded = m_TextEngine->LoadFont(mediumPath);
        if (loaded.ok) {
            m_MediumFont = loaded.value;
        }
    }
    if (!semiBoldPath.empty()) {
        const auto loaded = m_TextEngine->LoadFont(semiBoldPath);
        if (loaded.ok) {
            m_SemiBoldFont = loaded.value;
        }
    }

    if (m_RegularFont == we::runtime::text::kInvalidFontHandle) {
        WE_LOG_ERROR("TextUIService",
            "No .wefont assets found and TTF import failed. Ensure Assets/Fonts/Roboto-Regular.wefont exists.");
        return false;
    }

    if (m_MediumFont == we::runtime::text::kInvalidFontHandle) {
        m_MediumFont = m_RegularFont;
    }
    if (m_SemiBoldFont == we::runtime::text::kInvalidFontHandle) {
        m_SemiBoldFont = m_MediumFont != we::runtime::text::kInvalidFontHandle ? m_MediumFont : m_RegularFont;
    }

    SyncDirtyAtlasPages();
    if (m_DynamicPages.empty()) {
        if (GetDescriptorForFont(m_RegularFont) == we::rhi::RHIDescriptorSetHandle::Invalid) {
            WE_LOG_ERROR("TextUIService", "Failed to upload regular font atlas");
            return false;
        }
    }

    TextMetrics::SetMeasureProvider([this](const std::string_view text, const float fontSize, const bool bold) {
        return MeasureText(
            text,
            fontSize,
            bold ? we::runtime::text::layout::FontWeight::SemiBold
                 : we::runtime::text::layout::FontWeight::Regular);
    });

    if (m_DebugEnabled) {
        DumpAtlasPagesToDisk();
        WE_LOG_WARN("TextUIService", "WE_TEXT_DEBUG enabled — atlas dumps + glyph bound overlays active");
    }

    return true;
}

void TextUIService::SetDebugEnabled(const bool enabled) {
    m_DebugEnabled = enabled;
    if (enabled && !m_DumpedAtlas) {
        DumpAtlasPagesToDisk();
    }
}

void TextUIService::DumpAtlasPagesToDisk() {
    auto* atlas = m_TextEngine ? m_TextEngine->AtlasManager() : nullptr;
    if (!atlas) {
        return;
    }
    for (uint32_t i = 0; i < atlas->PageCount(); ++i) {
        const auto page = atlas->CopyPage(i);
        if (!page || page->page.rgba.empty()) {
            continue;
        }
        const auto path = we::core::PathService::ToUtf8(
            we::core::PathService::Get().LogsRoot()
                / ("TextAtlas_page" + std::to_string(i) + "_v" + std::to_string(page->version) + ".bmp"));
        if (SaveBmpRgba(path, page->page.rgba, page->page.width, page->page.height)) {
            WE_LOG_INFO("TextUIService",
                "Dumped atlas page " + std::to_string(i) + " " + std::to_string(page->page.width) + "x"
                    + std::to_string(page->page.height) + " -> " + path);
        }
    }
    m_DumpedAtlas = true;
}

void TextUIService::MaybeLogScaleDiagnostics(const we::runtime::text::layout::LayoutResult& layout) {
    if (m_LoggedScaleDiagnostics) {
        return;
    }
    for (const auto& glyph : layout.glyphs) {
        if (!glyph.glyph.metrics.hasDrawableQuad) {
            continue;
        }
        const float eff = glyph.glyph.EffectiveGeometryScale();
        WE_LOG_INFO(
            "TextUIService",
            "GlyphScaleDiag cp=" + std::to_string(glyph.glyph.metrics.codepoint)
                + " plane=" + std::to_string(glyph.glyph.metrics.bounds.width) + "x"
                + std::to_string(glyph.glyph.metrics.bounds.height)
                + " adv=" + std::to_string(glyph.glyph.metrics.advance)
                + " geomScale=" + std::to_string(glyph.glyph.geometryScale)
                + " effective=" + std::to_string(eff)
                + " quad=" + std::to_string(glyph.width) + "x" + std::to_string(glyph.height)
                + " msdf=" + std::to_string(glyph.msdfPixelRange)
                + " page=" + std::to_string(glyph.glyph.metrics.atlasPage)
                + " atlasGen=" + std::to_string(m_TextEngine ? m_TextEngine->AtlasGeneration() : 0));
        m_LoggedScaleDiagnostics = true;
        break;
    }
}

void TextUIService::Shutdown() {
    TextMetrics::SetMeasureProvider({});
    if (m_Renderer) {
        for (auto& [_, atlas] : m_FontAtlases) {
            if (atlas.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid) {
                m_Renderer->UnregisterTexture(atlas.descriptorSet);
            }
        }
        for (auto& [_, page] : m_DynamicPages) {
            if (page.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid) {
                m_Renderer->UnregisterTexture(page.descriptorSet);
            }
        }
    }
    m_FontAtlases.clear();
    m_DynamicPages.clear();
    m_GeometryCache.clear();
    m_GeometryCacheOrder.clear();
    m_TextEngine.reset();
    m_Renderer = nullptr;
}

void TextUIService::BeginFrame() {
    m_FrameStats = {};
    if (m_TextEngine) {
        m_TextEngine->BeginFrameStats();
    }
    SyncDirtyAtlasPages();
}

void TextUIService::SyncDirtyAtlasPages() {
    auto* atlas = m_TextEngine ? m_TextEngine->AtlasManager() : nullptr;
    if (!atlas || !m_Renderer) {
        return;
    }
    const uint64_t gen = atlas->Generation();
    if (gen != m_LastSeenAtlasGeneration) {
        m_LastSeenAtlasGeneration = gen;
        m_GeometryCache.clear();
        m_GeometryCacheOrder.clear();
        m_TextEngine->InvalidateLayoutCache();
        if (m_DebugEnabled && !m_DumpedAtlas) {
            DumpAtlasPagesToDisk();
        }
    }
    for (const uint32_t pageIndex : atlas->TakeDirtyPages()) {
        (void)EnsureAtlasPageUploaded(pageIndex);
        ++m_FrameStats.atlasUploads;
    }
}

we::rhi::RHIDescriptorSetHandle TextUIService::EnsureAtlasPageUploaded(const uint32_t pageIndex) {
    auto* atlas = m_TextEngine ? m_TextEngine->AtlasManager() : nullptr;
    if (!atlas || !m_Renderer) {
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    // Fast path: page version matches what we already uploaded, so skip the
    // full pixel copy + re-upload. Version bumps on every pack and dims are
    // fixed at page creation, so a match means identical content.
    auto& gpu = m_DynamicPages[pageIndex];
    const uint64_t version = atlas->PageVersion(pageIndex);
    if (version != 0 && gpu.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid
        && gpu.version == version) {
        return gpu.descriptorSet;
    }

    const auto pageCopy = atlas->CopyPage(pageIndex);
    if (!pageCopy || pageCopy->page.rgba.empty()) {
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    if (gpu.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid
        && gpu.width == pageCopy->page.width
        && gpu.height == pageCopy->page.height
        && gpu.version == pageCopy->version) {
        return gpu.descriptorSet;
    }

    // Same dimensions: refresh texels in place (no destroy/recreate, no submission invalidation).
    if (gpu.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid
        && gpu.width == pageCopy->page.width
        && gpu.height == pageCopy->page.height) {
        if (m_Renderer->UpdateRgbaTexturePixels(
                gpu.descriptorSet,
                pageCopy->page.width,
                pageCopy->page.height,
                pageCopy->page.rgba)) {
            gpu.version = pageCopy->version;
            return gpu.descriptorSet;
        }
    }

    if (gpu.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid) {
        m_Renderer->UnregisterTexture(gpu.descriptorSet);
        gpu.descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    gpu.width = pageCopy->page.width;
    gpu.height = pageCopy->page.height;
    gpu.version = pageCopy->version;
    gpu.descriptorSet = m_Renderer->UploadRgbaTexture(
        pageCopy->page.width, pageCopy->page.height, pageCopy->page.rgba, true);
    return gpu.descriptorSet;
}

bool TextUIService::UploadFontAtlasFallback(
    const we::runtime::text::FontHandle handle,
    GpuAtlasPage& gpuAtlas)
{
    if (!m_Renderer || !m_TextEngine) {
        return false;
    }

    const auto asset = m_TextEngine->Assets().GetAsset(handle);
    if (!asset || asset->atlasPages.empty()) {
        return false;
    }

    const auto& page = asset->atlasPages.front();
    if (page.rgba.empty() || page.width == 0 || page.height == 0) {
        return false;
    }

    if (gpuAtlas.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid
        && gpuAtlas.width == page.width
        && gpuAtlas.height == page.height) {
        return true;
    }

    if (gpuAtlas.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid) {
        m_Renderer->UnregisterTexture(gpuAtlas.descriptorSet);
        gpuAtlas.descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    gpuAtlas.width = page.width;
    gpuAtlas.height = page.height;
    gpuAtlas.descriptorSet = m_Renderer->UploadRgbaTexture(page.width, page.height, page.rgba, true);
    return gpuAtlas.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid;
}

we::runtime::text::layout::TextStyle TextUIService::BuildStyle(const DrawCommand& cmd) const {
    we::runtime::text::layout::TextStyle style{};
    style.sizePx = cmd.fontSize;
    style.weight = EffectiveWeight(cmd);
    style.italic = cmd.textItalic;
    style.color = {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a};
    return style;
}

we::runtime::text::FontHandle TextUIService::ResolveFont(
    we::runtime::text::layout::FontWeight weight) const {
    if (weight >= we::runtime::text::layout::FontWeight::SemiBold) {
        return m_SemiBoldFont;
    }
    if (weight >= we::runtime::text::layout::FontWeight::Medium) {
        return m_MediumFont;
    }
    return m_RegularFont;
}

uint64_t TextUIService::HashGeometryKey(
    std::string_view text,
    float fontSize,
    we::runtime::text::layout::FontWeight weight,
    bool italic) const
{
    uint64_t h = 14695981039346656037ULL;
    for (const unsigned char c : text) {
        h ^= static_cast<uint64_t>(c);
        h *= 1099511628211ULL;
    }
    const auto mix = [&](uint64_t v) {
        h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    };
    mix(static_cast<uint64_t>(fontSize * 1000.0f));
    mix(static_cast<uint64_t>(weight));
    mix(italic ? 1ULL : 0ULL);
    return h;
}

const TextUIService::GeometryCacheEntry* TextUIService::GetOrBuildGeometry(
    std::string_view text,
    const we::runtime::text::layout::TextStyle& style,
    we::runtime::text::FontHandle font,
    uint64_t geomKey)
{
    const uint64_t atlasGen = m_TextEngine->AtlasGeneration();
    auto& residency = UIResourceResidency::Get();
    if (auto it = m_GeometryCache.find(geomKey); it != m_GeometryCache.end()) {
        if (it->second.atlasGeneration == atlasGen) {
            ++m_FrameStats.geometryCacheHits;
            it->second.lastUsedFrame = residency.CurrentFrame();
            residency.NoteHit();
            return &it->second;
        }
        m_GeometryCache.erase(it);
    }

    ++m_FrameStats.geometryCacheMisses;
    residency.NoteMiss();

    we::runtime::text::layout::LayoutConstraints constraints{};
    constraints.maxWidth = 1.0e9f;
    constraints.wordWrap = false;
    constraints.dpiScale = 1.0f;

    const we::runtime::text::layout::LayoutResult* layoutPtr =
        m_TextEngine->GetOrCreateLayout(text, style, constraints, font);
    if (!layoutPtr || layoutPtr->glyphs.empty()) {
        return nullptr;
    }
    MaybeLogScaleDiagnostics(*layoutPtr);

    GeometryCacheEntry entry;
    entry.atlasGeneration = atlasGen;
    entry.quads.reserve(layoutPtr->glyphs.size());
    for (const auto& glyph : layoutPtr->glyphs) {
        if (!glyph.glyph.metrics.hasDrawableQuad) {
            continue;
        }
        if (entry.quads.empty()) {
            entry.atlasPage = glyph.glyph.metrics.atlasPage;
            entry.msdfRange = std::max(glyph.msdfPixelRange, 1.0f);
        }
        GlyphQuad q;
        q.x = glyph.x;
        q.y = glyph.y;
        q.w = std::max(glyph.width, 1.0f);
        q.h = std::max(glyph.height, 1.0f);
        q.u0 = glyph.glyph.metrics.atlasUv.u0;
        q.v0 = glyph.glyph.metrics.atlasUv.v0;
        q.u1 = glyph.glyph.metrics.atlasUv.u1;
        q.v1 = glyph.glyph.metrics.atlasUv.v1;
        q.msdf = std::max(glyph.msdfPixelRange, 1.0f);
        q.atlasPage = glyph.glyph.metrics.atlasPage;
        entry.quads.push_back(q);
    }
    if (entry.quads.empty()) {
        return nullptr;
    }

    while (m_GeometryCache.size() >= kMaxGeometryCacheEntries && !m_GeometryCacheOrder.empty()) {
        const uint64_t oldKey = m_GeometryCacheOrder.front();
        m_GeometryCacheOrder.erase(m_GeometryCacheOrder.begin());
        m_GeometryCache.erase(oldKey);
        UIResourceResidency::Get().NoteEviction();
    }
    entry.lastUsedFrame = UIResourceResidency::Get().CurrentFrame();
    auto [inserted, ok] = m_GeometryCache.emplace(geomKey, std::move(entry));
    (void)ok;
    m_GeometryCacheOrder.push_back(geomKey);
    return &inserted->second;
}

float TextUIService::MeasureText(std::string_view text, float fontSize, bool bold) const {
    return MeasureText(
        text,
        fontSize,
        bold ? we::runtime::text::layout::FontWeight::SemiBold
             : we::runtime::text::layout::FontWeight::Regular);
}

float TextUIService::MeasureText(
    std::string_view text,
    float fontSize,
    we::runtime::text::layout::FontWeight weight) const {
    if (!m_TextEngine || text.empty()) {
        return 0.0f;
    }
    ++m_FrameStats.measureCalls;

    we::runtime::text::layout::TextStyle style{};
    style.sizePx = fontSize;
    style.weight = weight;

    we::runtime::text::layout::LayoutConstraints constraints{};
    constraints.maxWidth = 1.0e9f;
    constraints.wordWrap = false;
    constraints.dpiScale = 1.0f;

    const we::runtime::text::FontHandle fontHandle = ResolveFont(weight);
    // Single layout cache — no KindUI-side string measure cache.
    if (const auto* layout =
            m_TextEngine->GetOrCreateLayout(text, style, constraints, fontHandle)) {
        return layout->bounds.width;
    }
    return 0.0f;
}

uint64_t TextUIService::EstimateMeasureCacheBytes() const {
    if (!m_TextEngine) {
        return 0;
    }
    return m_TextEngine->GetLayoutCacheStats().estimatedBytes;
}

size_t TextUIService::MeasureCacheEntryCount() const {
    if (!m_TextEngine) {
        return 0;
    }
    return m_TextEngine->GetLayoutCacheStats().entries;
}

uint32_t TextUIService::FontAtlasPageCount() const {
    return static_cast<uint32_t>(m_DynamicPages.size() + m_FontAtlases.size());
}

uint64_t TextUIService::EstimateFontAtlasCpuBytes() const {
    uint64_t bytes = 0;
    bytes += static_cast<uint64_t>(m_DynamicPages.size()) * sizeof(GpuAtlasPage);
    bytes += static_cast<uint64_t>(m_FontAtlases.size())
        * (sizeof(we::runtime::text::FontHandle) + sizeof(GpuAtlasPage));
    if (auto* atlas = m_TextEngine ? m_TextEngine->AtlasManager() : nullptr) {
        // Coarse: glyph entries + pages (pages dominate when present).
        bytes += static_cast<uint64_t>(atlas->GlyphCount()) * 64ull;
        for (uint32_t i = 0; i < atlas->PageCount(); ++i) {
            if (const auto* page = atlas->GetPage(i)) {
                bytes += page->page.rgba.size();
            }
        }
    }
    return bytes;
}

size_t TextUIService::GeometryCacheEntryCount() const {
    return m_GeometryCache.size();
}

uint64_t TextUIService::EstimateGeometryCacheBytes() const {
    uint64_t bytes = 0;
    for (const auto& [_, entry] : m_GeometryCache) {
        bytes += sizeof(entry) + entry.quads.capacity() * sizeof(GlyphQuad);
    }
    return bytes;
}

void TextUIService::OnResidencyTick() {
    auto& residency = UIResourceResidency::Get();
    if (!residency.ShouldRunEvictionPass()) {
        return;
    }

    const uint64_t frame = residency.CurrentFrame();
    const uint32_t idle = residency.TextGeomIdleFrames();
    const uint64_t budget = residency.TextGeometryBudgetBytes();
    uint64_t used = EstimateGeometryCacheBytes();
    uint32_t evicted = 0;
    const uint32_t maxEvict = residency.MaxEvictionsThisTick();

    // Trim idle geometry entries; also drop oldest when over byte budget.
    while (evicted < maxEvict && !m_GeometryCacheOrder.empty()) {
        const uint64_t key = m_GeometryCacheOrder.front();
        auto it = m_GeometryCache.find(key);
        if (it == m_GeometryCache.end()) {
            m_GeometryCacheOrder.erase(m_GeometryCacheOrder.begin());
            continue;
        }
        const bool isIdle = frame > it->second.lastUsedFrame
            && (frame - it->second.lastUsedFrame) >= idle;
        const bool overBudget = used > budget;
        if (!isIdle && !overBudget) {
            break;
        }
        if (!isIdle && overBudget
            && (frame - it->second.lastUsedFrame) < (idle / 4u + 1u)) {
            break;
        }
        used = used > (sizeof(it->second) + it->second.quads.capacity() * sizeof(GlyphQuad))
            ? used - (sizeof(it->second) + it->second.quads.capacity() * sizeof(GlyphQuad))
            : 0;
        m_GeometryCache.erase(it);
        m_GeometryCacheOrder.erase(m_GeometryCacheOrder.begin());
        ++evicted;
        residency.NoteEviction();
        if (!overBudget) {
            break;
        }
        if (used <= budget) {
            break;
        }
    }

    uint32_t atlasEvicted = 0;
    if (auto* atlas = m_TextEngine ? m_TextEngine->AtlasManager() : nullptr) {
        const size_t glyphs = atlas->GlyphCount();
        const size_t budgetGlyphs = residency.GlyphEntryBudget();
        if (glyphs > budgetGlyphs) {
            atlas->EvictUnused(budgetGlyphs);
            atlasEvicted = static_cast<uint32_t>(glyphs - budgetGlyphs);
            residency.NoteEviction(atlasEvicted);
        }
    }

    residency.SetResidentSnapshot(
        residency.Stats().residentIconCount,
        residency.Stats().residentIconGpuBytes,
        static_cast<uint32_t>(m_GeometryCache.size()),
        EstimateGeometryCacheBytes(),
        m_TextEngine && m_TextEngine->AtlasManager()
            ? static_cast<uint32_t>(m_TextEngine->AtlasManager()->GlyphCount())
            : 0,
        EstimateFontAtlasCpuBytes());
    residency.NoteEvictionPass(evicted > 0 || atlasEvicted > 0);
}

we::rhi::RHIDescriptorSetHandle TextUIService::GetDescriptorForFont(
    const we::runtime::text::FontHandle handle) {
    if (auto* atlas = m_TextEngine ? m_TextEngine->AtlasManager() : nullptr; atlas && atlas->PageCount() > 0) {
        const auto set = EnsureAtlasPageUploaded(0);
        if (set != we::rhi::RHIDescriptorSetHandle::Invalid) {
            return set;
        }
    }

    auto& gpu = m_FontAtlases[handle];
    if (gpu.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid) {
        return gpu.descriptorSet;
    }
    if (!UploadFontAtlasFallback(handle, gpu)) {
        WE_LOG_ERROR("TextUIService", "Failed to upload font atlas for handle " + std::to_string(handle));
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }
    return gpu.descriptorSet;
}

bool TextUIService::GenerateTextGeometry(
    const DrawCommand& cmd,
    std::vector<UIVertex2>& vertices,
    std::vector<uint32_t>& indices,
    we::rhi::RHIDescriptorSetHandle& outTextureSet,
    UIRenderBatch* outBatchInfo)
{
    if (!m_TextEngine || cmd.text.empty()) {
        return false;
    }

    ++m_FrameStats.textDraws;

    const auto weight = EffectiveWeight(cmd);
    const we::runtime::text::FontHandle layoutFont = ResolveFont(weight);
    const we::runtime::text::layout::TextStyle style = BuildStyle(cmd);
    const uint64_t geomKey = HashGeometryKey(cmd.text, cmd.fontSize, weight, cmd.textItalic);

    const GeometryCacheEntry* geom = GetOrBuildGeometry(cmd.text, style, layoutFont, geomKey);
    if (!geom || geom->quads.empty()) {
        return false;
    }

    const auto engineStats = m_TextEngine->GetLayoutCacheStats();
    m_FrameStats.layoutHits = static_cast<uint32_t>(engineStats.hits);
    m_FrameStats.layoutMisses = static_cast<uint32_t>(engineStats.misses);
    m_FrameStats.layoutCacheEntries = engineStats.entries;
    m_FrameStats.layoutCacheBytes = engineStats.estimatedBytes;
    m_FrameStats.geometryCacheEntries = static_cast<uint32_t>(m_GeometryCache.size());

    bool useDynamic = false;
    uint32_t atlasPageIndex = geom->atlasPage;
    if (auto* atlas = m_TextEngine->AtlasManager(); atlas && atlasPageIndex < atlas->PageCount()) {
        useDynamic = true;
    }

    we::runtime::text::FontHandle resolvedFont = layoutFont;
    if (useDynamic) {
        outTextureSet = EnsureAtlasPageUploaded(atlasPageIndex);
    } else {
        outTextureSet = GetDescriptorForFont(resolvedFont);
    }

    if (outTextureSet == we::rhi::RHIDescriptorSetHandle::Invalid
        || (m_Renderer && outTextureSet == m_Renderer->GetDummyDescriptorSet())) {
        return false;
    }

    constexpr float type = 3.0f;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    if (useDynamic) {
        if (const auto it = m_DynamicPages.find(atlasPageIndex); it != m_DynamicPages.end()) {
            atlasWidth = it->second.width;
            atlasHeight = it->second.height;
        }
    } else if (const auto it = m_FontAtlases.find(resolvedFont); it != m_FontAtlases.end()) {
        atlasWidth = it->second.width;
        atlasHeight = it->second.height;
    }

    const float originX = SnapPx(cmd.rect.x);
    const float originY = SnapPx(cmd.rect.y);
    const uint64_t atlasGen = m_TextEngine->AtlasGeneration();

    m_LastDebugGlyphs.clear();
    if (m_DebugEnabled) {
        m_LastDebugGlyphs.reserve(geom->quads.size());
    }

    const uint32_t startVertex = static_cast<uint32_t>(vertices.size());
    const size_t needVerts = geom->quads.size() * 4;
    const size_t needIdx = geom->quads.size() * 6;
    if (vertices.capacity() < vertices.size() + needVerts) {
        vertices.reserve(vertices.size() + needVerts);
    }
    if (indices.capacity() < indices.size() + needIdx) {
        indices.reserve(indices.size() + needIdx);
    }

    for (const auto& q : geom->quads) {
        const float x0 = originX + q.x;
        const float y0 = originY + q.y;
        const float x1 = x0 + q.w;
        const float y1 = y0 + q.h;

        if (m_DebugEnabled) {
            TextDebugGlyphInfo info;
            info.bounds = Rect{x0, y0, q.w, q.h};
            info.atlasPage = q.atlasPage;
            info.msdfRange = q.msdf;
            info.atlasGeneration = atlasGen;
            m_LastDebugGlyphs.push_back(info);
        }

        UIVertex2 v0{
            {x0, y0},
            {q.u0, q.v0},
            {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a},
            {cmd.clipRect.x, cmd.clipRect.y, cmd.clipRect.width, cmd.clipRect.height},
            {0.0f, type, q.msdf, 0.0f}};
        UIVertex2 v1{
            {x1, y0},
            {q.u1, q.v0},
            {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a},
            {cmd.clipRect.x, cmd.clipRect.y, cmd.clipRect.width, cmd.clipRect.height},
            {0.0f, type, q.msdf, 0.0f}};
        UIVertex2 v2{
            {x1, y1},
            {q.u1, q.v1},
            {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a},
            {cmd.clipRect.x, cmd.clipRect.y, cmd.clipRect.width, cmd.clipRect.height},
            {0.0f, type, q.msdf, 0.0f}};
        UIVertex2 v3{
            {x0, y1},
            {q.u0, q.v1},
            {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a},
            {cmd.clipRect.x, cmd.clipRect.y, cmd.clipRect.width, cmd.clipRect.height},
            {0.0f, type, q.msdf, 0.0f}};

        const uint32_t base = static_cast<uint32_t>(vertices.size());
        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    m_FrameStats.glyphsEmitted += static_cast<uint32_t>(geom->quads.size());

    if (outBatchInfo) {
        outBatchInfo->isText = true;
        outBatchInfo->atlasWidth = atlasWidth;
        outBatchInfo->atlasHeight = atlasHeight;
        outBatchInfo->msdfPixelRange = geom->msdfRange;
    }

    return vertices.size() > startVertex;
}

} // namespace we::runtime::kindui
