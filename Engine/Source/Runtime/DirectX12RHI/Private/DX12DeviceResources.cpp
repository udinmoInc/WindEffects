#include "DX12Device.h"
#include "DX12Internal.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Platform/NativeHandle.h"
#include "RHI/ShaderBytecode.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace we::rhi::dx12 {

RHIResult<RHIBufferHandle> DX12Device::CreateBuffer(const BufferDesc& desc) {
    if (!m_Device || desc.size == 0) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Buffer size is 0.", "CreateBuffer");
    }

    const bool upload = desc.memory == MemoryUsage::HostVisible || desc.memory == MemoryUsage::HostCached;
    D3D12_HEAP_TYPE heapType = upload ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES initial = upload ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = desc.size;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    DX12Buffer buffer{};
    buffer.desc = desc;
    buffer.state = initial;

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = heapType;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    const HRESULT hr = m_Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        initial,
        nullptr,
        IID_PPV_ARGS(&buffer.resource));
    if (FAILED(hr)) {
        return RHIError::Make(RHIErrorCode::OutOfMemory, "CreateCommittedResource(buffer) failed.", "CreateBuffer", hr);
    }

    if (upload) {
        void* mapped = nullptr;
        if (SUCCEEDED(buffer.resource->Map(0, nullptr, &mapped))) {
            buffer.mapped = mapped;
        }
    }

    const auto handle = static_cast<RHIBufferHandle>(AllocHandle());
    m_Buffers.emplace(static_cast<uint64_t>(handle), std::move(buffer));
    ++m_Diagnostics.resourcesCreated;
    m_Diagnostics.memory.bufferBytes += desc.size;
    m_Diagnostics.memory.allocatedBytes += desc.size;
    return handle;
}

RHIResult<void> DX12Device::DestroyBuffer(RHIBufferHandle handle) {
    if (!FindBuffer(handle)) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown buffer.", "DestroyBuffer");
    }
    EnqueueDeferred(DeferredKind::Buffer, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

void* DX12Device::MapBuffer(RHIBufferHandle handle) {
    auto* buf = FindBuffer(handle);
    if (!buf || !buf->resource) {
        return nullptr;
    }
    if (buf->mapped) {
        return buf->mapped;
    }
    void* mapped = nullptr;
    if (FAILED(buf->resource->Map(0, nullptr, &mapped))) {
        return nullptr;
    }
    buf->mapped = mapped;
    return mapped;
}

void DX12Device::UnmapBuffer(RHIBufferHandle handle) {
    auto* buf = FindBuffer(handle);
    if (!buf || !buf->resource || !buf->mapped) {
        return;
    }
    // Keep upload heaps persistently mapped for HostVisible convenience.
    if (buf->desc.memory == MemoryUsage::HostVisible || buf->desc.memory == MemoryUsage::HostCached) {
        return;
    }
    buf->resource->Unmap(0, nullptr);
    buf->mapped = nullptr;
}

RHIResult<void> DX12Device::UpdateBuffer(RHIBufferHandle handle, std::span<const uint8_t> data, uint64_t offset) {
    auto* buf = FindBuffer(handle);
    if (!buf) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown buffer.", "UpdateBuffer");
    }
    void* mapped = MapBuffer(handle);
    if (!mapped) {
        return RHIError::Make(RHIErrorCode::NotSupported, "Buffer is not host-visible.", "UpdateBuffer");
    }
    if (offset + data.size() > buf->desc.size) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Update exceeds buffer size.", "UpdateBuffer");
    }
    std::memcpy(static_cast<uint8_t*>(mapped) + offset, data.data(), data.size());
    return RHIResult<void>::Success();
}

RHIResult<RHITextureHandle> DX12Device::CreateTexture(const TextureDesc& desc) {
    if (!m_Device || desc.extent.width == 0 || desc.extent.height == 0) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Invalid texture extent.", "CreateTexture");
    }

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Width = desc.extent.width;
    resDesc.Height = desc.extent.height;
    resDesc.DepthOrArraySize = static_cast<UINT16>(desc.arrayLayers ? desc.arrayLayers : 1);
    resDesc.MipLevels = static_cast<UINT16>(desc.mipLevels ? desc.mipLevels : 1);
    resDesc.Format = ToDxgiFormat(desc.format);
    resDesc.SampleDesc.Count = desc.sampleCount ? desc.sampleCount : 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_RESOURCE_STATES initial = D3D12_RESOURCE_STATE_COMMON;
    D3D12_CLEAR_VALUE clearValue{};
    const D3D12_CLEAR_VALUE* pClear = nullptr;

    if (HasFlag(desc.usage, TextureUsage::ColorAttachment)) {
        resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        initial = D3D12_RESOURCE_STATE_RENDER_TARGET;
        clearValue.Format = resDesc.Format;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;
        pClear = &clearValue;
    }
    if (HasFlag(desc.usage, TextureUsage::DepthStencil) || IsDepthFormat(desc.format)) {
        resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        initial = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        clearValue.Format = resDesc.Format;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;
        pClear = &clearValue;
    }
    if (HasFlag(desc.usage, TextureUsage::Storage)) {
        resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    DX12Texture tex{};
    tex.desc = desc;
    tex.d3dState = initial;
    if (HasFlag(desc.usage, TextureUsage::DepthStencil) || IsDepthFormat(desc.format)) {
        tex.state = ResourceState::DepthWrite;
    } else if (HasFlag(desc.usage, TextureUsage::ColorAttachment)) {
        tex.state = ResourceState::RenderTarget;
    } else {
        tex.state = ResourceState::Undefined;
    }

    const HRESULT hr = m_Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        initial,
        pClear,
        IID_PPV_ARGS(&tex.resource));
    if (FAILED(hr)) {
        char buf[256];
        std::snprintf(
            buf,
            sizeof(buf),
            "CreateCommittedResource(texture) failed. hr=0x%08X %ux%u fmt=%u flags=0x%X",
            static_cast<unsigned>(hr),
            desc.extent.width,
            desc.extent.height,
            static_cast<unsigned>(desc.format),
            static_cast<unsigned>(resDesc.Flags));
        return RHIError::Make(RHIErrorCode::OutOfMemory, buf, "CreateTexture", static_cast<int32_t>(hr));
    }

    const auto handle = static_cast<RHITextureHandle>(AllocHandle());
    m_Textures.emplace(static_cast<uint64_t>(handle), std::move(tex));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyTexture(RHITextureHandle handle) {
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

RHIResult<RHITextureViewHandle> DX12Device::CreateTextureView(const TextureViewDesc& desc) {
    auto* tex = FindTexture(desc.texture);
    if (!tex) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture.", "CreateTextureView");
    }
    const auto handle = static_cast<RHITextureViewHandle>(AllocHandle());
    DX12TextureView view{};
    view.desc = desc;
    view.format = ToDxgiFormat(desc.format == Format::Unknown ? tex->desc.format : desc.format);
    m_TextureViews.emplace(static_cast<uint64_t>(handle), std::move(view));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyTextureView(RHITextureViewHandle handle) {
    if (!FindTextureView(handle)) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture view.", "DestroyTextureView");
    }
    EnqueueDeferred(DeferredKind::TextureView, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<void> DX12Device::UpdateTexture(RHITextureHandle handle, const TextureUpdateDesc& update) {
    auto* tex = FindTexture(handle);
    if (!tex || !tex->resource) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture.", "UpdateTexture");
    }
    if (update.data.empty()) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Empty texture update data.", "UpdateTexture");
    }

    const uint32_t width = update.extent.width ? update.extent.width : 1u;
    const uint32_t height = update.extent.height ? update.extent.height : 1u;
    const uint32_t depth = update.extent.depth ? update.extent.depth : 1u;
    const uint32_t srcRowBytes = update.rowPitch
        ? update.rowPitch
        : width * 4u; // R8G8B8A8 (and similar 4-byte formats) used by current callers
    const uint32_t alignedRowPitch = (srcRowBytes + 255u) & ~255u;
    const uint64_t stagingSize = static_cast<uint64_t>(alignedRowPitch) * height * depth;

    BufferDesc stagingDesc{};
    stagingDesc.size = stagingSize;
    stagingDesc.usage = BufferUsage::TransferSrc;
    stagingDesc.memory = MemoryUsage::HostVisible;
    auto staging = CreateBuffer(stagingDesc);
    if (!staging) {
        return staging.error;
    }
    if (void* mapped = MapBuffer(*staging)) {
        auto* dst = static_cast<uint8_t*>(mapped);
        const auto* src = update.data.data();
        const uint64_t srcPlaneBytes = static_cast<uint64_t>(srcRowBytes) * height;
        for (uint32_t z = 0; z < depth; ++z) {
            for (uint32_t y = 0; y < height; ++y) {
                const uint64_t srcOffset = z * srcPlaneBytes + static_cast<uint64_t>(y) * srcRowBytes;
                const uint64_t dstOffset =
                    (static_cast<uint64_t>(z) * height + y) * alignedRowPitch;
                if (srcOffset + srcRowBytes > update.data.size()) {
                    DestroyBuffer(*staging);
                    return RHIError::Make(
                        RHIErrorCode::InvalidArgument, "Texture update data too small.", "UpdateTexture");
                }
                std::memcpy(dst + dstOffset, src + srcOffset, srcRowBytes);
                if (alignedRowPitch > srcRowBytes) {
                    std::memset(dst + dstOffset + srcRowBytes, 0, alignedRowPitch - srcRowBytes);
                }
            }
        }
    }

    // One-shot upload using a temporary list on frame slot 0.
    WaitIdle();
    m_Allocators[0]->Reset();
    m_CmdLists[0]->Reset(m_Allocators[0].Get(), nullptr);
    BindShaderHeaps(m_CmdLists[0].Get());
    DX12CommandList list(this);
    list.Set(m_CmdLists[0].Get());
    list.Begin();
    const ResourceState old = tex->state;
    list.TransitionTexture(handle, old, ResourceState::CopyDst);
    BufferImageCopyRegion region{};
    region.bufferOffset = 0;
    region.bufferRowLength = alignedRowPitch;
    region.image.mipLevel = update.mipLevel;
    region.image.baseLayer = update.arrayLayer;
    region.image.layerCount = 1;
    region.imageOffsetX = update.offsetX;
    region.imageOffsetY = update.offsetY;
    region.imageOffsetZ = update.offsetZ;
    region.imageExtent = {width, height, depth};
    list.CopyBufferToTexture(*staging, handle, region);
    list.TransitionTexture(
        handle,
        ResourceState::CopyDst,
        old == ResourceState::Undefined ? ResourceState::ShaderResource : old);
    list.End();
    m_CmdLists[0]->Close();
    ID3D12CommandList* lists[] = {m_CmdLists[0].Get()};
    m_CommandQueue->ExecuteCommandLists(1, lists);
    WaitIdle();
    DestroyBuffer(*staging);
    return RHIResult<void>::Success();
}

RHIResult<RHISamplerHandle> DX12Device::CreateSampler(const SamplerDesc& desc) {
    const auto handle = static_cast<RHISamplerHandle>(AllocHandle());
    DX12Sampler sampler{};
    sampler.desc = desc;
    sampler.samplerDesc.Filter = ToSamplerFilter(desc);
    sampler.samplerDesc.AddressU = ToAddressMode(desc.addressU);
    sampler.samplerDesc.AddressV = ToAddressMode(desc.addressV);
    sampler.samplerDesc.AddressW = ToAddressMode(desc.addressW);
    sampler.samplerDesc.MaxAnisotropy = desc.anisotropy
        ? static_cast<UINT>((std::max)(1.0f, desc.maxAnisotropy))
        : 1u;
    sampler.samplerDesc.ComparisonFunc = desc.compareEnable
        ? ToCompare(desc.compareOp)
        : D3D12_COMPARISON_FUNC_NEVER;
    sampler.samplerDesc.MinLOD = desc.minLod;
    sampler.samplerDesc.MaxLOD = desc.maxLod;
    m_Samplers.emplace(static_cast<uint64_t>(handle), std::move(sampler));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroySampler(RHISamplerHandle handle) {
    if (m_Samplers.find(static_cast<uint64_t>(handle)) == m_Samplers.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown sampler.", "DestroySampler");
    }
    EnqueueDeferred(DeferredKind::Sampler, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHIShaderHandle> DX12Device::CreateShader(const ShaderDesc& desc) {
    if (desc.bytecode.empty()) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Empty shader bytecode.", "CreateShader");
    }
    const ShaderBytecodeFormat resolved = ShaderBytecodeLoader::ResolveFormat(this, desc.format);
    const ShaderBytecodeFormat detected = ShaderBytecodeLoader::DetectFormat(desc.bytecode);
    if (resolved != ShaderBytecodeFormat::Dxil
        && detected != ShaderBytecodeFormat::Dxil) {
        return RHIError::Make(
            RHIErrorCode::NotSupported,
            "DX12 CreateShader expects DXIL bytecode (.dxil).",
            "CreateShader");
    }
    const auto handle = static_cast<RHIShaderHandle>(AllocHandle());
    DX12Shader shader{};
    shader.desc = desc;
    shader.bytecode.assign(desc.bytecode.begin(), desc.bytecode.end());
    m_Shaders.emplace(static_cast<uint64_t>(handle), std::move(shader));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyShader(RHIShaderHandle handle) {
    if (m_Shaders.find(static_cast<uint64_t>(handle)) == m_Shaders.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown shader.", "DestroyShader");
    }
    EnqueueDeferred(DeferredKind::Shader, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}
} // namespace we::rhi::dx12

#endif
