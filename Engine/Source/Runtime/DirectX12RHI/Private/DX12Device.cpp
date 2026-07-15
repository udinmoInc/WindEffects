#include "DX12Device.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Platform/NativeHandle.h"
#include "RHI/ShaderBytecode.h"

#include <chrono>
#include <cstring>

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace we::rhi::dx12 {
namespace {

[[nodiscard]] constexpr UINT CalcSubresource(UINT mip, UINT arraySlice, UINT mipLevels) noexcept {
    return mip + arraySlice * mipLevels;
}

[[nodiscard]] DXGI_FORMAT ToDxgiFormat(Format format) noexcept {
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

[[nodiscard]] bool IsDepthFormat(Format format) noexcept {
    return format == Format::D32_SFLOAT
        || format == Format::D24_UNORM_S8_UINT
        || format == Format::D32_SFLOAT_S8_UINT;
}

[[nodiscard]] D3D12_RESOURCE_STATES ToD3DState(ResourceState state) noexcept {
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

[[nodiscard]] D3D12_PRIMITIVE_TOPOLOGY_TYPE ToTopologyType(PrimitiveTopology topology) noexcept {
    switch (topology) {
    case PrimitiveTopology::PointList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case PrimitiveTopology::LineList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case PrimitiveTopology::TriangleStrip:
    case PrimitiveTopology::TriangleList:
    default: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }
}

[[nodiscard]] D3D12_PRIMITIVE_TOPOLOGY ToTopology(PrimitiveTopology topology) noexcept {
    switch (topology) {
    case PrimitiveTopology::PointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case PrimitiveTopology::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case PrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case PrimitiveTopology::TriangleList:
    default: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

[[nodiscard]] D3D12_COMPARISON_FUNC ToCompare(CompareOp op) noexcept {
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

[[nodiscard]] D3D12_TEXTURE_ADDRESS_MODE ToAddressMode(AddressMode mode) noexcept {
    switch (mode) {
    case AddressMode::MirroredRepeat: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case AddressMode::ClampToEdge: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case AddressMode::ClampToBorder: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case AddressMode::Repeat:
    default: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    }
}

[[nodiscard]] D3D12_FILTER ToSamplerFilter(const SamplerDesc& desc) noexcept {
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

[[nodiscard]] bool BindingUsesSrvHeap(DescriptorType type) noexcept {
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

[[nodiscard]] bool BindingUsesSamplerHeap(DescriptorType type) noexcept {
    return type == DescriptorType::Sampler || type == DescriptorType::CombinedImageSampler;
}

[[nodiscard]] D3D12_DESCRIPTOR_RANGE_TYPE ToSrvHeapRangeType(DescriptorType type) noexcept {
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

[[nodiscard]] D3D12_BLEND ToBlend(BlendFactor factor) noexcept {
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

[[nodiscard]] D3D12_BLEND_OP ToBlendOp(BlendOp op) noexcept {
    switch (op) {
    case BlendOp::Subtract: return D3D12_BLEND_OP_SUBTRACT;
    case BlendOp::ReverseSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
    case BlendOp::Min: return D3D12_BLEND_OP_MIN;
    case BlendOp::Max: return D3D12_BLEND_OP_MAX;
    case BlendOp::Add:
    default: return D3D12_BLEND_OP_ADD;
    }
}

constexpr UINT kRtvHeapCapacity = 64;
constexpr UINT kDsvHeapCapacity = 16;

} // namespace

// ---------------------------------------------------------------------------
// DX12Queue
// ---------------------------------------------------------------------------

RHIResult<void> DX12Queue::Submit(IRHICommandList& commandList) {
    if (!m_Queue) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Queue is null.", "Submit");
    }
    auto* dxCmd = static_cast<DX12CommandList*>(&commandList);
    ID3D12GraphicsCommandList* list = dxCmd->Get();
    if (!list) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Command list is null.", "Submit");
    }
    commandList.End();
    const HRESULT hr = list->Close();
    if (FAILED(hr)) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "ID3D12GraphicsCommandList::Close failed.", "Submit", hr);
    }
    ID3D12CommandList* lists[] = {list};
    m_Queue->ExecuteCommandLists(1, lists);
    return RHIResult<void>::Success();
}

RHIResult<void> DX12Queue::Submit(const SubmitDesc& desc) {
    if (!desc.commandList) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "SubmitDesc has no command list.", "Submit");
    }
    return Submit(*desc.commandList);
}

RHIResult<void> DX12Queue::WaitIdle() {
    if (!m_Queue) {
        return RHIResult<void>::Success();
    }
    ComPtr<ID3D12Device> device;
    if (FAILED(m_Queue->GetDevice(IID_PPV_ARGS(&device))) || !device) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to get device from queue.", "WaitIdle");
    }
    ComPtr<ID3D12Fence> fence;
    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateFence failed.", "WaitIdle");
    }
    const UINT64 value = 1;
    if (FAILED(m_Queue->Signal(fence.Get(), value))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Queue Signal failed.", "WaitIdle");
    }
    if (fence->GetCompletedValue() < value) {
        HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!event) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "CreateEvent failed.", "WaitIdle");
        }
        fence->SetEventOnCompletion(value, event);
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }
    return RHIResult<void>::Success();
}

// ---------------------------------------------------------------------------
// DX12CommandList
// ---------------------------------------------------------------------------

DX12CommandList::DX12CommandList(DX12Device* device)
    : m_Device(device)
{
}

void DX12CommandList::Begin() {
    m_Recording = true;
}

void DX12CommandList::End() {
    m_Recording = false;
}

void DX12CommandList::BeginRendering(const RenderingInfo& info) {
    if (!m_List || !m_Device) {
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[8]{};
    const ColorAttachmentDesc* attached[8]{};
    UINT rtvCount = 0;
    for (const auto& att : info.colorAttachments) {
        if (rtvCount >= 8) {
            break;
        }
        auto* tex = m_Device->FindTexture(att.texture);
        if (!tex || !tex->resource) {
            continue;
        }
        if (tex->state != ResourceState::RenderTarget) {
            TransitionTexture(att.texture, tex->state, ResourceState::RenderTarget);
        }
        const DXGI_FORMAT format = ToDxgiFormat(tex->desc.format);
        rtvs[rtvCount] = m_Device->AllocateRtv(tex->resource.Get(), format);
        attached[rtvCount] = &att;
        ++rtvCount;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
    const D3D12_CPU_DESCRIPTOR_HANDLE* pDsv = nullptr;
    if (info.depth.enabled) {
        auto* depthTex = m_Device->FindTexture(info.depth.texture);
        if (depthTex && depthTex->resource) {
            if (depthTex->state != ResourceState::DepthWrite) {
                TransitionTexture(info.depth.texture, depthTex->state, ResourceState::DepthWrite);
            }
            dsv = m_Device->AllocateDsv(depthTex->resource.Get(), ToDxgiFormat(depthTex->desc.format));
            pDsv = &dsv;
        }
    }

    m_List->OMSetRenderTargets(rtvCount, rtvCount > 0 ? rtvs : nullptr, FALSE, pDsv);

    for (UINT i = 0; i < rtvCount; ++i) {
        if (attached[i] && attached[i]->loadOp == LoadOp::Clear) {
            const float clear[4] = {
                attached[i]->clearColor.r,
                attached[i]->clearColor.g,
                attached[i]->clearColor.b,
                attached[i]->clearColor.a};
            m_List->ClearRenderTargetView(rtvs[i], clear, 0, nullptr);
        }
    }

    if (pDsv && info.depth.loadOp == LoadOp::Clear) {
        m_List->ClearDepthStencilView(
            dsv,
            D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
            info.depth.clearDepth,
            static_cast<UINT8>(info.depth.clearStencil),
            0,
            nullptr);
    }

    if (info.renderArea.width > 0 && info.renderArea.height > 0) {
        Viewport vp{};
        vp.width = static_cast<float>(info.renderArea.width);
        vp.height = static_cast<float>(info.renderArea.height);
        vp.maxDepth = 1.0f;
        SetViewport(vp);
        Scissor sc{};
        sc.width = info.renderArea.width;
        sc.height = info.renderArea.height;
        SetScissor(sc);
    }
}

void DX12CommandList::EndRendering() {}

void DX12CommandList::SetViewport(const Viewport& viewport) {
    if (!m_List) {
        return;
    }
    D3D12_VIEWPORT vp{};
    vp.TopLeftX = viewport.x;
    vp.TopLeftY = viewport.y;
    vp.Width = viewport.width;
    vp.Height = viewport.height;
    vp.MinDepth = viewport.minDepth;
    vp.MaxDepth = viewport.maxDepth;
    m_List->RSSetViewports(1, &vp);
}

void DX12CommandList::SetScissor(const Scissor& scissor) {
    if (!m_List) {
        return;
    }
    D3D12_RECT rect{};
    rect.left = scissor.x;
    rect.top = scissor.y;
    rect.right = scissor.x + static_cast<LONG>(scissor.width);
    rect.bottom = scissor.y + static_cast<LONG>(scissor.height);
    m_List->RSSetScissorRects(1, &rect);
}

void DX12CommandList::BindGraphicsPipeline(RHIGraphicsPipelineHandle pipeline) {
    if (!m_List || !m_Device) {
        return;
    }
    auto* gfx = m_Device->FindGraphicsPipeline(pipeline);
    if (!gfx) {
        return;
    }
    m_BoundGfx = pipeline;
    if (gfx->rootSignature) {
        m_List->SetGraphicsRootSignature(gfx->rootSignature.Get());
    }
    if (gfx->pso) {
        m_List->SetPipelineState(gfx->pso.Get());
    }
    m_List->IASetPrimitiveTopology(ToTopology(gfx->desc.topology));
}

void DX12CommandList::BindComputePipeline(RHIComputePipelineHandle pipeline) {
    if (!m_List || !m_Device) {
        return;
    }
    auto* compute = m_Device->FindComputePipeline(pipeline);
    if (!compute) {
        return;
    }
    if (compute->rootSignature) {
        m_List->SetComputeRootSignature(compute->rootSignature.Get());
    }
    if (compute->pso) {
        m_List->SetPipelineState(compute->pso.Get());
    }
}

void DX12CommandList::BindVertexBuffer(uint32_t binding, RHIBufferHandle buffer, uint64_t offset) {
    if (!m_List || !m_Device) {
        return;
    }
    auto* buf = m_Device->FindBuffer(buffer);
    if (!buf || !buf->resource) {
        return;
    }
    UINT stride = 0;
    if (auto* gfx = m_Device->FindGraphicsPipeline(m_BoundGfx)) {
        for (const auto& vb : gfx->desc.vertexBindings) {
            if (vb.binding == binding) {
                stride = vb.stride;
                break;
            }
        }
    }
    if (stride == 0) {
        stride = 12;
    }
    D3D12_VERTEX_BUFFER_VIEW view{};
    view.BufferLocation = buf->resource->GetGPUVirtualAddress() + offset;
    view.SizeInBytes = static_cast<UINT>(buf->desc.size > offset ? buf->desc.size - offset : 0);
    view.StrideInBytes = stride;
    m_List->IASetVertexBuffers(binding, 1, &view);
}

void DX12CommandList::BindIndexBuffer(RHIBufferHandle buffer, uint64_t offset, IndexType type) {
    if (!m_List || !m_Device) {
        return;
    }
    auto* buf = m_Device->FindBuffer(buffer);
    if (!buf || !buf->resource) {
        return;
    }
    D3D12_INDEX_BUFFER_VIEW view{};
    view.BufferLocation = buf->resource->GetGPUVirtualAddress() + offset;
    view.SizeInBytes = static_cast<UINT>(buf->desc.size > offset ? buf->desc.size - offset : 0);
    view.Format = type == IndexType::UInt16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    m_List->IASetIndexBuffer(&view);
}

void DX12CommandList::BindDescriptorSets(
    PipelineBindPoint bindPoint,
    RHIPipelineLayoutHandle layoutHandle,
    uint32_t firstSet,
    std::span<const RHIDescriptorSetHandle> sets)
{
    if (!m_List || !m_Device || sets.empty()) {
        return;
    }
    auto* layout = m_Device->FindPipelineLayout(layoutHandle);
    if (!layout || !layout->rootSignature) {
        return;
    }
    m_Device->BindShaderHeaps(m_List);
    for (size_t i = 0; i < sets.size(); ++i) {
        const uint32_t setIndex = firstSet + static_cast<uint32_t>(i);
        if (setIndex >= layout->setRoots.size()) {
            break;
        }
        auto* set = m_Device->FindDescriptorSet(sets[i]);
        if (!set) {
            continue;
        }
        const auto& mapping = layout->setRoots[setIndex];
        if (mapping.srvUavCbvRootParam != UINT32_MAX && set->srvUavCbvCount > 0) {
            const auto gpu = m_Device->SrvGpu(set->srvUavCbvHeapOffset);
            if (bindPoint == PipelineBindPoint::Compute) {
                m_List->SetComputeRootDescriptorTable(mapping.srvUavCbvRootParam, gpu);
            } else {
                m_List->SetGraphicsRootDescriptorTable(mapping.srvUavCbvRootParam, gpu);
            }
        }
        if (mapping.samplerRootParam != UINT32_MAX && set->samplerCount > 0) {
            const auto gpu = m_Device->SamplerGpu(set->samplerHeapOffset);
            if (bindPoint == PipelineBindPoint::Compute) {
                m_List->SetComputeRootDescriptorTable(mapping.samplerRootParam, gpu);
            } else {
                m_List->SetGraphicsRootDescriptorTable(mapping.samplerRootParam, gpu);
            }
        }
    }
}

void DX12CommandList::PushConstants(
    RHIPipelineLayoutHandle layoutHandle,
    ShaderStageFlags,
    uint32_t offset,
    std::span<const uint8_t> data)
{
    if (!m_List || !m_Device || data.empty()) {
        return;
    }
    auto* layout = m_Device->FindPipelineLayout(layoutHandle);
    if (!layout || layout->pushConstantRootParam == UINT32_MAX) {
        return;
    }
    const uint32_t dwordOffset = offset / 4u;
    const uint32_t dwordCount = static_cast<uint32_t>((data.size() + 3u) / 4u);
    if (dwordCount == 0 || dwordOffset + dwordCount > layout->pushConstantDwords) {
        return;
    }
    // Pad to 4-byte alignment for SetGraphicsRoot32BitConstants.
    alignas(4) uint8_t scratch[256];
    const size_t copySize = (std::min)(data.size(), sizeof(scratch));
    std::memcpy(scratch, data.data(), copySize);
    if (copySize < dwordCount * 4u) {
        std::memset(scratch + copySize, 0, dwordCount * 4u - copySize);
    }
    m_List->SetGraphicsRoot32BitConstants(
        layout->pushConstantRootParam,
        dwordCount,
        scratch,
        dwordOffset);
}

void DX12CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    if (!m_List) {
        return;
    }
    m_List->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void DX12CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    if (!m_List) {
        return;
    }
    m_List->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void DX12CommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    if (!m_List) {
        return;
    }
    m_List->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void DX12CommandList::CopyBuffer(RHIBufferHandle src, RHIBufferHandle dst, uint64_t size, uint64_t srcOffset, uint64_t dstOffset) {
    if (!m_List || !m_Device) {
        return;
    }
    auto* srcBuf = m_Device->FindBuffer(src);
    auto* dstBuf = m_Device->FindBuffer(dst);
    if (!srcBuf || !dstBuf || !srcBuf->resource || !dstBuf->resource) {
        return;
    }
    m_List->CopyBufferRegion(dstBuf->resource.Get(), dstOffset, srcBuf->resource.Get(), srcOffset, size);
}

void DX12CommandList::CopyTexture(RHITextureHandle src, RHITextureHandle dst, const TextureCopyRegion& region) {
    if (!m_List || !m_Device) {
        return;
    }
    auto* srcTex = m_Device->FindTexture(src);
    auto* dstTex = m_Device->FindTexture(dst);
    if (!srcTex || !dstTex || !srcTex->resource || !dstTex->resource) {
        return;
    }
    D3D12_TEXTURE_COPY_LOCATION srcLoc{};
    srcLoc.pResource = srcTex->resource.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLoc.SubresourceIndex = CalcSubresource(region.src.mipLevel, region.src.baseLayer, srcTex->desc.mipLevels);

    D3D12_TEXTURE_COPY_LOCATION dstLoc{};
    dstLoc.pResource = dstTex->resource.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = CalcSubresource(region.dst.mipLevel, region.dst.baseLayer, dstTex->desc.mipLevels);

    D3D12_BOX box{};
    box.left = region.srcOffsetX;
    box.top = region.srcOffsetY;
    box.front = region.srcOffsetZ;
    box.right = region.srcOffsetX + region.extent.width;
    box.bottom = region.srcOffsetY + region.extent.height;
    box.back = region.srcOffsetZ + region.extent.depth;

    m_List->CopyTextureRegion(&dstLoc, region.dstOffsetX, region.dstOffsetY, region.dstOffsetZ, &srcLoc, &box);
}

void DX12CommandList::BlitTexture(RHITextureHandle src, RHITextureHandle dst, const TextureBlitRegion& region, Filter) {
    TextureCopyRegion copy{};
    copy.src = region.src;
    copy.dst = region.dst;
    copy.srcOffsetX = static_cast<uint32_t>(region.srcOffset0X);
    copy.srcOffsetY = static_cast<uint32_t>(region.srcOffset0Y);
    copy.srcOffsetZ = static_cast<uint32_t>(region.srcOffset0Z);
    copy.dstOffsetX = static_cast<uint32_t>(region.dstOffset0X);
    copy.dstOffsetY = static_cast<uint32_t>(region.dstOffset0Y);
    copy.dstOffsetZ = static_cast<uint32_t>(region.dstOffset0Z);
    const uint32_t w = static_cast<uint32_t>(region.srcOffset1X - region.srcOffset0X);
    const uint32_t h = static_cast<uint32_t>(region.srcOffset1Y - region.srcOffset0Y);
    const uint32_t d = static_cast<uint32_t>(region.srcOffset1Z - region.srcOffset0Z);
    copy.extent = {w ? w : 1u, h ? h : 1u, d ? d : 1u};
    CopyTexture(src, dst, copy);
}

void DX12CommandList::CopyBufferToTexture(RHIBufferHandle src, RHITextureHandle dst, const BufferImageCopyRegion& region) {
    if (!m_List || !m_Device) {
        return;
    }
    auto* srcBuf = m_Device->FindBuffer(src);
    auto* dstTex = m_Device->FindTexture(dst);
    if (!srcBuf || !dstTex || !srcBuf->resource || !dstTex->resource) {
        return;
    }
    D3D12_TEXTURE_COPY_LOCATION dstLoc{};
    dstLoc.pResource = dstTex->resource.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = CalcSubresource(region.image.mipLevel, region.image.baseLayer, dstTex->desc.mipLevels);

    D3D12_TEXTURE_COPY_LOCATION srcLoc{};
    srcLoc.pResource = srcBuf->resource.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.PlacedFootprint.Offset = region.bufferOffset;
    srcLoc.PlacedFootprint.Footprint.Format = ToDxgiFormat(dstTex->desc.format);
    srcLoc.PlacedFootprint.Footprint.Width = region.imageExtent.width;
    srcLoc.PlacedFootprint.Footprint.Height = region.imageExtent.height;
    srcLoc.PlacedFootprint.Footprint.Depth = region.imageExtent.depth;
    const UINT rowPitch = region.bufferRowLength
        ? region.bufferRowLength
        : region.imageExtent.width * 4u;
    srcLoc.PlacedFootprint.Footprint.RowPitch = (rowPitch + 255u) & ~255u;

    m_List->CopyTextureRegion(&dstLoc, region.imageOffsetX, region.imageOffsetY, region.imageOffsetZ, &srcLoc, nullptr);
}

void DX12CommandList::CopyTextureToBuffer(RHITextureHandle src, RHIBufferHandle dst, const BufferImageCopyRegion& region) {
    if (!m_List || !m_Device) {
        return;
    }
    auto* srcTex = m_Device->FindTexture(src);
    auto* dstBuf = m_Device->FindBuffer(dst);
    if (!srcTex || !dstBuf || !srcTex->resource || !dstBuf->resource) {
        return;
    }
    D3D12_TEXTURE_COPY_LOCATION srcLoc{};
    srcLoc.pResource = srcTex->resource.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLoc.SubresourceIndex = CalcSubresource(region.image.mipLevel, region.image.baseLayer, srcTex->desc.mipLevels);

    D3D12_TEXTURE_COPY_LOCATION dstLoc{};
    dstLoc.pResource = dstBuf->resource.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLoc.PlacedFootprint.Offset = region.bufferOffset;
    dstLoc.PlacedFootprint.Footprint.Format = ToDxgiFormat(srcTex->desc.format);
    dstLoc.PlacedFootprint.Footprint.Width = region.imageExtent.width;
    dstLoc.PlacedFootprint.Footprint.Height = region.imageExtent.height;
    dstLoc.PlacedFootprint.Footprint.Depth = region.imageExtent.depth;
    const UINT rowPitch = region.bufferRowLength
        ? region.bufferRowLength
        : region.imageExtent.width * 4u;
    dstLoc.PlacedFootprint.Footprint.RowPitch = (rowPitch + 255u) & ~255u;

    D3D12_BOX box{};
    box.left = region.imageOffsetX;
    box.top = region.imageOffsetY;
    box.front = region.imageOffsetZ;
    box.right = region.imageOffsetX + region.imageExtent.width;
    box.bottom = region.imageOffsetY + region.imageExtent.height;
    box.back = region.imageOffsetZ + region.imageExtent.depth;
    m_List->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, &box);
}

void DX12CommandList::TransitionTexture(RHITextureHandle texture, ResourceState before, ResourceState after) {
    if (!m_List || !m_Device || before == after) {
        return;
    }
    auto* tex = m_Device->FindTexture(texture);
    if (!tex || !tex->resource) {
        return;
    }
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = tex->resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = ToD3DState(before);
    barrier.Transition.StateAfter = ToD3DState(after);
    m_List->ResourceBarrier(1, &barrier);
    tex->state = after;
    tex->d3dState = barrier.Transition.StateAfter;
}

void DX12CommandList::ResourceBarrier(std::span<const ResourceBarrierDesc> barriers) {
    if (!m_List || !m_Device || barriers.empty()) {
        return;
    }
    std::vector<D3D12_RESOURCE_BARRIER> d3dBarriers;
    d3dBarriers.reserve(barriers.size());
    for (const auto& b : barriers) {
        if (b.isTexture) {
            auto* tex = m_Device->FindTexture(b.texture.texture);
            if (!tex || !tex->resource || b.texture.before == b.texture.after) {
                continue;
            }
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = tex->resource.Get();
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = ToD3DState(b.texture.before);
            barrier.Transition.StateAfter = ToD3DState(b.texture.after);
            d3dBarriers.push_back(barrier);
            tex->state = b.texture.after;
            tex->d3dState = barrier.Transition.StateAfter;
        } else {
            auto* buf = m_Device->FindBuffer(b.buffer.buffer);
            if (!buf || !buf->resource || b.buffer.before == b.buffer.after) {
                continue;
            }
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = buf->resource.Get();
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = ToD3DState(b.buffer.before);
            barrier.Transition.StateAfter = ToD3DState(b.buffer.after);
            d3dBarriers.push_back(barrier);
            buf->state = barrier.Transition.StateAfter;
        }
    }
    if (!d3dBarriers.empty()) {
        m_List->ResourceBarrier(static_cast<UINT>(d3dBarriers.size()), d3dBarriers.data());
    }
}

void DX12CommandList::PushDebugGroup(std::string_view) {}
void DX12CommandList::PopDebugGroup() {}
void DX12CommandList::InsertDebugMarker(std::string_view) {}

// ---------------------------------------------------------------------------
// DX12Swapchain
// ---------------------------------------------------------------------------

DX12Swapchain::DX12Swapchain(DX12Device* device)
    : m_Device(device)
{
}

DX12Swapchain::~DX12Swapchain() {
    Destroy();
}

RHIResult<void> DX12Swapchain::Create(const DeviceDesc& desc) {
    if (!m_Device || !m_Device->GetD3DDevice()) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Device not ready.", "Swapchain::Create");
    }
    if (desc.window.type != we::platform::NativeWindowType::Win32Hwnd || !desc.window.window) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Expected Win32Hwnd for DXGI swapchain.", "Swapchain::Create");
    }

    const HWND hwnd = static_cast<HWND>(desc.window.window);
    RECT client{};
    GetClientRect(hwnd, &client);
    m_Extent.width = client.right > client.left ? static_cast<uint32_t>(client.right - client.left) : 1280u;
    m_Extent.height = client.bottom > client.top ? static_cast<uint32_t>(client.bottom - client.top) : 720u;
    if (m_Extent.width == 0) {
        m_Extent.width = 1;
    }
    if (m_Extent.height == 0) {
        m_Extent.height = 1;
    }
    m_Format = Format::B8G8R8A8_UNORM;

    // Prefer the device factory; fall back to a local one if needed.
    ComPtr<IDXGIFactory4> factory = m_Device->GetFactory();
    if (!factory) {
        UINT flags = 0;
        if (desc.enableValidation) {
            flags |= DXGI_CREATE_FACTORY_DEBUG;
        }
        if (FAILED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)))) {
            if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
                return RHIError::Make(RHIErrorCode::BackendFailure, "CreateDXGIFactory2 failed.", "Swapchain::Create");
            }
        }
    }

    DXGI_SWAP_CHAIN_DESC1 scDesc{};
    scDesc.Width = m_Extent.width;
    scDesc.Height = m_Extent.height;
    scDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scDesc.SampleDesc.Count = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.Scaling = DXGI_SCALING_STRETCH;
    scDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    ComPtr<IDXGISwapChain1> swap1;
    auto& q = m_Device->GetQueue(QueueType::Graphics);
    ID3D12CommandQueue* queue = static_cast<DX12Queue&>(q).Get();
    if (!queue) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Graphics queue is null.", "Swapchain::Create");
    }

    const HRESULT hr = factory->CreateSwapChainForHwnd(
        queue,
        hwnd,
        &scDesc,
        nullptr,
        nullptr,
        &swap1);
    if (FAILED(hr) || !swap1) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateSwapChainForHwnd failed.", "Swapchain::Create", hr);
    }
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    if (FAILED(swap1.As(&m_Swap)) || !m_Swap) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "QueryInterface IDXGISwapChain3 failed.", "Swapchain::Create");
    }

    m_Handles.clear();
    m_Handles.resize(scDesc.BufferCount);
    for (UINT i = 0; i < scDesc.BufferCount; ++i) {
        ComPtr<ID3D12Resource> buffer;
        if (FAILED(m_Swap->GetBuffer(i, IID_PPV_ARGS(&buffer)))) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "GetBuffer failed.", "Swapchain::Create");
        }
        m_Handles[i] = m_Device->RegisterSwapchainTexture(buffer, m_Extent, m_Format);
    }
    m_Index = m_Swap->GetCurrentBackBufferIndex();
    return RHIResult<void>::Success();
}

void DX12Swapchain::Destroy() {
    if (m_Device && !m_Handles.empty()) {
        m_Device->ClearSwapchainTextures(m_Handles);
    }
    m_Handles.clear();
    m_Swap.Reset();
}

RHITextureHandle DX12Swapchain::GetCurrentImage() const {
    if (m_Handles.empty()) {
        return RHITextureHandle::Invalid;
    }
    return m_Handles[m_Index % m_Handles.size()];
}

RHIResult<void> DX12Swapchain::Resize(Extent2D extent) {
    if (!m_Swap || !m_Device) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Swapchain not created.", "Resize");
    }
    m_Device->WaitIdle();
    m_Device->ClearSwapchainTextures(m_Handles);
    m_Handles.clear();

    m_Extent.width = extent.width ? extent.width : 1;
    m_Extent.height = extent.height ? extent.height : 1;

    const HRESULT hr = m_Swap->ResizeBuffers(2, m_Extent.width, m_Extent.height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    if (FAILED(hr)) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "ResizeBuffers failed.", "Resize", hr);
    }

    m_Handles.resize(2);
    for (UINT i = 0; i < 2; ++i) {
        ComPtr<ID3D12Resource> buffer;
        if (FAILED(m_Swap->GetBuffer(i, IID_PPV_ARGS(&buffer)))) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "GetBuffer failed after resize.", "Resize");
        }
        m_Handles[i] = m_Device->RegisterSwapchainTexture(buffer, m_Extent, m_Format);
    }
    m_Index = m_Swap->GetCurrentBackBufferIndex();
    return RHIResult<void>::Success();
}

RHIResult<uint32_t> DX12Swapchain::AcquireNextImage() {
    if (!m_Swap) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Swapchain not created.", "AcquireNextImage");
    }
    m_Index = m_Swap->GetCurrentBackBufferIndex();
    return m_Index;
}

RHIResult<void> DX12Swapchain::Present() {
    if (!m_Swap) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Swapchain not created.", "Present");
    }
    const HRESULT hr = m_Swap->Present(1, 0);
    if (FAILED(hr)) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Present failed.", "Present", hr);
    }
    return RHIResult<void>::Success();
}

// ---------------------------------------------------------------------------
// DX12Device
// ---------------------------------------------------------------------------

DX12Device::DX12Device(const DeviceDesc& desc)
    : m_Desc(desc)
    , m_FramesInFlight(desc.framesInFlight ? desc.framesInFlight : 2u)
    , m_FrameCmd(this)
{
    if (m_FramesInFlight > 3) {
        m_FramesInFlight = 3;
    }
    if (m_FramesInFlight < 1) {
        m_FramesInFlight = 1;
    }

    const auto start = std::chrono::steady_clock::now();
    if (auto r = CreateDeviceAndQueue(); !r) {
        WE_LOG_ERROR(we::LogCategory::Startup, r.error.message);
        return;
    }

    m_Caps.dynamicRendering = true;
    m_Caps.timestamps = true;
    m_Caps.geometryShaders = true;
    m_Caps.tessellation = true;
    m_Caps.multiDrawIndirect = true;
    m_Caps.samplerAnisotropy = true;
    m_Caps.fillModeNonSolid = true;
    m_Caps.depthClamp = true;
    m_Caps.independentBlend = true;
    m_Caps.shaderFloat64 = false;
    m_Caps.shaderInt64 = true;
    m_Caps.drawIndirectFirstInstance = true;
    m_Caps.textureCompressionBC = true;
    m_Caps.timelineSemaphores = false;
    m_Caps.asyncCompute = false;
    m_Caps.bindlessResources = false;
    m_Caps.debugMarkers = true;
    m_Caps.multipleWindows = false;
    m_Caps.maxSamplerAnisotropy = 16.0f;
    m_Caps.maxColorAttachments = 8;
    m_Caps.maxPushConstantBytes = 256;
    m_Caps.maxBoundDescriptorSets = 4;
    m_Caps.maxDescriptorSetSamplers = 16;
    m_Caps.maxDescriptorSetUniformBuffers = 14;
    m_Caps.maxDescriptorSetStorageBuffers = 64;
    m_Caps.maxDescriptorSetSampledImages = 128;
    m_Caps.maxDescriptorSetStorageImages = 8;
    m_Caps.maxComputeWorkGroupInvocations = 1024;
    m_Caps.maxComputeWorkGroupSizeX = 1024;
    m_Caps.maxComputeWorkGroupSizeY = 1024;
    m_Caps.maxComputeWorkGroupSizeZ = 64;
    m_Caps.maxTextureDimension2D = 16384;
    m_Caps.minUniformBufferOffsetAlignment = 256;
    m_Caps.minStorageBufferOffsetAlignment = 16;
    m_Caps.maxFramesInFlight = m_FramesInFlight;

    if (we::platform::IsValid(desc.window) && !desc.headless
        && desc.window.type == we::platform::NativeWindowType::Win32Hwnd) {
        m_Swapchain = std::make_unique<DX12Swapchain>(this);
        if (auto r = m_Swapchain->Create(desc); !r) {
            WE_LOG_ERROR(we::LogCategory::Startup, r.error.message);
            m_Swapchain.reset();
            return;
        }
    }

    const auto end = std::chrono::steady_clock::now();
    m_Diagnostics.deviceCreateMs = std::chrono::duration<double, std::milli>(end - start).count();
    m_Valid = true;
    WE_LOG_INFO(we::LogCategory::Startup, "DirectX12RHI device created.");
}

DX12Device::~DX12Device() {
    WaitIdle();
    m_Swapchain.reset();
    m_Buffers.clear();
    m_Textures.clear();
    m_TextureViews.clear();
    m_Samplers.clear();
    m_Shaders.clear();
    m_SetLayouts.clear();
    m_Pools.clear();
    m_Sets.clear();
    m_Layouts.clear();
    m_GfxPipelines.clear();
    m_ComputePipelines.clear();
    m_Fences.clear();
    m_Semaphores.clear();
    for (uint32_t i = 0; i < 3; ++i) {
        m_CmdLists[i].Reset();
        m_Allocators[i].Reset();
    }
    m_RtvHeap.Reset();
    m_DsvHeap.Reset();
    m_FrameFence.Reset();
    m_CommandQueue.Reset();
    m_Device.Reset();
    m_Factory.Reset();
    if (m_FenceEvent) {
        CloseHandle(m_FenceEvent);
        m_FenceEvent = nullptr;
    }
}

RHIResult<void> DX12Device::CreateDeviceAndQueue() {
    UINT factoryFlags = 0;
    if (m_Desc.enableValidation) {
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
            debug->EnableDebugLayer();
            factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }

    if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_Factory)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateDXGIFactory2 failed.", "CreateDeviceAndQueue");
    }

    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIAdapter1> chosen;
    for (UINT i = 0; m_Factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc{};
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            adapter.Reset();
            continue;
        }
        if (m_Desc.adapterIndex == i || !chosen) {
            chosen = adapter;
        }
        if (m_Desc.adapterIndex == i) {
            break;
        }
        adapter.Reset();
    }
    if (!chosen) {
        if (FAILED(m_Factory->EnumAdapters1(0, &chosen)) || !chosen) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "No DXGI adapter found.", "CreateDeviceAndQueue");
        }
    }

    constexpr D3D_FEATURE_LEVEL kLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    HRESULT hrDevice = E_FAIL;
    for (D3D_FEATURE_LEVEL level : kLevels) {
        hrDevice = D3D12CreateDevice(chosen.Get(), level, IID_PPV_ARGS(&m_Device));
        if (SUCCEEDED(hrDevice)) {
            break;
        }
        m_Device.Reset();
    }
    if (FAILED(hrDevice) || !m_Device) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "D3D12CreateDevice failed (11_0+).", "CreateDeviceAndQueue", hrDevice);
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (FAILED(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateCommandQueue failed.", "CreateDeviceAndQueue");
    }
    m_GraphicsQueue.Set(m_CommandQueue.Get());
    m_ComputeQueue.Set(m_CommandQueue.Get());
    m_TransferQueue.Set(m_CommandQueue.Get());

    for (uint32_t i = 0; i < m_FramesInFlight; ++i) {
        if (FAILED(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_Allocators[i])))) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "CreateCommandAllocator failed.", "CreateDeviceAndQueue");
        }
        if (FAILED(m_Device->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                m_Allocators[i].Get(),
                nullptr,
                IID_PPV_ARGS(&m_CmdLists[i])))) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "CreateCommandList failed.", "CreateDeviceAndQueue");
        }
        m_CmdLists[i]->Close();
    }

    if (FAILED(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_FrameFence)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateFence failed.", "CreateDeviceAndQueue");
    }
    m_FenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!m_FenceEvent) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateEvent failed.", "CreateDeviceAndQueue");
    }

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = kRtvHeapCapacity;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateDescriptorHeap(RTV) failed.", "CreateDeviceAndQueue");
    }
    m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.NumDescriptors = kDsvHeapCapacity;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(m_Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DsvHeap)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateDescriptorHeap(DSV) failed.", "CreateDeviceAndQueue");
    }
    m_DsvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    if (auto heaps = CreateShaderVisibleHeaps(); !heaps) {
        return heaps;
    }

    return RHIResult<void>::Success();
}

RHIResult<void> DX12Device::CreateShaderVisibleHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = kSrvHeapCapacity;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(m_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvHeap)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateDescriptorHeap(SRV) failed.", "CreateShaderVisibleHeaps");
    }
    m_SrvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc{};
    samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerHeapDesc.NumDescriptors = kSamplerHeapCapacity;
    samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(m_Device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_SamplerHeap)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateDescriptorHeap(Sampler) failed.", "CreateShaderVisibleHeaps");
    }
    m_SamplerDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_SrvCursor = 0;
    m_SamplerCursor = 0;
    return RHIResult<void>::Success();
}

void DX12Device::BindShaderHeaps(ID3D12GraphicsCommandList* list) {
    if (!list || !m_SrvHeap || !m_SamplerHeap) {
        return;
    }
    ID3D12DescriptorHeap* heaps[] = {m_SrvHeap.Get(), m_SamplerHeap.Get()};
    list->SetDescriptorHeaps(2, heaps);
}

bool DX12Device::AllocateShaderVisibleSlots(
    uint32_t srvCount,
    uint32_t samplerCount,
    uint32_t& outSrvOffset,
    uint32_t& outSamplerOffset)
{
    if (m_SrvCursor + srvCount > kSrvHeapCapacity || m_SamplerCursor + samplerCount > kSamplerHeapCapacity) {
        return false;
    }
    outSrvOffset = m_SrvCursor;
    outSamplerOffset = m_SamplerCursor;
    m_SrvCursor += srvCount;
    m_SamplerCursor += samplerCount;
    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Device::SrvCpu(uint32_t offset) const {
    D3D12_CPU_DESCRIPTOR_HANDLE h = m_SrvHeap->GetCPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<SIZE_T>(offset) * m_SrvDescriptorSize;
    return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12Device::SrvGpu(uint32_t offset) const {
    D3D12_GPU_DESCRIPTOR_HANDLE h = m_SrvHeap->GetGPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<UINT64>(offset) * m_SrvDescriptorSize;
    return h;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Device::SamplerCpu(uint32_t offset) const {
    D3D12_CPU_DESCRIPTOR_HANDLE h = m_SamplerHeap->GetCPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<SIZE_T>(offset) * m_SamplerDescriptorSize;
    return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12Device::SamplerGpu(uint32_t offset) const {
    D3D12_GPU_DESCRIPTOR_HANDLE h = m_SamplerHeap->GetGPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<UINT64>(offset) * m_SamplerDescriptorSize;
    return h;
}

uint64_t DX12Device::AllocHandle() {
    std::lock_guard lock(m_HandleMutex);
    return m_NextHandle++;
}

void DX12Device::ResetFrameDescriptors() {
    m_RtvCursor = 0;
    m_DsvCursor = 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Device::AllocateRtv(ID3D12Resource* resource, DXGI_FORMAT format) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    handle.ptr = 0;
    if (!m_RtvHeap || !resource || m_RtvCursor >= kRtvHeapCapacity) {
        return handle;
    }
    handle = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(m_RtvCursor) * m_RtvDescriptorSize;
    ++m_RtvCursor;

    D3D12_RENDER_TARGET_VIEW_DESC desc{};
    desc.Format = format;
    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;
    m_Device->CreateRenderTargetView(resource, format == DXGI_FORMAT_UNKNOWN ? nullptr : &desc, handle);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Device::AllocateDsv(ID3D12Resource* resource, DXGI_FORMAT format) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    handle.ptr = 0;
    if (!m_DsvHeap || !resource || m_DsvCursor >= kDsvHeapCapacity) {
        return handle;
    }
    handle = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(m_DsvCursor) * m_DsvDescriptorSize;
    ++m_DsvCursor;

    D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
    desc.Format = format;
    desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;
    m_Device->CreateDepthStencilView(resource, format == DXGI_FORMAT_UNKNOWN ? nullptr : &desc, handle);
    return handle;
}

DX12Buffer* DX12Device::FindBuffer(RHIBufferHandle handle) {
    auto it = m_Buffers.find(static_cast<uint64_t>(handle));
    return it == m_Buffers.end() ? nullptr : &it->second;
}

DX12Texture* DX12Device::FindTexture(RHITextureHandle handle) {
    auto it = m_Textures.find(static_cast<uint64_t>(handle));
    return it == m_Textures.end() ? nullptr : &it->second;
}

DX12GraphicsPipeline* DX12Device::FindGraphicsPipeline(RHIGraphicsPipelineHandle handle) {
    auto it = m_GfxPipelines.find(static_cast<uint64_t>(handle));
    return it == m_GfxPipelines.end() ? nullptr : &it->second;
}

DX12ComputePipeline* DX12Device::FindComputePipeline(RHIComputePipelineHandle handle) {
    auto it = m_ComputePipelines.find(static_cast<uint64_t>(handle));
    return it == m_ComputePipelines.end() ? nullptr : &it->second;
}

DX12TextureView* DX12Device::FindTextureView(RHITextureViewHandle handle) {
    auto it = m_TextureViews.find(static_cast<uint64_t>(handle));
    return it == m_TextureViews.end() ? nullptr : &it->second;
}

DX12Sampler* DX12Device::FindSampler(RHISamplerHandle handle) {
    auto it = m_Samplers.find(static_cast<uint64_t>(handle));
    return it == m_Samplers.end() ? nullptr : &it->second;
}

DX12PipelineLayout* DX12Device::FindPipelineLayout(RHIPipelineLayoutHandle handle) {
    auto it = m_Layouts.find(static_cast<uint64_t>(handle));
    return it == m_Layouts.end() ? nullptr : &it->second;
}

DX12DescriptorSet* DX12Device::FindDescriptorSet(RHIDescriptorSetHandle handle) {
    auto it = m_Sets.find(static_cast<uint64_t>(handle));
    return it == m_Sets.end() ? nullptr : &it->second;
}

DX12DescriptorSetLayout* DX12Device::FindDescriptorSetLayout(RHIDescriptorSetLayoutHandle handle) {
    auto it = m_SetLayouts.find(static_cast<uint64_t>(handle));
    return it == m_SetLayouts.end() ? nullptr : &it->second;
}

RHITextureHandle DX12Device::RegisterSwapchainTexture(ComPtr<ID3D12Resource> resource, Extent2D extent, Format format) {
    const auto handle = static_cast<RHITextureHandle>(AllocHandle());
    DX12Texture tex{};
    tex.desc.extent = {extent.width, extent.height, 1};
    tex.desc.format = format;
    tex.desc.usage = TextureUsage::ColorAttachment | TextureUsage::Present | TextureUsage::TransferDst;
    tex.resource = std::move(resource);
    tex.isSwapchain = true;
    tex.state = ResourceState::Present;
    tex.d3dState = D3D12_RESOURCE_STATE_PRESENT;
    m_Textures.emplace(static_cast<uint64_t>(handle), std::move(tex));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

void DX12Device::ClearSwapchainTextures(const std::vector<RHITextureHandle>& handles) {
    for (auto h : handles) {
        m_Textures.erase(static_cast<uint64_t>(h));
    }
}

IRHIQueue& DX12Device::GetQueue(QueueType type) {
    switch (type) {
    case QueueType::Compute: return m_ComputeQueue;
    case QueueType::Transfer: return m_TransferQueue;
    default: return m_GraphicsQueue;
    }
}

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
    auto it = m_Buffers.find(static_cast<uint64_t>(handle));
    if (it == m_Buffers.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown buffer.", "DestroyBuffer");
    }
    if (it->second.mapped && it->second.resource) {
        it->second.resource->Unmap(0, nullptr);
        it->second.mapped = nullptr;
    }
    m_Buffers.erase(it);
    ++m_Diagnostics.resourcesDestroyed;
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
        return RHIError::Make(RHIErrorCode::OutOfMemory, "CreateCommittedResource(texture) failed.", "CreateTexture", hr);
    }

    const auto handle = static_cast<RHITextureHandle>(AllocHandle());
    m_Textures.emplace(static_cast<uint64_t>(handle), std::move(tex));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyTexture(RHITextureHandle handle) {
    auto it = m_Textures.find(static_cast<uint64_t>(handle));
    if (it == m_Textures.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture.", "DestroyTexture");
    }
    if (it->second.isSwapchain) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Cannot destroy swapchain texture.", "DestroyTexture");
    }
    m_Textures.erase(it);
    ++m_Diagnostics.resourcesDestroyed;
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
    if (m_TextureViews.erase(static_cast<uint64_t>(handle)) == 0) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture view.", "DestroyTextureView");
    }
    ++m_Diagnostics.resourcesDestroyed;
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

    BufferDesc stagingDesc{};
    stagingDesc.size = update.data.size();
    stagingDesc.usage = BufferUsage::TransferSrc;
    stagingDesc.memory = MemoryUsage::HostVisible;
    auto staging = CreateBuffer(stagingDesc);
    if (!staging) {
        return staging.error;
    }
    if (void* mapped = MapBuffer(*staging)) {
        std::memcpy(mapped, update.data.data(), update.data.size());
    }

    // One-shot upload using a temporary list on frame slot 0.
    WaitIdle();
    m_Allocators[0]->Reset();
    m_CmdLists[0]->Reset(m_Allocators[0].Get(), nullptr);
    DX12CommandList list(this);
    list.Set(m_CmdLists[0].Get());
    list.Begin();
    const ResourceState old = tex->state;
    list.TransitionTexture(handle, old, ResourceState::CopyDst);
    BufferImageCopyRegion region{};
    region.bufferOffset = 0;
    region.bufferRowLength = update.rowPitch;
    region.image.mipLevel = update.mipLevel;
    region.image.baseLayer = update.arrayLayer;
    region.image.layerCount = 1;
    region.imageOffsetX = update.offsetX;
    region.imageOffsetY = update.offsetY;
    region.imageOffsetZ = update.offsetZ;
    region.imageExtent = update.extent;
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
    if (m_Samplers.erase(static_cast<uint64_t>(handle)) == 0) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown sampler.", "DestroySampler");
    }
    ++m_Diagnostics.resourcesDestroyed;
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
    if (m_Shaders.erase(static_cast<uint64_t>(handle)) == 0) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown shader.", "DestroyShader");
    }
    ++m_Diagnostics.resourcesDestroyed;
    return RHIResult<void>::Success();
}

RHIResult<RHIDescriptorSetLayoutHandle> DX12Device::CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc) {
    const auto handle = static_cast<RHIDescriptorSetLayoutHandle>(AllocHandle());
    DX12DescriptorSetLayout layout{};
    layout.desc = desc;
    layout.bindings.reserve(desc.bindings.size());
    uint32_t srvCursor = 0;
    uint32_t samplerCursor = 0;
    for (const auto& b : desc.bindings) {
        const uint32_t count = b.count ? b.count : 1u;
        DX12DescriptorBinding binding{};
        binding.binding = b;
        if (BindingUsesSrvHeap(b.type)) {
            binding.usesSrvUavCbv = true;
            binding.srvUavCbvOffset = srvCursor;
            srvCursor += count;
        }
        if (BindingUsesSamplerHeap(b.type)) {
            binding.usesSampler = true;
            binding.samplerOffset = samplerCursor;
            samplerCursor += count;
        }
        layout.bindings.push_back(binding);
    }
    layout.srvUavCbvCount = srvCursor;
    layout.samplerCount = samplerCursor;
    m_SetLayouts.emplace(static_cast<uint64_t>(handle), std::move(layout));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyDescriptorSetLayout(RHIDescriptorSetLayoutHandle handle) {
    if (m_SetLayouts.erase(static_cast<uint64_t>(handle)) == 0) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown descriptor set layout.", "DestroyDescriptorSetLayout");
    }
    ++m_Diagnostics.resourcesDestroyed;
    return RHIResult<void>::Success();
}

RHIResult<RHIDescriptorPoolHandle> DX12Device::CreateDescriptorPool(const DescriptorPoolDesc& desc) {
    const auto handle = static_cast<RHIDescriptorPoolHandle>(AllocHandle());
    DX12DescriptorPool pool{};
    pool.desc = desc;
    m_Pools.emplace(static_cast<uint64_t>(handle), std::move(pool));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyDescriptorPool(RHIDescriptorPoolHandle handle) {
    for (auto it = m_Sets.begin(); it != m_Sets.end();) {
        if (it->second.pool == handle) {
            it = m_Sets.erase(it);
        } else {
            ++it;
        }
    }
    if (m_Pools.erase(static_cast<uint64_t>(handle)) == 0) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown descriptor pool.", "DestroyDescriptorPool");
    }
    ++m_Diagnostics.resourcesDestroyed;
    return RHIResult<void>::Success();
}

RHIResult<void> DX12Device::ResetDescriptorPool(RHIDescriptorPoolHandle handle) {
    if (!m_Pools.count(static_cast<uint64_t>(handle))) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown descriptor pool.", "ResetDescriptorPool");
    }
    for (auto it = m_Sets.begin(); it != m_Sets.end();) {
        if (it->second.pool == handle) {
            it = m_Sets.erase(it);
        } else {
            ++it;
        }
    }
    return RHIResult<void>::Success();
}

RHIResult<RHIDescriptorSetHandle> DX12Device::AllocateDescriptorSet(const DescriptorSetAllocateDesc& desc) {
    auto* setLayout = FindDescriptorSetLayout(desc.layout);
    if (!m_Pools.count(static_cast<uint64_t>(desc.pool)) || !setLayout) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Invalid pool or layout.", "AllocateDescriptorSet");
    }
    uint32_t srvOffset = 0;
    uint32_t samplerOffset = 0;
    if (!AllocateShaderVisibleSlots(
            setLayout->srvUavCbvCount, setLayout->samplerCount, srvOffset, samplerOffset)) {
        return RHIError::Make(RHIErrorCode::OutOfMemory, "Descriptor heap exhausted.", "AllocateDescriptorSet");
    }
    const auto handle = static_cast<RHIDescriptorSetHandle>(AllocHandle());
    DX12DescriptorSet set{};
    set.pool = desc.pool;
    set.layout = desc.layout;
    set.srvUavCbvHeapOffset = srvOffset;
    set.samplerHeapOffset = samplerOffset;
    set.srvUavCbvCount = setLayout->srvUavCbvCount;
    set.samplerCount = setLayout->samplerCount;
    m_Sets.emplace(static_cast<uint64_t>(handle), std::move(set));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

void DX12Device::UpdateDescriptorSets(std::span<const WriteDescriptorSet> writes) {
    for (const auto& write : writes) {
        auto* set = FindDescriptorSet(write.set);
        auto* layout = set ? FindDescriptorSetLayout(set->layout) : nullptr;
        if (!set || !layout || write.count == 0) {
            continue;
        }

        const DX12DescriptorBinding* bindingMeta = nullptr;
        for (const auto& b : layout->bindings) {
            if (b.binding.binding == write.binding) {
                bindingMeta = &b;
                break;
            }
        }
        if (!bindingMeta) {
            continue;
        }

        for (uint32_t i = 0; i < write.count; ++i) {
            if (write.bufferInfos
                && (write.type == DescriptorType::UniformBuffer
                    || write.type == DescriptorType::StorageBuffer)) {
                const auto& bi = write.bufferInfos[i];
                auto* buf = FindBuffer(bi.buffer);
                if (!buf || !buf->resource) {
                    continue;
                }
                const uint32_t slot = set->srvUavCbvHeapOffset
                    + bindingMeta->srvUavCbvOffset
                    + write.arrayElement
                    + i;
                if (write.type == DescriptorType::UniformBuffer) {
                    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv{};
                    cbv.BufferLocation = buf->resource->GetGPUVirtualAddress() + bi.offset;
                    const uint64_t rawSize = bi.range == ~0ull
                        ? (buf->desc.size > bi.offset ? buf->desc.size - bi.offset : 0)
                        : bi.range;
                    cbv.SizeInBytes = static_cast<UINT>((rawSize + 255ull) & ~255ull);
                    if (cbv.SizeInBytes == 0) {
                        continue;
                    }
                    m_Device->CreateConstantBufferView(&cbv, SrvCpu(slot));
                } else {
                    D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
                    uav.Format = DXGI_FORMAT_R32_TYPELESS;
                    uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                    uav.Buffer.FirstElement = bi.offset / 4;
                    const uint64_t rawSize = bi.range == ~0ull
                        ? (buf->desc.size > bi.offset ? buf->desc.size - bi.offset : 0)
                        : bi.range;
                    uav.Buffer.NumElements = static_cast<UINT>(rawSize / 4);
                    uav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
                    m_Device->CreateUnorderedAccessView(buf->resource.Get(), nullptr, &uav, SrvCpu(slot));
                }
            }

            if (write.imageInfos
                && (write.type == DescriptorType::SampledImage
                    || write.type == DescriptorType::CombinedImageSampler
                    || write.type == DescriptorType::StorageImage
                    || write.type == DescriptorType::Sampler)) {
                const auto& ii = write.imageInfos[i];
                if (write.type != DescriptorType::Sampler
                    && bindingMeta->usesSrvUavCbv) {
                    auto* view = FindTextureView(ii.view);
                    auto* tex = view ? FindTexture(view->desc.texture) : nullptr;
                    if (tex && tex->resource) {
                        const uint32_t slot = set->srvUavCbvHeapOffset
                            + bindingMeta->srvUavCbvOffset
                            + write.arrayElement
                            + i;
                        if (write.type == DescriptorType::StorageImage) {
                            D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
                            uav.Format = view->format;
                            uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                            uav.Texture2D.MipSlice = view->desc.baseMip;
                            m_Device->CreateUnorderedAccessView(
                                tex->resource.Get(), nullptr, &uav, SrvCpu(slot));
                        } else {
                            D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
                            srv.Format = view->format;
                            srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                            srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                            srv.Texture2D.MostDetailedMip = view->desc.baseMip;
                            srv.Texture2D.MipLevels = view->desc.mipCount ? view->desc.mipCount : 1;
                            m_Device->CreateShaderResourceView(
                                tex->resource.Get(), &srv, SrvCpu(slot));
                        }
                    }
                }
                if ((write.type == DescriptorType::Sampler
                        || write.type == DescriptorType::CombinedImageSampler)
                    && bindingMeta->usesSampler) {
                    auto* sampler = FindSampler(ii.sampler);
                    if (sampler) {
                        const uint32_t slot = set->samplerHeapOffset
                            + bindingMeta->samplerOffset
                            + write.arrayElement
                            + i;
                        m_Device->CreateSampler(&sampler->samplerDesc, SamplerCpu(slot));
                    }
                }
            }
        }
    }
}

RHIResult<RHIPipelineLayoutHandle> DX12Device::CreatePipelineLayout(const PipelineLayoutDesc& desc) {
    std::vector<D3D12_ROOT_PARAMETER> rootParams;
    std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
    ranges.reserve(64);
    rootParams.reserve(16);

    DX12PipelineLayout layout{};
    layout.desc = desc;
    layout.setRoots.assign(desc.setLayouts.size(), {});

    uint32_t pushBytes = desc.pushConstantSize;
    if (!desc.pushConstantRanges.empty()) {
        uint32_t end = 0;
        for (const auto& r : desc.pushConstantRanges) {
            end = (std::max)(end, r.offset + r.size);
        }
        pushBytes = end;
    }
    if (pushBytes > 0) {
        D3D12_ROOT_PARAMETER pc{};
        pc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        pc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        pc.Constants.ShaderRegister = 0;
        pc.Constants.RegisterSpace = 0;
        pc.Constants.Num32BitValues = (pushBytes + 3u) / 4u;
        layout.pushConstantRootParam = static_cast<uint32_t>(rootParams.size());
        layout.pushConstantDwords = pc.Constants.Num32BitValues;
        rootParams.push_back(pc);
    }

    for (size_t setIdx = 0; setIdx < desc.setLayouts.size(); ++setIdx) {
        auto* setLayout = FindDescriptorSetLayout(desc.setLayouts[setIdx]);
        if (!setLayout) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown set layout.", "CreatePipelineLayout");
        }
        auto& mapping = layout.setRoots[setIdx];

        std::vector<D3D12_DESCRIPTOR_RANGE> srvRanges;
        std::vector<D3D12_DESCRIPTOR_RANGE> samplerRanges;
        for (const auto& b : setLayout->bindings) {
            const uint32_t count = b.binding.count ? b.binding.count : 1u;
            if (b.usesSrvUavCbv) {
                D3D12_DESCRIPTOR_RANGE range{};
                range.RangeType = ToSrvHeapRangeType(b.binding.type);
                range.NumDescriptors = count;
                range.BaseShaderRegister = b.binding.binding;
                range.RegisterSpace = static_cast<UINT>(setIdx);
                range.OffsetInDescriptorsFromTableStart = b.srvUavCbvOffset;
                srvRanges.push_back(range);
            }
            if (b.usesSampler) {
                D3D12_DESCRIPTOR_RANGE range{};
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                range.NumDescriptors = count;
                range.BaseShaderRegister = b.binding.binding;
                range.RegisterSpace = static_cast<UINT>(setIdx);
                range.OffsetInDescriptorsFromTableStart = b.samplerOffset;
                samplerRanges.push_back(range);
            }
        }

        if (!srvRanges.empty()) {
            const size_t base = ranges.size();
            ranges.insert(ranges.end(), srvRanges.begin(), srvRanges.end());
            mapping.srvUavCbvRootParam = static_cast<uint32_t>(rootParams.size());
            D3D12_ROOT_PARAMETER param{};
            param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(srvRanges.size());
            param.DescriptorTable.pDescriptorRanges = ranges.data() + base;
            rootParams.push_back(param);
        }
        if (!samplerRanges.empty()) {
            const size_t base = ranges.size();
            ranges.insert(ranges.end(), samplerRanges.begin(), samplerRanges.end());
            mapping.samplerRootParam = static_cast<uint32_t>(rootParams.size());
            D3D12_ROOT_PARAMETER param{};
            param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(samplerRanges.size());
            param.DescriptorTable.pDescriptorRanges = ranges.data() + base;
            rootParams.push_back(param);
        }
    }

    // Re-point DescriptorTable range pointers after final ranges vector growth.
    {
        size_t rangeCursor = 0;
        size_t paramIdx = (layout.pushConstantRootParam != UINT32_MAX) ? 1u : 0u;
        for (size_t setIdx = 0; setIdx < layout.setRoots.size(); ++setIdx) {
            auto& mapping = layout.setRoots[setIdx];
            auto* setLayout = FindDescriptorSetLayout(desc.setLayouts[setIdx]);
            if (!setLayout) {
                continue;
            }
            uint32_t srvRangeCount = 0;
            uint32_t samplerRangeCount = 0;
            for (const auto& b : setLayout->bindings) {
                if (b.usesSrvUavCbv) {
                    ++srvRangeCount;
                }
                if (b.usesSampler) {
                    ++samplerRangeCount;
                }
            }
            if (mapping.srvUavCbvRootParam != UINT32_MAX && paramIdx < rootParams.size()) {
                rootParams[paramIdx].DescriptorTable.pDescriptorRanges = ranges.data() + rangeCursor;
                rootParams[paramIdx].DescriptorTable.NumDescriptorRanges = srvRangeCount;
                rangeCursor += srvRangeCount;
                ++paramIdx;
            }
            if (mapping.samplerRootParam != UINT32_MAX && paramIdx < rootParams.size()) {
                rootParams[paramIdx].DescriptorTable.pDescriptorRanges = ranges.data() + rangeCursor;
                rootParams[paramIdx].DescriptorTable.NumDescriptorRanges = samplerRangeCount;
                rangeCursor += samplerRangeCount;
                ++paramIdx;
            }
        }
    }

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = static_cast<UINT>(rootParams.size());
    rsDesc.pParameters = rootParams.empty() ? nullptr : rootParams.data();
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    const HRESULT hrSerialize = D3D12SerializeRootSignature(
        &rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hrSerialize)) {
        const char* msg = (error && error->GetBufferPointer())
            ? static_cast<const char*>(error->GetBufferPointer())
            : "D3D12SerializeRootSignature failed.";
        return RHIError::Make(RHIErrorCode::BackendFailure, msg, "CreatePipelineLayout", hrSerialize);
    }
    const HRESULT hrCreate = m_Device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&layout.rootSignature));
    if (FAILED(hrCreate)) {
        return RHIError::Make(
            RHIErrorCode::BackendFailure, "CreateRootSignature failed.", "CreatePipelineLayout", hrCreate);
    }

    const auto handle = static_cast<RHIPipelineLayoutHandle>(AllocHandle());
    m_Layouts.emplace(static_cast<uint64_t>(handle), std::move(layout));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyPipelineLayout(RHIPipelineLayoutHandle handle) {
    if (m_Layouts.erase(static_cast<uint64_t>(handle)) == 0) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown pipeline layout.", "DestroyPipelineLayout");
    }
    ++m_Diagnostics.resourcesDestroyed;
    return RHIResult<void>::Success();
}

RHIResult<RHIGraphicsPipelineHandle> DX12Device::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) {
    DX12GraphicsPipeline pipeline{};
    pipeline.desc = desc;
    pipeline.layoutHandle = desc.layout;

    auto* layout = FindPipelineLayout(desc.layout);
    if (!layout || !layout->rootSignature) {
        return RHIError::Make(
            RHIErrorCode::InvalidHandle, "Missing pipeline layout / root signature.", "CreateGraphicsPipeline");
    }

    auto vsIt = m_Shaders.find(static_cast<uint64_t>(desc.vertexShader));
    auto fsIt = m_Shaders.find(static_cast<uint64_t>(desc.fragmentShader));
    const bool haveShaders = vsIt != m_Shaders.end() && fsIt != m_Shaders.end()
        && !vsIt->second.bytecode.empty() && !fsIt->second.bytecode.empty();
    if (!haveShaders || !m_Device) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Missing shaders.", "CreateGraphicsPipeline");
    }

    std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
    elements.reserve(desc.vertexAttributes.size());
    for (const auto& attr : desc.vertexAttributes) {
        D3D12_INPUT_ELEMENT_DESC el{};
        el.SemanticName = attr.semanticName ? attr.semanticName : "TEXCOORD";
        el.SemanticIndex = attr.semanticIndex != ~0u ? attr.semanticIndex : attr.location;
        el.Format = ToDxgiFormat(attr.format);
        el.InputSlot = attr.binding;
        el.AlignedByteOffset = attr.offset;
        el.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        el.InstanceDataStepRate = 0;
        for (const auto& binding : desc.vertexBindings) {
            if (binding.binding == attr.binding && binding.perInstance) {
                el.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                el.InstanceDataStepRate = 1;
                break;
            }
        }
        elements.push_back(el);
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = layout->rootSignature.Get();
    psoDesc.VS = {vsIt->second.bytecode.data(), vsIt->second.bytecode.size()};
    psoDesc.PS = {fsIt->second.bytecode.data(), fsIt->second.bytecode.size()};
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    if (desc.blend.enable) {
        auto& rt = psoDesc.BlendState.RenderTarget[0];
        rt.BlendEnable = TRUE;
        rt.SrcBlend = ToBlend(desc.blend.srcColor);
        rt.DestBlend = ToBlend(desc.blend.dstColor);
        rt.BlendOp = ToBlendOp(desc.blend.colorOp);
        rt.SrcBlendAlpha = ToBlend(desc.blend.srcAlpha);
        rt.DestBlendAlpha = ToBlend(desc.blend.dstAlpha);
        rt.BlendOpAlpha = ToBlendOp(desc.blend.alphaOp);
    }
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = desc.cullMode == CullMode::None
        ? D3D12_CULL_MODE_NONE
        : (desc.cullMode == CullMode::Front ? D3D12_CULL_MODE_FRONT : D3D12_CULL_MODE_BACK);
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.DepthStencilState.DepthEnable = desc.depthTest ? TRUE : FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = desc.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc = ToCompare(desc.depthCompare);
    psoDesc.InputLayout = {elements.data(), static_cast<UINT>(elements.size())};
    psoDesc.PrimitiveTopologyType = ToTopologyType(desc.topology);
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = ToDxgiFormat(desc.colorFormat);
    psoDesc.DSVFormat = desc.depthAttachment ? ToDxgiFormat(desc.depthFormat) : DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;

    ComPtr<ID3D12PipelineState> pso;
    const HRESULT hr = m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr)) {
        return RHIError::Make(
            RHIErrorCode::BackendFailure, "CreateGraphicsPipelineState failed.", "CreateGraphicsPipeline", hr);
    }
    pipeline.rootSignature = layout->rootSignature;
    pipeline.pso = std::move(pso);

    const auto handle = static_cast<RHIGraphicsPipelineHandle>(AllocHandle());
    m_GfxPipelines.emplace(static_cast<uint64_t>(handle), std::move(pipeline));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyGraphicsPipeline(RHIGraphicsPipelineHandle handle) {
    if (m_GfxPipelines.erase(static_cast<uint64_t>(handle)) == 0) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown graphics pipeline.", "DestroyGraphicsPipeline");
    }
    ++m_Diagnostics.resourcesDestroyed;
    return RHIResult<void>::Success();
}

RHIResult<RHIComputePipelineHandle> DX12Device::CreateComputePipeline(const ComputePipelineDesc& desc) {
    DX12ComputePipeline pipeline{};
    pipeline.desc = desc;

    auto* layout = FindPipelineLayout(desc.layout);
    if (!layout || !layout->rootSignature) {
        return RHIError::Make(
            RHIErrorCode::InvalidHandle, "Missing pipeline layout / root signature.", "CreateComputePipeline");
    }

    auto csIt = m_Shaders.find(static_cast<uint64_t>(desc.computeShader));
    if (csIt == m_Shaders.end() || csIt->second.bytecode.empty() || !m_Device) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Missing compute shader.", "CreateComputePipeline");
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = layout->rootSignature.Get();
    psoDesc.CS = {csIt->second.bytecode.data(), csIt->second.bytecode.size()};
    ComPtr<ID3D12PipelineState> pso;
    const HRESULT hr = m_Device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr)) {
        return RHIError::Make(
            RHIErrorCode::BackendFailure, "CreateComputePipelineState failed.", "CreateComputePipeline", hr);
    }
    pipeline.rootSignature = layout->rootSignature;
    pipeline.pso = std::move(pso);

    const auto handle = static_cast<RHIComputePipelineHandle>(AllocHandle());
    m_ComputePipelines.emplace(static_cast<uint64_t>(handle), std::move(pipeline));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyComputePipeline(RHIComputePipelineHandle handle) {
    if (m_ComputePipelines.erase(static_cast<uint64_t>(handle)) == 0) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown compute pipeline.", "DestroyComputePipeline");
    }
    ++m_Diagnostics.resourcesDestroyed;
    return RHIResult<void>::Success();
}

RHIResult<RHIFenceHandle> DX12Device::CreateFence(const FenceDesc& desc) {
    DX12Fence fence{};
    fence.desc = desc;
    fence.value = desc.signaled ? 1ull : 0ull;
    if (FAILED(m_Device->CreateFence(fence.value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence.fence)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateFence failed.", "CreateFence");
    }
    const auto handle = static_cast<RHIFenceHandle>(AllocHandle());
    m_Fences.emplace(static_cast<uint64_t>(handle), std::move(fence));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyFence(RHIFenceHandle handle) {
    if (m_Fences.erase(static_cast<uint64_t>(handle)) == 0) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown fence.", "DestroyFence");
    }
    ++m_Diagnostics.resourcesDestroyed;
    return RHIResult<void>::Success();
}

RHIResult<void> DX12Device::WaitForFences(std::span<const RHIFenceHandle> fences, bool waitAll, uint64_t timeoutNs) {
    if (fences.empty()) {
        return RHIResult<void>::Success();
    }
    const DWORD timeoutMs = timeoutNs == ~0ull
        ? INFINITE
        : static_cast<DWORD>((timeoutNs + 999999ull) / 1000000ull);

    auto waitOne = [&](RHIFenceHandle handle) -> bool {
        auto it = m_Fences.find(static_cast<uint64_t>(handle));
        if (it == m_Fences.end() || !it->second.fence) {
            return false;
        }
        if (it->second.fence->GetCompletedValue() >= it->second.value) {
            return true;
        }
        HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!event) {
            return false;
        }
        it->second.fence->SetEventOnCompletion(it->second.value, event);
        const DWORD result = WaitForSingleObject(event, timeoutMs);
        CloseHandle(event);
        return result == WAIT_OBJECT_0;
    };

    if (waitAll) {
        for (auto h : fences) {
            if (!waitOne(h)) {
                return RHIError::Make(RHIErrorCode::Timeout, "WaitForFences timed out.", "WaitForFences");
            }
        }
        return RHIResult<void>::Success();
    }

    // waitAny
    for (auto h : fences) {
        if (waitOne(h)) {
            return RHIResult<void>::Success();
        }
    }
    return RHIError::Make(RHIErrorCode::Timeout, "WaitForFences timed out.", "WaitForFences");
}

RHIResult<void> DX12Device::ResetFences(std::span<const RHIFenceHandle> fences) {
    for (auto h : fences) {
        auto it = m_Fences.find(static_cast<uint64_t>(h));
        if (it == m_Fences.end()) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown fence.", "ResetFences");
        }
        it->second.value = it->second.fence ? it->second.fence->GetCompletedValue() : 0;
    }
    return RHIResult<void>::Success();
}

bool DX12Device::IsFenceSignaled(RHIFenceHandle handle) {
    auto it = m_Fences.find(static_cast<uint64_t>(handle));
    if (it == m_Fences.end() || !it->second.fence) {
        return false;
    }
    return it->second.fence->GetCompletedValue() >= it->second.value;
}

RHIResult<RHISemaphoreHandle> DX12Device::CreateSemaphore(const SemaphoreDesc& desc) {
    const auto handle = static_cast<RHISemaphoreHandle>(AllocHandle());
    DX12Semaphore sem{};
    sem.desc = desc;
    m_Semaphores.emplace(static_cast<uint64_t>(handle), std::move(sem));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroySemaphore(RHISemaphoreHandle handle) {
    if (m_Semaphores.erase(static_cast<uint64_t>(handle)) == 0) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown semaphore.", "DestroySemaphore");
    }
    ++m_Diagnostics.resourcesDestroyed;
    return RHIResult<void>::Success();
}

IRHICommandList* DX12Device::BeginFrame() {
    if (!m_Valid || m_FrameActive) {
        return nullptr;
    }

    const uint64_t fenceToWait = m_FenceValues[m_FrameSlot];
    if (m_FrameFence && m_FrameFence->GetCompletedValue() < fenceToWait) {
        m_FrameFence->SetEventOnCompletion(fenceToWait, m_FenceEvent);
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }

    m_Allocators[m_FrameSlot]->Reset();
    m_CmdLists[m_FrameSlot]->Reset(m_Allocators[m_FrameSlot].Get(), nullptr);
    ResetFrameDescriptors();
    BindShaderHeaps(m_CmdLists[m_FrameSlot].Get());

    if (m_Swapchain) {
        if (auto acquire = m_Swapchain->AcquireNextImage(); !acquire) {
            return nullptr;
        }
    }

    m_FrameCmd.Set(m_CmdLists[m_FrameSlot].Get());
    m_FrameCmd.Begin();
    m_FrameActive = true;
    return &m_FrameCmd;
}

RHIResult<void> DX12Device::Submit(IRHICommandList* commandList) {
    if (!commandList || !m_FrameActive) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "No active frame command list.", "Submit");
    }
    commandList->End();
    auto* dxCmd = static_cast<DX12CommandList*>(commandList);
    ID3D12GraphicsCommandList* list = dxCmd->Get();
    if (!list) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Command list is null.", "Submit");
    }
    const HRESULT hrClose = list->Close();
    if (FAILED(hrClose)) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Close failed.", "Submit", hrClose);
    }

    ID3D12CommandList* lists[] = {list};
    m_CommandQueue->ExecuteCommandLists(1, lists);

    const uint64_t signalValue = m_FenceValues[m_FrameSlot] + 1;
    if (FAILED(m_CommandQueue->Signal(m_FrameFence.Get(), signalValue))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Signal failed.", "Submit");
    }
    m_FenceValues[m_FrameSlot] = signalValue;
    return RHIResult<void>::Success();
}

RHIResult<void> DX12Device::Present() {
    if (!m_Swapchain) {
        return RHIResult<void>::Success();
    }
    return m_Swapchain->Present();
}

RHIResult<void> DX12Device::EndFrame() {
    m_FrameActive = false;
    m_FrameSlot = (m_FrameSlot + 1) % m_FramesInFlight;
    ++m_FrameNumber;
    ++m_Diagnostics.lastFrame.frameIndex;
    TickDeferredDestruction();
    return RHIResult<void>::Success();
}

RHIResult<void> DX12Device::WaitIdle() {
    if (!m_CommandQueue || !m_FrameFence) {
        return RHIResult<void>::Success();
    }
    const uint64_t value = m_FenceValues[m_FrameSlot] + 1;
    m_CommandQueue->Signal(m_FrameFence.Get(), value);
    m_FenceValues[m_FrameSlot] = value;
    if (m_FrameFence->GetCompletedValue() < value) {
        m_FrameFence->SetEventOnCompletion(value, m_FenceEvent);
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }
    return RHIResult<void>::Success();
}

void DX12Device::TickDeferredDestruction() {}

std::unique_ptr<IRHIDevice> CreateDX12Device(const DeviceDesc& desc) {
    auto device = std::make_unique<DX12Device>(desc);
    if (!device->IsValid()) {
        return nullptr;
    }
    return device;
}

} // namespace we::rhi::dx12

#endif // _WIN32
