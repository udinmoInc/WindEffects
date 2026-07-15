#include "Modules/IModuleInterface.h"
#include "RHI/RHIFactory.h"
#include "RHI/IRHI.h"
#include "RHI/Result.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"

#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>
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
    void Draw(uint32_t, uint32_t, uint32_t, uint32_t) override {}
    void DrawIndexed(uint32_t, uint32_t, uint32_t, int32_t, uint32_t) override {}
    void Dispatch(uint32_t, uint32_t, uint32_t) override {}
    void CopyBuffer(RHIBufferHandle, RHIBufferHandle, uint64_t, uint64_t, uint64_t) override {}
    void TransitionTexture(RHITextureHandle, ResourceState, ResourceState) override {}
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
};

struct NullTexture {
    TextureDesc desc{};
};

class NullDevice final : public IRHIDevice {
public:
    explicit NullDevice(const DeviceDesc& desc)
        : m_GraphicsQueue(QueueType::Graphics)
        , m_ComputeQueue(QueueType::Compute)
        , m_TransferQueue(QueueType::Transfer)
    {
        m_Caps.dynamicRendering = true;
        m_Caps.maxFramesInFlight = desc.framesInFlight ? desc.framesInFlight : 2;
        m_Caps.debugMarkers = true;

        Extent2D extent{1280, 720};
        if (we::platform::IsValid(desc.window)) {
            // Headless null still creates a logical swapchain for Present API coverage.
        }
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
        m_Textures.emplace(static_cast<uint64_t>(handle), NullTexture{desc});
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

    [[nodiscard]] RHIResult<RHISamplerHandle> CreateSampler(const SamplerDesc&) override {
        return static_cast<RHISamplerHandle>(AllocHandle());
    }

    RHIResult<void> DestroySampler(RHISamplerHandle) override { return RHIResult<void>::Success(); }

    [[nodiscard]] RHIResult<RHIShaderHandle> CreateShader(const ShaderDesc&) override {
        return static_cast<RHIShaderHandle>(AllocHandle());
    }

    RHIResult<void> DestroyShader(RHIShaderHandle) override { return RHIResult<void>::Success(); }

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
        ++m_Diagnostics.lastFrame.frameIndex;
        TickDeferredDestruction();
        return RHIResult<void>::Success();
    }

    RHIResult<void> WaitIdle() override { return RHIResult<void>::Success(); }

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
};

class NullRHIBackend final : public IRHI {
public:
    bool Initialize(const RHIInitDesc& desc = {}) override {
        m_Desc = desc;
        m_Caps.maxFramesInFlight = desc.framesInFlight ? desc.framesInFlight : 2;
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
        return std::unique_ptr<IRHIDevice>(std::make_unique<NullDevice>(desc));
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

// Register at DLL load so RHI::Initialize works before ModuleManager::LoadModule.
static RHIBackendRegistrar g_NullRHIRegistrar(RHIBackend::Null, &CreateNullRHI, "Null");

} // namespace

class NullRHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        // Ensured by static registrar; keep explicit register for late loads.
        RHIFactory::Register(RHIBackend::Null, &CreateNullRHI, "Null");
    }

    void ShutdownModule() override {}
};

} // namespace we::rhi

IMPLEMENT_MODULE(we::rhi::NullRHIModule, WindEffects_NullRHI)
