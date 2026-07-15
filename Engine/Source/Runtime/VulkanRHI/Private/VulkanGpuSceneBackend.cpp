#include "RHI/GpuBackends.h"
#include "RHI/RHIFactory.h"

#include "VulkanBackendShared.h"
#include "Core/DeviceContext.h"
#include "Core/SwapchainManager.h"
#include "Core/CommandContext.h"
#include "Core/FrameSync.h"
#include "Core/ImageBarriers.h"
#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Resource/ResourceManager.h"
#include "Resource/VulkanDepthTarget.h"
#include "Camera/CameraSystem.h"
#include "Camera/CameraUniform.h"
#include "Scene/SceneRenderer.h"
#include "Pipeline/GraphicsPipelineFactory.h"
#include "Graph/FrameContext.h"
#include "Passes/SkyPass.h"
#include "Lighting/SceneEnvironmentUniform.h"
#include "Shader/ShaderLibrary.h"

#include "Platform/Platform.h"

#include <cstring>
#include <memory>

#if WE_HAS_GLM
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#endif

namespace we::rhi::vulkan {
namespace {

constexpr uint32_t kMaxFramesInFlight = 2;

[[nodiscard]] RHITextureViewHandle FromVkImageView(VkImageView view) noexcept {
    return static_cast<RHITextureViewHandle>(reinterpret_cast<uintptr_t>(view));
}

[[nodiscard]] RHISamplerHandle FromVkSampler(VkSampler sampler) noexcept {
    return static_cast<RHISamplerHandle>(reinterpret_cast<uintptr_t>(sampler));
}

class VulkanGpuSceneBackend final : public IGpuSceneBackend {
public:
    bool Initialize(IRHIDevice& device, we::platform::WindowId window,
                    const we::platform::NativeWindowHandle& nativeWindow) override
    {
        using namespace we::runtime::renderer;

        if (!we::platform::IsValid(nativeWindow) || window == we::platform::WindowId::Invalid) {
            return false;
        }

        if (device.GetBackend() == RHIBackend::Vulkan && device.GetSwapchain() != nullptr) {
            WE_LOG_WARN(
                we::LogCategory::Renderer.data(),
                "VulkanGpuSceneBackend deferred: IRHIDevice already owns the window swapchain. "
                "Scene will present via RHI until the stacks are unified.");
            return false;
        }

        m_Window = window;

        try {
            ShaderLibrary::Get().Initialize(
                we::platform::Platform::Get().GetExecutableDirectory() + "/Engine/Shaders",
                we::platform::Platform::Get().GetExecutableDirectory() + "/Engine/Shaders/Bytecode");
        } catch (...) {
            WE_LOG_WARN(we::LogCategory::Renderer.data(), "ShaderLibrary init failed.");
        }

        m_DeviceContext = std::make_unique<DeviceContext>();
        DeviceContextConfig deviceConfig{};
        deviceConfig.nativeWindow = nativeWindow;
        m_DeviceContext->Init(deviceConfig);

        m_PipelineFactory = std::make_unique<GraphicsPipelineFactory>(m_DeviceContext.get());

        m_Swapchain = std::make_unique<SwapchainManager>();
        SwapchainManagerConfig swapConfig{};
        swapConfig.deviceContext = m_DeviceContext.get();
        swapConfig.window = window;
        swapConfig.nativeWindow = nativeWindow;
        m_Swapchain->Init(swapConfig);

        m_Resources = std::make_unique<ResourceManager>();
        ResourceManagerConfig resConfig{};
        resConfig.deviceContext = m_DeviceContext.get();
        m_Resources->Init(resConfig);

        m_Commands = std::make_unique<CommandContext>();
        CommandContextConfig cmdConfig{};
        cmdConfig.deviceContext = m_DeviceContext.get();
        cmdConfig.maxFramesInFlight = kMaxFramesInFlight;
        m_Commands->Init(cmdConfig);

        m_FrameSync = std::make_unique<FrameSync>();
        FrameSyncConfig syncConfig{};
        syncConfig.deviceContext = m_DeviceContext.get();
        syncConfig.maxFramesInFlight = kMaxFramesInFlight;
        m_FrameSync->Init(syncConfig);
        m_Swapchain->SetFrameSync(m_FrameSync.get());

        m_Camera = std::make_unique<CameraSystem>();
        CameraSystemConfig cameraConfig{};
        cameraConfig.deviceContext = m_DeviceContext.get();
        cameraConfig.resourceManager = m_Resources.get();
        cameraConfig.maxFramesInFlight = kMaxFramesInFlight;
        m_Camera->Init(cameraConfig);

        m_SceneRenderer = std::make_unique<SceneRenderer>();
        SceneRendererConfig sceneConfig{};
        sceneConfig.deviceContext = m_DeviceContext.get();
        sceneConfig.resourceManager = m_Resources.get();
        sceneConfig.pipelineFactory = m_PipelineFactory.get();
        sceneConfig.maxFramesInFlight = kMaxFramesInFlight;
        sceneConfig.colorFormat = m_Swapchain->GetImageFormat();
        sceneConfig.depthFormat = VK_FORMAT_D32_SFLOAT;
        m_SceneRenderer->Init(sceneConfig);

        m_SkyPass = std::make_unique<SkyPass>();
        SkyPassConfig skyConfig{};
        skyConfig.deviceContext = m_DeviceContext.get();
        skyConfig.pipelineFactory = m_PipelineFactory.get();
        skyConfig.cameraDescriptorSetLayout = m_Camera->GetDescriptorSetLayout();
        skyConfig.environmentDescriptorSetLayout =
            m_SceneRenderer->GetEnvironmentUniform()->GetDescriptorSetLayout();
        skyConfig.colorFormat = m_Swapchain->GetImageFormat();
        m_SkyPass->Init(skyConfig);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        vkCreateSampler(m_DeviceContext->GetDevice(), &samplerInfo, nullptr, &m_ViewportSampler);

        auto& shared = VulkanBackendShared::Get();
        shared.deviceContext = m_DeviceContext.get();
        shared.resourceManager = m_Resources.get();
        shared.swapchain = m_Swapchain.get();
        shared.commands = m_Commands.get();

        m_Ready = true;
        WE_LOG_INFO(we::LogCategory::Startup, "VulkanGpuSceneBackend initialized.");
        return true;
    }

    void Shutdown() override {
        if (!m_Ready) {
            return;
        }
        WaitIdle();

        auto& shared = VulkanBackendShared::Get();
        if (shared.deviceContext == m_DeviceContext.get()) {
            shared.deviceContext = nullptr;
            shared.resourceManager = nullptr;
            shared.swapchain = nullptr;
            shared.commands = nullptr;
        }

        DestroyViewportTargets();
        if (m_ViewportSampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_DeviceContext->GetDevice(), m_ViewportSampler, nullptr);
            m_ViewportSampler = VK_NULL_HANDLE;
        }
        if (m_SkyPass) {
            m_SkyPass->Shutdown();
        }
        if (m_SceneRenderer) {
            m_SceneRenderer->Shutdown();
        }
        if (m_Camera) {
            m_Camera->Shutdown();
        }
        if (m_FrameSync) {
            m_FrameSync->Shutdown();
        }
        if (m_Commands) {
            m_Commands->Shutdown();
        }
        if (m_Resources) {
            m_Resources->Shutdown();
        }
        if (m_Swapchain) {
            m_Swapchain->Shutdown();
        }
        if (m_DeviceContext) {
            m_DeviceContext->Shutdown();
        }
        m_SkyPass.reset();
        m_SceneRenderer.reset();
        m_PipelineFactory.reset();
        m_Camera.reset();
        m_FrameSync.reset();
        m_Commands.reset();
        m_Resources.reset();
        m_Swapchain.reset();
        m_DeviceContext.reset();
        m_Ready = false;
    }

    [[nodiscard]] bool IsReady() const override { return m_Ready; }

    bool BeginFrame(uint32_t& outImageIndex) override {
        if (!m_Ready) {
            return false;
        }
        using namespace we::runtime::renderer;
        m_FrameSync->WaitForFrame(m_FrameSlot);
        if (!m_Swapchain->AcquireNextImage(m_FrameSlot, m_ImageIndex)) {
            return false;
        }
        outImageIndex = m_ImageIndex;
        m_Cmd = m_Commands->BeginFrame(m_FrameSlot);
        return m_Cmd != VK_NULL_HANDLE;
    }

    void RenderScene(const SceneFrameParams& params) override {
        if (!m_Ready || !m_Cmd) {
            return;
        }
        using namespace we::runtime::renderer;

        EnsureViewportTargets(
            params.viewportExtent.width ? params.viewportExtent.width : m_ViewportW,
            params.viewportExtent.height ? params.viewportExtent.height : m_ViewportH);

        // Keep swapchain ready for the UI overlay (loadOp = LOAD).
        const auto& images = m_Swapchain->GetImages();
        if (m_ImageIndex < images.size()) {
            TransitionImageLayout(
                m_Cmd,
                images[m_ImageIndex],
                m_HasPresented ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                m_HasPresented ? VK_ACCESS_MEMORY_READ_BIT : 0,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                m_HasPresented ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);

            VkClearColorValue clear{};
            clear.float32[0] = params.clearColor[0];
            clear.float32[1] = params.clearColor[1];
            clear.float32[2] = params.clearColor[2];
            clear.float32[3] = params.clearColor[3];
            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.levelCount = 1;
            range.layerCount = 1;
            vkCmdClearColorImage(
                m_Cmd, images[m_ImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);

            TransitionImageLayout(
                m_Cmd,
                images[m_ImageIndex],
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        }

        if (m_ViewportColorImage == VK_NULL_HANDLE || m_ViewportColorView == VK_NULL_HANDLE
            || m_ViewportW == 0 || m_ViewportH == 0 || !m_SkyPass || !m_Camera || !m_SceneRenderer) {
            return;
        }

        CameraUniform cameraUBO{};
#if WE_HAS_GLM
        std::memcpy(glm::value_ptr(cameraUBO.view), params.cameraView, sizeof(params.cameraView));
        std::memcpy(glm::value_ptr(cameraUBO.proj), params.cameraProj, sizeof(params.cameraProj));
        cameraUBO.position = glm::vec3(
            params.cameraPosition[0], params.cameraPosition[1], params.cameraPosition[2]);
        cameraUBO.invViewProj = glm::inverse(cameraUBO.proj * cameraUBO.view);
#else
        std::memcpy(cameraUBO.view, params.cameraView, sizeof(params.cameraView));
        std::memcpy(cameraUBO.proj, params.cameraProj, sizeof(params.cameraProj));
        std::memcpy(cameraUBO.position, params.cameraPosition, sizeof(params.cameraPosition));
        std::memcpy(cameraUBO.invViewProj, params.cameraInvViewProj, sizeof(params.cameraInvViewProj));
#endif
        cameraUBO.padding = params.cameraPadding;
        m_Camera->Upload(m_FrameSlot, cameraUBO);

        SceneEnvironmentUniform env{};
        env.eyeAltitude = params.cameraPosition[1];
        m_SceneRenderer->GetEnvironmentUniform()->Upload(m_FrameSlot, env);

        TransitionImageLayout(
            m_Cmd,
            m_ViewportColorImage,
            m_ViewportColorLayout,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            m_ViewportColorLayout == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            m_ViewportColorLayout == VK_IMAGE_LAYOUT_UNDEFINED
                ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        m_ViewportColorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        FrameContext frame{};
        frame.frameIndex = m_FrameSlot;
        frame.imageIndex = m_ImageIndex;
        frame.commandBuffer = m_Cmd;
        frame.extent = {m_ViewportW, m_ViewportH};
        frame.sceneRenderArea = {{0, 0}, {m_ViewportW, m_ViewportH}};
        frame.colorImage = m_ViewportColorImage;
        frame.colorImageView = m_ViewportColorView;
        frame.colorFormat = m_Swapchain->GetImageFormat();
        frame.camera = m_Camera.get();
        frame.environmentUniform = m_SceneRenderer->GetEnvironmentUniform();
        frame.sceneRenderer = m_SceneRenderer.get();

        m_SkyPass->Execute(frame);

        TransitionImageLayout(
            m_Cmd,
            m_ViewportColorImage,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        m_ViewportColorLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    void SubmitAndPresent() override {
        if (!m_Ready || !m_Cmd) {
            return;
        }
        using namespace we::runtime::renderer;
        const auto& images = m_Swapchain->GetImages();
        if (m_ImageIndex < images.size()) {
            TransitionImageLayout(
                m_Cmd,
                images[m_ImageIndex],
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                0,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        }

        m_Commands->EndFrame(m_FrameSlot);
        m_FrameSync->ResetFence(m_FrameSlot);
        m_Swapchain->SubmitFrame(m_FrameSlot, m_ImageIndex, m_Cmd);
        m_Swapchain->Present(m_FrameSlot, m_ImageIndex);
        m_HasPresented = true;
        m_Cmd = VK_NULL_HANDLE;
        m_FrameSlot = (m_FrameSlot + 1) % kMaxFramesInFlight;
    }

    void WaitIdle() override {
        if (m_DeviceContext && m_DeviceContext->GetDevice()) {
            vkDeviceWaitIdle(m_DeviceContext->GetDevice());
        }
    }

    void SetViewportColor(RHITextureHandle) override {}
    void SetViewportDepth(RHITextureHandle) override {}
    void SetViewportExtent(uint32_t w, uint32_t h) override {
        m_ViewportW = w;
        m_ViewportH = h;
    }
    void SetViewportBlitRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override {
        m_BlitX = x;
        m_BlitY = y;
        m_BlitW = w;
        m_BlitH = h;
    }

    [[nodiscard]] RHITextureViewHandle GetViewportColorView() const override {
        return FromVkImageView(m_ViewportColorView);
    }
    [[nodiscard]] RHISamplerHandle GetViewportColorSampler() const override {
        return FromVkSampler(m_ViewportSampler);
    }

    [[nodiscard]] uint32_t GetSwapchainWidth() const override {
        return m_Swapchain ? m_Swapchain->GetExtent().width : 0;
    }
    [[nodiscard]] uint32_t GetSwapchainHeight() const override {
        return m_Swapchain ? m_Swapchain->GetExtent().height : 0;
    }
    [[nodiscard]] Format GetSwapchainFormat() const override {
        if (!m_Swapchain) {
            return Format::Unknown;
        }
        const uint32_t raw = static_cast<uint32_t>(m_Swapchain->GetImageFormat());
        switch (raw) {
        case 37: return Format::R8G8B8A8_UNORM;
        case 44: return Format::B8G8R8A8_UNORM;
        default: return Format::B8G8R8A8_UNORM;
        }
    }
    [[nodiscard]] uint32_t GetCurrentFrameIndex() const override { return m_FrameSlot; }
    [[nodiscard]] uint32_t GetCurrentImageIndex() const override { return m_ImageIndex; }
    [[nodiscard]] IRHICommandList* GetActiveCommandList() override { return nullptr; }
    [[nodiscard]] void* GetNativeCommandBuffer() const override {
        return reinterpret_cast<void*>(m_Cmd);
    }

private:
    void DestroyViewportTargets() {
        if (!m_DeviceContext || !m_Resources) {
            return;
        }
        if (m_ViewportColorView != VK_NULL_HANDLE) {
            m_Resources->DestroyImageView(m_ViewportColorView);
            m_ViewportColorView = VK_NULL_HANDLE;
        }
        if (m_ViewportColorImage != VK_NULL_HANDLE) {
            m_Resources->DestroyImage(m_ViewportColorImage, m_ViewportColorMemory);
            m_ViewportColorImage = VK_NULL_HANDLE;
            m_ViewportColorMemory = VK_NULL_HANDLE;
        }
        m_ViewportColorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_ViewportAllocatedW = 0;
        m_ViewportAllocatedH = 0;
    }

    void EnsureViewportTargets(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0 || !m_Resources || !m_Swapchain) {
            return;
        }
        m_ViewportW = width;
        m_ViewportH = height;
        if (m_ViewportColorImage != VK_NULL_HANDLE
            && m_ViewportAllocatedW == width
            && m_ViewportAllocatedH == height) {
            return;
        }

        WaitIdle();
        DestroyViewportTargets();

        m_Resources->CreateImage(
            width,
            height,
            m_Swapchain->GetImageFormat(),
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_ViewportColorImage,
            m_ViewportColorMemory);
        if (m_ViewportColorImage == VK_NULL_HANDLE) {
            return;
        }
        m_ViewportColorView = m_Resources->CreateImageView(
            m_ViewportColorImage, m_Swapchain->GetImageFormat(), VK_IMAGE_ASPECT_COLOR_BIT);
        m_ViewportAllocatedW = width;
        m_ViewportAllocatedH = height;
        m_ViewportColorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    bool m_Ready = false;
    bool m_HasPresented = false;
    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;
    uint32_t m_FrameSlot = 0;
    uint32_t m_ImageIndex = 0;
    uint32_t m_ViewportW = 0, m_ViewportH = 0;
    uint32_t m_ViewportAllocatedW = 0, m_ViewportAllocatedH = 0;
    uint32_t m_BlitX = 0, m_BlitY = 0, m_BlitW = 0, m_BlitH = 0;
    VkCommandBuffer m_Cmd = VK_NULL_HANDLE;

    VkImage m_ViewportColorImage = VK_NULL_HANDLE;
    VkDeviceMemory m_ViewportColorMemory = VK_NULL_HANDLE;
    VkImageView m_ViewportColorView = VK_NULL_HANDLE;
    VkSampler m_ViewportSampler = VK_NULL_HANDLE;
    VkImageLayout m_ViewportColorLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    std::unique_ptr<we::runtime::renderer::DeviceContext> m_DeviceContext;
    std::unique_ptr<we::runtime::renderer::SwapchainManager> m_Swapchain;
    std::unique_ptr<we::runtime::renderer::ResourceManager> m_Resources;
    std::unique_ptr<we::runtime::renderer::CommandContext> m_Commands;
    std::unique_ptr<we::runtime::renderer::FrameSync> m_FrameSync;
    std::unique_ptr<we::runtime::renderer::CameraSystem> m_Camera;
    std::unique_ptr<we::runtime::renderer::GraphicsPipelineFactory> m_PipelineFactory;
    std::unique_ptr<we::runtime::renderer::SceneRenderer> m_SceneRenderer;
    std::unique_ptr<we::runtime::renderer::SkyPass> m_SkyPass;
};

std::unique_ptr<IGpuSceneBackend> CreateVulkanGpuSceneBackend() {
    return std::make_unique<VulkanGpuSceneBackend>();
}

struct VulkanSceneRegistrar {
    VulkanSceneRegistrar() {
        GpuBackendRegistry::RegisterScene(&CreateVulkanGpuSceneBackend);
    }
};

static VulkanSceneRegistrar g_VulkanSceneRegistrar;

} // namespace
} // namespace we::rhi::vulkan
