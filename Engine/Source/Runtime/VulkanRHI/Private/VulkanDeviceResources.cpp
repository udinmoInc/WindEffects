#include "VulkanDevice.h"
#include "VulkanFormats.h"
#include "VulkanPlatformSurface.h"
#include "VulkanInternal.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "RHI/ShaderBytecode.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace we::rhi::vulkan {
RHIResult<RHIBufferHandle> VulkanDevice::CreateBuffer(const BufferDesc& desc) {
    if (desc.size == 0) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Buffer size is 0.", "CreateBuffer");
    }

    VulkanBuffer buffer{};
    buffer.desc = desc;
    buffer.hostVisible = desc.memory != MemoryUsage::GpuLocal;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.size;
    bufferInfo.usage = ToVkBufferUsage(desc.usage);
    if (bufferInfo.usage == 0) {
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer.buffer) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::OutOfMemory, "vkCreateBuffer failed.", "CreateBuffer");
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(m_Device, buffer.buffer, &memRequirements);

    VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (buffer.hostVisible) {
        memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, memFlags);
    if (allocInfo.memoryTypeIndex == ~0u) {
        vkDestroyBuffer(m_Device, buffer.buffer, nullptr);
        return RHIError::Make(RHIErrorCode::OutOfMemory, "No suitable memory type.", "CreateBuffer");
    }

    if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &buffer.memory) != VK_SUCCESS) {
        vkDestroyBuffer(m_Device, buffer.buffer, nullptr);
        return RHIError::Make(RHIErrorCode::OutOfMemory, "vkAllocateMemory failed.", "CreateBuffer");
    }
    vkBindBufferMemory(m_Device, buffer.buffer, buffer.memory, 0);

    if (buffer.hostVisible) {
        // Map the full allocation — desc.size may be smaller than memRequirements.size.
        if (vkMapMemory(m_Device, buffer.memory, 0, VK_WHOLE_SIZE, 0, &buffer.mapped) != VK_SUCCESS
            || !buffer.mapped) {
            vkDestroyBuffer(m_Device, buffer.buffer, nullptr);
            vkFreeMemory(m_Device, buffer.memory, nullptr);
            return RHIError::Make(RHIErrorCode::OutOfMemory, "vkMapMemory failed.", "CreateBuffer");
        }
    }

    const auto handle = static_cast<RHIBufferHandle>(AllocHandle());
    m_Buffers.emplace(static_cast<uint64_t>(handle), buffer);
    ++m_Diagnostics.resourcesCreated;
    m_Diagnostics.memory.bufferBytes += desc.size;
    m_Diagnostics.memory.allocatedBytes += desc.size;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyBuffer(RHIBufferHandle handle) {
    if (!FindBuffer(handle)) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown buffer.", "DestroyBuffer");
    }
    EnqueueDeferred(DeferredKind::Buffer, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

void* VulkanDevice::MapBuffer(RHIBufferHandle handle) {
    auto* buf = FindBuffer(handle);
    return buf ? buf->mapped : nullptr;
}

void VulkanDevice::UnmapBuffer(RHIBufferHandle) {}

RHIResult<void> VulkanDevice::UpdateBuffer(RHIBufferHandle handle, std::span<const uint8_t> data, uint64_t offset) {
    auto* buf = FindBuffer(handle);
    if (!buf) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown buffer.", "UpdateBuffer");
    }
    if (!buf->mapped || !buf->hostVisible) {
        return RHIError::Make(RHIErrorCode::NotSupported, "Buffer is not host-visible.", "UpdateBuffer");
    }
    if (offset + data.size() > buf->desc.size) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Update exceeds buffer size.", "UpdateBuffer");
    }
    std::memcpy(static_cast<uint8_t*>(buf->mapped) + offset, data.data(), data.size());
    return RHIResult<void>::Success();
}

RHIResult<RHITextureHandle> VulkanDevice::CreateTexture(const TextureDesc& desc) {
    VulkanTexture tex{};
    tex.desc = desc;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {desc.extent.width, desc.extent.height, desc.extent.depth ? desc.extent.depth : 1};
    imageInfo.mipLevels = desc.mipLevels ? desc.mipLevels : 1;
    imageInfo.arrayLayers = desc.arrayLayers ? desc.arrayLayers : 1;
    imageInfo.format = ToVkFormat(desc.format);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = ToVkImageUsage(desc.usage);
    if (imageInfo.usage == 0) {
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_Device, &imageInfo, nullptr, &tex.image) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::OutOfMemory, "vkCreateImage failed.", "CreateTexture");
    }

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(m_Device, tex.image, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocInfo.memoryTypeIndex == ~0u
        || vkAllocateMemory(m_Device, &allocInfo, nullptr, &tex.memory) != VK_SUCCESS) {
        vkDestroyImage(m_Device, tex.image, nullptr);
        return RHIError::Make(RHIErrorCode::OutOfMemory, "Texture memory allocation failed.", "CreateTexture");
    }
    vkBindImageMemory(m_Device, tex.image, tex.memory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = tex.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageInfo.format;
    viewInfo.subresourceRange.aspectMask = IsDepthFormat(desc.format)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;
    viewInfo.subresourceRange.layerCount = imageInfo.arrayLayers;
    if (vkCreateImageView(m_Device, &viewInfo, nullptr, &tex.view) != VK_SUCCESS) {
        vkDestroyImage(m_Device, tex.image, nullptr);
        vkFreeMemory(m_Device, tex.memory, nullptr);
        return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to create image view.", "CreateTexture");
    }

    const auto handle = static_cast<RHITextureHandle>(AllocHandle());
    m_Textures.emplace(static_cast<uint64_t>(handle), tex);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyTexture(RHITextureHandle handle) {
    auto* tex = FindTexture(handle);
    if (!tex) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture.", "DestroyTexture");
    }
    if (tex->isSwapchain) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Cannot destroy swapchain texture.", "DestroyTexture");
    }
    EnqueueDeferred(DeferredKind::Texture, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHITextureViewHandle> VulkanDevice::CreateTextureView(const TextureViewDesc& desc) {
    auto* tex = FindTexture(desc.texture);
    if (!tex) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture.", "CreateTextureView");
    }
    VulkanTextureView view{};
    view.desc = desc;
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = tex->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = desc.format == Format::Unknown ? ToVkFormat(tex->desc.format) : ToVkFormat(desc.format);
    viewInfo.subresourceRange.aspectMask = IsDepthFormat(tex->desc.format)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = desc.baseMip;
    viewInfo.subresourceRange.levelCount = desc.mipCount;
    viewInfo.subresourceRange.baseArrayLayer = desc.baseLayer;
    viewInfo.subresourceRange.layerCount = desc.layerCount;
    if (vkCreateImageView(m_Device, &viewInfo, nullptr, &view.view) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to create texture view.", "CreateTextureView");
    }
    const auto handle = static_cast<RHITextureViewHandle>(AllocHandle());
    m_TextureViews.emplace(static_cast<uint64_t>(handle), view);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyTextureView(RHITextureViewHandle handle) {
    if (m_TextureViews.find(static_cast<uint64_t>(handle)) == m_TextureViews.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture view.", "DestroyTextureView");
    }
    EnqueueDeferred(DeferredKind::TextureView, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHISamplerHandle> VulkanDevice::CreateSampler(const SamplerDesc& desc) {
    VulkanSampler sampler{};
    sampler.desc = desc;
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = ToVkFilter(desc.magFilter);
    info.minFilter = ToVkFilter(desc.minFilter);
    info.mipmapMode = desc.mipFilter == Filter::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = ToVkAddressMode(desc.addressU);
    info.addressModeV = ToVkAddressMode(desc.addressV);
    info.addressModeW = ToVkAddressMode(desc.addressW);
    info.anisotropyEnable = desc.anisotropy ? VK_TRUE : VK_FALSE;
    info.maxAnisotropy = desc.maxAnisotropy;
    info.compareEnable = desc.compareEnable ? VK_TRUE : VK_FALSE;
    info.compareOp = ToVkCompareOp(desc.compareOp);
    info.minLod = desc.minLod;
    info.maxLod = desc.maxLod;
    if (vkCreateSampler(m_Device, &info, nullptr, &sampler.sampler) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateSampler failed.", "CreateSampler");
    }
    const auto handle = static_cast<RHISamplerHandle>(AllocHandle());
    m_Samplers.emplace(static_cast<uint64_t>(handle), sampler);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroySampler(RHISamplerHandle handle) {
    if (m_Samplers.find(static_cast<uint64_t>(handle)) == m_Samplers.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown sampler.", "DestroySampler");
    }
    EnqueueDeferred(DeferredKind::Sampler, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHIShaderHandle> VulkanDevice::CreateShader(const ShaderDesc& desc) {
    if (desc.bytecode.empty()) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Empty shader bytecode.", "CreateShader");
    }
    ShaderBytecodeFormat format = desc.format;
    if (format == ShaderBytecodeFormat::Auto) {
        format = we::rhi::ShaderBytecodeLoader::DetectFormat(desc.bytecode);
    }
    if (format != ShaderBytecodeFormat::Auto && format != ShaderBytecodeFormat::SpirV) {
        return RHIError::Make(RHIErrorCode::InvalidArgument,
            "Vulkan CreateShader requires SPIR-V bytecode.", "CreateShader");
    }
    if (desc.bytecode.size() % 4 != 0) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "SPIR-V size must be 4-byte aligned.", "CreateShader");
    }
    VulkanShader shader{};
    shader.desc = desc;
    shader.desc.format = ShaderBytecodeFormat::SpirV;
    shader.entryPoint = desc.entryPoint ? desc.entryPoint : "main";
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = desc.bytecode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(desc.bytecode.data());
    if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &shader.module) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateShaderModule failed.", "CreateShader");
    }
    const auto handle = static_cast<RHIShaderHandle>(AllocHandle());
    m_Shaders.emplace(static_cast<uint64_t>(handle), std::move(shader));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyShader(RHIShaderHandle handle) {
    if (m_Shaders.find(static_cast<uint64_t>(handle)) == m_Shaders.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown shader.", "DestroyShader");
    }
    EnqueueDeferred(DeferredKind::Shader, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

} // namespace we::rhi::vulkan
