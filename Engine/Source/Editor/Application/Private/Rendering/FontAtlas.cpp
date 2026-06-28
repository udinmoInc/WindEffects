#include "FontAtlas.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Core/Logger.hpp"

#include <fstream>
#include <vector>
#include <iostream>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace we::UI {

FontAtlas::~FontAtlas() {
    Shutdown();
}

bool FontAtlas::Init(const std::shared_ptr<VulkanContext>& context, const std::string& fontName, int firstChar, int numChars, int width, int height) {
    m_Context = context;
    VkDevice device = m_Context->GetDevice();
    
    m_AtlasWidth = width;
    m_AtlasHeight = height;
    m_FirstChar = firstChar;
    m_NumChars = numChars;
    
    m_BakedChars.resize(m_NumChars);

    // 1. Locate and read font file
    std::vector<std::string> searchPaths = {
        "Fonts/" + fontName,
        "../Fonts/" + fontName,
        "../../Fonts/" + fontName,
        "Build/bin/Fonts/" + fontName,
        "../Build/bin/Fonts/" + fontName,
        fontName
    };

    std::string fontPath = "";
    std::vector<char> fontBuffer;

    for (const auto& path : searchPaths) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            fontBuffer.resize(static_cast<size_t>(size));
            if (file.read(fontBuffer.data(), size)) {
                fontPath = path;
                break;
            }
        }
    }

    if (fontPath.empty() || fontBuffer.empty() || fontBuffer.size() == 0) {
        HE_ERROR("FontAtlas: Failed to locate or load " + fontName);
        return false;
    }
    HE_INFO("FontAtlas: Successfully loaded font from " + fontPath + " (" + std::to_string(fontBuffer.size()) + " bytes)");

    // 2. Rasterize glyphs using stb_truetype into a temporary alpha buffer
    std::vector<uint8_t> alphaBuffer(m_AtlasWidth * m_AtlasHeight);
    int res = stbtt_BakeFontBitmap(
        reinterpret_cast<const unsigned char*>(fontBuffer.data()), 0,
        m_FontHeight,
        alphaBuffer.data(), m_AtlasWidth, m_AtlasHeight,
        m_FirstChar, m_NumChars,
        m_BakedChars.data()
    );

    if (res <= 0) {
        HE_ERROR("FontAtlas: Failed to bake font bitmap! baked height is " + std::to_string(res));
        return false;
    }

    // 3. Convert single-channel alpha map to 4-channel RGBA (255, 255, 255, alpha)
    std::vector<uint8_t> rgbaBuffer(m_AtlasWidth * m_AtlasHeight * 4);
    for (int i = 0; i < m_AtlasWidth * m_AtlasHeight; i++) {
        uint8_t alpha = alphaBuffer[i];
        rgbaBuffer[i * 4 + 0] = 255;
        rgbaBuffer[i * 4 + 1] = 255;
        rgbaBuffer[i * 4 + 2] = 255;
        rgbaBuffer[i * 4 + 3] = alpha;
    }

    // 4. Upload to Vulkan GPU Image
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkDeviceSize imageSize = m_AtlasWidth * m_AtlasHeight * 4;

    m_Context->CreateBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, rgbaBuffer.data(), static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    m_Context->CreateImage(
        m_AtlasWidth, m_AtlasHeight,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_Image,
        m_Memory
    );

    m_Context->TransitionImageLayout(
        m_Image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    // Copy Buffer to Image
    VkCommandBuffer cmd = m_Context->BeginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = { static_cast<uint32_t>(m_AtlasWidth), static_cast<uint32_t>(m_AtlasHeight), 1 };

    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    m_Context->EndSingleTimeCommands(cmd);

    m_Context->TransitionImageLayout(
        m_Image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    // Create Image View
    m_ImageView = m_Context->CreateImageView(m_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    // Create Sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS) {
        HE_ERROR("FontAtlas: Failed to create Vulkan Sampler!");
        return false;
    }

    HE_INFO("FontAtlas: Vulkan font atlas texture initialized successfully.");
    return true;
}

void FontAtlas::Shutdown() {
    if (!m_Context) return;
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
}

bool FontAtlas::GetCharQuad(int c, float* xpos, float* ypos, GlyphInfo& quad) {
    if (c < m_FirstChar || c >= m_FirstChar + m_NumChars) {
        return false; // Character out of baked range
    }

    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(m_BakedChars.data(), m_AtlasWidth, m_AtlasHeight, c - m_FirstChar, xpos, ypos, &q, 1);

    quad.x0 = q.x0;
    quad.y0 = q.y0;
    quad.x1 = q.x1;
    quad.y1 = q.y1;
    quad.u0 = q.s0;
    quad.v0 = q.t0;
    quad.u1 = q.s1;
    quad.v1 = q.t1;
    quad.xadvance = 0.0f; // Handled by stbtt update of xpos

    return true;
}

} // namespace we::editor::application::UI
