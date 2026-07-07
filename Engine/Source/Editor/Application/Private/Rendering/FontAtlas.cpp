#include "Rendering/FontAtlas.h"
#include "Rendering/UiGpuUpload.h"
#include "Core/DeviceContext.h"
#include "Resource/ResourceManager.h"
#include "Core/ImageBarriers.h"
#include "Core/Logger.h"

#include <fstream>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace we::UI {

FontAtlas::~FontAtlas() {
    Shutdown();
}

bool FontAtlas::Init(we::runtime::renderer::DeviceContext* context,
                     we::runtime::renderer::ResourceManager* resources,
                     UiGpuUpload* gpuUpload,
                     const std::string& fontName,
                     int firstChar,
                     int numChars,
                     int width,
                     int height) {
    m_Context = context;
    m_Resources = resources;
    m_GpuUpload = gpuUpload;
    if (!m_Context || !m_Resources || !m_GpuUpload) {
        return false;
    }

    VkDevice device = m_Context->GetDevice();

    m_AtlasWidth = width;
    m_AtlasHeight = height;
    m_FirstChar = firstChar;
    m_NumChars = numChars;
    m_BakedChars.resize(static_cast<size_t>(m_NumChars));

    std::vector<std::string> searchPaths = {
        fontName,
        "Assets/Fonts/" + fontName,
        "Fonts/" + fontName,
        "../Assets/Fonts/" + fontName,
        "../Fonts/" + fontName,
    };

    const auto slash = fontName.find_last_of("/\\");
    if (slash != std::string::npos) {
        const std::string baseName = fontName.substr(slash + 1);
        searchPaths.push_back("Assets/Fonts/" + baseName);
        searchPaths.push_back("Fonts/" + baseName);
    }

    std::string fontPath;
    std::vector<char> fontBuffer;
    for (const auto& path : searchPaths) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            continue;
        }
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        fontBuffer.resize(static_cast<size_t>(size));
        if (file.read(fontBuffer.data(), size)) {
            fontPath = path;
            break;
        }
    }

    if (fontPath.empty() || fontBuffer.empty()) {
        HE_ERROR("FontAtlas: Failed to locate or load " + fontName);
        return false;
    }
    HE_INFO("FontAtlas: Successfully loaded font from " + fontPath);

    std::vector<uint8_t> alphaBuffer(static_cast<size_t>(m_AtlasWidth * m_AtlasHeight));
    const int offset = stbtt_GetFontOffsetForIndex(reinterpret_cast<const unsigned char*>(fontBuffer.data()), 0);
    if (offset < 0) {
        HE_ERROR("FontAtlas: Invalid font data.");
        return false;
    }

    const int bakedHeight = stbtt_BakeFontBitmap(
        reinterpret_cast<const unsigned char*>(fontBuffer.data()),
        offset,
        m_FontHeight,
        alphaBuffer.data(),
        m_AtlasWidth,
        m_AtlasHeight,
        m_FirstChar,
        m_NumChars,
        m_BakedChars.data());

    if (bakedHeight <= 0) {
        HE_ERROR("FontAtlas: Failed to bake font bitmap.");
        return false;
    }

    std::vector<uint8_t> rgbaBuffer(static_cast<size_t>(m_AtlasWidth * m_AtlasHeight * 4));
    for (int i = 0; i < m_AtlasWidth * m_AtlasHeight; ++i) {
        const uint8_t alpha = alphaBuffer[static_cast<size_t>(i)];
        rgbaBuffer[static_cast<size_t>(i) * 4 + 0] = 255;
        rgbaBuffer[static_cast<size_t>(i) * 4 + 1] = 255;
        rgbaBuffer[static_cast<size_t>(i) * 4 + 2] = 255;
        rgbaBuffer[static_cast<size_t>(i) * 4 + 3] = alpha;
    }

    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(rgbaBuffer.size());
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
    std::memcpy(mapped, rgbaBuffer.data(), rgbaBuffer.size());
    vkUnmapMemory(device, stagingMemory);

    m_Resources->CreateImage(
        static_cast<uint32_t>(m_AtlasWidth),
        static_cast<uint32_t>(m_AtlasHeight),
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_Image,
        m_Memory);

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
        region.imageExtent = { static_cast<uint32_t>(m_AtlasWidth), static_cast<uint32_t>(m_AtlasHeight), 1 };
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
        HE_ERROR("FontAtlas: Failed to create Vulkan sampler.");
        return false;
    }

    HE_INFO("FontAtlas: Vulkan font atlas texture initialized successfully.");
    return true;
}

void FontAtlas::Shutdown() {
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

bool FontAtlas::GetCharQuad(int c, float* xpos, float* ypos, GlyphInfo& quad) {
    if (c < m_FirstChar || c >= m_FirstChar + m_NumChars) {
        return false;
    }

    stbtt_aligned_quad q{};
    stbtt_GetBakedQuad(m_BakedChars.data(), m_AtlasWidth, m_AtlasHeight, c - m_FirstChar, xpos, ypos, &q, 1);

    quad.x0 = q.x0;
    quad.y0 = q.y0;
    quad.x1 = q.x1;
    quad.y1 = q.y1;
    quad.u0 = q.s0;
    quad.v0 = q.t0;
    quad.u1 = q.s1;
    quad.v1 = q.t1;
    quad.xadvance = 0.0f;
    return true;
}

} // namespace we::UI
