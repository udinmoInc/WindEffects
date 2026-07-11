#include "Rendering/IconRenderer.h"
#include "Rendering/Icons/SvgRasterizer.h"
#include "Core/ImageBarriers.h"
#include "Resource/ResourceManager.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Logger.h"
#include <volk.h>
#include <stdexcept>
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <functional>

#include <unordered_set>

namespace WindEffects::Editor::UI {

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
    m_SvgRasterizer = std::make_unique<Icons::SvgRasterizer>();
    IconRegistry::Get().InitializeDefaultIcons();
    return context != nullptr && resources != nullptr && gpuUpload != nullptr
        && descriptorPool != VK_NULL_HANDLE && textureLayout != VK_NULL_HANDLE;
}

void IconRenderer::Shutdown() {
    ClearCache();
    m_SvgRasterizer.reset();
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
    if (!m_Context || m_TextureLayout == VK_NULL_HANDLE || !m_SvgRasterizer) return VK_NULL_HANDLE;

    const std::string lucideName = Icons::ResolveLucideName(iconName);

    if (IconRegistry::Get().HasIcon(lucideName)) {
        const std::string relativePath = IconRegistry::Get().GetIconPath(lucideName);
        const std::string resolvedPath = Icons::SvgRasterizer::ResolveAssetPath(relativePath);
        if (!resolvedPath.empty() && std::filesystem::exists(resolvedPath)) {
            const std::string key = std::string("maskreg_") + lucideName + "_" + std::to_string(size);
            auto cached = m_Cache.find(key);
            if (cached != m_Cache.end()) {
                return cached->second.descriptorSet;
            }

            Icons::SvgRasterizeRequest request{};
            request.svgPath = relativePath;
            request.width = size;
            request.height = size;
            request.applyTint = false;
            request.tint = color;
            request.strokeWidth = strokeWidth;

            const Icons::SvgRasterizeResult raster = m_SvgRasterizer->Rasterize(request);
            if (raster.success && !raster.rgba.empty()) {
                IconTexture texture;
                if (CreateTexture(raster.rgba, raster.width, raster.height, texture)) {
                    m_Cache[key] = texture;
                    return texture.descriptorSet;
                }
            }
        }
    }

    const std::string svgPath = ResolveLucideSvgPath(lucideName);
    if (svgPath.empty()) {
        static std::unordered_set<std::string> s_ReportedMissing;
        if (s_ReportedMissing.insert(lucideName).second) {
            HE_ERROR("[UI] Lucide icon not found: " + lucideName);
        }
        return VK_NULL_HANDLE;
    }

    const std::string key = std::string("mask_") + lucideName + "_" + std::to_string(size) + "_"
        + std::to_string(static_cast<int>(color.r * 255.0f)) + "_"
        + std::to_string(static_cast<int>(color.g * 255.0f)) + "_"
        + std::to_string(static_cast<int>(color.b * 255.0f)) + "_"
        + std::to_string(static_cast<int>(color.a * 255.0f)) + "_"
        + std::to_string(static_cast<int>(strokeWidth * 100.0f));

    auto it = m_Cache.find(key);
    if (it != m_Cache.end()) {
        return it->second.descriptorSet;
    }

    Icons::SvgRasterizeRequest request{};
    request.svgPath = svgPath;
    request.width = size;
    request.height = size;
    request.applyTint = false;
    request.tint = color;
    request.strokeWidth = strokeWidth;

    const Icons::SvgRasterizeResult raster = m_SvgRasterizer->Rasterize(request);
    if (!raster.success || raster.rgba.empty()) return VK_NULL_HANDLE;

    IconTexture texture;
    if (CreateTexture(raster.rgba, raster.width, raster.height, texture)) {
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

        Icons::SvgRasterizeRequest request{};
        request.svgPath = iconName;
        request.width = size;
        request.height = size;
        request.applyTint = false;
        request.tint = m_DefaultColor;

        const Icons::SvgRasterizeResult raster = m_SvgRasterizer->Rasterize(request);
        if (!raster.success || raster.rgba.empty()) return VK_NULL_HANDLE;

        IconTexture texture;
        if (CreateTexture(raster.rgba, raster.width, raster.height, texture)) {
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
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
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

} // namespace WindEffects::Editor::UI
