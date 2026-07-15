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
    case Format::R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
    case Format::R32G32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
    case Format::R32G32B32_SFLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
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
    case VK_FORMAT_R32_SFLOAT: return Format::R32_SFLOAT;
    case VK_FORMAT_R32G32_SFLOAT: return Format::R32G32_SFLOAT;
    case VK_FORMAT_R32G32B32_SFLOAT: return Format::R32G32B32_SFLOAT;
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

[[nodiscard]] inline VkDescriptorType ToVkDescriptorType(DescriptorType type) noexcept {
    switch (type) {
    case DescriptorType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
    case DescriptorType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case DescriptorType::SampledImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case DescriptorType::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case DescriptorType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case DescriptorType::UniformBufferDynamic: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    case DescriptorType::StorageBufferDynamic: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    default: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
}

[[nodiscard]] inline VkShaderStageFlags ToVkShaderStageFlags(ShaderStageFlags stages) noexcept {
    VkShaderStageFlags flags = 0;
    if (HasFlag(stages, ShaderStageFlags::Vertex)) flags |= VK_SHADER_STAGE_VERTEX_BIT;
    if (HasFlag(stages, ShaderStageFlags::Fragment)) flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (HasFlag(stages, ShaderStageFlags::Compute)) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    if (HasFlag(stages, ShaderStageFlags::Geometry)) flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
    if (HasFlag(stages, ShaderStageFlags::TessControl)) flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if (HasFlag(stages, ShaderStageFlags::TessEvaluation)) flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    return flags ? flags : VK_SHADER_STAGE_ALL;
}

[[nodiscard]] inline VkFilter ToVkFilter(Filter filter) noexcept {
    return filter == Filter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
}

[[nodiscard]] inline VkSamplerAddressMode ToVkAddressMode(AddressMode mode) noexcept {
    switch (mode) {
    case AddressMode::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case AddressMode::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case AddressMode::ClampToBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

[[nodiscard]] inline VkPrimitiveTopology ToVkTopology(PrimitiveTopology topo) noexcept {
    switch (topo) {
    case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case PrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case PrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

[[nodiscard]] inline VkCullModeFlags ToVkCullMode(CullMode mode) noexcept {
    switch (mode) {
    case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
    case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
    default: return VK_CULL_MODE_NONE;
    }
}

[[nodiscard]] inline VkCompareOp ToVkCompareOp(CompareOp op) noexcept {
    switch (op) {
    case CompareOp::Never: return VK_COMPARE_OP_NEVER;
    case CompareOp::Less: return VK_COMPARE_OP_LESS;
    case CompareOp::Equal: return VK_COMPARE_OP_EQUAL;
    case CompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOp::Greater: return VK_COMPARE_OP_GREATER;
    case CompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
    case CompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    default: return VK_COMPARE_OP_ALWAYS;
    }
}

[[nodiscard]] inline VkBlendFactor ToVkBlendFactor(BlendFactor f) noexcept {
    switch (f) {
    case BlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
    case BlendFactor::One: return VK_BLEND_FACTOR_ONE;
    case BlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
    case BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case BlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
    case BlendFactor::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case BlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
    case BlendFactor::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    default: return VK_BLEND_FACTOR_ONE;
    }
}

[[nodiscard]] inline VkBlendOp ToVkBlendOp(BlendOp op) noexcept {
    switch (op) {
    case BlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
    case BlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
    case BlendOp::Min: return VK_BLEND_OP_MIN;
    case BlendOp::Max: return VK_BLEND_OP_MAX;
    default: return VK_BLEND_OP_ADD;
    }
}

[[nodiscard]] inline VkAccessFlags AccessForState(ResourceState state) noexcept {
    switch (state) {
    case ResourceState::VertexBuffer: return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    case ResourceState::IndexBuffer: return VK_ACCESS_INDEX_READ_BIT;
    case ResourceState::UniformBuffer: return VK_ACCESS_UNIFORM_READ_BIT;
    case ResourceState::ShaderResource: return VK_ACCESS_SHADER_READ_BIT;
    case ResourceState::UnorderedAccess: return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    case ResourceState::RenderTarget: return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case ResourceState::DepthWrite: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case ResourceState::DepthRead: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    case ResourceState::CopySrc: return VK_ACCESS_TRANSFER_READ_BIT;
    case ResourceState::CopyDst: return VK_ACCESS_TRANSFER_WRITE_BIT;
    case ResourceState::Present: return 0;
    default: return 0;
    }
}

[[nodiscard]] inline VkPipelineStageFlags StageForState(ResourceState state) noexcept {
    switch (state) {
    case ResourceState::VertexBuffer:
    case ResourceState::IndexBuffer:
        return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    case ResourceState::UniformBuffer:
    case ResourceState::ShaderResource:
        return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
            | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    case ResourceState::UnorderedAccess:
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case ResourceState::RenderTarget:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    case ResourceState::DepthWrite:
    case ResourceState::DepthRead:
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    case ResourceState::CopySrc:
    case ResourceState::CopyDst:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
    case ResourceState::Present:
        return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    default:
        return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
}

} // namespace we::rhi::vulkan
