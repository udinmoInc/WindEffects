#include "Rendering/IconRenderer.h"
#include "Core/ImageBarriers.h"
#include "Resource/ResourceManager.h"
#include "Core/Theme.h"
#include "Core/Logger.h"
#include <volk.h>
#include <stdexcept>
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <functional>

#include <unordered_set>

#define NANOSVG_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4456 4244 4702)
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>
#pragma warning(pop)

namespace we::UI {

IconRenderer::IconRenderer() = default;

IconRenderer::~IconRenderer() {
    Shutdown();
}

bool IconRenderer::Init(we::runtime::renderer::DeviceContext* context,
                        we::runtime::renderer::ResourceManager* resources,
                        UiGpuUpload* gpuUpload,
                        VkDescriptorPool descriptorPool,
                        VkDescriptorSetLayout textureLayout) {
    m_Context = context;
    m_Resources = resources;
    m_GpuUpload = gpuUpload;
    m_DescriptorPool = descriptorPool;
    m_TextureLayout = textureLayout;
    IconRegistry::Get().InitializeDefaultIcons();
    return context != nullptr && resources != nullptr && gpuUpload != nullptr
        && descriptorPool != VK_NULL_HANDLE && textureLayout != VK_NULL_HANDLE;
}

void IconRenderer::Shutdown() {
    ClearCache();
}

std::string IconRenderer::ResolveLucideSvgPath(const std::string& lucideName) {
    const std::string fileName = lucideName + ".svg";
    const std::string candidates[] = {
        "Assets/Icons/icons/" + fileName,
        "Icons/icons/" + fileName,
        "../Assets/Icons/icons/" + fileName,
        "../../Assets/Icons/icons/" + fileName,
        "Engine/Content/Icons/icons/" + fileName,
        "../Engine/Content/Icons/icons/" + fileName,
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) return path;
    }
    return {};
}

VkDescriptorSet IconRenderer::GetLucideIcon(const std::string& iconName, uint32_t size, const Color& color, float strokeWidth) {
    if (!m_Context || m_TextureLayout == VK_NULL_HANDLE) return VK_NULL_HANDLE;

    const std::string lucideName = Icons::ResolveLucideName(iconName);
    const std::string svgPath = ResolveLucideSvgPath(lucideName);
    if (svgPath.empty()) {
        static std::unordered_set<std::string> s_ReportedMissing;
        if (s_ReportedMissing.insert(lucideName).second) {
            HE_ERROR("[UI] Lucide icon not found: " + lucideName);
        }
        return VK_NULL_HANDLE;
    }

    const std::string key = lucideName + "_" + std::to_string(size) + "_"
        + std::to_string(static_cast<int>(color.r * 255.0f)) + "_"
        + std::to_string(static_cast<int>(color.g * 255.0f)) + "_"
        + std::to_string(static_cast<int>(color.b * 255.0f)) + "_"
        + std::to_string(static_cast<int>(color.a * 255.0f)) + "_"
        + std::to_string(static_cast<int>(strokeWidth * 100.0f));

    auto it = m_Cache.find(key);
    if (it != m_Cache.end()) {
        return it->second.descriptorSet;
    }

    std::vector<uint8_t> bitmap = RenderSVGToBitmap(svgPath, size, color, strokeWidth);
    if (bitmap.empty()) return VK_NULL_HANDLE;

    IconTexture texture;
    if (CreateTexture(bitmap, size, size, texture)) {
        m_Cache[key] = texture;
        return texture.descriptorSet;
    }
    return VK_NULL_HANDLE;
}

VkDescriptorSet IconRenderer::GetIcon(const std::string& iconName, uint32_t size) {
    if (!m_Context || m_TextureLayout == VK_NULL_HANDLE) return VK_NULL_HANDLE;

    if (iconName.find('/') != std::string::npos || iconName.find('\\') != std::string::npos) {
        std::string key = iconName + "_" + std::to_string(size);
        auto it = m_Cache.find(key);
        if (it != m_Cache.end()) {
            return it->second.descriptorSet;
        }

        std::vector<uint8_t> bitmap = RenderSVGToBitmap(iconName, size, m_DefaultColor);
        if (bitmap.empty()) return VK_NULL_HANDLE;

        IconTexture texture;
        if (CreateTexture(bitmap, size, size, texture)) {
            m_Cache[key] = texture;
            return texture.descriptorSet;
        }
        return VK_NULL_HANDLE;
    }

    return GetLucideIcon(iconName, size, m_DefaultColor);
}

VkDescriptorSet IconRenderer::CreateTextureFromBitmap(const std::vector<uint8_t>& bitmap, uint32_t width, uint32_t height) {
    if (!m_Context || m_TextureLayout == VK_NULL_HANDLE || bitmap.empty() || width == 0 || height == 0) return VK_NULL_HANDLE;

    IconTexture texture;
    if (CreateTexture(bitmap, width, height, texture)) {
        const std::string key = "thumb_" + std::to_string(width) + "x" + std::to_string(height) + "_" +
            std::to_string(std::hash<std::string>{}(std::string(
                reinterpret_cast<const char*>(bitmap.data()),
                std::min(bitmap.size(), static_cast<size_t>(64)))));
        m_Cache[key] = texture;
        return texture.descriptorSet;
    }
    return VK_NULL_HANDLE;
}

void IconRenderer::ClearCache() {
    for (auto& pair : m_Cache) {
        DestroyTexture(pair.second);
    }
    m_Cache.clear();
}

std::vector<uint8_t> IconRenderer::RenderSVGToBitmap(const std::string& svgPath, uint32_t size, const Color& color, float strokeWidth) {
    std::string finalPath = svgPath;
    if (!std::filesystem::exists(finalPath)) {
        if (std::filesystem::exists("../" + svgPath)) finalPath = "../" + svgPath;
        else if (std::filesystem::exists("../../" + svgPath)) finalPath = "../../" + svgPath;
        else if (std::filesystem::exists("../../../" + svgPath)) finalPath = "../../../" + svgPath;
    }

    NSVGimage* image = nsvgParseFromFile(finalPath.c_str(), "px", 96.0f);
    if (!image) {
        HE_ERROR("[UI] IconRenderer failed to parse SVG: " + finalPath);
        return {};
    }

    int width = static_cast<int>(image->width);
    int height = static_cast<int>(image->height);
    if (width <= 0 || height <= 0) {
        nsvgDelete(image);
        return {};
    }

    if (strokeWidth > 0.0f) {
        for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) {
            shape->strokeWidth = strokeWidth;
        }
    }

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(image);
        return {};
    }

    const int baseDim = std::max(width, height);
    
    // Use 4x SSAA (Supersampling Anti-Aliasing) on the CPU side since nsvgRasterize can produce aliased edges at 1x
    constexpr int kScaleFactor = 4;
    const int rasterSize = size * kScaleFactor;
    const float scale = static_cast<float>(rasterSize) / static_cast<float>(baseDim);
    
    std::vector<uint8_t> rasterData(static_cast<size_t>(rasterSize) * static_cast<size_t>(rasterSize) * 4, 0);
    nsvgRasterize(rast, image, 0, 0, scale, rasterData.data(), rasterSize, rasterSize, rasterSize * 4);

    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    // Downsample to target size
    std::vector<uint8_t> finalData(static_cast<size_t>(size) * static_cast<size_t>(size) * 4, 0);
    for (int y = 0; y < static_cast<int>(size); ++y) {
        for (int x = 0; x < static_cast<int>(size); ++x) {
            uint32_t sumR = 0, sumG = 0, sumB = 0, sumA = 0;
            
            for (int dy = 0; dy < kScaleFactor; ++dy) {
                for (int dx = 0; dx < kScaleFactor; ++dx) {
                    int sx = x * kScaleFactor + dx;
                    int sy = y * kScaleFactor + dy;
                    int srcIdx = (sy * rasterSize + sx) * 4;
                    
                    uint8_t r = rasterData[srcIdx];
                    uint8_t g = rasterData[srcIdx + 1];
                    uint8_t b = rasterData[srcIdx + 2];
                    uint8_t a = rasterData[srcIdx + 3];
                    
                    // Premultiply alpha for correct color blending during downsample
                    sumR += r * a;
                    sumG += g * a;
                    sumB += b * a;
                    sumA += a;
                }
            }
            
            int dstIdx = (y * size + x) * 4;
            uint8_t outA = static_cast<uint8_t>(sumA / (kScaleFactor * kScaleFactor));
            finalData[dstIdx + 3] = outA;
            
            if (sumA > 0) {
                finalData[dstIdx] = static_cast<uint8_t>(sumR / sumA);
                finalData[dstIdx + 1] = static_cast<uint8_t>(sumG / sumA);
                finalData[dstIdx + 2] = static_cast<uint8_t>(sumB / sumA);
            } else {
                finalData[dstIdx] = 0;
                finalData[dstIdx + 1] = 0;
                finalData[dstIdx + 2] = 0;
            }

            const float mask = finalData[dstIdx + 3] / 255.0f;
            if (mask > 0.0f) {
                finalData[dstIdx]     = static_cast<uint8_t>(color.r * 255.0f);
                finalData[dstIdx + 1] = static_cast<uint8_t>(color.g * 255.0f);
                finalData[dstIdx + 2] = static_cast<uint8_t>(color.b * 255.0f);
                finalData[dstIdx + 3] = static_cast<uint8_t>(mask * color.a * 255.0f);
            }
        }
    }

    return finalData;
}

bool IconRenderer::CreateTexture(const std::vector<uint8_t>& bitmap, uint32_t width, uint32_t height, IconTexture& outTexture) {
    if (!m_Context || !m_Resources || !m_GpuUpload || m_DescriptorPool == VK_NULL_HANDLE || m_TextureLayout == VK_NULL_HANDLE) {
        return false;
    }
    if (bitmap.empty() || width == 0 || height == 0) {
        return false;
    }

    VkDevice device = m_Context->GetDevice();
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(bitmap.size());

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
    std::memcpy(mapped, bitmap.data(), bitmap.size());
    vkUnmapMemory(device, stagingMemory);

    m_Resources->CreateImage(
        width,
        height,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        outTexture.image,
        outTexture.memory);

    m_GpuUpload->SubmitOneTime([&](VkCommandBuffer cmd) {
        we::runtime::renderer::TransitionImageLayout(
            cmd,
            outTexture.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {width, height, 1};
        vkCmdCopyBufferToImage(cmd, stagingBuffer, outTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        we::runtime::renderer::TransitionImageLayout(
            cmd,
            outTexture.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    });

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    outTexture.view = m_Resources->CreateImageView(outTexture.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    outTexture.width = width;
    outTexture.height = height;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &outTexture.sampler) != VK_SUCCESS) {
        return false;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_TextureLayout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &outTexture.descriptorSet) != VK_SUCCESS) {
        return false;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = outTexture.view;
    imageInfo.sampler = outTexture.sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = outTexture.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

    return true;
}

void IconRenderer::DestroyTexture(IconTexture& texture) {
    if (!m_Context) {
        texture = {};
        return;
    }

    VkDevice device = m_Context->GetDevice();
    if (texture.descriptorSet != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(device, m_DescriptorPool, 1, &texture.descriptorSet);
    }
    if (texture.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, texture.sampler, nullptr);
    }
    if (texture.view != VK_NULL_HANDLE) {
        vkDestroyImageView(device, texture.view, nullptr);
    }
    if (texture.image != VK_NULL_HANDLE) {
        vkDestroyImage(device, texture.image, nullptr);
    }
    if (texture.memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, texture.memory, nullptr);
    }
    texture = {};
}

} // namespace we::UI
