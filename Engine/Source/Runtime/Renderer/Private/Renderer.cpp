#include "Renderer/Renderer.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Renderer/Validation.h"
#include "Platform/Platform.h"
#include "RHI/Desc.h"
#include "RHI/GpuBackends.h"
#include "RHI/RHI.h"

#include <cstring>

namespace we::runtime::renderer {
namespace {

Renderer* s_Instance = nullptr;

} // namespace

Renderer& Renderer::Get() {
    WE_VALIDATE_INIT(s_Instance != nullptr, "Renderer", "Renderer instance is null.");
    return *s_Instance;
}

Renderer::Renderer() {
    WE_VALIDATE_INIT(s_Instance == nullptr, "Renderer", "Renderer instance already exists.");
    s_Instance = this;
}

Renderer::~Renderer() {
    Shutdown();
    s_Instance = nullptr;
}

void Renderer::Init(we::platform::WindowId window) {
    WE_VALIDATE_INIT(!m_Initialized, "Renderer", "Renderer already initialized.");
    WE_VALIDATE_INIT(window != we::platform::WindowId::Invalid, "Renderer", "Window is invalid.");

    m_Window = window;
    const auto nativeWindow = we::platform::Platform::Get().GetNativeWindowHandle(window);
    WE_VALIDATE_INIT(we::platform::IsValid(nativeWindow), "Renderer", "Native window handle is invalid.");

    we::rhi::RHIInitDesc rhiInit{};
    // When a Vulkan GPU scene backend is registered it owns the real VkInstance/Device
    // and Win32 surface. Creating a second VulkanRHI device fights volk's global
    // dispatch table and access-violates on the first UI frame.
    rhiInit.preferredBackend = we::rhi::GpuBackendRegistry::HasScene()
        ? we::rhi::RHIBackend::Null
        : we::rhi::RHIBackend::Auto;
    rhiInit.appName = "WindEffects";
    rhiInit.framesInFlight = kMaxFramesInFlight;
    (void)we::rhi::RHI::Initialize(rhiInit);

    we::rhi::DeviceDesc deviceDesc{};
    deviceDesc.framesInFlight = kMaxFramesInFlight;
    deviceDesc.vsync = true;
    auto deviceResult = we::rhi::RHI::Get().CreateDevice(deviceDesc);
    WE_VALIDATE_INIT(deviceResult.Ok() && deviceResult.value, "Renderer",
        deviceResult.Ok() ? "RHI CreateDevice returned null." : deviceResult.error.message.c_str());
    m_RHIDevice = std::move(deviceResult.value);

    m_SceneBackend = we::rhi::GpuBackendRegistry::CreateScene();
    if (m_SceneBackend) {
        if (!m_SceneBackend->Initialize(*m_RHIDevice, window, nativeWindow)) {
            WE_LOG_WARN(we::LogCategory::Renderer.data(),
                "IGpuSceneBackend Initialize failed; falling back to Vulkan RHI present.");
            m_SceneBackend.reset();

            we::rhi::RHIInitDesc vulkanInit = rhiInit;
            vulkanInit.preferredBackend = we::rhi::RHIBackend::Vulkan;
            (void)we::rhi::RHI::Shutdown();
            (void)we::rhi::RHI::Initialize(vulkanInit);

            we::rhi::DeviceDesc windowDeviceDesc = deviceDesc;
            windowDeviceDesc.windowId = window;
            windowDeviceDesc.window = nativeWindow;
            auto fallback = we::rhi::RHI::Get().CreateDevice(windowDeviceDesc);
            WE_VALIDATE_INIT(fallback.Ok() && fallback.value, "Renderer",
                fallback.Ok() ? "Fallback Vulkan CreateDevice returned null." : fallback.error.message.c_str());
            m_RHIDevice = std::move(fallback.value);
        }
    } else if (m_RHIDevice->GetBackend() == we::rhi::RHIBackend::Null) {
        WE_LOG_INFO(we::LogCategory::Renderer.data(),
            "No IGpuSceneBackend registered. Creating Vulkan RHI window swapchain.");
        we::rhi::RHIInitDesc vulkanInit = rhiInit;
        vulkanInit.preferredBackend = we::rhi::RHIBackend::Vulkan;
        (void)we::rhi::RHI::Shutdown();
        (void)we::rhi::RHI::Initialize(vulkanInit);

        we::rhi::DeviceDesc windowDeviceDesc = deviceDesc;
        windowDeviceDesc.windowId = window;
        windowDeviceDesc.window = nativeWindow;
        auto windowDevice = we::rhi::RHI::Get().CreateDevice(windowDeviceDesc);
        WE_VALIDATE_INIT(windowDevice.Ok() && windowDevice.value, "Renderer",
            windowDevice.Ok() ? "Vulkan CreateDevice returned null." : windowDevice.error.message.c_str());
        m_RHIDevice = std::move(windowDevice.value);
    }

    m_Initialized = true;
    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        std::string("Renderer initialized via RHI backend: ") +
        we::rhi::ToString(m_RHIDevice->GetBackend()) +
        (m_SceneBackend ? " (scene GPU stack owns present)" : ""));
}

void Renderer::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    if (m_SceneBackend) {
        m_SceneBackend->WaitIdle();
        m_SceneBackend->Shutdown();
        m_SceneBackend.reset();
    }
    if (m_RHIDevice) {
        (void)m_RHIDevice->WaitIdle();
        m_RHIDevice.reset();
    }
    m_FrameCmd = nullptr;
    m_Initialized = false;
    m_FrameActive = false;
}

bool Renderer::IsGpuReady() const {
    return m_Initialized && m_RHIDevice && m_RHIDevice->IsValid();
}

void Renderer::ResetPresentPathAudit() {
    m_AcquiredImageIndex = UINT32_MAX;
    m_SceneImageIndex = UINT32_MAX;
    m_UiImageIndex = UINT32_MAX;
    m_OverlayPassRan = false;
    m_OverlayPassEnded = false;
}

void Renderer::RecordUiPresentPath(uint32_t imageIndex) {
    m_OverlayPassRan = true;
    m_UiImageIndex = imageIndex;
}

void Renderer::MarkOverlayPassEnded() {
    m_OverlayPassEnded = true;
}

bool Renderer::BeginFrame() {
    WE_VALIDATE_RENDER(m_Initialized, "Renderer::BeginFrame", "Renderer not initialized.");
    WE_VALIDATE_RENDER(!m_FrameActive, "Renderer::BeginFrame", "Frame already active.");

    // When the scene backend owns the Win32 surface + DeviceContext, do not drive the
    // separate RHI VulkanDevice frame cycle. That device has its own VkInstance/Device;
    // mixing it with the scene stack corrupts volk's global device dispatch table.
    if (m_SceneBackend) {
        m_FrameCmd = nullptr;
        if (!m_SceneBackend->BeginFrame(m_CurrentImageIndex)) {
            return false;
        }
        m_CurrentFrame = m_SceneBackend->GetCurrentFrameIndex();
        m_CurrentImageIndex = m_SceneBackend->GetCurrentImageIndex();
        m_FrameActive = true;
        ++m_PresentAuditFrameNumber;
        ResetPresentPathAudit();
        m_AcquiredImageIndex = m_CurrentImageIndex;
        return true;
    }

    m_FrameCmd = m_RHIDevice->BeginFrame();
    if (!m_FrameCmd) {
        return false;
    }

    if (auto* swap = m_RHIDevice->GetSwapchain()) {
        auto acquire = swap->AcquireNextImage();
        if (!acquire) {
            (void)m_RHIDevice->EndFrame();
            m_FrameCmd = nullptr;
            return false;
        }
        m_CurrentImageIndex = *acquire;
    }

    m_FrameActive = true;
    ++m_PresentAuditFrameNumber;
    ResetPresentPathAudit();
    m_AcquiredImageIndex = m_CurrentImageIndex;
    return true;
}

void Renderer::RenderScene() {
    WE_VALIDATE_RENDER(m_Initialized && m_FrameActive, "Renderer::RenderScene", "No active frame.");
    if (!m_SceneBackend) {
        return;
    }

    we::rhi::SceneFrameParams params{};
    params.frameIndex = m_CurrentFrame;
    params.imageIndex = m_CurrentImageIndex;
    params.viewportExtent = {m_ViewportTargetWidth, m_ViewportTargetHeight};
    params.colorTarget = m_ViewportColorTexture;
    params.depthTarget = m_ViewportDepthTexture;
#if WE_HAS_GLM
    params.cameraPosition[0] = m_LastCamera.position.x;
    params.cameraPosition[1] = m_LastCamera.position.y;
    params.cameraPosition[2] = m_LastCamera.position.z;
    std::memcpy(params.cameraView, &m_LastCamera.view[0][0], sizeof(params.cameraView));
    std::memcpy(params.cameraProj, &m_LastCamera.proj[0][0], sizeof(params.cameraProj));
    std::memcpy(params.cameraInvViewProj, &m_LastCamera.invViewProj[0][0], sizeof(params.cameraInvViewProj));
#else
    params.cameraPosition[0] = m_LastCamera.position[0];
    params.cameraPosition[1] = m_LastCamera.position[1];
    params.cameraPosition[2] = m_LastCamera.position[2];
    std::memcpy(params.cameraView, m_LastCamera.view, sizeof(params.cameraView));
    std::memcpy(params.cameraProj, m_LastCamera.proj, sizeof(params.cameraProj));
    std::memcpy(params.cameraInvViewProj, m_LastCamera.invViewProj, sizeof(params.cameraInvViewProj));
#endif
    params.cameraPadding = m_LastCamera.padding;
    m_SceneBackend->RenderScene(params);
    m_SceneImageIndex = m_CurrentImageIndex;
}

void Renderer::SubmitAndPresent() {
    WE_VALIDATE_RENDER(m_Initialized && m_FrameActive, "Renderer::SubmitAndPresent", "No active frame.");

    if (m_SceneBackend) {
        m_SceneBackend->SubmitAndPresent();
        m_FrameActive = false;
        m_CurrentFrame = (m_CurrentFrame + 1) % kMaxFramesInFlight;
        return;
    }

    if (m_FrameCmd) {
        (void)m_RHIDevice->Submit(m_FrameCmd);
        m_FrameCmd = nullptr;
    }
    (void)m_RHIDevice->Present();
    (void)m_RHIDevice->EndFrame();
    m_FrameActive = false;
    m_CurrentFrame = (m_CurrentFrame + 1) % kMaxFramesInFlight;
}

void Renderer::RenderFrame() {
    if (!BeginFrame()) {
        return;
    }
    RenderScene();
    SubmitAndPresent();
}

void Renderer::UploadCameraUniform(const CameraUniform& uniform) {
    m_LastCamera = uniform;
}

void Renderer::InsertOverlayPassBarrier() {}

void Renderer::SetViewportRenderTargetSize(uint32_t width, uint32_t height) {
    m_ViewportTargetWidth = width;
    m_ViewportTargetHeight = height;
    if (m_SceneBackend) {
        m_SceneBackend->SetViewportExtent(width, height);
    }
}

void Renderer::SetViewportBlitRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    m_ViewportBlitX = x;
    m_ViewportBlitY = y;
    m_ViewportBlitW = width;
    m_ViewportBlitH = height;
    if (m_SceneBackend) {
        m_SceneBackend->SetViewportBlitRect(x, y, width, height);
    }
}

void Renderer::GetViewportBlitRect(uint32_t& x, uint32_t& y, uint32_t& width, uint32_t& height) const {
    x = m_ViewportBlitX;
    y = m_ViewportBlitY;
    width = m_ViewportBlitW;
    height = m_ViewportBlitH;
}

void Renderer::SetViewportRenderTargetColor(we::rhi::RHITextureHandle colorTexture) {
    m_ViewportColorTexture = colorTexture;
    if (m_SceneBackend) {
        m_SceneBackend->SetViewportColor(colorTexture);
    }
}

void Renderer::SetViewportDepthTarget(we::rhi::RHITextureHandle depthTexture) {
    m_ViewportDepthTexture = depthTexture;
    if (m_SceneBackend) {
        m_SceneBackend->SetViewportDepth(depthTexture);
    }
}

we::rhi::RHITextureViewHandle Renderer::GetViewportColorView() const {
    return m_SceneBackend ? m_SceneBackend->GetViewportColorView()
                          : we::rhi::RHITextureViewHandle::Invalid;
}

we::rhi::RHISamplerHandle Renderer::GetViewportColorSampler() const {
    return m_SceneBackend ? m_SceneBackend->GetViewportColorSampler()
                          : we::rhi::RHISamplerHandle::Invalid;
}

uint32_t Renderer::GetSwapchainWidth() const {
    if (m_SceneBackend) {
        return m_SceneBackend->GetSwapchainWidth();
    }
    if (m_RHIDevice && m_RHIDevice->GetSwapchain()) {
        return m_RHIDevice->GetSwapchain()->GetExtent().width;
    }
    return 0;
}

uint32_t Renderer::GetSwapchainHeight() const {
    if (m_SceneBackend) {
        return m_SceneBackend->GetSwapchainHeight();
    }
    if (m_RHIDevice && m_RHIDevice->GetSwapchain()) {
        return m_RHIDevice->GetSwapchain()->GetExtent().height;
    }
    return 0;
}

we::rhi::Format Renderer::GetSwapchainFormat() const {
    if (m_SceneBackend) {
        return m_SceneBackend->GetSwapchainFormat();
    }
    if (m_RHIDevice && m_RHIDevice->GetSwapchain()) {
        return m_RHIDevice->GetSwapchain()->GetFormat();
    }
    return we::rhi::Format::B8G8R8A8_UNORM;
}

void Renderer::RecreateSwapchain(uint32_t width, uint32_t height) {
    if (!m_Initialized || !m_RHIDevice) {
        return;
    }
    if (auto* swap = m_RHIDevice->GetSwapchain()) {
        (void)swap->Resize({width, height});
    }
}

} // namespace we::runtime::renderer
