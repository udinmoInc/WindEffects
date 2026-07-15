#include "RHI/GpuBackends.h"
#include "RHI/RHIFactory.h"

#include <atomic>
#include <memory>

namespace we::rhi {
namespace {

std::atomic<uint64_t> g_NullNextTextureId{1};

class NullGpuSceneBackend final : public IGpuSceneBackend {
public:
    bool Initialize(IRHIDevice& device, we::platform::WindowId, const we::platform::NativeWindowHandle&) override {
        m_Device = &device;
        m_Ready = true;
        return true;
    }

    void Shutdown() override {
        m_Device = nullptr;
        m_Ready = false;
    }

    [[nodiscard]] bool IsReady() const override { return m_Ready; }

    bool BeginFrame(uint32_t& outImageIndex) override {
        if (!m_Ready || !m_Device) {
            return false;
        }
        if (auto* swap = m_Device->GetSwapchain()) {
            auto acquire = swap->AcquireNextImage();
            if (!acquire) {
                return false;
            }
            outImageIndex = *acquire;
            m_ImageIndex = *acquire;
            return true;
        }
        outImageIndex = 0;
        m_ImageIndex = 0;
        return true;
    }

    void RenderScene(const SceneFrameParams&) override {}

    void SubmitAndPresent() override {
        if (m_Device) {
            (void)m_Device->Present();
            (void)m_Device->EndFrame();
        }
    }

    void WaitIdle() override {
        if (m_Device) {
            (void)m_Device->WaitIdle();
        }
    }

    void SetViewportColor(RHITextureHandle) override {}
    void SetViewportDepth(RHITextureHandle) override {}
    void SetViewportExtent(uint32_t w, uint32_t h) override { m_Extent = {w, h}; }
    void SetViewportBlitRect(uint32_t, uint32_t, uint32_t, uint32_t) override {}
    [[nodiscard]] RHITextureViewHandle GetViewportColorView() const override {
        return RHITextureViewHandle::Invalid;
    }
    [[nodiscard]] RHISamplerHandle GetViewportColorSampler() const override {
        return RHISamplerHandle::Invalid;
    }

    [[nodiscard]] uint32_t GetSwapchainWidth() const override {
        if (m_Device && m_Device->GetSwapchain()) {
            return m_Device->GetSwapchain()->GetExtent().width;
        }
        return m_Extent.width;
    }
    [[nodiscard]] uint32_t GetSwapchainHeight() const override {
        if (m_Device && m_Device->GetSwapchain()) {
            return m_Device->GetSwapchain()->GetExtent().height;
        }
        return m_Extent.height;
    }
    [[nodiscard]] Format GetSwapchainFormat() const override {
        if (m_Device && m_Device->GetSwapchain()) {
            return m_Device->GetSwapchain()->GetFormat();
        }
        return Format::B8G8R8A8_UNORM;
    }
    [[nodiscard]] uint32_t GetCurrentFrameIndex() const override { return m_FrameIndex; }
    [[nodiscard]] uint32_t GetCurrentImageIndex() const override { return m_ImageIndex; }
    [[nodiscard]] IRHICommandList* GetActiveCommandList() override { return nullptr; }
    [[nodiscard]] void* GetNativeCommandBuffer() const override { return nullptr; }

private:
    IRHIDevice* m_Device = nullptr;
    Extent2D m_Extent{};
    uint32_t m_FrameIndex = 0;
    uint32_t m_ImageIndex = 0;
    bool m_Ready = false;
};

class NullGpuUIBackend final : public IGpuUIBackend {
public:
    bool Initialize(IRHIDevice& device, Format, uint32_t framesInFlight) override {
        m_Device = &device;
        m_Frames = framesInFlight ? framesInFlight : 2;
        m_DummySet = static_cast<RHIDescriptorSetHandle>(g_NullNextTextureId.fetch_add(1));
        m_DefaultSampler = static_cast<RHISamplerHandle>(1);
        return true;
    }

    void Shutdown() override { m_Device = nullptr; }

    [[nodiscard]] RHIDescriptorSetHandle RegisterTexture(RHITextureViewHandle, RHISamplerHandle) override {
        return static_cast<RHIDescriptorSetHandle>(g_NullNextTextureId.fetch_add(1));
    }

    void UpdateTexture(RHIDescriptorSetHandle, RHITextureViewHandle, RHISamplerHandle) override {}
    void UnregisterTexture(RHIDescriptorSetHandle) override {}

    [[nodiscard]] RHIDescriptorSetHandle UploadRgbaTexture(
        uint32_t, uint32_t, std::span<const uint8_t>, bool) override {
        return static_cast<RHIDescriptorSetHandle>(g_NullNextTextureId.fetch_add(1));
    }

    [[nodiscard]] RHIDescriptorSetHandle GetDummyTexture() const override { return m_DummySet; }
    [[nodiscard]] RHISamplerHandle GetDefaultSampler() const override { return m_DefaultSampler; }

    void BeginFrame(const FramePresentParams&) override {}
    void SubmitDrawList(const UIDrawList& list, uint32_t) override {
        m_LastVertices = static_cast<uint32_t>(list.vertices.size());
        m_LastBatches = static_cast<uint32_t>(list.batches.size());
    }
    void EndFrame() override {}

private:
    IRHIDevice* m_Device = nullptr;
    uint32_t m_Frames = 2;
    RHIDescriptorSetHandle m_DummySet = RHIDescriptorSetHandle::Invalid;
    RHISamplerHandle m_DefaultSampler = RHISamplerHandle::Invalid;
    uint32_t m_LastVertices = 0;
    uint32_t m_LastBatches = 0;
};

std::unique_ptr<IGpuSceneBackend> CreateNullGpuSceneBackend() {
    return std::make_unique<NullGpuSceneBackend>();
}

std::unique_ptr<IGpuUIBackend> CreateNullGpuUIBackend() {
    return std::make_unique<NullGpuUIBackend>();
}

struct NullGpuRegistrar {
    NullGpuRegistrar() {
        GpuBackendRegistry::RegisterScene(&CreateNullGpuSceneBackend);
        GpuBackendRegistry::RegisterUI(&CreateNullGpuUIBackend);
    }
};

static NullGpuRegistrar g_NullGpuRegistrar;

} // namespace
} // namespace we::rhi
