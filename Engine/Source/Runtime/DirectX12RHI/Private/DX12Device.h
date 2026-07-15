#pragma once

#include "RHI/IRHI.h"

#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#ifdef CreateSemaphore
#undef CreateSemaphore
#endif
#ifdef CreateSemaphoreA
#undef CreateSemaphoreA
#endif
#ifdef CreateSemaphoreW
#undef CreateSemaphoreW
#endif
#endif

namespace we::rhi::dx12 {

#if defined(_WIN32)

using Microsoft::WRL::ComPtr;

class DX12Device;

class DX12Queue final : public IRHIQueue {
public:
    explicit DX12Queue(QueueType type) : m_Type(type) {}
    void Set(ID3D12CommandQueue* queue) { m_Queue = queue; }
    [[nodiscard]] ID3D12CommandQueue* Get() const { return m_Queue; }
    [[nodiscard]] QueueType GetType() const override { return m_Type; }
    RHIResult<void> Submit(IRHICommandList& commandList) override;
    RHIResult<void> Submit(const SubmitDesc& desc) override;
    RHIResult<void> WaitIdle() override;

private:
    QueueType m_Type = QueueType::Graphics;
    ID3D12CommandQueue* m_Queue = nullptr;
};

class DX12CommandList final : public IRHICommandList {
public:
    explicit DX12CommandList(DX12Device* device);
    void Set(ID3D12GraphicsCommandList* list) { m_List = list; }
    [[nodiscard]] ID3D12GraphicsCommandList* Get() const { return m_List; }

    void Begin() override;
    void End() override;
    void BeginRendering(const RenderingInfo& info) override;
    void EndRendering() override;
    void SetViewport(const Viewport& viewport) override;
    void SetScissor(const Scissor& scissor) override;
    void BindGraphicsPipeline(RHIGraphicsPipelineHandle pipeline) override;
    void BindComputePipeline(RHIComputePipelineHandle pipeline) override;
    void BindVertexBuffer(uint32_t binding, RHIBufferHandle buffer, uint64_t offset = 0) override;
    void BindIndexBuffer(RHIBufferHandle buffer, uint64_t offset = 0, IndexType type = IndexType::UInt32) override;
    void BindDescriptorSets(PipelineBindPoint, RHIPipelineLayoutHandle, uint32_t, std::span<const RHIDescriptorSetHandle>) override;
    void PushConstants(RHIPipelineLayoutHandle, ShaderStageFlags, uint32_t, std::span<const uint8_t>) override;
    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) override;
    void DrawIndexed(uint32_t, uint32_t = 1, uint32_t = 0, int32_t = 0, uint32_t = 0) override;
    void Dispatch(uint32_t, uint32_t, uint32_t) override;
    void CopyBuffer(RHIBufferHandle, RHIBufferHandle, uint64_t, uint64_t = 0, uint64_t = 0) override;
    void CopyTexture(RHITextureHandle, RHITextureHandle, const TextureCopyRegion&) override;
    void BlitTexture(RHITextureHandle, RHITextureHandle, const TextureBlitRegion&, Filter = Filter::Linear) override;
    void CopyBufferToTexture(RHIBufferHandle, RHITextureHandle, const BufferImageCopyRegion&) override;
    void CopyTextureToBuffer(RHITextureHandle, RHIBufferHandle, const BufferImageCopyRegion&) override;
    void TransitionTexture(RHITextureHandle, ResourceState, ResourceState) override;
    void ResourceBarrier(std::span<const ResourceBarrierDesc> barriers) override;
    void PushDebugGroup(std::string_view) override;
    void PopDebugGroup() override;
    void InsertDebugMarker(std::string_view) override;

private:
    DX12Device* m_Device = nullptr;
    ID3D12GraphicsCommandList* m_List = nullptr;
    bool m_Recording = false;
    RHIGraphicsPipelineHandle m_BoundGfx = RHIGraphicsPipelineHandle::Invalid;
};

class DX12Swapchain final : public IRHISwapchain {
public:
    explicit DX12Swapchain(DX12Device* device);
    ~DX12Swapchain() override;
    RHIResult<void> Create(const DeviceDesc& desc);
    void Destroy();

    [[nodiscard]] Extent2D GetExtent() const override { return m_Extent; }
    [[nodiscard]] Format GetFormat() const override { return m_Format; }
    [[nodiscard]] uint32_t GetImageCount() const override { return static_cast<uint32_t>(m_Handles.size()); }
    [[nodiscard]] RHITextureHandle GetCurrentImage() const override;
    [[nodiscard]] uint32_t GetCurrentImageIndex() const override { return m_Index; }
    RHIResult<void> Resize(Extent2D extent) override;
    RHIResult<uint32_t> AcquireNextImage() override;
    RHIResult<void> Present() override;

private:
    DX12Device* m_Device = nullptr;
    ComPtr<IDXGISwapChain3> m_Swap;
    Extent2D m_Extent{};
    Format m_Format = Format::B8G8R8A8_UNORM;
    uint32_t m_Index = 0;
    std::vector<RHITextureHandle> m_Handles;
};

struct DX12Buffer {
    BufferDesc desc{};
    ComPtr<ID3D12Resource> resource;
    void* mapped = nullptr;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
};

struct DX12Texture {
    TextureDesc desc{};
    ComPtr<ID3D12Resource> resource;
    ResourceState state = ResourceState::Undefined;
    D3D12_RESOURCE_STATES d3dState = D3D12_RESOURCE_STATE_COMMON;
    bool isSwapchain = false;
};

struct DX12TextureView {
    TextureViewDesc desc{};
};

struct DX12Sampler {
    SamplerDesc desc{};
};

struct DX12Shader {
    ShaderDesc desc{};
    std::vector<uint8_t> bytecode;
};

struct DX12DescriptorSetLayout {
    DescriptorSetLayoutDesc desc{};
};

struct DX12DescriptorPool {
    DescriptorPoolDesc desc{};
};

struct DX12DescriptorSet {
    RHIDescriptorPoolHandle pool = RHIDescriptorPoolHandle::Invalid;
    RHIDescriptorSetLayoutHandle layout = RHIDescriptorSetLayoutHandle::Invalid;
};

struct DX12PipelineLayout {
    PipelineLayoutDesc desc{};
};

struct DX12GraphicsPipeline {
    GraphicsPipelineDesc desc{};
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;
};

struct DX12ComputePipeline {
    ComputePipelineDesc desc{};
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;
};

struct DX12Fence {
    FenceDesc desc{};
    ComPtr<ID3D12Fence> fence;
    uint64_t value = 0;
};

struct DX12Semaphore {
    SemaphoreDesc desc{};
};

class DX12Device final : public IRHIDevice {
public:
    explicit DX12Device(const DeviceDesc& desc);
    ~DX12Device() override;

    [[nodiscard]] bool IsValid() const override { return m_Valid; }
    [[nodiscard]] RHIBackend GetBackend() const override { return RHIBackend::DirectX12; }
    [[nodiscard]] const RHICapabilities& GetCapabilities() const override { return m_Caps; }
    [[nodiscard]] const RHIDiagnostics& GetDiagnostics() const override { return m_Diagnostics; }
    void ResetDiagnostics() override { m_Diagnostics.Reset(); }

    [[nodiscard]] IRHIQueue& GetQueue(QueueType type) override;
    [[nodiscard]] IRHISwapchain* GetSwapchain() override { return m_Swapchain.get(); }

    [[nodiscard]] RHIResult<RHIBufferHandle> CreateBuffer(const BufferDesc& desc) override;
    RHIResult<void> DestroyBuffer(RHIBufferHandle handle) override;
    [[nodiscard]] void* MapBuffer(RHIBufferHandle handle) override;
    void UnmapBuffer(RHIBufferHandle handle) override;
    RHIResult<void> UpdateBuffer(RHIBufferHandle handle, std::span<const uint8_t> data, uint64_t offset = 0) override;

    [[nodiscard]] RHIResult<RHITextureHandle> CreateTexture(const TextureDesc& desc) override;
    RHIResult<void> DestroyTexture(RHITextureHandle handle) override;
    [[nodiscard]] RHIResult<RHITextureViewHandle> CreateTextureView(const TextureViewDesc& desc) override;
    RHIResult<void> DestroyTextureView(RHITextureViewHandle handle) override;
    RHIResult<void> UpdateTexture(RHITextureHandle handle, const TextureUpdateDesc& update) override;

    [[nodiscard]] RHIResult<RHISamplerHandle> CreateSampler(const SamplerDesc& desc = {}) override;
    RHIResult<void> DestroySampler(RHISamplerHandle handle) override;
    [[nodiscard]] RHIResult<RHIShaderHandle> CreateShader(const ShaderDesc& desc) override;
    RHIResult<void> DestroyShader(RHIShaderHandle handle) override;

    [[nodiscard]] RHIResult<RHIDescriptorSetLayoutHandle> CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc) override;
    RHIResult<void> DestroyDescriptorSetLayout(RHIDescriptorSetLayoutHandle handle) override;
    [[nodiscard]] RHIResult<RHIDescriptorPoolHandle> CreateDescriptorPool(const DescriptorPoolDesc& desc) override;
    RHIResult<void> DestroyDescriptorPool(RHIDescriptorPoolHandle handle) override;
    RHIResult<void> ResetDescriptorPool(RHIDescriptorPoolHandle handle) override;
    [[nodiscard]] RHIResult<RHIDescriptorSetHandle> AllocateDescriptorSet(const DescriptorSetAllocateDesc& desc) override;
    void UpdateDescriptorSets(std::span<const WriteDescriptorSet> writes) override;

    [[nodiscard]] RHIResult<RHIPipelineLayoutHandle> CreatePipelineLayout(const PipelineLayoutDesc& desc = {}) override;
    RHIResult<void> DestroyPipelineLayout(RHIPipelineLayoutHandle handle) override;
    [[nodiscard]] RHIResult<RHIGraphicsPipelineHandle> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    RHIResult<void> DestroyGraphicsPipeline(RHIGraphicsPipelineHandle handle) override;
    [[nodiscard]] RHIResult<RHIComputePipelineHandle> CreateComputePipeline(const ComputePipelineDesc& desc) override;
    RHIResult<void> DestroyComputePipeline(RHIComputePipelineHandle handle) override;

    [[nodiscard]] RHIResult<RHIFenceHandle> CreateFence(const FenceDesc& desc = {}) override;
    RHIResult<void> DestroyFence(RHIFenceHandle handle) override;
    RHIResult<void> WaitForFences(std::span<const RHIFenceHandle> fences, bool waitAll, uint64_t timeoutNs) override;
    RHIResult<void> ResetFences(std::span<const RHIFenceHandle> fences) override;
    [[nodiscard]] bool IsFenceSignaled(RHIFenceHandle handle) override;
    [[nodiscard]] RHIResult<RHISemaphoreHandle> CreateSemaphore(const SemaphoreDesc& desc = {}) override;
    RHIResult<void> DestroySemaphore(RHISemaphoreHandle handle) override;

    [[nodiscard]] IRHICommandList* BeginFrame() override;
    RHIResult<void> Submit(IRHICommandList* commandList) override;
    RHIResult<void> Present() override;
    RHIResult<void> EndFrame() override;
    RHIResult<void> WaitIdle() override;
    [[nodiscard]] uint64_t GetFrameNumber() const override { return m_FrameNumber; }
    [[nodiscard]] uint32_t GetFramesInFlight() const override { return m_FramesInFlight; }
    [[nodiscard]] uint32_t GetCurrentFrameSlot() const override { return m_FrameSlot; }
    void SetResourceName(RHIBufferHandle, std::string_view) override {}
    void SetResourceName(RHITextureHandle, std::string_view) override {}
    void TickDeferredDestruction() override;

    [[nodiscard]] ID3D12Device* GetD3DDevice() const { return m_Device.Get(); }
    [[nodiscard]] IDXGIFactory4* GetFactory() const { return m_Factory.Get(); }
    [[nodiscard]] DX12Buffer* FindBuffer(RHIBufferHandle handle);
    [[nodiscard]] DX12Texture* FindTexture(RHITextureHandle handle);
    [[nodiscard]] DX12GraphicsPipeline* FindGraphicsPipeline(RHIGraphicsPipelineHandle handle);
    [[nodiscard]] DX12ComputePipeline* FindComputePipeline(RHIComputePipelineHandle handle);
    RHITextureHandle RegisterSwapchainTexture(ComPtr<ID3D12Resource> resource, Extent2D extent, Format format);
    void ClearSwapchainTextures(const std::vector<RHITextureHandle>& handles);
    void ResetFrameDescriptors();
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE AllocateRtv(ID3D12Resource* resource, DXGI_FORMAT format);
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE AllocateDsv(ID3D12Resource* resource, DXGI_FORMAT format);

private:
    [[nodiscard]] uint64_t AllocHandle();
    RHIResult<void> CreateDeviceAndQueue();

    DeviceDesc m_Desc{};
    bool m_Valid = false;
    RHICapabilities m_Caps{};
    RHIDiagnostics m_Diagnostics{};

    ComPtr<IDXGIFactory4> m_Factory;
    ComPtr<ID3D12Device> m_Device;
    ComPtr<ID3D12CommandQueue> m_CommandQueue;
    ComPtr<ID3D12CommandAllocator> m_Allocators[3];
    ComPtr<ID3D12GraphicsCommandList> m_CmdLists[3];
    ComPtr<ID3D12Fence> m_FrameFence;
    HANDLE m_FenceEvent = nullptr;
    uint64_t m_FenceValues[3]{};

    ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_DsvHeap;
    UINT m_RtvDescriptorSize = 0;
    UINT m_DsvDescriptorSize = 0;
    uint32_t m_RtvCursor = 0;
    uint32_t m_DsvCursor = 0;

    DX12Queue m_GraphicsQueue{QueueType::Graphics};
    DX12Queue m_ComputeQueue{QueueType::Compute};
    DX12Queue m_TransferQueue{QueueType::Transfer};
    std::unique_ptr<DX12Swapchain> m_Swapchain;
    DX12CommandList m_FrameCmd{this};

    uint32_t m_FramesInFlight = 2;
    uint32_t m_FrameSlot = 0;
    uint64_t m_FrameNumber = 0;
    bool m_FrameActive = false;
    uint64_t m_NextHandle = 1;

    std::unordered_map<uint64_t, DX12Buffer> m_Buffers;
    std::unordered_map<uint64_t, DX12Texture> m_Textures;
    std::unordered_map<uint64_t, DX12TextureView> m_TextureViews;
    std::unordered_map<uint64_t, DX12Sampler> m_Samplers;
    std::unordered_map<uint64_t, DX12Shader> m_Shaders;
    std::unordered_map<uint64_t, DX12DescriptorSetLayout> m_SetLayouts;
    std::unordered_map<uint64_t, DX12DescriptorPool> m_Pools;
    std::unordered_map<uint64_t, DX12DescriptorSet> m_Sets;
    std::unordered_map<uint64_t, DX12PipelineLayout> m_Layouts;
    std::unordered_map<uint64_t, DX12GraphicsPipeline> m_GfxPipelines;
    std::unordered_map<uint64_t, DX12ComputePipeline> m_ComputePipelines;
    std::unordered_map<uint64_t, DX12Fence> m_Fences;
    std::unordered_map<uint64_t, DX12Semaphore> m_Semaphores;
    std::mutex m_HandleMutex;
};

[[nodiscard]] std::unique_ptr<IRHIDevice> CreateDX12Device(const DeviceDesc& desc);

#endif // _WIN32

} // namespace we::rhi::dx12
