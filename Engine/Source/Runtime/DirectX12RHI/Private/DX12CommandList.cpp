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

void DX12CommandList::DrawIndirect(RHIBufferHandle buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) {
    if (!m_List || !m_Device || drawCount == 0) {
        return;
    }
    auto* buf = m_Device->FindBuffer(buffer);
    auto* sig = m_Device->GetDrawIndirectSignature();
    if (!buf || !buf->resource || !sig) {
        return;
    }
    const UINT byteStride = stride ? stride : static_cast<UINT>(sizeof(D3D12_DRAW_ARGUMENTS));
    m_List->ExecuteIndirect(sig, drawCount, buf->resource.Get(), offset, nullptr, 0);
    (void)byteStride;
}

void DX12CommandList::DrawIndexedIndirect(RHIBufferHandle buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) {
    if (!m_List || !m_Device || drawCount == 0) {
        return;
    }
    auto* buf = m_Device->FindBuffer(buffer);
    auto* sig = m_Device->GetDrawIndexedIndirectSignature();
    if (!buf || !buf->resource || !sig) {
        return;
    }
    (void)stride;
    m_List->ExecuteIndirect(sig, drawCount, buf->resource.Get(), offset, nullptr, 0);
}

void DX12CommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    if (!m_List) {
        return;
    }
    m_List->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void DX12CommandList::DispatchIndirect(RHIBufferHandle buffer, uint64_t offset) {
    if (!m_List || !m_Device) {
        return;
    }
    auto* buf = m_Device->FindBuffer(buffer);
    auto* sig = m_Device->GetDispatchIndirectSignature();
    if (!buf || !buf->resource || !sig) {
        return;
    }
    m_List->ExecuteIndirect(sig, 1, buf->resource.Get(), offset, nullptr, 0);
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
        : ((region.imageExtent.width * 4u + 255u) & ~255u);
    // D3D12 placed footprints require 256-byte row alignment.
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
            buf->resourceState = b.buffer.after;
            buf->state = barrier.Transition.StateAfter;
        }
    }
    if (!d3dBarriers.empty()) {
        m_List->ResourceBarrier(static_cast<UINT>(d3dBarriers.size()), d3dBarriers.data());
    }
}

void DX12CommandList::WriteTimestamp(RHIQueryPoolHandle pool, uint32_t queryIndex) {
    if (!m_List || !m_Device) {
        return;
    }
    auto* q = m_Device->FindQueryPool(pool);
    if (!q || !q->heap || queryIndex >= q->desc.count) {
        return;
    }
    m_List->EndQuery(q->heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
}

void DX12CommandList::BeginQuery(RHIQueryPoolHandle pool, uint32_t queryIndex) {
    if (!m_List || !m_Device) {
        return;
    }
    auto* q = m_Device->FindQueryPool(pool);
    if (!q || !q->heap || queryIndex >= q->desc.count) {
        return;
    }
    const D3D12_QUERY_TYPE type = q->desc.type == QueryType::Occlusion
        ? D3D12_QUERY_TYPE_OCCLUSION
        : D3D12_QUERY_TYPE_TIMESTAMP;
    if (type == D3D12_QUERY_TYPE_OCCLUSION) {
        m_List->BeginQuery(q->heap.Get(), type, queryIndex);
    }
}

void DX12CommandList::EndQuery(RHIQueryPoolHandle pool, uint32_t queryIndex) {
    if (!m_List || !m_Device) {
        return;
    }
    auto* q = m_Device->FindQueryPool(pool);
    if (!q || !q->heap || queryIndex >= q->desc.count) {
        return;
    }
    const D3D12_QUERY_TYPE type = q->desc.type == QueryType::Occlusion
        ? D3D12_QUERY_TYPE_OCCLUSION
        : D3D12_QUERY_TYPE_TIMESTAMP;
    m_List->EndQuery(q->heap.Get(), type, queryIndex);
}

void DX12CommandList::ResetQueryPool(RHIQueryPoolHandle, uint32_t, uint32_t) {
    // DX12 query heaps do not require an explicit reset between frames when resolved.
}

void DX12CommandList::PushDebugGroup(std::string_view) {}
void DX12CommandList::PopDebugGroup() {}
void DX12CommandList::InsertDebugMarker(std::string_view) {}
} // namespace we::rhi::dx12

#endif