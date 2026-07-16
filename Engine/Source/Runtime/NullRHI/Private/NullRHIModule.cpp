#include "Modules/IModuleInterface.h"
#include "RHI/RHIFactory.h"
#include "RHI/IRHI.h"
#include "RHI/Result.h"
#include "Platform/Platform.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"

#include <cstring>
#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace we::rhi {
namespace {

uint64_t g_NextHandle = 1;

[[nodiscard]] uint64_t AllocHandle() {
    return g_NextHandle++;
}

class NullCommandList final : public IRHICommandList {
public:
    void Begin() override { m_Recording = true; }
    void End() override { m_Recording = false; }
    void BeginRendering(const RenderingInfo&) override {}
    void EndRendering() override {}
    void SetViewport(const Viewport&) override {}
    void SetScissor(const Scissor&) override {}
    void BindGraphicsPipeline(RHIGraphicsPipelineHandle) override {}
    void BindComputePipeline(RHIComputePipelineHandle) override {}
    void BindVertexBuffer(uint32_t, RHIBufferHandle, uint64_t) override {}
    void BindIndexBuffer(RHIBufferHandle, uint64_t, IndexType) override {}
    void BindDescriptorSets(PipelineBindPoint, RHIPipelineLayoutHandle, uint32_t, std::span<const RHIDescriptorSetHandle>) override {}
    void PushConstants(RHIPipelineLayoutHandle, ShaderStageFlags, uint32_t, std::span<const uint8_t>) override {}
    void Draw(uint32_t, uint32_t, uint32_t, uint32_t) override {}
    void DrawIndexed(uint32_t, uint32_t, uint32_t, int32_t, uint32_t) override {}
    void DrawIndirect(RHIBufferHandle, uint64_t, uint32_t, uint32_t) override {}
    void DrawIndexedIndirect(RHIBufferHandle, uint64_t, uint32_t, uint32_t) override {}
    void Dispatch(uint32_t, uint32_t, uint32_t) override {}
    void DispatchIndirect(RHIBufferHandle, uint64_t) override {}
    void CopyBuffer(RHIBufferHandle, RHIBufferHandle, uint64_t, uint64_t, uint64_t) override {}
    void CopyTexture(RHITextureHandle, RHITextureHandle, const TextureCopyRegion&) override {}
    void BlitTexture(RHITextureHandle, RHITextureHandle, const TextureBlitRegion&, Filter) override {}
    void CopyBufferToTexture(RHIBufferHandle, RHITextureHandle, const BufferImageCopyRegion&) override {}
    void CopyTextureToBuffer(RHITextureHandle, RHIBufferHandle, const BufferImageCopyRegion&) override {}
    void TransitionTexture(RHITextureHandle, ResourceState, ResourceState) override {}
    void ResourceBarrier(std::span<const ResourceBarrierDesc>) override {}
    void WriteTimestamp(RHIQueryPoolHandle, uint32_t) override {}
    void BeginQuery(RHIQueryPoolHandle, uint32_t) override {}
    void EndQuery(RHIQueryPoolHandle, uint32_t) override {}
    void ResetQueryPool(RHIQueryPoolHandle, uint32_t, uint32_t) override {}
    void PushDebugGroup(std::string_view) override {}
    void PopDebugGroup() override {}
    void InsertDebugMarker(std::string_view) override {}

private:
    bool m_Recording = false;
};

class NullQueue final : public IRHIQueue {
public:
    explicit NullQueue(QueueType type) : m_Type(type) {}
    [[nodiscard]] QueueType GetType() const override { return m_Type; }
    RHIResult<void> Submit(IRHICommandList&) override { return RHIResult<void>::Success(); }
    RHIResult<void> Submit(const SubmitDesc&) override { return RHIResult<void>::Success(); }
    RHIResult<void> WaitIdle() override { return RHIResult<void>::Success(); }

private:
    QueueType m_Type = QueueType::Graphics;
};

class NullSwapchain final : public IRHISwapchain {
public:
    NullSwapchain(Extent2D extent, Format format)
        : m_Extent(extent)
        , m_Format(format)
    {
        for (uint32_t i = 0; i < 3; ++i) {
            m_Images.push_back(static_cast<RHITextureHandle>(AllocHandle()));
        }
    }

    [[nodiscard]] Extent2D GetExtent() const override { return m_Extent; }
    [[nodiscard]] Format GetFormat() const override { return m_Format; }
    [[nodiscard]] uint32_t GetImageCount() const override { return static_cast<uint32_t>(m_Images.size()); }
    [[nodiscard]] RHITextureHandle GetCurrentImage() const override {
        return m_Images.empty() ? RHITextureHandle::Invalid : m_Images[m_Index % m_Images.size()];
    }
    [[nodiscard]] uint32_t GetCurrentImageIndex() const override { return m_Index; }

    RHIResult<void> Resize(Extent2D extent) override {
        m_Extent = extent;
        return RHIResult<void>::Success();
    }

    RHIResult<uint32_t> AcquireNextImage() override {
        if (m_Images.empty()) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "NullSwapchain has no images.", "AcquireNextImage");
        }
        m_Index = (m_Index + 1) % static_cast<uint32_t>(m_Images.size());
        return m_Index;
    }

    RHIResult<void> Present() override { return RHIResult<void>::Success(); }

private:
    Extent2D m_Extent{};
    Format m_Format = Format::B8G8R8A8_UNORM;
    uint32_t m_Index = 0;
    std::vector<RHITextureHandle> m_Images;
};

struct NullBuffer {
    BufferDesc desc{};
    std::vector<uint8_t> storage;
    ResourceState state = ResourceState::Undefined;
};

struct NullTexture {
    TextureDesc desc{};
    std::vector<uint8_t> pixels;
    ResourceState state = ResourceState::Undefined;
};

struct NullCommandPool {
    CommandPoolDesc desc{};
    std::vector<std::unique_ptr<NullCommandList>> lists;
};

struct NullQueryPool {
    QueryPoolDesc desc{};
    std::vector<uint64_t> results;
};

struct NullFence {
    bool signaled = false;
};

class NullDevice final : public IRHIDevice {
public:
    explicit NullDevice(const DeviceDesc& desc)
        : m_GraphicsQueue(QueueType::Graphics)
        , m_ComputeQueue(QueueType::Compute)
        , m_TransferQueue(QueueType::Transfer)
        , m_FramesInFlight(desc.framesInFlight ? desc.framesInFlight : 2)
    {
        m_Caps.dynamicRendering = true;
        m_Caps.maxFramesInFlight = m_FramesInFlight;
        m_Caps.debugMarkers = true;
        m_Caps.maxPushConstantBytes = 128;
        m_Caps.maxBoundDescriptorSets = 8;
        m_Caps.maxColorAttachments = 8;
        m_Caps.maxTextureDimension2D = 16384;
        m_Caps.samplerAnisotropy = true;
        m_Caps.fillModeNonSolid = true;

        Extent2D extent = desc.headlessExtent.width && desc.headlessExtent.height
            ? desc.headlessExtent
            : Extent2D{1280, 720};
        m_Swapchain = std::make_unique<NullSwapchain>(extent, Format::B8G8R8A8_UNORM);
        m_Diagnostics.deviceCreateMs = 0.1;
        ++m_Diagnostics.resourcesCreated;
    }

    [[nodiscard]] bool IsValid() const override { return true; }
    [[nodiscard]] RHIBackend GetBackend() const override { return RHIBackend::Null; }
    [[nodiscard]] const RHICapabilities& GetCapabilities() const override { return m_Caps; }
    [[nodiscard]] const RHIDiagnostics& GetDiagnostics() const override { return m_Diagnostics; }
    void ResetDiagnostics() override { m_Diagnostics.Reset(); }

    [[nodiscard]] IRHIQueue& GetQueue(QueueType type) override {
        switch (type) {
        case QueueType::Compute: return m_ComputeQueue;
        case QueueType::Transfer: return m_TransferQueue;
        default: return m_GraphicsQueue;
        }
    }

    [[nodiscard]] IRHISwapchain* GetSwapchain() override { return m_Swapchain.get(); }

    [[nodiscard]] RHIResult<RHIBufferHandle> CreateBuffer(const BufferDesc& desc) override {
        if (desc.size == 0) {
            return RHIError::Make(RHIErrorCode::InvalidArgument, "Buffer size is 0.", "CreateBuffer");
        }
        const auto handle = static_cast<RHIBufferHandle>(AllocHandle());
        NullBuffer buffer;
        buffer.desc = desc;
        buffer.storage.resize(static_cast<size_t>(desc.size));
        m_Buffers.emplace(static_cast<uint64_t>(handle), std::move(buffer));
        ++m_Diagnostics.resourcesCreated;
        return handle;
    }

    RHIResult<void> DestroyBuffer(RHIBufferHandle handle) override {
        if (m_Buffers.erase(static_cast<uint64_t>(handle)) == 0) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown buffer.", "DestroyBuffer");
        }
        ++m_Diagnostics.resourcesDestroyed;
        ++m_Diagnostics.deferredDestroys;
        return RHIResult<void>::Success();
    }

    [[nodiscard]] void* MapBuffer(RHIBufferHandle handle) override {
        auto it = m_Buffers.find(static_cast<uint64_t>(handle));
        return it == m_Buffers.end() ? nullptr : it->second.storage.data();
    }

    void UnmapBuffer(RHIBufferHandle) override {}

    RHIResult<void> UpdateBuffer(RHIBufferHandle handle, std::span<const uint8_t> data, uint64_t offset) override {
        auto it = m_Buffers.find(static_cast<uint64_t>(handle));
        if (it == m_Buffers.end()) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown buffer.", "UpdateBuffer");
        }
        if (offset + data.size() > it->second.storage.size()) {
            return RHIError::Make(RHIErrorCode::InvalidArgument, "Update exceeds buffer size.", "UpdateBuffer");
        }
        std::memcpy(it->second.storage.data() + offset, data.data(), data.size());
        return RHIResult<void>::Success();
    }

    [[nodiscard]] RHIResult<RHITextureHandle> CreateTexture(const TextureDesc& desc) override {
        const auto handle = static_cast<RHITextureHandle>(AllocHandle());
        NullTexture tex{};
        tex.desc = desc;
        const size_t bytes = static_cast<size_t>(desc.extent.width) * desc.extent.height * desc.extent.depth * 4u;
        tex.pixels.resize(bytes);
        m_Textures.emplace(static_cast<uint64_t>(handle), std::move(tex));
        ++m_Diagnostics.resourcesCreated;
        return handle;
    }

    RHIResult<void> DestroyTexture(RHITextureHandle handle) override {
        if (m_Textures.erase(static_cast<uint64_t>(handle)) == 0) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture.", "DestroyTexture");
        }
        ++m_Diagnostics.resourcesDestroyed;
        return RHIResult<void>::Success();
    }

    [[nodiscard]] RHIResult<RHITextureViewHandle> CreateTextureView(const TextureViewDesc&) override {
        return static_cast<RHITextureViewHandle>(AllocHandle());
    }

    RHIResult<void> DestroyTextureView(RHITextureViewHandle) override { return RHIResult<void>::Success(); }

    RHIResult<void> UpdateTexture(RHITextureHandle handle, const TextureUpdateDesc& update) override {
        auto it = m_Textures.find(static_cast<uint64_t>(handle));
        if (it == m_Textures.end()) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture.", "UpdateTexture");
        }
        if (update.data.empty()) {
            return RHIError::Make(RHIErrorCode::InvalidArgument, "Empty texture update.", "UpdateTexture");
        }
        auto& pixels = it->second.pixels;
        const size_t copy = std::min(pixels.size(), update.data.size());
        std::memcpy(pixels.data(), update.data.data(), copy);
        it->second.state = ResourceState::ShaderResource;
        return RHIResult<void>::Success();
    }

    [[nodiscard]] RHIResult<RHISamplerHandle> CreateSampler(const SamplerDesc&) override {
        return static_cast<RHISamplerHandle>(AllocHandle());
    }

    RHIResult<void> DestroySampler(RHISamplerHandle) override { return RHIResult<void>::Success(); }

    [[nodiscard]] RHIResult<RHIShaderHandle> CreateShader(const ShaderDesc&) override {
        return static_cast<RHIShaderHandle>(AllocHandle());
    }

    RHIResult<void> DestroyShader(RHIShaderHandle) override { return RHIResult<void>::Success(); }

    [[nodiscard]] RHIResult<RHIDescriptorSetLayoutHandle> CreateDescriptorSetLayout(const DescriptorSetLayoutDesc&) override {
        return static_cast<RHIDescriptorSetLayoutHandle>(AllocHandle());
    }

    RHIResult<void> DestroyDescriptorSetLayout(RHIDescriptorSetLayoutHandle) override { return RHIResult<void>::Success(); }

    [[nodiscard]] RHIResult<RHIDescriptorPoolHandle> CreateDescriptorPool(const DescriptorPoolDesc&) override {
        const auto handle = static_cast<RHIDescriptorPoolHandle>(AllocHandle());
        m_Pools.insert(static_cast<uint64_t>(handle));
        return handle;
    }

    RHIResult<void> DestroyDescriptorPool(RHIDescriptorPoolHandle handle) override {
        m_Pools.erase(static_cast<uint64_t>(handle));
        return RHIResult<void>::Success();
    }

    RHIResult<void> ResetDescriptorPool(RHIDescriptorPoolHandle handle) override {
        if (m_Pools.find(static_cast<uint64_t>(handle)) == m_Pools.end()) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown descriptor pool.", "ResetDescriptorPool");
        }
        return RHIResult<void>::Success();
    }

    [[nodiscard]] RHIResult<RHIDescriptorSetHandle> AllocateDescriptorSet(const DescriptorSetAllocateDesc& desc) override {
        if (m_Pools.find(static_cast<uint64_t>(desc.pool)) == m_Pools.end()) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown descriptor pool.", "AllocateDescriptorSet");
        }
        return static_cast<RHIDescriptorSetHandle>(AllocHandle());
    }

    void UpdateDescriptorSets(std::span<const WriteDescriptorSet>) override {}

    [[nodiscard]] RHIResult<RHIPipelineLayoutHandle> CreatePipelineLayout(const PipelineLayoutDesc&) override {
        return static_cast<RHIPipelineLayoutHandle>(AllocHandle());
    }

    RHIResult<void> DestroyPipelineLayout(RHIPipelineLayoutHandle) override { return RHIResult<void>::Success(); }

    [[nodiscard]] RHIResult<RHIGraphicsPipelineHandle> CreateGraphicsPipeline(const GraphicsPipelineDesc&) override {
        return static_cast<RHIGraphicsPipelineHandle>(AllocHandle());
    }

    RHIResult<void> DestroyGraphicsPipeline(RHIGraphicsPipelineHandle) override { return RHIResult<void>::Success(); }

    [[nodiscard]] RHIResult<RHIComputePipelineHandle> CreateComputePipeline(const ComputePipelineDesc&) override {
        return static_cast<RHIComputePipelineHandle>(AllocHandle());
    }

    RHIResult<void> DestroyComputePipeline(RHIComputePipelineHandle) override { return RHIResult<void>::Success(); }

    [[nodiscard]] RHIResult<RHIFenceHandle> CreateFence(const FenceDesc& desc = {}) override {
        const auto handle = static_cast<RHIFenceHandle>(AllocHandle());
        m_Fences.emplace(static_cast<uint64_t>(handle), NullFence{desc.signaled});
        return handle;
    }

    RHIResult<void> DestroyFence(RHIFenceHandle handle) override {
        m_Fences.erase(static_cast<uint64_t>(handle));
        return RHIResult<void>::Success();
    }

    RHIResult<void> WaitForFences(std::span<const RHIFenceHandle> fences, bool, uint64_t) override {
        for (auto h : fences) {
            auto it = m_Fences.find(static_cast<uint64_t>(h));
            if (it != m_Fences.end()) {
                it->second.signaled = true;
            }
        }
        return RHIResult<void>::Success();
    }

    RHIResult<void> ResetFences(std::span<const RHIFenceHandle> fences) override {
        for (auto h : fences) {
            auto it = m_Fences.find(static_cast<uint64_t>(h));
            if (it != m_Fences.end()) {
                it->second.signaled = false;
            }
        }
        return RHIResult<void>::Success();
    }

    [[nodiscard]] bool IsFenceSignaled(RHIFenceHandle handle) override {
        auto it = m_Fences.find(static_cast<uint64_t>(handle));
        return it != m_Fences.end() && it->second.signaled;
    }

    [[nodiscard]] RHIResult<RHISemaphoreHandle> CreateSemaphore(const SemaphoreDesc&) override {
        return static_cast<RHISemaphoreHandle>(AllocHandle());
    }

    RHIResult<void> DestroySemaphore(RHISemaphoreHandle) override { return RHIResult<void>::Success(); }

    [[nodiscard]] RHIResult<RHICommandPoolHandle> CreateCommandPool(const CommandPoolDesc& desc = {}) override {
        const auto handle = static_cast<RHICommandPoolHandle>(AllocHandle());
        NullCommandPool pool{};
        pool.desc = desc;
        m_CommandPools.emplace(static_cast<uint64_t>(handle), std::move(pool));
        return handle;
    }

    RHIResult<void> DestroyCommandPool(RHICommandPoolHandle handle) override {
        m_CommandPools.erase(static_cast<uint64_t>(handle));
        return RHIResult<void>::Success();
    }

    RHIResult<void> ResetCommandPool(RHICommandPoolHandle handle) override {
        if (m_CommandPools.find(static_cast<uint64_t>(handle)) == m_CommandPools.end()) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown command pool.", "ResetCommandPool");
        }
        return RHIResult<void>::Success();
    }

    [[nodiscard]] RHIResult<IRHICommandList*> AllocateCommandList(RHICommandPoolHandle pool) override {
        auto it = m_CommandPools.find(static_cast<uint64_t>(pool));
        if (it == m_CommandPools.end()) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown command pool.", "AllocateCommandList");
        }
        auto cmd = std::make_unique<NullCommandList>();
        IRHICommandList* raw = cmd.get();
        it->second.lists.push_back(std::move(cmd));
        return raw;
    }

    [[nodiscard]] RHIResult<RHIQueryPoolHandle> CreateQueryPool(const QueryPoolDesc& desc) override {
        const auto handle = static_cast<RHIQueryPoolHandle>(AllocHandle());
        NullQueryPool pool{};
        pool.desc = desc;
        pool.results.assign(desc.count, 0);
        m_QueryPools.emplace(static_cast<uint64_t>(handle), std::move(pool));
        return handle;
    }

    RHIResult<void> DestroyQueryPool(RHIQueryPoolHandle handle) override {
        m_QueryPools.erase(static_cast<uint64_t>(handle));
        return RHIResult<void>::Success();
    }

    RHIResult<void> GetQueryPoolResults(
        RHIQueryPoolHandle handle,
        uint32_t firstQuery,
        uint32_t queryCount,
        std::span<uint64_t> results,
        bool) override
    {
        auto it = m_QueryPools.find(static_cast<uint64_t>(handle));
        if (it == m_QueryPools.end()) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown query pool.", "GetQueryPoolResults");
        }
        if (firstQuery + queryCount > it->second.desc.count || results.size() < queryCount) {
            return RHIError::Make(RHIErrorCode::InvalidArgument, "Query range invalid.", "GetQueryPoolResults");
        }
        for (uint32_t i = 0; i < queryCount; ++i) {
            results[i] = it->second.results[firstQuery + i];
        }
        return RHIResult<void>::Success();
    }

    [[nodiscard]] ResourceState GetTextureState(RHITextureHandle handle) const override {
        auto it = m_Textures.find(static_cast<uint64_t>(handle));
        return it == m_Textures.end() ? ResourceState::Undefined : it->second.state;
    }

    [[nodiscard]] ResourceState GetBufferState(RHIBufferHandle handle) const override {
        auto it = m_Buffers.find(static_cast<uint64_t>(handle));
        return it == m_Buffers.end() ? ResourceState::Undefined : it->second.state;
    }

    [[nodiscard]] IRHICommandList* BeginFrame() override {
        if (m_Swapchain) {
            (void)m_Swapchain->AcquireNextImage();
        }
        m_FrameCmd.Begin();
        return &m_FrameCmd;
    }

    RHIResult<void> Submit(IRHICommandList* commandList) override {
        if (!commandList) {
            return RHIError::Make(RHIErrorCode::InvalidArgument, "Null command list.", "Submit");
        }
        commandList->End();
        return m_GraphicsQueue.Submit(*commandList);
    }

    RHIResult<void> Present() override {
        return m_Swapchain ? m_Swapchain->Present() : RHIResult<void>::Success();
    }

    RHIResult<void> EndFrame() override {
        m_FrameSlot = (m_FrameSlot + 1) % m_FramesInFlight;
        ++m_FrameNumber;
        ++m_Diagnostics.lastFrame.frameIndex;
        TickDeferredDestruction();
        return RHIResult<void>::Success();
    }

    RHIResult<void> WaitIdle() override { return RHIResult<void>::Success(); }

    [[nodiscard]] uint64_t GetFrameNumber() const override { return m_FrameNumber; }
    [[nodiscard]] uint32_t GetFramesInFlight() const override { return m_FramesInFlight; }
    [[nodiscard]] uint32_t GetCurrentFrameSlot() const override { return m_FrameSlot; }

    void SetResourceName(RHIBufferHandle, std::string_view) override {}
    void SetResourceName(RHITextureHandle, std::string_view) override {}
    void TickDeferredDestruction() override {}

private:
    RHICapabilities m_Caps{};
    RHIDiagnostics m_Diagnostics{};
    NullQueue m_GraphicsQueue;
    NullQueue m_ComputeQueue;
    NullQueue m_TransferQueue;
    std::unique_ptr<NullSwapchain> m_Swapchain;
    NullCommandList m_FrameCmd;
    std::unordered_map<uint64_t, NullBuffer> m_Buffers;
    std::unordered_map<uint64_t, NullTexture> m_Textures;
    std::unordered_set<uint64_t> m_Pools;
    std::unordered_map<uint64_t, NullFence> m_Fences;
    std::unordered_map<uint64_t, NullCommandPool> m_CommandPools;
    std::unordered_map<uint64_t, NullQueryPool> m_QueryPools;
    uint32_t m_FramesInFlight = 2;
    uint32_t m_FrameSlot = 0;
    uint64_t m_FrameNumber = 0;
};

class NullRHIBackend final : public IRHI {
public:
    bool Initialize(const RHIInitDesc& desc = {}) override {
        m_Desc = desc;
        m_Caps.maxFramesInFlight = desc.framesInFlight ? desc.framesInFlight : 2;
        m_Caps.dynamicRendering = true;
        m_Caps.debugMarkers = true;
        m_Initialized = true;
        WE_LOG_INFO(we::LogCategory::Startup, "NullRHI initialized.");
        return true;
    }

    void Shutdown() override {
        m_Initialized = false;
        WE_LOG_INFO(we::LogCategory::Startup, "NullRHI shut down.");
    }

    [[nodiscard]] bool IsInitialized() const override { return m_Initialized; }
    [[nodiscard]] RHIBackend GetActiveBackend() const override { return RHIBackend::Null; }
    [[nodiscard]] const char* GetBackendName() const override { return "Null"; }

    [[nodiscard]] std::vector<AdapterDesc> EnumerateAdapters() const override {
        AdapterDesc a;
        a.index = 0;
        a.name = "Null Adapter";
        a.isDiscrete = false;
        a.isSoftware = true;
        return {a};
    }

    [[nodiscard]] RHIResult<std::unique_ptr<IRHIDevice>> CreateDevice(const DeviceDesc& desc) override {
        if (!m_Initialized) {
            return RHIError::Make(RHIErrorCode::NotInitialized, "NullRHI not initialized.", "CreateDevice");
        }
        DeviceDesc resolved = desc;
        if (m_Desc.headless || desc.headless) {
            resolved.headless = true;
            if (!resolved.headlessExtent.width) {
                resolved.headlessExtent = {1280, 720};
            }
        }
        return std::unique_ptr<IRHIDevice>(std::make_unique<NullDevice>(resolved));
    }

    [[nodiscard]] const RHICapabilities& GetCapabilities() const override { return m_Caps; }

private:
    bool m_Initialized = false;
    RHIInitDesc m_Desc{};
    RHICapabilities m_Caps{};
};

std::unique_ptr<IRHI> CreateNullRHI() {
    return std::make_unique<NullRHIBackend>();
}

static RHIBackendRegistrar g_NullRHIRegistrar(RHIBackend::Null, &CreateNullRHI, "Null");

} // namespace

class NullRHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        RHIFactory::Register(RHIBackend::Null, &CreateNullRHI, "Null");
    }

    void ShutdownModule() override {}
};

} // namespace we::rhi

IMPLEMENT_MODULE(we::rhi::NullRHIModule, WindEffects_NullRHI)

