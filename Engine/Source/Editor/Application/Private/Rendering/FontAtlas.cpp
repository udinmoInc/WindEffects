#include "Rendering/FontAtlas.h"
#include "Rendering/Text/MsdfFontAtlasBackend.h"
#include "Rendering/Text/Utf8.h"
#include "Rendering/UiGpuUpload.h"
#include "Core/DeviceContext.h"
#include "Resource/ResourceManager.h"
#include "Core/ImageBarriers.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace we::UI {

FontAtlas::FontAtlas() = default;

FontAtlas::~FontAtlas() {
    Shutdown();
}

bool FontAtlas::Init(we::runtime::renderer::DeviceContext* context,
                     we::runtime::renderer::ResourceManager* resources,
                     UiGpuUpload* gpuUpload,
                     const std::string& fontName,
                     float fontHeightPx,
                     int /*firstChar*/,
                     int /*numChars*/,
                     int width,
                     int height) {
    m_Context = context;
    m_Resources = resources;
    m_GpuUpload = gpuUpload;
    if (!m_Context || !m_Resources || !m_GpuUpload) {
        return false;
    }

    m_FontHeight = std::clamp(fontHeightPx, 10.0f, 96.0f);
    m_Backend = Text::CreateMsdfFontAtlasBackend();

    Text::FontAtlasBakeRequest request{};
    request.primaryFontPath = fontName;
    request.fallbackFontPath = "Assets/Fonts/NotoSans-Regular.ttf";
    request.emSizePx = m_FontHeight;
    request.msdfPixelRange = 4.0f;
    request.atlasWidth = width;
    request.atlasHeight = height;

    if (!m_Backend->Initialize(request)) {
        HE_ERROR("FontAtlas: MSDF backend initialization failed");
        m_Backend.reset();
        return false;
    }

    if (!UploadAtlasIfDirty()) {
        HE_ERROR("FontAtlas: GPU atlas upload failed");
        return false;
    }

    HE_INFO("FontAtlas: MSDF atlas initialized (FreeType + msdf-atlas-gen)");
    return true;
}

void FontAtlas::Shutdown() {
    if (m_Backend) {
        m_Backend->Shutdown();
        m_Backend.reset();
    }

    if (!m_Context) {
        return;
    }

    VkDevice device = m_Context->GetDevice();
    if (m_Sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_Sampler, nullptr);
        m_Sampler = VK_NULL_HANDLE;
    }
    if (m_ImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_ImageView, nullptr);
        m_ImageView = VK_NULL_HANDLE;
    }
    if (m_Image != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_Image, nullptr);
        m_Image = VK_NULL_HANDLE;
    }
    if (m_Memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_Memory, nullptr);
        m_Memory = VK_NULL_HANDLE;
    }

    m_Context = nullptr;
    m_Resources = nullptr;
    m_GpuUpload = nullptr;
}

bool FontAtlas::EnsureTextGlyphs(std::string_view utf8Text) {
    if (!m_Backend) {
        return false;
    }

    std::vector<uint32_t> codepoints;
    Text::DecodeUtf8(utf8Text, codepoints);
    if (!m_Backend->EnsureGlyphs(codepoints)) {
        return false;
    }
    return UploadAtlasIfDirty();
}

bool FontAtlas::GetGlyphQuad(uint32_t codepoint, float* xpos, float* ypos, GlyphInfo& quad) {
    if (!m_Backend) {
        return false;
    }
    return m_Backend->GetGlyphQuad(codepoint, xpos, ypos, quad);
}

bool FontAtlas::GetCharQuad(int c, float* xpos, float* ypos, GlyphInfo& quad) {
    if (c < 0) {
        return false;
    }
    return GetGlyphQuad(static_cast<uint32_t>(c), xpos, ypos, quad);
}

float FontAtlas::GetMsdfPixelRange() const {
    return m_Backend ? m_Backend->GetMsdfPixelRange() : 4.0f;
}

bool FontAtlas::UploadAtlasIfDirty() {
    if (!m_Backend || !m_Backend->IsDirty()) {
        return true;
    }

    const auto& pixels = m_Backend->GetAtlasPixels();
    const int atlasWidth = m_Backend->GetAtlasWidth();
    const int atlasHeight = m_Backend->GetAtlasHeight();
    if (pixels.empty() || atlasWidth <= 0 || atlasHeight <= 0) {
        return false;
    }

    VkDevice device = m_Context->GetDevice();
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(pixels.size());

    if (m_Image == VK_NULL_HANDLE) {
        m_Resources->CreateImage(
            static_cast<uint32_t>(atlasWidth),
            static_cast<uint32_t>(atlasHeight),
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_Image,
            m_Memory);
        m_ImageView = m_Resources->CreateImageView(m_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        if (vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS) {
            HE_ERROR("FontAtlas: failed to create sampler");
            return false;
        }
    }

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    m_Resources->CreateBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    void* mapped = nullptr;
    vkMapMemory(device, stagingMemory, 0, imageSize, 0, &mapped);
    std::memcpy(mapped, pixels.data(), pixels.size());
    vkUnmapMemory(device, stagingMemory);

    m_GpuUpload->SubmitOneTime([&](VkCommandBuffer cmd) {
        we::runtime::renderer::TransitionImageLayout(
            cmd,
            m_Image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = { static_cast<uint32_t>(atlasWidth), static_cast<uint32_t>(atlasHeight), 1 };
        vkCmdCopyBufferToImage(cmd, stagingBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        we::runtime::renderer::TransitionImageLayout(
            cmd,
            m_Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    });

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    m_Backend->ClearDirty();
    return true;
}

} // namespace we::UI
