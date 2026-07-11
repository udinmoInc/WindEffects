#include "Rendering/TextUIService.h"

#include "Rendering/FontImportService.h"
#include "Rendering/OverlayRenderer.h"
#include "Rendering/UiGpuUpload.h"
#include "AssetRegistry.h"
#include "Core/DeviceContext.h"
#include "Core/ImageBarriers.h"
#include "Core/Logger.h"
#include "Resource/ResourceManager.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace we::UI {

namespace {

constexpr float kFontBakeSizePx = 18.0f;

inline float SnapPx(const float v)
{
    return std::floor(v + 0.5f);
}

std::filesystem::path ResolveWeFontPath(const std::string& baseName)
{
    const std::vector<std::string> candidates = {
        "Assets/Fonts/" + baseName + ".wefont",
        "Fonts/" + baseName + ".wefont",
        "../Assets/Fonts/" + baseName + ".wefont",
        "../Fonts/" + baseName + ".wefont",
    };
    const auto resolved = we::core::AssetRegistry::ResolveAssetPath(candidates);
    return resolved.empty() ? std::filesystem::path{} : std::filesystem::path(resolved);
}

std::filesystem::path ResolveTtfPath(const std::string& baseName)
{
    const std::vector<std::string> candidates = {
        "Assets/Fonts/" + baseName + ".ttf",
        "Fonts/" + baseName + ".ttf",
        "../Assets/Fonts/" + baseName + ".ttf",
        "../Fonts/" + baseName + ".ttf",
    };
    const auto resolved = we::core::AssetRegistry::ResolveAssetPath(candidates);
    return resolved.empty() ? std::filesystem::path{} : std::filesystem::path(resolved);
}

std::filesystem::path EnsureWeFontAsset(const std::string& baseName)
{
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

std::filesystem::path ResolveSemiBoldWeFontPath()
{
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

bool TextUIService::Initialize(OverlayRenderer* renderer)
{
    m_Renderer = renderer;
    m_TextEngine = we::runtime::text::CreateTextEngine();
    m_TextBackend = we::runtime::text::rendering::CreateTextGpuBackend(we::runtime::text::GraphicsApi::Vulkan);

    we::runtime::text::rendering::GpuBackendConfig config;
    config.device = renderer ? renderer->GetDeviceContext() : nullptr;
    if (!m_TextBackend->Initialize(config)) {
        WE_LOG_ERROR("TextUIService", "Failed to initialize text GPU backend");
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
        WE_LOG_ERROR("TextUIService", "No .wefont assets found and TTF import failed. Ensure Assets/Fonts/Inter-Regular.ttf exists.");
        return false;
    }

    if (m_SemiBoldFont == we::runtime::text::kInvalidFontHandle) {
        m_SemiBoldFont = m_RegularFont;
    }

    const VkDescriptorSet regularAtlas = GetDescriptorForFont(m_RegularFont);
    if (regularAtlas == VK_NULL_HANDLE) {
        WE_LOG_ERROR("TextUIService", "Failed to upload regular font atlas");
        return false;
    }

    if (m_SemiBoldFont != m_RegularFont) {
        const VkDescriptorSet semiBoldAtlas = GetDescriptorForFont(m_SemiBoldFont);
        if (semiBoldAtlas == VK_NULL_HANDLE) {
            WE_LOG_WARN("TextUIService", "Semi-bold font atlas upload failed; falling back to regular font for bold text");
            m_SemiBoldFont = m_RegularFont;
        }
    }

    return true;
}

void TextUIService::Shutdown()
{
    if (m_Renderer && m_Renderer->GetDeviceContext()) {
        const VkDevice device = m_Renderer->GetDeviceContext()->GetDevice();
        for (auto& [_, atlas] : m_FontAtlases) {
            if (atlas.descriptorSet != VK_NULL_HANDLE) {
                m_Renderer->UnregisterTexture(atlas.descriptorSet);
            }
            if (atlas.sampler) {
                vkDestroySampler(device, atlas.sampler, nullptr);
            }
            if (atlas.imageView) {
                m_Renderer->GetResourceManager()->DestroyImageView(atlas.imageView);
            }
            if (atlas.image) {
                m_Renderer->GetResourceManager()->DestroyImage(atlas.image, atlas.memory);
            }
        }
    }
    m_FontAtlases.clear();
    if (m_TextBackend) {
        m_TextBackend->Shutdown();
        m_TextBackend.reset();
    }
    m_TextEngine.reset();
    m_Renderer = nullptr;
}

bool TextUIService::UploadFontAtlas(
    const we::runtime::text::FontHandle handle,
    FontGpuAtlas& gpuAtlas)
{
    if (!m_Renderer || !m_TextEngine) {
        return false;
    }

    auto* deviceContext = m_Renderer->GetDeviceContext();
    auto* resources = m_Renderer->GetResourceManager();
    auto* gpuUpload = m_Renderer->GetGpuUpload();
    if (!deviceContext || !resources || !gpuUpload) {
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

    if (gpuAtlas.image != VK_NULL_HANDLE
        && gpuAtlas.width == page.width
        && gpuAtlas.height == page.height
        && gpuAtlas.descriptorSet != VK_NULL_HANDLE) {
        return true;
    }

    const VkDevice device = deviceContext->GetDevice();
    if (gpuAtlas.imageView) {
        resources->DestroyImageView(gpuAtlas.imageView);
        gpuAtlas.imageView = VK_NULL_HANDLE;
    }
    if (gpuAtlas.image) {
        resources->DestroyImage(gpuAtlas.image, gpuAtlas.memory);
        gpuAtlas.image = VK_NULL_HANDLE;
        gpuAtlas.memory = VK_NULL_HANDLE;
    }

    resources->CreateImage(
        page.width,
        page.height,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        gpuAtlas.image,
        gpuAtlas.memory);
    gpuAtlas.imageView = resources->CreateImageView(gpuAtlas.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    if (!gpuAtlas.sampler) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        if (vkCreateSampler(device, &samplerInfo, nullptr, &gpuAtlas.sampler) != VK_SUCCESS) {
            return false;
        }
    }

    const VkDeviceSize imageSize = page.rgba.size();
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    resources->CreateBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    void* mapped = nullptr;
    vkMapMemory(device, stagingMemory, 0, imageSize, 0, &mapped);
    std::memcpy(mapped, page.rgba.data(), page.rgba.size());
    vkUnmapMemory(device, stagingMemory);

    gpuUpload->SubmitOneTime([&](VkCommandBuffer cmd) {
        const VkImageLayout oldLayout = gpuAtlas.layout;
        we::runtime::renderer::TransitionImageLayout(
            cmd,
            gpuAtlas.image,
            oldLayout,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            oldLayout == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            oldLayout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {page.width, page.height, 1};
        vkCmdCopyBufferToImage(cmd, stagingBuffer, gpuAtlas.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        we::runtime::renderer::TransitionImageLayout(
            cmd,
            gpuAtlas.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        gpuAtlas.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    });

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    gpuAtlas.width = page.width;
    gpuAtlas.height = page.height;
    gpuAtlas.descriptorSet = m_Renderer->RegisterTexture(gpuAtlas.imageView, gpuAtlas.sampler);
    (void)m_TextBackend->UploadAtlas(page);
    return gpuAtlas.descriptorSet != VK_NULL_HANDLE;
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
    // Widgets pass logical font sizes already scaled via DPIContext where needed.
    constraints.dpiScale = 1.0f;
    const we::runtime::text::FontHandle fontHandle = bold ? m_SemiBoldFont : m_RegularFont;
    return m_TextEngine->Layout(text, style, constraints, fontHandle).bounds.width;
}

VkDescriptorSet TextUIService::GetDescriptorForFont(const we::runtime::text::FontHandle handle)
{
    auto& atlas = m_FontAtlases[handle];
    if (atlas.descriptorSet != VK_NULL_HANDLE) {
        return atlas.descriptorSet;
    }
    if (!UploadFontAtlas(handle, atlas)) {
        WE_LOG_ERROR("TextUIService", "Failed to upload font atlas for handle " + std::to_string(handle));
        return VK_NULL_HANDLE;
    }
    return atlas.descriptorSet;
}

bool TextUIService::GenerateTextGeometry(
    const DrawCommand& cmd,
    std::vector<UIVertex2>& vertices,
    std::vector<uint32_t>& indices,
    VkDescriptorSet& outTextureSet,
    UIRenderBatch* outBatchInfo)
{
    if (!m_TextEngine) {
        return false;
    }

    we::runtime::text::FontHandle layoutFont = cmd.textBold ? m_SemiBoldFont : m_RegularFont;
    outTextureSet = GetDescriptorForFont(layoutFont);
    if (outTextureSet == VK_NULL_HANDLE
        || (m_Renderer && outTextureSet == m_Renderer->GetDummyDescriptorSet())) {
        if (cmd.textBold && m_RegularFont != we::runtime::text::kInvalidFontHandle) {
            layoutFont = m_RegularFont;
            outTextureSet = GetDescriptorForFont(layoutFont);
        }
        if (outTextureSet == VK_NULL_HANDLE
            || (m_Renderer && outTextureSet == m_Renderer->GetDummyDescriptorSet())) {
            return false;
        }
    }

    we::runtime::text::layout::LayoutConstraints constraints;
    constraints.maxWidth = 1.0e9f;
    constraints.dpiScale = 1.0f;

    const auto layout = m_TextEngine->Layout(cmd.text, BuildStyle(cmd), constraints, layoutFont);
    if (layout.glyphs.empty()) {
        return false;
    }

    const float type = 3.0f;
    float batchMsdfRange = 4.0f;

    const uint32_t startVertex = static_cast<uint32_t>(vertices.size());
    for (const auto& glyph : layout.glyphs) {
        if (!glyph.glyph.metrics.hasDrawableQuad) {
            continue;
        }

        const float msdfRange = glyph.msdfPixelRange;
        if (vertices.size() == startVertex) {
            batchMsdfRange = msdfRange;
        }
        const float x0 = SnapPx(cmd.rect.x + glyph.x);
        const float y0 = SnapPx(cmd.rect.y + glyph.y);
        const float x1 = SnapPx(x0 + std::max(glyph.width, 1.0f));
        const float y1 = SnapPx(y0 + std::max(glyph.height, 1.0f));

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
        const auto& atlas = m_FontAtlases[layoutFont];
        outBatchInfo->isText = true;
        outBatchInfo->atlasWidth = atlas.width;
        outBatchInfo->atlasHeight = atlas.height;
        outBatchInfo->msdfPixelRange = batchMsdfRange;
    }

    return vertices.size() > startVertex;
}

} // namespace we::UI
