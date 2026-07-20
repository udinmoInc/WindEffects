#include "Renderer/Renderer.h"
#include "Renderer/Graph/RenderGraph.h"
#include "Renderer/Graph/ScenePasses.h"
#include "Graph/ViewportSkyRenderer.h"
#include "Graph/ViewportGridRenderer.h"

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
    rhiInit.framesInFlight = kMaxFramesInFlight;
    (void)we::rhi::RHI::Initialize(rhiInit);

    we::rhi::DeviceDesc deviceDesc{};
    deviceDesc.windowId = window;
    deviceDesc.window = nativeWindow;
    deviceDesc.framesInFlight = kMaxFramesInFlight;
    deviceDesc.vsync = true;
    auto deviceResult = we::rhi::RHI::Get().CreateDevice(deviceDesc);
    WE_VALIDATE_INIT(deviceResult.Ok() && deviceResult.value, "Renderer",
        deviceResult.Ok() ? "RHI CreateDevice returned null." : deviceResult.error.message.c_str());
    m_RHIDevice = std::move(deviceResult.value);
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

    m_Initialized = true;
    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        std::string("Renderer initialized via RHI backend: ") +
        we::rhi::ToString(m_RHIDevice->GetBackend()));
}

void Renderer::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    DestroyViewportTargets();
    if (m_ViewportGrid) {
        m_ViewportGrid->Shutdown();
        m_ViewportGrid.reset();
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
    depthDesc.usage = we::rhi::TextureUsage::DepthStencil;
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

    m_RenderGraph->ClearPasses();
    m_RenderGraph->SetScheduleMode(RGScheduleMode::AsyncPlanned);

    // Placeholder targets for named stub systems (neverRealize — dump/deps only).
    TransientTextureDesc stubTex{};
    stubTex.extent = {std::max(1u, viewportExtent.width), std::max(1u, viewportExtent.height), 1};
    stubTex.neverRealize = true;

    stubTex.debugName = "RG.DepthPrepass";
    stubTex.format = we::rhi::Format::D32_SFLOAT;
    stubTex.usage = we::rhi::TextureUsage::DepthStencil;
    const uint32_t depthId = m_RenderGraph->CreateTexture(stubTex);

    stubTex.debugName = "RG.ShadowMap";
    const uint32_t shadowId = m_RenderGraph->CreateTexture(stubTex);

    stubTex.debugName = "RG.AtmosphereLUT";
    stubTex.format = we::rhi::Format::R16G16B16A16_SFLOAT;
    stubTex.usage = we::rhi::TextureUsage::Storage | we::rhi::TextureUsage::Sampled;
    const uint32_t atmosphereId = m_RenderGraph->CreateTexture(stubTex);

    stubTex.debugName = "RG.GBuffer";
    stubTex.usage = we::rhi::TextureUsage::ColorAttachment | we::rhi::TextureUsage::Sampled;
    const uint32_t gbufferId = m_RenderGraph->CreateTexture(stubTex);

    stubTex.debugName = "RG.Clouds";
    stubTex.usage = we::rhi::TextureUsage::Storage | we::rhi::TextureUsage::Sampled;
    const uint32_t cloudsId = m_RenderGraph->CreateTexture(stubTex);

    stubTex.debugName = "RG.Water";
    stubTex.usage = we::rhi::TextureUsage::ColorAttachment | we::rhi::TextureUsage::Sampled;
    const uint32_t waterId = m_RenderGraph->CreateTexture(stubTex);

    stubTex.debugName = "RG.Decals";
    const uint32_t decalsId = m_RenderGraph->CreateTexture(stubTex);

    stubTex.debugName = "RG.Transparency";
    const uint32_t transparencyId = m_RenderGraph->CreateTexture(stubTex);

    m_RenderGraph->AddPass(std::make_unique<EnvUploadPass>(
        m_ViewportSky.get(), &m_LastCamera, &m_LastEnvironment));
    m_RenderGraph->AddPass(std::make_unique<ClearPass>(
        swapImage,
        we::rhi::Color4f{0.09f, 0.09f, 0.10f, 1.0f},
        swapExtent));
    m_RenderGraph->AddPass(std::make_unique<StubGraphicsPass>(
        "DepthPrepass", depthId, we::rhi::ResourceState::DepthWrite));
    m_RenderGraph->AddPass(std::make_unique<StubGraphicsPass>(
        "ShadowPass", shadowId, we::rhi::ResourceState::DepthWrite));
    m_RenderGraph->AddPass(std::make_unique<StubComputePass>(
        "AtmospherePass", atmosphereId));
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
    m_RenderGraph->AddPass(std::make_unique<TerrainPass>(
        m_TerrainDrawer,
        m_ViewportColorTexture,
        m_ViewportDepthTexture,
        viewportExtent,
        &m_LastCamera,
        &m_LastEnvironment));
    m_RenderGraph->AddPass(std::make_unique<PbrOpaquePass>(
        gbufferId,
        shadowId,
        m_ExtractedFrame));
    m_RenderGraph->AddPass(std::make_unique<StubComputePass>(
        "CloudsPass", cloudsId, atmosphereId));
    m_RenderGraph->AddPass(std::make_unique<StubGraphicsPass>(
        "WaterPass", waterId, we::rhi::ResourceState::RenderTarget, depthId, we::rhi::ResourceState::DepthRead));
    m_RenderGraph->AddPass(std::make_unique<StubGraphicsPass>(
        "DecalsPass", decalsId, we::rhi::ResourceState::RenderTarget, gbufferId, we::rhi::ResourceState::ShaderResource));
    m_RenderGraph->AddPass(std::make_unique<StubGraphicsPass>(
        "TransparencyPass",
        transparencyId,
        we::rhi::ResourceState::RenderTarget,
        depthId,
        we::rhi::ResourceState::DepthRead));
    m_RenderGraph->AddPass(std::make_unique<TonemapPass>(m_ViewportColorTexture, swapImage));
    m_RenderGraph->AddPass(std::make_unique<UiOverlayPass>(swapImage, m_OverlayRecorder));
    m_RenderGraph->AddPass(std::make_unique<PresentPass>(swapImage));

    m_RenderGraph->Execute(*m_FrameCmd, m_CurrentFrame);
    m_SceneImageIndex = m_CurrentImageIndex;
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

void Renderer::SubmitAndPresent() {
    WE_VALIDATE_RENDER(m_Initialized && m_FrameActive, "Renderer::SubmitAndPresent", "No active frame.");

    // PresentPass already transitioned the swapchain to Present via RG barriers.
    if (m_FrameCmd && m_RHIDevice) {
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

