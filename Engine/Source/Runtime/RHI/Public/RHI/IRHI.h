#pragma once

#include "RHI/Capabilities.h"
#include "RHI/Desc.h"
#include "RHI/Diagnostics.h"
#include "RHI/Export.h"
#include "RHI/Result.h"
#include "RHI/Types.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::rhi {

class IRHICommandList;
class IRHIDevice;
class IRHISwapchain;
class IRHIQueue;

// API-independent command recording. Thread-local lists may be created from pools later.
class RHI_API IRHICommandList {
public:
    virtual ~IRHICommandList() = default;

    virtual void Begin() = 0;
    virtual void End() = 0;

    virtual void BeginRendering(const RenderingInfo& info) = 0;
    virtual void EndRendering() = 0;

    virtual void SetViewport(const Viewport& viewport) = 0;
    virtual void SetScissor(const Scissor& scissor) = 0;

    virtual void BindGraphicsPipeline(RHIGraphicsPipelineHandle pipeline) = 0;
    virtual void BindComputePipeline(RHIComputePipelineHandle pipeline) = 0;
    virtual void BindVertexBuffer(uint32_t binding, RHIBufferHandle buffer, uint64_t offset = 0) = 0;
    virtual void BindIndexBuffer(RHIBufferHandle buffer, uint64_t offset = 0, IndexType type = IndexType::UInt32) = 0;

    virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) = 0;
    virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;

    virtual void CopyBuffer(RHIBufferHandle src, RHIBufferHandle dst, uint64_t size, uint64_t srcOffset = 0, uint64_t dstOffset = 0) = 0;
    virtual void TransitionTexture(RHITextureHandle texture, ResourceState before, ResourceState after) = 0;

    virtual void PushDebugGroup(std::string_view name) = 0;
    virtual void PopDebugGroup() = 0;
    virtual void InsertDebugMarker(std::string_view name) = 0;
};

class RHI_API IRHIQueue {
public:
    virtual ~IRHIQueue() = default;
    [[nodiscard]] virtual QueueType GetType() const = 0;
    virtual RHIResult<void> Submit(IRHICommandList& commandList) = 0;
    virtual RHIResult<void> WaitIdle() = 0;
};

class RHI_API IRHISwapchain {
public:
    virtual ~IRHISwapchain() = default;

    [[nodiscard]] virtual Extent2D GetExtent() const = 0;
    [[nodiscard]] virtual Format GetFormat() const = 0;
    [[nodiscard]] virtual uint32_t GetImageCount() const = 0;
    [[nodiscard]] virtual RHITextureHandle GetCurrentImage() const = 0;
    [[nodiscard]] virtual uint32_t GetCurrentImageIndex() const = 0;

    virtual RHIResult<void> Resize(Extent2D extent) = 0;
    virtual RHIResult<uint32_t> AcquireNextImage() = 0;
    virtual RHIResult<void> Present() = 0;
};

class RHI_API IRHIDevice {
public:
    virtual ~IRHIDevice() = default;

    [[nodiscard]] virtual bool IsValid() const = 0;
    [[nodiscard]] virtual RHIBackend GetBackend() const = 0;
    [[nodiscard]] virtual const RHICapabilities& GetCapabilities() const = 0;
    [[nodiscard]] virtual const RHIDiagnostics& GetDiagnostics() const = 0;
    virtual void ResetDiagnostics() = 0;

    [[nodiscard]] virtual IRHIQueue& GetQueue(QueueType type) = 0;
    [[nodiscard]] virtual IRHISwapchain* GetSwapchain() = 0;

    [[nodiscard]] virtual RHIResult<RHIBufferHandle> CreateBuffer(const BufferDesc& desc) = 0;
    virtual RHIResult<void> DestroyBuffer(RHIBufferHandle handle) = 0;
    [[nodiscard]] virtual void* MapBuffer(RHIBufferHandle handle) = 0;
    virtual void UnmapBuffer(RHIBufferHandle handle) = 0;
    virtual RHIResult<void> UpdateBuffer(RHIBufferHandle handle, std::span<const uint8_t> data, uint64_t offset = 0) = 0;

    [[nodiscard]] virtual RHIResult<RHITextureHandle> CreateTexture(const TextureDesc& desc) = 0;
    virtual RHIResult<void> DestroyTexture(RHITextureHandle handle) = 0;
    [[nodiscard]] virtual RHIResult<RHITextureViewHandle> CreateTextureView(const TextureViewDesc& desc) = 0;
    virtual RHIResult<void> DestroyTextureView(RHITextureViewHandle handle) = 0;

    [[nodiscard]] virtual RHIResult<RHISamplerHandle> CreateSampler(const SamplerDesc& desc = {}) = 0;
    virtual RHIResult<void> DestroySampler(RHISamplerHandle handle) = 0;

    [[nodiscard]] virtual RHIResult<RHIShaderHandle> CreateShader(const ShaderDesc& desc) = 0;
    virtual RHIResult<void> DestroyShader(RHIShaderHandle handle) = 0;

    [[nodiscard]] virtual RHIResult<RHIPipelineLayoutHandle> CreatePipelineLayout(const PipelineLayoutDesc& desc = {}) = 0;
    virtual RHIResult<void> DestroyPipelineLayout(RHIPipelineLayoutHandle handle) = 0;

    [[nodiscard]] virtual RHIResult<RHIGraphicsPipelineHandle> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
    virtual RHIResult<void> DestroyGraphicsPipeline(RHIGraphicsPipelineHandle handle) = 0;

    [[nodiscard]] virtual RHIResult<RHIComputePipelineHandle> CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
    virtual RHIResult<void> DestroyComputePipeline(RHIComputePipelineHandle handle) = 0;

    [[nodiscard]] virtual IRHICommandList* BeginFrame() = 0;
    virtual RHIResult<void> Submit(IRHICommandList* commandList) = 0;
    virtual RHIResult<void> Present() = 0;
    virtual RHIResult<void> EndFrame() = 0;
    virtual RHIResult<void> WaitIdle() = 0;

    virtual void SetResourceName(RHIBufferHandle handle, std::string_view name) = 0;
    virtual void SetResourceName(RHITextureHandle handle, std::string_view name) = 0;

    // Advance deferred destruction / frame recycling.
    virtual void TickDeferredDestruction() = 0;
};

class RHI_API IRHI {
public:
    virtual ~IRHI() = default;

    virtual bool Initialize(const RHIInitDesc& desc = {}) = 0;
    virtual void Shutdown() = 0;
    [[nodiscard]] virtual bool IsInitialized() const = 0;
    [[nodiscard]] virtual RHIBackend GetActiveBackend() const = 0;
    [[nodiscard]] virtual const char* GetBackendName() const = 0;

    [[nodiscard]] virtual std::vector<AdapterDesc> EnumerateAdapters() const = 0;
    [[nodiscard]] virtual RHIResult<std::unique_ptr<IRHIDevice>> CreateDevice(const DeviceDesc& desc = {}) = 0;

    [[nodiscard]] virtual const RHICapabilities& GetCapabilities() const = 0;
};

} // namespace we::rhi
