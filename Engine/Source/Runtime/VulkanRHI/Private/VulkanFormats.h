#pragma once

#include "RHI/Types.h"

#include <volk.h>

namespace we::rhi::vulkan {

[[nodiscard]] inline VkFormat ToVkFormat(Format format) noexcept {
    switch (format) {
    case Format::R8_UNORM: return VK_FORMAT_R8_UNORM;
    case Format::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
    case Format::R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
    case Format::B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
    case Format::B8G8R8A8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
    case Format::R16G16B16A16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case Format::R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case Format::D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
    case Format::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
    case Format::D32_SFLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;
    default: return VK_FORMAT_UNDEFINED;
    }
}

[[nodiscard]] inline Format FromVkFormat(VkFormat format) noexcept {
    switch (format) {
    case VK_FORMAT_R8_UNORM: return Format::R8_UNORM;
    case VK_FORMAT_R8G8B8A8_UNORM: return Format::R8G8B8A8_UNORM;
    case VK_FORMAT_R8G8B8A8_SRGB: return Format::R8G8B8A8_SRGB;
    case VK_FORMAT_B8G8R8A8_UNORM: return Format::B8G8R8A8_UNORM;
    case VK_FORMAT_B8G8R8A8_SRGB: return Format::B8G8R8A8_SRGB;
    case VK_FORMAT_R16G16B16A16_SFLOAT: return Format::R16G16B16A16_SFLOAT;
    case VK_FORMAT_R32G32B32A32_SFLOAT: return Format::R32G32B32A32_SFLOAT;
    case VK_FORMAT_D32_SFLOAT: return Format::D32_SFLOAT;
    case VK_FORMAT_D24_UNORM_S8_UINT: return Format::D24_UNORM_S8_UINT;
    case VK_FORMAT_D32_SFLOAT_S8_UINT: return Format::D32_SFLOAT_S8_UINT;
    default: return Format::Unknown;
    }
}

[[nodiscard]] inline VkBufferUsageFlags ToVkBufferUsage(BufferUsage usage) noexcept {
    VkBufferUsageFlags flags = 0;
    if (HasFlag(usage, BufferUsage::Vertex)) {
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (HasFlag(usage, BufferUsage::Index)) {
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (HasFlag(usage, BufferUsage::Uniform)) {
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (HasFlag(usage, BufferUsage::Storage)) {
        flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (HasFlag(usage, BufferUsage::TransferSrc)) {
        flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (HasFlag(usage, BufferUsage::TransferDst)) {
        flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (HasFlag(usage, BufferUsage::Indirect)) {
        flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }
    return flags;
}

[[nodiscard]] inline VkImageUsageFlags ToVkImageUsage(TextureUsage usage) noexcept {
    VkImageUsageFlags flags = 0;
    if (HasFlag(usage, TextureUsage::Sampled)) {
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (HasFlag(usage, TextureUsage::Storage)) {
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (HasFlag(usage, TextureUsage::ColorAttachment)) {
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (HasFlag(usage, TextureUsage::DepthStencil)) {
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (HasFlag(usage, TextureUsage::TransferSrc)) {
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (HasFlag(usage, TextureUsage::TransferDst)) {
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    return flags;
}

[[nodiscard]] inline VkImageLayout ToVkImageLayout(ResourceState state) noexcept {
    switch (state) {
    case ResourceState::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
    case ResourceState::General: return VK_IMAGE_LAYOUT_GENERAL;
    case ResourceState::ShaderResource: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ResourceState::UnorderedAccess: return VK_IMAGE_LAYOUT_GENERAL;
    case ResourceState::RenderTarget: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case ResourceState::DepthWrite: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case ResourceState::DepthRead: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case ResourceState::CopySrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case ResourceState::CopyDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ResourceState::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default: return VK_IMAGE_LAYOUT_GENERAL;
    }
}

[[nodiscard]] inline bool IsDepthFormat(Format format) noexcept {
    return format == Format::D32_SFLOAT
        || format == Format::D24_UNORM_S8_UINT
        || format == Format::D32_SFLOAT_S8_UINT;
}

} // namespace we::rhi::vulkan
