// ==============================================================================
// WindEffects — Renderer — Renderer
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Renderer/Renderer.h"
#include "Renderer/Graph/RenderGraph.h"
#include "Renderer/Graph/ScenePasses.h"
#include "Lighting/LightingSystem.h"
#include "Graph/ViewportSkyRenderer.h"
#include "Graph/ViewportGridRenderer.h"
#include "Graph/ViewportCloudRenderer.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Renderer/Validation.h"
#include "Platform/Platform.h"
#include "RHI/Desc.h"
#include "RHI/RHI.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include "Core/Math/GlmInterop.h"
#include <glm/gtc/matrix_inverse.hpp>

namespace we::runtime::renderer {
namespace {

Renderer* s_Instance = nullptr;

we::rhi::RHIBackend PreferBackendFromEnvironment(we::rhi::RHIBackend fallback) {
    if (const char* env = std::getenv("WE_RHI_BACKEND")) {
        if (std::strcmp(env, "null") == 0 || std::strcmp(env, "Null") == 0) {
            return we::rhi::RHIBackend::Null;
        }
        if (std::strcmp(env, "vulkan") == 0 || std::strcmp(env, "Vulkan") == 0) {
            return we::rhi::RHIBackend::Vulkan;
        }
        if (std::strcmp(env, "dx12") == 0 || std::strcmp(env, "d3d12") == 0
            || std::strcmp(env, "DirectX12") == 0) {
            return we::rhi::RHIBackend::DirectX12;
        }
    }
    return fallback;
}

bool VsyncFromEnvironment() {
    // Default OFF: FIFO present otherwise caps the whole editor loop at the
    // display refresh (often exactly 60 FPS). Opt in with WE_VSYNC=1.
    if (const char* env = std::getenv("WE_VSYNC")) {
        return env[0] != '\0' && env[0] != '0';
    }
    return false;
}

uint32_t FramesInFlightFromEnvironment() {
    if (const char* env = std::getenv("WE_FRAMES_IN_FLIGHT")) {
        const int value = std::atoi(env);
        if (value >= 1 && value <= 3) {
            return static_cast<uint32_t>(value);
        }
    }
    return kMaxFramesInFlight;
}

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
    rhiInit.preferredBackend = PreferBackendFromEnvironment(we::rhi::RHIBackend::Auto);
    rhiInit.appName = "WindEffects";
    const uint32_t framesInFlight = FramesInFlightFromEnvironment();
    rhiInit.framesInFlight = framesInFlight;
    (void)we::rhi::RHI::Initialize(rhiInit);

    we::rhi::DeviceDesc deviceDesc{};
    deviceDesc.windowId = window;
    deviceDesc.window = nativeWindow;
    deviceDesc.framesInFlight = framesInFlight;
    deviceDesc.vsync = VsyncFromEnvironment();
    auto deviceResult = we::rhi::RHI::Get().CreateDevice(deviceDesc);
    WE_VALIDATE_INIT(deviceResult.Ok() && deviceResult.value, "Renderer",
        deviceResult.Ok() ? "RHI CreateDevice returned null." : deviceResult.error.message.c_str());
    m_RHIDevice = std::move(deviceResult.value);
    WE_LOG_INFO(we::LogCategory::Renderer.data(), "RHI device created.");

    m_RenderGraph = std::make_unique<RenderGraph>();
    m_RenderGraph->Init(m_RHIDevice.get());
    m_ViewportSky = std::make_unique<ViewportSkyRenderer>();
    if (!m_ViewportSky->Init(m_RHIDevice.get())) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            "ViewportSkyRenderer init failed; viewport will clear without sky.");
    }
    m_ViewportGrid = std::make_unique<ViewportGridRenderer>();
    if (!m_ViewportGrid->Init(m_RHIDevice.get())) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            "ViewportGridRenderer init failed; viewport grid disabled.");
    }
    // Defer ViewportCloudRenderer::Init — 3D DDS upload during device bring-up
    // has crashed Vulkan drivers. Lazy-init on first CloudPass instead.
    m_ViewportClouds = std::make_unique<ViewportCloudRenderer>();
    m_CloudsInitAttempted = false;

    m_Lighting = std::make_unique<LightingSystem>();
    if (!m_Lighting->Initialize(LightingCreateInfo{m_RHIDevice.get()})) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            "LightingSystem init failed; lighting buffers unavailable.");
    }

    m_Scalability.Initialize();
    m_Scalability.SetRHIBackend(m_RHIDevice->GetBackend());
    m_Scalability.SetRHICapabilities(m_RHIDevice->GetCapabilities());
    m_Scalability.PublishFrameSettings();

    {
        const auto& published = m_Scalability.GetPublishedSettings();
        m_Lighting->Configure(published.lighting, published.shadows);
    }

    m_Initialized = true;
    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        std::string("Renderer initialized via RHI backend: ") +
        we::rhi::ToString(m_RHIDevice->GetBackend()) +
        " vsync=" + (deviceDesc.vsync ? "on" : "off") +
        " framesInFlight=" + std::to_string(framesInFlight) +
        " profile=" + m_Scalability.GetPublishedSettings().profileName);
}

void Renderer::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    DestroyViewportTargets();
    m_Scalability.Shutdown();
    if (m_Lighting) {
        m_Lighting->Shutdown();
        m_Lighting.reset();
    }
    if (m_ViewportGrid) {
        m_ViewportGrid->Shutdown();
        m_ViewportGrid.reset();
    }
    if (m_ViewportClouds) {
        m_ViewportClouds->Shutdown();
        m_ViewportClouds.reset();
    }
    if (m_ViewportSky) {
        m_ViewportSky->Shutdown();
        m_ViewportSky.reset();
    }
    if (m_RenderGraph) {
        m_RenderGraph->Shutdown();
        m_RenderGraph.reset();
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

ScalabilityUpdateFlags Renderer::SetRenderingProfile(RenderingProfileId id) {
    const ScalabilityUpdateFlags flags = m_Scalability.SetProfile(id);
    // Outside an active frame, publish immediately so UI/diagnostics stay coherent.
    if (!m_FrameActive) {
        m_Scalability.PublishFrameSettings();
    }
    if (HasFlag(flags, ScalabilityUpdateFlags::RestartRequired)) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            "Scalability change requires restart for full effect (e.g. MSAA).");
    }
    return flags;
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

void Renderer::DestroyViewportTargets() {
    if (!m_RHIDevice) {
        return;
    }
    // Drop cloud depth SRV before destroying the depth texture (UAF / null-descriptor AV).
    if (m_ViewportClouds) {
        m_ViewportClouds->InvalidateDepthBinding();
    }
    if (m_ViewportColorView != we::rhi::RHITextureViewHandle::Invalid) {
        (void)m_RHIDevice->DestroyTextureView(m_ViewportColorView);
        m_ViewportColorView = we::rhi::RHITextureViewHandle::Invalid;
    }
    if (m_ViewportColorSampler != we::rhi::RHISamplerHandle::Invalid) {
        (void)m_RHIDevice->DestroySampler(m_ViewportColorSampler);
        m_ViewportColorSampler = we::rhi::RHISamplerHandle::Invalid;
    }
    if (m_ViewportColorTexture != we::rhi::RHITextureHandle::Invalid) {
        (void)m_RHIDevice->DestroyTexture(m_ViewportColorTexture);
        m_ViewportColorTexture = we::rhi::RHITextureHandle::Invalid;
    }
    if (m_ViewportDepthTexture != we::rhi::RHITextureHandle::Invalid) {
        (void)m_RHIDevice->DestroyTexture(m_ViewportDepthTexture);
        m_ViewportDepthTexture = we::rhi::RHITextureHandle::Invalid;
    }
    m_OwnedViewportWidth = 0;
    m_OwnedViewportHeight = 0;
}

void Renderer::EnsureViewportTargets() {
    if (!m_RHIDevice) {
        return;
    }
    const uint32_t width = std::max(1u, m_ViewportTargetWidth ? m_ViewportTargetWidth : GetSwapchainWidth());
    const uint32_t height = std::max(1u, m_ViewportTargetHeight ? m_ViewportTargetHeight : GetSwapchainHeight());
    if (m_ViewportColorTexture != we::rhi::RHITextureHandle::Invalid
        && m_OwnedViewportWidth == width
        && m_OwnedViewportHeight == height) {
        return;
    }
    DestroyViewportTargets();

    we::rhi::TextureDesc colorDesc{};
    colorDesc.extent = {width, height, 1};
    colorDesc.format = we::rhi::Format::R16G16B16A16_SFLOAT;
    colorDesc.usage = we::rhi::TextureUsage::ColorAttachment | we::rhi::TextureUsage::Sampled
        | we::rhi::TextureUsage::TransferSrc;
    colorDesc.debugName = "ViewportColor";
    auto color = m_RHIDevice->CreateTexture(colorDesc);
    if (!color) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(), color.error.message);
        return;
    }
    m_ViewportColorTexture = *color;

    we::rhi::TextureDesc depthDesc{};
    depthDesc.extent = {width, height, 1};
    depthDesc.format = we::rhi::Format::D32_SFLOAT;
    depthDesc.usage = we::rhi::TextureUsage::DepthStencil | we::rhi::TextureUsage::Sampled;
    depthDesc.debugName = "ViewportDepth";
    auto depth = m_RHIDevice->CreateTexture(depthDesc);
    if (depth) {
        m_ViewportDepthTexture = *depth;
    }

    we::rhi::TextureViewDesc viewDesc{};
    viewDesc.texture = m_ViewportColorTexture;
    auto view = m_RHIDevice->CreateTextureView(viewDesc);
    if (view) {
        m_ViewportColorView = *view;
    }

    we::rhi::SamplerDesc samplerDesc{};
    samplerDesc.addressU = we::rhi::AddressMode::ClampToEdge;
    samplerDesc.addressV = we::rhi::AddressMode::ClampToEdge;
    samplerDesc.addressW = we::rhi::AddressMode::ClampToEdge;
    auto sampler = m_RHIDevice->CreateSampler(samplerDesc);
    if (sampler) {
        m_ViewportColorSampler = *sampler;
    }

    m_OwnedViewportWidth = width;
    m_OwnedViewportHeight = height;
}

bool Renderer::BeginFrame() {
    WE_VALIDATE_RENDER(m_Initialized, "Renderer::BeginFrame", "Renderer not initialized.");
    WE_VALIDATE_RENDER(!m_FrameActive, "Renderer::BeginFrame", "Frame already active.");

    m_Scalability.PublishFrameSettings();

    m_FrameCmd = m_RHIDevice->BeginFrame();
    if (!m_FrameCmd) {
        return false;
    }

    if (auto* swap = m_RHIDevice->GetSwapchain()) {
        m_CurrentImageIndex = swap->GetCurrentImageIndex();
    }

    m_FrameActive = true;
    ++m_PresentAuditFrameNumber;
    ResetPresentPathAudit();
    m_AcquiredImageIndex = m_CurrentImageIndex;
    return true;
}

void Renderer::ClearSwapchainChrome() {}

void Renderer::RenderViewportSky() {}

namespace {
// Set false to hard-skip CloudPass / ViewportCloudRenderer init.
constexpr bool kEnableVolumetricClouds = true;
} // namespace

void Renderer::EnsureCloudsReady() {
    if constexpr (kEnableVolumetricClouds) {
        if (m_CloudsInitAttempted || !m_ViewportClouds || !m_RHIDevice) {
            return;
        }
        m_CloudsInitAttempted = true;
        WE_LOG_INFO(we::LogCategory::Renderer.data(),
            "ViewportCloudRenderer: deferred init starting...");
        if (!m_ViewportClouds->Init(m_RHIDevice.get())) {
            WE_LOG_WARN(we::LogCategory::Renderer.data(),
                "ViewportCloudRenderer init failed; volumetric clouds disabled.");
        }
    }
}

void Renderer::RenderScene() {
    WE_VALIDATE_RENDER(m_Initialized && m_FrameActive, "Renderer::RenderScene", "No active frame.");
    EnsureViewportTargets();
    if (!m_FrameCmd || !m_RenderGraph || !m_RHIDevice) {
        return;
    }

    auto* swap = m_RHIDevice->GetSwapchain();
    const we::rhi::Extent2D swapExtent = swap ? swap->GetExtent() : we::rhi::Extent2D{1, 1};
    const we::rhi::RHITextureHandle swapImage = swap ? swap->GetCurrentImage() : we::rhi::RHITextureHandle::Invalid;
    const we::rhi::Extent2D viewportExtent{m_OwnedViewportWidth, m_OwnedViewportHeight};
    const ResolvedRenderingSettings& settings = m_Scalability.GetPublishedSettings();

    m_RenderGraph->ClearPasses();
    m_RenderGraph->SetScheduleMode(
        settings.capabilities.asyncComputeActive
            ? RGScheduleMode::AsyncPlanned
            : RGScheduleMode::SingleQueue);

    if (m_Lighting) {
        m_Lighting->Configure(settings.lighting, settings.shadows);
        LightingFrameContext lightingCtx{};
        lightingCtx.extract = m_ExtractedFrame;
        lightingCtx.camera = &m_LastCamera;
        lightingCtx.environment = &m_LastEnvironment;
        lightingCtx.viewportWidth = m_OwnedViewportWidth;
        lightingCtx.viewportHeight = m_OwnedViewportHeight;
        m_Lighting->BeginFrame(lightingCtx);
        m_Lighting->BuildRenderGraph(*m_RenderGraph);
    }

    m_RenderGraph->AddPass(std::make_unique<EnvUploadPass>(
        m_ViewportSky.get(), &m_LastCamera, &m_LastEnvironment));
    m_RenderGraph->AddPass(std::make_unique<ClearPass>(
        swapImage,
        m_SwapchainClearColor,
        swapExtent));
    m_RenderGraph->AddPass(std::make_unique<SkyPass>(
        m_ViewportSky.get(),
        m_ViewportColorTexture,
        m_ViewportDepthTexture,
        viewportExtent,
        &m_LastCamera,
        &m_LastEnvironment));
    m_RenderGraph->AddPass(std::make_unique<GridPass>(
        m_ViewportGrid.get(),
        m_ViewportColorTexture,
        m_ViewportDepthTexture,
        viewportExtent,
        &m_LastCamera));
    if (settings.terrain.enabled) {
        m_RenderGraph->AddPass(std::make_unique<TerrainPass>(
            m_TerrainDrawer,
            m_ViewportColorTexture,
            m_ViewportDepthTexture,
            viewportExtent,
            &m_LastCamera,
            &m_LastEnvironment));
    }
    // Mesh extract path — no placeholder GBuffer/shadow resources until those systems exist.
    m_RenderGraph->AddPass(std::make_unique<PbrOpaquePass>(
        kInvalidGraphResourceId,
        kInvalidGraphResourceId,
        m_ExtractedFrame,
        m_Lighting.get()));

    if constexpr (kEnableVolumetricClouds) {
        if (settings.volumetrics.enabled) {
            EnsureCloudsReady();
        }
        if (settings.volumetrics.enabled && m_ViewportClouds && m_ViewportClouds->IsReady()) {
            m_CloudUniform.enabled = 1.0f;
            m_CloudUniform.maxSteps = settings.volumetrics.maxSteps > 0
                ? settings.volumetrics.maxSteps
                : 64u;
            m_CloudUniform.timeSeconds = static_cast<float>(m_CurrentFrame) * (1.0f / 60.0f);
            m_RenderGraph->AddPass(std::make_unique<CloudPass>(
                m_ViewportClouds.get(),
                m_ViewportColorTexture,
                m_ViewportDepthTexture,
                viewportExtent,
                &m_LastCamera,
                &m_LastEnvironment,
                &m_CloudUniform));
        }
    }

    m_RenderGraph->AddPass(std::make_unique<TonemapPass>(m_ViewportColorTexture, swapImage));
    m_RenderGraph->AddPass(std::make_unique<UiOverlayPass>(swapImage, m_OverlayRecorder));
    m_RenderGraph->AddPass(std::make_unique<PresentPass>(swapImage));

    m_RenderGraph->Execute(*m_FrameCmd, m_CurrentFrame);
    if (m_Lighting) {
        m_Lighting->EndFrame();
    }
    m_SceneImageIndex = m_CurrentImageIndex;
}

void Renderer::RenderUiPaintOnly() {
    WE_VALIDATE_RENDER(m_Initialized && m_FrameActive, "Renderer::RenderUiPaintOnly", "No active frame.");
    if (!m_FrameCmd || !m_RenderGraph || !m_RHIDevice) {
        return;
    }

    auto* swap = m_RHIDevice->GetSwapchain();
    const we::rhi::Extent2D swapExtent = swap ? swap->GetExtent() : we::rhi::Extent2D{1, 1};
    const we::rhi::RHITextureHandle swapImage = swap ? swap->GetCurrentImage() : we::rhi::RHITextureHandle::Invalid;

    m_RenderGraph->ClearPasses();
    m_RenderGraph->SetScheduleMode(RGScheduleMode::AsyncPlanned);

    m_RenderGraph->AddPass(std::make_unique<ClearPass>(
        swapImage,
        m_SwapchainClearColor,
        swapExtent));
    // Keep the last HDR viewport in ShaderResource so UI sampling matches a full scene frame
    // (TonemapPass normally emits this transition).
    if (m_ViewportColorTexture != we::rhi::RHITextureHandle::Invalid) {
        m_RenderGraph->AddPass(std::make_unique<TonemapPass>(m_ViewportColorTexture, swapImage));
    }
    m_RenderGraph->AddPass(std::make_unique<UiOverlayPass>(swapImage, m_OverlayRecorder));
    m_RenderGraph->AddPass(std::make_unique<PresentPass>(swapImage));

    m_RenderGraph->Execute(*m_FrameCmd, m_CurrentFrame);
}

void Renderer::SetOverlayRecorder(OverlayRecordFn recorder) {
    m_OverlayRecorder = std::move(recorder);
}

void Renderer::SetExtractedFrame(const we::runtime::ecs::ExtractedFrameData* frame) {
    m_ExtractedFrame = frame;
}

void Renderer::ClearOverlayRecorder() {
    m_OverlayRecorder = {};
}

void Renderer::SetSwapchainClearColor(const we::rhi::Color4f& color) {
    m_SwapchainClearColor = color;
}

we::rhi::Color4f Renderer::GetSwapchainClearColor() const {
    return m_SwapchainClearColor;
}

void Renderer::SetTerrainDrawer(TerrainDrawFn drawer) {
    m_TerrainDrawer = std::move(drawer);
}

void Renderer::ClearTerrainDrawer() {
    m_TerrainDrawer = {};
}

std::string Renderer::DumpRenderGraph() const {
    if (!m_RenderGraph) {
        return {};
    }
    return m_RenderGraph->Dump();
}

void Renderer::SubmitFrame() {
    WE_VALIDATE_RENDER(m_Initialized && m_FrameActive, "Renderer::SubmitFrame", "No active frame.");
    if (m_FrameCmd && m_RHIDevice) {
        (void)m_RHIDevice->Submit(m_FrameCmd);
        m_FrameCmd = nullptr;
    }
}

void Renderer::PresentFrame() {
    WE_VALIDATE_RENDER(m_Initialized, "Renderer::PresentFrame", "Renderer not initialized.");
    if (m_RHIDevice) {
        (void)m_RHIDevice->Present();
        (void)m_RHIDevice->EndFrame();
    }
    m_FrameActive = false;
    m_CurrentFrame = (m_CurrentFrame + 1) % std::max(1u, FramesInFlightFromEnvironment());
}

void Renderer::SubmitAndPresent() {
    SubmitFrame();
    PresentFrame();
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
    const glm::mat4 viewProj = we::math::AsGlm(uniform.proj) * we::math::AsGlm(uniform.view);
    m_LastCamera.invViewProj = we::math::FromGlm(glm::inverse(viewProj));
}

void Renderer::UploadEnvironmentUniform(const SceneEnvironmentUniform& uniform) {
    m_LastEnvironment = uniform;
}

void Renderer::InsertOverlayPassBarrier() {
    if (!m_FrameCmd || !m_RHIDevice) {
        return;
    }
    if (auto* swap = m_RHIDevice->GetSwapchain()) {
        m_FrameCmd->TransitionTexture(
            swap->GetCurrentImage(),
            we::rhi::ResourceState::RenderTarget,
            we::rhi::ResourceState::RenderTarget);
    }
}

void Renderer::SetViewportRenderTargetSize(uint32_t width, uint32_t height) {
    m_ViewportTargetWidth = width;
    m_ViewportTargetHeight = height;
}

void Renderer::SetViewportBlitRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    m_ViewportBlitX = x;
    m_ViewportBlitY = y;
    m_ViewportBlitW = width;
    m_ViewportBlitH = height;
}

void Renderer::GetViewportBlitRect(uint32_t& x, uint32_t& y, uint32_t& width, uint32_t& height) const {
    x = m_ViewportBlitX;
    y = m_ViewportBlitY;
    width = m_ViewportBlitW;
    height = m_ViewportBlitH;
}

void Renderer::SetViewportRenderTargetColor(we::rhi::RHITextureHandle colorTexture) {
    (void)colorTexture;
}

void Renderer::SetViewportDepthTarget(we::rhi::RHITextureHandle depthTexture) {
    (void)depthTexture;
}

we::rhi::RHITextureViewHandle Renderer::GetViewportColorView() const {
    return m_ViewportColorView;
}

we::rhi::RHISamplerHandle Renderer::GetViewportColorSampler() const {
    return m_ViewportColorSampler;
}

uint32_t Renderer::GetSwapchainWidth() const {
    if (m_RHIDevice && m_RHIDevice->GetSwapchain()) {
        return m_RHIDevice->GetSwapchain()->GetExtent().width;
    }
    return 0;
}

uint32_t Renderer::GetSwapchainHeight() const {
    if (m_RHIDevice && m_RHIDevice->GetSwapchain()) {
        return m_RHIDevice->GetSwapchain()->GetExtent().height;
    }
    return 0;
}

we::rhi::Format Renderer::GetSwapchainFormat() const {
    if (m_RHIDevice && m_RHIDevice->GetSwapchain()) {
        return m_RHIDevice->GetSwapchain()->GetFormat();
    }
    return we::rhi::Format::B8G8R8A8_SRGB;
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
