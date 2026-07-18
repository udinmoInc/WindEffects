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
    m_Caps.timestamps = true; // query heaps + WriteTimestamp
    m_Caps.pipelineStatistics = false;
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
    m_Caps.asyncCompute = false; // single DIRECT queue shared today
    m_Caps.bindlessResources = false;
    m_Caps.debugMarkers = false; // PIX markers optional; object naming via SetName
    m_Caps.multipleWindows = false;
    m_Caps.meshShaders = false;
    m_Caps.rayTracing = false;
    m_Caps.sparseResources = false;
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
    while (!m_Deferred.empty()) {
        auto item = m_Deferred.front();
        m_Deferred.pop_front();
        DestroyImmediate(item.kind, item.handle);
    }
    m_Swapchain.reset();
    m_CommandPools.clear();
    m_QueryPools.clear();
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
    m_DrawIndirectSig.Reset();
    m_DrawIndexedIndirectSig.Reset();
    m_DispatchIndirectSig.Reset();
    m_RtvHeap.Reset();
    m_DsvHeap.Reset();
    m_SrvHeap.Reset();
    m_SamplerHeap.Reset();
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
    if (auto sigs = CreateIndirectSignatures(); !sigs) {
        return sigs;
    }

    return RHIResult<void>::Success();
}

RHIResult<void> DX12Device::CreateIndirectSignatures() {
    if (!m_Device) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Device null.", "CreateIndirectSignatures");
    }

    D3D12_INDIRECT_ARGUMENT_DESC arg{};
    D3D12_COMMAND_SIGNATURE_DESC sigDesc{};
    sigDesc.NumArgumentDescs = 1;
    sigDesc.pArgumentDescs = &arg;

    arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
    sigDesc.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS);
    if (FAILED(m_Device->CreateCommandSignature(&sigDesc, nullptr, IID_PPV_ARGS(&m_DrawIndirectSig)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateCommandSignature(Draw) failed.", "CreateIndirectSignatures");
    }

    arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
    sigDesc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
    if (FAILED(m_Device->CreateCommandSignature(&sigDesc, nullptr, IID_PPV_ARGS(&m_DrawIndexedIndirectSig)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateCommandSignature(DrawIndexed) failed.", "CreateIndirectSignatures");
    }

    arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    sigDesc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
    if (FAILED(m_Device->CreateCommandSignature(&sigDesc, nullptr, IID_PPV_ARGS(&m_DispatchIndirectSig)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateCommandSignature(Dispatch) failed.", "CreateIndirectSignatures");
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
} // namespace we::rhi::dx12

#endif
