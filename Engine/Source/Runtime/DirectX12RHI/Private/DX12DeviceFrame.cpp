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
    if (m_Fences.find(static_cast<uint64_t>(handle)) == m_Fences.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown fence.", "DestroyFence");
    }
    EnqueueDeferred(DeferredKind::Fence, static_cast<uint64_t>(handle));
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
    if (m_Semaphores.find(static_cast<uint64_t>(handle)) == m_Semaphores.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown semaphore.", "DestroySemaphore");
    }
    EnqueueDeferred(DeferredKind::Semaphore, static_cast<uint64_t>(handle));
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
    while (!m_Deferred.empty()) {
        auto item = m_Deferred.front();
        m_Deferred.pop_front();
        DestroyImmediate(item.kind, item.handle);
    }
    return RHIResult<void>::Success();
}

void DX12Device::EnqueueDeferred(DeferredKind kind, uint64_t handle) {
    m_Deferred.push_back(DeferredDestroyItem{kind, handle, m_FrameNumber + m_FramesInFlight});
    ++m_Diagnostics.deferredDestroys;
}

void DX12Device::DestroyImmediate(DeferredKind kind, uint64_t handle) {
    switch (kind) {
    case DeferredKind::Buffer: {
        auto it = m_Buffers.find(handle);
        if (it == m_Buffers.end()) {
            break;
        }
        if (it->second.mapped && it->second.resource) {
            it->second.resource->Unmap(0, nullptr);
            it->second.mapped = nullptr;
        }
        m_Buffers.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::Texture: {
        auto it = m_Textures.find(handle);
        if (it == m_Textures.end() || it->second.isSwapchain) {
            break;
        }
        m_Textures.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::TextureView:
        if (m_TextureViews.erase(handle)) {
            ++m_Diagnostics.resourcesDestroyed;
        }
        break;
    case DeferredKind::Sampler:
        if (m_Samplers.erase(handle)) {
            ++m_Diagnostics.resourcesDestroyed;
        }
        break;
    case DeferredKind::Shader:
        if (m_Shaders.erase(handle)) {
            ++m_Diagnostics.resourcesDestroyed;
        }
        break;
    case DeferredKind::DescriptorSetLayout:
        if (m_SetLayouts.erase(handle)) {
            ++m_Diagnostics.resourcesDestroyed;
        }
        break;
    case DeferredKind::DescriptorPool: {
        for (auto it = m_Sets.begin(); it != m_Sets.end();) {
            if (static_cast<uint64_t>(it->second.pool) == handle) {
                it = m_Sets.erase(it);
            } else {
                ++it;
            }
        }
        if (m_Pools.erase(handle)) {
            ++m_Diagnostics.resourcesDestroyed;
        }
        break;
    }
    case DeferredKind::PipelineLayout:
        if (m_Layouts.erase(handle)) {
            ++m_Diagnostics.resourcesDestroyed;
        }
        break;
    case DeferredKind::GraphicsPipeline:
        if (m_GfxPipelines.erase(handle)) {
            ++m_Diagnostics.resourcesDestroyed;
        }
        break;
    case DeferredKind::ComputePipeline:
        if (m_ComputePipelines.erase(handle)) {
            ++m_Diagnostics.resourcesDestroyed;
        }
        break;
    case DeferredKind::Fence:
        if (m_Fences.erase(handle)) {
            ++m_Diagnostics.resourcesDestroyed;
        }
        break;
    case DeferredKind::Semaphore:
        if (m_Semaphores.erase(handle)) {
            ++m_Diagnostics.resourcesDestroyed;
        }
        break;
    case DeferredKind::CommandPool:
        if (m_CommandPools.erase(handle)) {
            ++m_Diagnostics.resourcesDestroyed;
        }
        break;
    case DeferredKind::QueryPool:
        if (m_QueryPools.erase(handle)) {
            ++m_Diagnostics.resourcesDestroyed;
        }
        break;
    }
}

void DX12Device::TickDeferredDestruction() {
    while (!m_Deferred.empty() && m_Deferred.front().frame <= m_FrameNumber) {
        auto item = m_Deferred.front();
        m_Deferred.pop_front();
        DestroyImmediate(item.kind, item.handle);
    }
}

RHIResult<RHICommandPoolHandle> DX12Device::CreateCommandPool(const CommandPoolDesc& desc) {
    if (!m_Device) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Device null.", "CreateCommandPool");
    }
    DX12CommandPool pool{};
    pool.desc = desc;
    if (FAILED(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pool.allocator)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateCommandAllocator failed.", "CreateCommandPool");
    }
    const auto handle = static_cast<RHICommandPoolHandle>(AllocHandle());
    m_CommandPools.emplace(static_cast<uint64_t>(handle), std::move(pool));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyCommandPool(RHICommandPoolHandle handle) {
    if (m_CommandPools.find(static_cast<uint64_t>(handle)) == m_CommandPools.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown command pool.", "DestroyCommandPool");
    }
    EnqueueDeferred(DeferredKind::CommandPool, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<void> DX12Device::ResetCommandPool(RHICommandPoolHandle handle) {
    auto it = m_CommandPools.find(static_cast<uint64_t>(handle));
    if (it == m_CommandPools.end() || !it->second.allocator) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown command pool.", "ResetCommandPool");
    }
    if (FAILED(it->second.allocator->Reset())) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Command allocator Reset failed.", "ResetCommandPool");
    }
    return RHIResult<void>::Success();
}

RHIResult<IRHICommandList*> DX12Device::AllocateCommandList(RHICommandPoolHandle poolHandle) {
    auto it = m_CommandPools.find(static_cast<uint64_t>(poolHandle));
    if (it == m_CommandPools.end() || !it->second.allocator || !m_Device) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown command pool.", "AllocateCommandList");
    }
    ComPtr<ID3D12GraphicsCommandList> list;
    if (FAILED(m_Device->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT, it->second.allocator.Get(), nullptr, IID_PPV_ARGS(&list)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateCommandList failed.", "AllocateCommandList");
    }
    list->Close();
    auto cmd = std::make_unique<DX12CommandList>(this);
    cmd->Set(list.Get());
    IRHICommandList* raw = cmd.get();
    it->second.lists.push_back(std::move(cmd));
    it->second.nativeLists.push_back(std::move(list));
    return raw;
}

RHIResult<RHIQueryPoolHandle> DX12Device::CreateQueryPool(const QueryPoolDesc& desc) {
    if (!m_Device || desc.count == 0) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Invalid query pool desc.", "CreateQueryPool");
    }
    D3D12_QUERY_HEAP_DESC heapDesc{};
    heapDesc.Count = desc.count;
    switch (desc.type) {
    case QueryType::Occlusion:
        heapDesc.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
        break;
    case QueryType::PipelineStatistics:
        heapDesc.Type = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
        break;
    case QueryType::Timestamp:
    default:
        heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        break;
    }
    DX12QueryPool pool{};
    pool.desc = desc;
    pool.results.assign(desc.count, 0);
    if (FAILED(m_Device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&pool.heap)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateQueryHeap failed.", "CreateQueryPool");
    }
    const auto handle = static_cast<RHIQueryPoolHandle>(AllocHandle());
    m_QueryPools.emplace(static_cast<uint64_t>(handle), std::move(pool));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyQueryPool(RHIQueryPoolHandle handle) {
    if (m_QueryPools.find(static_cast<uint64_t>(handle)) == m_QueryPools.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown query pool.", "DestroyQueryPool");
    }
    EnqueueDeferred(DeferredKind::QueryPool, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<void> DX12Device::GetQueryPoolResults(
    RHIQueryPoolHandle handle,
    uint32_t firstQuery,
    uint32_t queryCount,
    std::span<uint64_t> results,
    bool)
{
    auto* pool = FindQueryPool(handle);
    if (!pool) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown query pool.", "GetQueryPoolResults");
    }
    if (firstQuery + queryCount > pool->desc.count || results.size() < queryCount) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Query range invalid.", "GetQueryPoolResults");
    }
    // Full resolve requires a readback buffer + ResolveQueryData on a command list.
    // Return cached zeros until a frame resolve path is wired.
    for (uint32_t i = 0; i < queryCount; ++i) {
        results[i] = pool->results[firstQuery + i];
    }
    return RHIResult<void>::Success();
}

ResourceState DX12Device::GetTextureState(RHITextureHandle handle) const {
    auto it = m_Textures.find(static_cast<uint64_t>(handle));
    return it == m_Textures.end() ? ResourceState::Undefined : it->second.state;
}

ResourceState DX12Device::GetBufferState(RHIBufferHandle handle) const {
    auto it = m_Buffers.find(static_cast<uint64_t>(handle));
    return it == m_Buffers.end() ? ResourceState::Undefined : it->second.resourceState;
}

DX12QueryPool* DX12Device::FindQueryPool(RHIQueryPoolHandle handle) {
    auto it = m_QueryPools.find(static_cast<uint64_t>(handle));
    return it == m_QueryPools.end() ? nullptr : &it->second;
}

void DX12Device::SetResourceName(RHIBufferHandle handle, std::string_view name) {
    auto* buf = FindBuffer(handle);
    if (!buf || !buf->resource || name.empty()) {
        return;
    }
    std::wstring wide(name.begin(), name.end());
    buf->resource->SetName(wide.c_str());
}

void DX12Device::SetResourceName(RHITextureHandle handle, std::string_view name) {
    auto* tex = FindTexture(handle);
    if (!tex || !tex->resource || name.empty()) {
        return;
    }
    std::wstring wide(name.begin(), name.end());
    tex->resource->SetName(wide.c_str());
}

std::unique_ptr<IRHIDevice> CreateDX12Device(const DeviceDesc& desc) {
    auto device = std::make_unique<DX12Device>(desc);
    if (!device->IsValid()) {
        return nullptr;
    }
    return device;
}
} // namespace we::rhi::dx12

#endif
