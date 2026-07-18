#pragma once

#include "DX12Device.h"

#if defined(_WIN32)

namespace we::rhi::dx12 {

[[nodiscard]] constexpr UINT CalcSubresource(UINT mip, UINT arraySlice, UINT mipLevels) noexcept {
    return mip + arraySlice * mipLevels;
}

inline [[nodiscard]] DXGI_FORMAT ToDxgiFormat(Format format) noexcept {
    switch (format) {
    case Format::R8_UNORM: return DXGI_FORMAT_R8_UNORM;
    case Format::R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case Format::R8G8B8A8_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case Format::B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case Format::B8G8R8A8_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case Format::R16G16B16A16_SFLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case Format::R32_SFLOAT: return DXGI_FORMAT_R32_FLOAT;
    case Format::R32G32_SFLOAT: return DXGI_FORMAT_R32G32_FLOAT;
    case Format::R32G32B32_SFLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
    case Format::R32G32B32A32_SFLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case Format::D32_SFLOAT: return DXGI_FORMAT_D32_FLOAT;
    case Format::D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case Format::D32_SFLOAT_S8_UINT: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    default: return DXGI_FORMAT_UNKNOWN;
    }
}

inline [[nodiscard]] bool IsDepthFormat(Format format) noexcept {
    return format == Format::D32_SFLOAT
        || format == Format::D24_UNORM_S8_UINT
        || format == Format::D32_SFLOAT_S8_UINT;
}

inline [[nodiscard]] D3D12_RESOURCE_STATES ToD3DState(ResourceState state) noexcept {
    switch (state) {
    case ResourceState::Undefined: return D3D12_RESOURCE_STATE_COMMON;
    case ResourceState::General: return D3D12_RESOURCE_STATE_COMMON;
    case ResourceState::VertexBuffer: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case ResourceState::IndexBuffer: return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    case ResourceState::UniformBuffer: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case ResourceState::ShaderResource: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case ResourceState::UnorderedAccess: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case ResourceState::RenderTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case ResourceState::DepthWrite: return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case ResourceState::DepthRead: return D3D12_RESOURCE_STATE_DEPTH_READ;
    case ResourceState::CopySrc: return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case ResourceState::CopyDst: return D3D12_RESOURCE_STATE_COPY_DEST;
    case ResourceState::Present: return D3D12_RESOURCE_STATE_PRESENT;
    default: return D3D12_RESOURCE_STATE_COMMON;
    }
}

inline [[nodiscard]] D3D12_PRIMITIVE_TOPOLOGY_TYPE ToTopologyType(PrimitiveTopology topology) noexcept {
    switch (topology) {
    case PrimitiveTopology::PointList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case PrimitiveTopology::LineList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case PrimitiveTopology::TriangleStrip:
    case PrimitiveTopology::TriangleList:
    default: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }
}

inline [[nodiscard]] D3D12_PRIMITIVE_TOPOLOGY ToTopology(PrimitiveTopology topology) noexcept {
    switch (topology) {
    case PrimitiveTopology::PointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case PrimitiveTopology::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case PrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case PrimitiveTopology::TriangleList:
    default: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

inline [[nodiscard]] D3D12_COMPARISON_FUNC ToCompare(CompareOp op) noexcept {
    switch (op) {
    case CompareOp::Never: return D3D12_COMPARISON_FUNC_NEVER;
    case CompareOp::Less: return D3D12_COMPARISON_FUNC_LESS;
    case CompareOp::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
    case CompareOp::LessOrEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case CompareOp::Greater: return D3D12_COMPARISON_FUNC_GREATER;
    case CompareOp::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case CompareOp::GreaterOrEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case CompareOp::Always:
    default: return D3D12_COMPARISON_FUNC_ALWAYS;
    }
}

inline [[nodiscard]] D3D12_STENCIL_OP ToStencilOp(StencilOp op) noexcept {
    switch (op) {
    case StencilOp::Zero: return D3D12_STENCIL_OP_ZERO;
    case StencilOp::Replace: return D3D12_STENCIL_OP_REPLACE;
    case StencilOp::IncrementAndClamp: return D3D12_STENCIL_OP_INCR_SAT;
    case StencilOp::DecrementAndClamp: return D3D12_STENCIL_OP_DECR_SAT;
    case StencilOp::Invert: return D3D12_STENCIL_OP_INVERT;
    case StencilOp::IncrementAndWrap: return D3D12_STENCIL_OP_INCR;
    case StencilOp::DecrementAndWrap: return D3D12_STENCIL_OP_DECR;
    case StencilOp::Keep:
    default: return D3D12_STENCIL_OP_KEEP;
    }
}

inline [[nodiscard]] D3D12_TEXTURE_ADDRESS_MODE ToAddressMode(AddressMode mode) noexcept {
    switch (mode) {
    case AddressMode::MirroredRepeat: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case AddressMode::ClampToEdge: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case AddressMode::ClampToBorder: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case AddressMode::Repeat:
    default: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    }
}

inline [[nodiscard]] D3D12_FILTER ToSamplerFilter(const SamplerDesc& desc) noexcept {
    const bool linearMag = desc.magFilter == Filter::Linear;
    const bool linearMin = desc.minFilter == Filter::Linear;
    const bool linearMip = desc.mipFilter == Filter::Linear;
    if (desc.anisotropy && desc.maxAnisotropy > 1.0f) {
        return D3D12_FILTER_ANISOTROPIC;
    }
    if (linearMin && linearMag && linearMip) {
        return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    }
    if (linearMin && linearMag) {
        return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    }
    if (linearMin) {
        return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    }
    if (linearMag) {
        return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
    }
    return D3D12_FILTER_MIN_MAG_MIP_POINT;
}

inline [[nodiscard]] bool BindingUsesSrvHeap(DescriptorType type) noexcept {
    switch (type) {
    case DescriptorType::UniformBuffer:
    case DescriptorType::StorageBuffer:
    case DescriptorType::UniformBufferDynamic:
    case DescriptorType::StorageBufferDynamic:
    case DescriptorType::SampledImage:
    case DescriptorType::StorageImage:
    case DescriptorType::CombinedImageSampler:
        return true;
    default:
        return false;
    }
}

inline [[nodiscard]] bool BindingUsesSamplerHeap(DescriptorType type) noexcept {
    return type == DescriptorType::Sampler || type == DescriptorType::CombinedImageSampler;
}

inline [[nodiscard]] D3D12_DESCRIPTOR_RANGE_TYPE ToSrvHeapRangeType(DescriptorType type) noexcept {
    switch (type) {
    case DescriptorType::UniformBuffer:
    case DescriptorType::UniformBufferDynamic:
        return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case DescriptorType::StorageBuffer:
    case DescriptorType::StorageBufferDynamic:
    case DescriptorType::StorageImage:
        return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    case DescriptorType::SampledImage:
    case DescriptorType::CombinedImageSampler:
    default:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
}

inline [[nodiscard]] D3D12_BLEND ToBlend(BlendFactor factor) noexcept {
    switch (factor) {
    case BlendFactor::Zero: return D3D12_BLEND_ZERO;
    case BlendFactor::One: return D3D12_BLEND_ONE;
    case BlendFactor::SrcAlpha: return D3D12_BLEND_SRC_ALPHA;
    case BlendFactor::OneMinusSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
    case BlendFactor::DstAlpha: return D3D12_BLEND_DEST_ALPHA;
    case BlendFactor::OneMinusDstAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
    case BlendFactor::SrcColor: return D3D12_BLEND_SRC_COLOR;
    case BlendFactor::OneMinusSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
    default: return D3D12_BLEND_ONE;
    }
}

inline [[nodiscard]] D3D12_BLEND_OP ToBlendOp(BlendOp op) noexcept {
    switch (op) {
    case BlendOp::Subtract: return D3D12_BLEND_OP_SUBTRACT;
    case BlendOp::ReverseSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
    case BlendOp::Min: return D3D12_BLEND_OP_MIN;
    case BlendOp::Max: return D3D12_BLEND_OP_MAX;
    case BlendOp::Add:
    default: return D3D12_BLEND_OP_ADD;
    }
}

inline void ApplyBlendTarget(D3D12_RENDER_TARGET_BLEND_DESC& rt, const BlendStateDesc& blend) noexcept {
    rt.RenderTargetWriteMask = blend.writeMask & 0xF;
    if (!rt.RenderTargetWriteMask) {
        rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    if (blend.enable) {
        rt.BlendEnable = TRUE;
        rt.SrcBlend = ToBlend(blend.srcColor);
        rt.DestBlend = ToBlend(blend.dstColor);
        rt.BlendOp = ToBlendOp(blend.colorOp);
        rt.SrcBlendAlpha = ToBlend(blend.srcAlpha);
        rt.DestBlendAlpha = ToBlend(blend.dstAlpha);
        rt.BlendOpAlpha = ToBlendOp(blend.alphaOp);
    }
}

constexpr UINT kRtvHeapCapacity = 64;
constexpr UINT kDsvHeapCapacity = 16;

} // namespace we::rhi::dx12

#endif