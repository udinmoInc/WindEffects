#include "Rendering/TextUIService.h"

#include "Rendering/OverlayRenderer.h"
#include "Rendering/UiGpuUpload.h"
#include "Core/DPIContext.h"
#include "Core/Logger.h"
#include "Runtime/Core/AssetRegistry.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace we::UI {

namespace {

std::filesystem::path ResolveWeFontPath(const std::string& baseName)
{
    const std::string candidates[] = {
        "Assets/Fonts/" + baseName + ".wefont",
        "Fonts/" + baseName + ".wefont",
        "../Assets/Fonts/" + baseName + ".wefont",
        "../Fonts/" + baseName + ".wefont",
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

} // namespace

TextUIService::TextUIService() = default;
TextUIService::~TextUIService() { Shutdown(); }

bool TextUIService::Initialize(OverlayRenderer* renderer)
{
    m_Renderer = renderer;
    m_TextEngine = we::runtime::text::CreateTextEngine();
    m_TextBackend = we::runtime::text::rendering::CreateTextGpuBackend(we::runtime::text::GraphicsApi::Vulkan);

    we::runtime::text::rendering::GpuBackendConfig config;
    m_TextBackend->Initialize(config);

    const auto regularPath = ResolveWeFontPath("Inter-Regular");
    const auto semiBoldPath = ResolveWeFontPath("Inter-SemiBold");
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
        WE_LOG_ERROR("TextUIService", "No .wefont assets found. Run we-font-import to generate Assets/Fonts/*.wefont");
        return false;
    }

    if (m_SemiBoldFont == we::runtime::text::kInvalidFontHandle) {
        m_SemiBoldFont = m_RegularFont;
    }

    return true;
}

void TextUIService::Shutdown()
{
    m_FontDescriptors.clear();
    if (m_TextBackend) {
        m_TextBackend->Shutdown();
        m_TextBackend.reset();
    }
    m_TextEngine.reset();
    m_Renderer = nullptr;
}

we::runtime::text::layout::TextStyle TextUIService::BuildStyle(const DrawCommand& cmd) const
{
    we::runtime::text::layout::TextStyle style;
    style.sizePx = cmd.fontSize;
    style.color = {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a};
    style.weight = cmd.textBold
        ? we::runtime::text::layout::FontWeight::SemiBold
        : we::runtime::text::layout::FontWeight::Regular;
    style.italic = cmd.textItalic;
    return style;
}

float TextUIService::MeasureText(const std::string& text, const float fontSize, const bool bold) const
{
    if (!m_TextEngine) {
        return 0.0f;
    }
    we::runtime::text::layout::TextStyle style;
    style.sizePx = fontSize;
    style.weight = bold
        ? we::runtime::text::layout::FontWeight::SemiBold
        : we::runtime::text::layout::FontWeight::Regular;

    we::runtime::text::layout::LayoutConstraints constraints;
    constraints.maxWidth = 1.0e9f;
    constraints.dpiScale = std::max(1.0f, DPIContext::GetScale());
    return m_TextEngine->Layout(text, style, constraints).bounds.width;
}

VkDescriptorSet TextUIService::GetDescriptorForFont(const we::runtime::text::FontHandle handle)
{
    if (const auto it = m_FontDescriptors.find(handle); it != m_FontDescriptors.end()) {
        return it->second;
    }
    if (!m_Renderer || !m_TextEngine) {
        return VK_NULL_HANDLE;
    }

    const auto asset = m_TextEngine->Assets().GetAsset(handle);
    if (!asset || asset->atlasPages.empty()) {
        return VK_NULL_HANDLE;
    }

    const auto& page = asset->atlasPages.front();
    m_TextBackend->UploadAtlas(page);
    return m_Renderer->GetDummyDescriptorSet();
}

bool TextUIService::GenerateTextGeometry(
    const DrawCommand& cmd,
    std::vector<UIVertex2>& vertices,
    std::vector<uint32_t>& indices,
    VkDescriptorSet& outTextureSet)
{
    if (!m_TextEngine) {
        return false;
    }

    const we::runtime::text::FontHandle fontHandle = cmd.textBold ? m_SemiBoldFont : m_RegularFont;
    outTextureSet = GetDescriptorForFont(fontHandle);
    if (outTextureSet == VK_NULL_HANDLE) {
        outTextureSet = m_Renderer ? m_Renderer->GetDummyDescriptorSet() : VK_NULL_HANDLE;
    }

    we::runtime::text::layout::LayoutConstraints constraints;
    constraints.maxWidth = 1.0e9f;
    constraints.dpiScale = std::max(1.0f, DPIContext::GetScale());

    const auto layout = m_TextEngine->Layout(cmd.text, BuildStyle(cmd), constraints);
    if (layout.glyphs.empty()) {
        return false;
    }

    const auto asset = m_TextEngine->Assets().GetAsset(fontHandle);
    const float bakeSize = asset ? asset->metrics.bakeSizePx : cmd.fontSize;
    const float scale = bakeSize > 0.0f ? (cmd.fontSize / bakeSize) : 1.0f;
    const float msdfRange = asset ? asset->metrics.msdfPixelRange * scale : 4.0f;
    const float type = 3.0f;
    const float baselineY = cmd.rect.y + cmd.fontSize * 0.8f;

    const uint32_t startVertex = static_cast<uint32_t>(vertices.size());
    for (const auto& glyph : layout.glyphs) {
        if (!glyph.glyph.metrics.hasDrawableQuad) {
            continue;
        }

        const float x0 = cmd.rect.x + glyph.x * scale;
        const float y0 = baselineY + glyph.y * scale;
        const float x1 = x0 + glyph.width * scale;
        const float y1 = y0 + glyph.height * scale;

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

    return vertices.size() > startVertex;
}

} // namespace we::UI
