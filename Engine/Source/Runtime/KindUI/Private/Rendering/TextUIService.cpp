#include "KindUI/Rendering/TextUIService.h"

#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Rendering/FontImportService.h"
#include "KindUI/Rendering/OverlayRenderer.h"
#include "Core/AssetRegistry.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace we::runtime::kindui {

namespace {

constexpr float kFontBakeSizePx = 18.0f;

inline float SnapPx(const float v) {
    return std::floor(v + 0.5f);
}

std::filesystem::path ResolveWeFontPath(const std::string& baseName) {
    const std::vector<std::string> candidates = {
        "Assets/Fonts/" + baseName + ".wefont",
        "Fonts/" + baseName + ".wefont",
        "../Assets/Fonts/" + baseName + ".wefont",
        "../Fonts/" + baseName + ".wefont",
    };
    const auto resolved = we::core::AssetRegistry::ResolveAssetPath(candidates);
    return resolved.empty() ? std::filesystem::path{} : std::filesystem::path(resolved);
}

std::filesystem::path ResolveTtfPath(const std::string& baseName) {
    const std::vector<std::string> candidates = {
        "Assets/Fonts/" + baseName + ".ttf",
        "Fonts/" + baseName + ".ttf",
        "../Assets/Fonts/" + baseName + ".ttf",
        "../Fonts/" + baseName + ".ttf",
        "Engine/Content/Fonts/" + baseName + ".ttf",
    };
    const auto resolved = we::core::AssetRegistry::ResolveAssetPath(candidates);
    return resolved.empty() ? std::filesystem::path{} : std::filesystem::path(resolved);
}

std::filesystem::path EnsureWeFontAsset(const std::string& baseName) {
    if (const auto existing = ResolveWeFontPath(baseName); !existing.empty()) {
        return existing;
    }

    const auto ttfPath = ResolveTtfPath(baseName);
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

std::filesystem::path ResolveSemiBoldWeFontPath() {
    const std::string names[] = {"Inter-SemiBold", "Inter-Bold"};
    for (const auto& name : names) {
        if (const auto path = EnsureWeFontAsset(name); !path.empty()) {
            return path;
        }
    }
    return {};
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

    const auto regularPath = EnsureWeFontAsset("Inter-Regular");
    const auto semiBoldPath = ResolveSemiBoldWeFontPath();
    if (!regularPath.empty()) {
        const auto loaded = m_TextEngine->LoadFont(regularPath);
        if (loaded.ok) {
            m_RegularFont = loaded.value;
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
            "No .wefont assets found and TTF import failed. Ensure Assets/Fonts/Inter-Regular.wefont exists.");
        return false;
    }

    if (m_SemiBoldFont == we::runtime::text::kInvalidFontHandle) {
        m_SemiBoldFont = m_RegularFont;
    }

    SyncDirtyAtlasPages();
    if (m_DynamicPages.empty()) {
        if (GetDescriptorForFont(m_RegularFont) == we::rhi::RHIDescriptorSetHandle::Invalid) {
            WE_LOG_ERROR("TextUIService", "Failed to upload regular font atlas");
            return false;
        }
    }

    TextMetrics::SetMeasureProvider([this](const std::string_view text, const float fontSize, const bool bold) {
        return MeasureText(std::string(text), fontSize, bold);
    });

    return true;
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
    m_TextEngine.reset();
    m_Renderer = nullptr;
}

void TextUIService::SyncDirtyAtlasPages() {
    auto* atlas = m_TextEngine ? m_TextEngine->AtlasManager() : nullptr;
    if (!atlas || !m_Renderer) {
        return;
    }
    for (const uint32_t pageIndex : atlas->TakeDirtyPages()) {
        (void)EnsureAtlasPageUploaded(pageIndex);
    }
    // Ensure all existing pages are uploaded at least once.
    for (uint32_t i = 0; i < atlas->PageCount(); ++i) {
        (void)EnsureAtlasPageUploaded(i);
    }
}

we::rhi::RHIDescriptorSetHandle TextUIService::EnsureAtlasPageUploaded(const uint32_t pageIndex) {
    auto* atlas = m_TextEngine ? m_TextEngine->AtlasManager() : nullptr;
    if (!atlas || !m_Renderer) {
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }
    const auto pageCopy = atlas->CopyPage(pageIndex);
    if (!pageCopy || pageCopy->page.rgba.empty()) {
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    auto& gpu = m_DynamicPages[pageIndex];
    if (gpu.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid
        && gpu.width == pageCopy->page.width
        && gpu.height == pageCopy->page.height
        && gpu.version == pageCopy->version) {
        return gpu.descriptorSet;
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
    style.weight = cmd.textBold ? we::runtime::text::layout::FontWeight::SemiBold
                                : we::runtime::text::layout::FontWeight::Regular;
    style.italic = cmd.textItalic;
    style.color = {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a};
    return style;
}

float TextUIService::MeasureText(const std::string& text, float fontSize, bool bold) const {
    if (!m_TextEngine) {
        return 0.0f;
    }
    we::runtime::text::layout::TextStyle style{};
    // fontSize is logical (CSS-like) px; DPI is applied via constraints.
    style.sizePx = fontSize;
    style.weight = bold ? we::runtime::text::layout::FontWeight::SemiBold
                        : we::runtime::text::layout::FontWeight::Regular;

    we::runtime::text::layout::LayoutConstraints constraints{};
    constraints.maxWidth = 1.0e9f;
    constraints.wordWrap = false;
    constraints.dpiScale = 1.0f;
    const we::runtime::text::FontHandle fontHandle = bold ? m_SemiBoldFont : m_RegularFont;
    return m_TextEngine->Measure(text, style, constraints, fontHandle).width;
}

we::rhi::RHIDescriptorSetHandle TextUIService::GetDescriptorForFont(
    const we::runtime::text::FontHandle handle) {
    // Prefer dynamic atlas page 0 when seeded.
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
    if (!m_TextEngine) {
        return false;
    }

    SyncDirtyAtlasPages();

    we::runtime::text::FontHandle layoutFont = cmd.textBold ? m_SemiBoldFont : m_RegularFont;

    we::runtime::text::layout::LayoutConstraints constraints{};
    constraints.maxWidth = 1.0e9f;
    constraints.wordWrap = false;
    constraints.dpiScale = 1.0f;

    auto layout = m_TextEngine->Layout(cmd.text, BuildStyle(cmd), constraints, layoutFont);
    if (layout.glyphs.empty()) {
        return false;
    }

    uint32_t atlasPageIndex = 0;
    bool useDynamic = false;
    for (const auto& glyph : layout.glyphs) {
        if (glyph.glyph.metrics.hasDrawableQuad) {
            atlasPageIndex = glyph.glyph.metrics.atlasPage;
            useDynamic = m_TextEngine->AtlasManager() != nullptr
                && atlasPageIndex < m_TextEngine->AtlasManager()->PageCount();
            break;
        }
    }

    if (useDynamic) {
        outTextureSet = EnsureAtlasPageUploaded(atlasPageIndex);
    } else {
        for (const auto& glyph : layout.glyphs) {
            if (glyph.glyph.fontHandle != we::runtime::text::kInvalidFontHandle) {
                layoutFont = glyph.glyph.fontHandle;
                break;
            }
        }
        outTextureSet = GetDescriptorForFont(layoutFont);
    }

    if (outTextureSet == we::rhi::RHIDescriptorSetHandle::Invalid
        || (m_Renderer && outTextureSet == m_Renderer->GetDummyDescriptorSet())) {
        return false;
    }

    constexpr float type = 3.0f;
    float batchMsdfRange = 4.0f;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    if (useDynamic) {
        if (const auto it = m_DynamicPages.find(atlasPageIndex); it != m_DynamicPages.end()) {
            atlasWidth = it->second.width;
            atlasHeight = it->second.height;
        }
    } else if (const auto it = m_FontAtlases.find(layoutFont); it != m_FontAtlases.end()) {
        atlasWidth = it->second.width;
        atlasHeight = it->second.height;
    }

    const uint32_t startVertex = static_cast<uint32_t>(vertices.size());
    for (const auto& glyph : layout.glyphs) {
        if (!glyph.glyph.metrics.hasDrawableQuad) {
            continue;
        }

        // Snap glyph origin only; preserve measured width/height for consistent spacing.
        const float x0 = SnapPx(cmd.rect.x + glyph.x);
        const float y0 = SnapPx(cmd.rect.y + glyph.y);
        const float x1 = x0 + std::max(glyph.width, 1.0f);
        const float y1 = y0 + std::max(glyph.height, 1.0f);

        // Atlas texel distance range (not scaled by font size — fwidth handles scale).
        const float msdfRange = std::max(glyph.msdfPixelRange, 1.0f);
        if (vertices.size() == startVertex) {
            batchMsdfRange = msdfRange;
        }

        UIVertex2 v0{
            {x0, y0},
            {glyph.glyph.metrics.atlasUv.u0, glyph.glyph.metrics.atlasUv.v0},
            {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a},
            {x0, y0, x1 - x0, y1 - y0},
            {0.0f, type, msdfRange, 0.0f}};
        UIVertex2 v1{
            {x1, y0},
            {glyph.glyph.metrics.atlasUv.u1, glyph.glyph.metrics.atlasUv.v0},
            {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a},
            {x0, y0, x1 - x0, y1 - y0},
            {0.0f, type, msdfRange, 0.0f}};
        UIVertex2 v2{
            {x1, y1},
            {glyph.glyph.metrics.atlasUv.u1, glyph.glyph.metrics.atlasUv.v1},
            {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a},
            {x0, y0, x1 - x0, y1 - y0},
            {0.0f, type, msdfRange, 0.0f}};
        UIVertex2 v3{
            {x0, y1},
            {glyph.glyph.metrics.atlasUv.u0, glyph.glyph.metrics.atlasUv.v1},
            {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a},
            {x0, y0, x1 - x0, y1 - y0},
            {0.0f, type, msdfRange, 0.0f}};

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

    if (outBatchInfo) {
        outBatchInfo->isText = true;
        outBatchInfo->atlasWidth = atlasWidth;
        outBatchInfo->atlasHeight = atlasHeight;
        outBatchInfo->msdfPixelRange = batchMsdfRange;
    }

    return vertices.size() > startVertex;
}

} // namespace we::runtime::kindui
