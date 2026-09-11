// ==============================================================================
// WindEffects — Editor — RenderPipelineSubsystem
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Framework/RenderPipelineSubsystem.h"
#include "Framework/KindUIServerSubsystem.h"
#include "Framework/PlatformInputSubsystem.h"

#include "Core/DiagnosticMacros.h"
#include "Core/EngineWatchdog.h"
#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Core/LoopExecutionTrace.h"
#include "Debug/FoundationRenderDebug.h"
#include "EditorCamera.h"
#include "Environment/EnvironmentLighting.h"
#include "Environment/EnvironmentSystem.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Profiling/UiColorCompositionDiagnostic.h"
#include "KindUI/Profiling/UiColorPipelineDiagnostic.h"
#include "KindUI/Profiling/UiInputLatencyAudit.h"
#include "KindUI/Rendering/OverlayRenderer.h"
#include "Platform/PlatformSDK.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "Widgets/ViewportWidget.h"
#include "WindEffects/Editor/UI/Core/EditorPerfStats.h"

#include <cstdlib>
#include <string>

#include "Platform/UndefWin32Macros.h"

namespace we::programs::editor {
namespace {

uint64_t HashCameraUniform(const we::runtime::renderer::CameraUniform& camera) {
    uint64_t hash = 14695981039346656037ull;
    auto mix = [&](float value) {
        const uint32_t bits = *reinterpret_cast<const uint32_t*>(&value);
        hash ^= static_cast<uint64_t>(bits);
        hash *= 1099511628211ull;
    };
    for (int i = 0; i < 16; ++i) {
        mix(camera.view.m[i]);
        mix(camera.proj.m[i]);
    }
    mix(camera.position.x);
    mix(camera.position.y);
    mix(camera.position.z);
    return hash;
}

} // namespace

RenderPipelineSubsystem::RenderPipelineSubsystem(
    IEditorLoopHost& host,
    we::runtime::core::ApplicationFramework& framework,
    EditorLoopComponentRegistry& components,
    KindUIServerSubsystem& kindUi,
    PlatformInputSubsystem& platformInput)
    : m_Host(host)
    , m_Framework(framework)
    , m_Components(components)
    , m_KindUi(kindUi)
    , m_PlatformInput(platformInput) {}

void RenderPipelineSubsystem::Initialize() {
    if (auto* swapchain = m_Framework.FindSubsystem<we::runtime::core::SwapchainSubsystem>()) {
        swapchain->SetRecreateCallback([this]() {
            m_Host.HostForceSwapchainRecreate() = true;
            m_Host.HostEnsureVisibleSwapchain();
            return true;
        });
    }
    HE_INFO("[RenderPipelineSubsystem] Initialized");
}

void RenderPipelineSubsystem::Shutdown() {
    HE_INFO("[RenderPipelineSubsystem] Shutdown");
}

void RenderPipelineSubsystem::ProcessCommand(const we::runtime::core::ApplicationCommand& command) {
    if (command.type == we::runtime::core::ApplicationCommand::Type::SwapchainRecreateRequest
        || command.type == we::runtime::core::ApplicationCommand::Type::GpuPresentRequest) {
        // Only arm the force flag; SwapchainSubsystem::Tick owns the single Ensure pass.
        // Never call HostEnsureVisibleSwapchain here — that re-enters recreate mid-pump.
        if (auto* swapchain = m_Framework.FindSubsystem<we::runtime::core::SwapchainSubsystem>()) {
            if (swapchain->IsRecreating() || swapchain->NeedsRecreation()) {
                m_Host.HostForceSwapchainRecreate() = true;
                return;
            }
        }
        m_Host.HostForceSwapchainRecreate() = true;
    }
}

void RenderPipelineSubsystem::Tick(float /*deltaTime*/) {
    we::runtime::core::EngineWatchdog::Scoped watchdogScope("RenderPipeline.Tick");
    we::runtime::core::LoopExecutionTrace::Scoped scope("RenderPipeline.Tick");
    m_BeganFrame = false;
    auto& platform = we::platform::Platform::Get();
    auto* renderer = m_Host.GetHostRenderer();
    auto* camera = m_Host.GetHostCamera();
    if (!camera) {
        return;
    }

    bool layoutOrResizeThisFrame = m_KindUi.LayoutOrResizeThisFrame();

    we::runtime::renderer::CameraUniform cameraUBO{};
    cameraUBO.view = camera->GetViewMatrix();
    cameraUBO.proj = camera->GetProjectionMatrix();
    cameraUBO.position = camera->GetPosition();
    {
        static int s_SkyDebugMode = []() {
            if (const char* v = std::getenv("WE_SKY_DEBUG")) {
                return std::atoi(v);
            }
            return 0;
        }();
        cameraUBO.padding = static_cast<float>(s_SkyDebugMode);
        we::runtime::renderer::FoundationRenderDebug::MaybeLog(s_SkyDebugMode);
    }

    const uint64_t cameraHash = HashCameraUniform(cameraUBO);
    const bool windowMinimized = platform.IsWindowMinimized(m_Host.GetHostWindow());
    const bool windowFocused = platform.IsWindowFocused(m_Host.GetHostWindow());

    // SwapchainSubsystem::Tick (priority 50) owns the sole Ensure pass via its
    // recreate callback. Do not call HostEnsureVisibleSwapchain here — a second
    // Ensure in the same frame recreates the Vulkan swapchain twice and stalls
    // the fence wait / message pump pairing observed after focus-gain.

    if (windowMinimized) {
        we::runtime::core::LoopExecutionTrace::Event(
            "RenderPipeline.SkipGpu", "windowMinimized=1 (message pump continues; no BeginFrame)");
        m_BeginFrameFailStreak = 0;
    } else if (renderer && [&]() {
            we::runtime::core::LoopExecutionTrace::Enter("RenderPipeline.BeginFrame");
            const bool ok = renderer->BeginFrame();
            we::runtime::core::LoopExecutionTrace::Exit(
                "RenderPipeline.BeginFrame", ok ? "ok" : "FAILED — likely vkWaitForFences timeout");
            return ok;
        }()) {
        m_BeganFrame = true;
        m_BeginFrameFailStreak = 0;
        renderer->UploadCameraUniform(cameraUBO);
        {
            auto& env = we::runtime::world::environment::EnvironmentSystem::Get();
            const auto envUBO = we::runtime::world::environment::BuildSceneEnvironmentUniform(
                env.GetSun(),
                env.GetSkyLight(),
                env.GetSkyAtmosphere(),
                env.GetHeightFog(),
                env.GetVolumetricClouds(),
                env.GetExposureController(),
                camera->GetPosition());
            renderer->UploadEnvironmentUniform(envUBO);
        }

        auto* scene = m_Host.GetHostScene();
        renderer->SetExtractedFrame(scene ? scene->GetExtractedFrame() : nullptr);
        ::we::editor::services::EditorPerfStats::Get().Mark("rhi");

        if (auto vp = std::dynamic_pointer_cast<::we::editor::viewport::ViewportWidget>(
                m_Host.GetHostViewportWidget())) {
            if (vp->FlushPendingResize()) {
                m_Host.HostHasRenderedScene() = false;
                layoutOrResizeThisFrame = true;
            }
            vp->SyncRendererViewport();
        }

        if (auto* overlay = m_Host.GetHostOverlayRenderer()) {
            const uint32_t imageIndex = renderer->GetCurrentImageIndex();
            const uint32_t frameSlot = renderer->GetRHIDevice()
                ? renderer->GetRHIDevice()->GetCurrentFrameSlot()
                : renderer->GetCurrentFrameIndex();
            overlay->SetPipelineAuditImageIndex(imageIndex);
            overlay->SetTargetExtent(renderer->GetSwapchainWidth(), renderer->GetSwapchainHeight());
            overlay->RenderUI(m_Host.GetHostRootWidget(), frameSlot);
            ::we::editor::services::EditorPerfStats::Get().Mark("ui");

            renderer->SetOverlayRecorder(
                [this, renderer](const we::runtime::renderer::GraphPassContext& ctx,
                    we::rhi::RHITextureHandle swapImage) {
                    auto* overlayRenderer = m_Host.GetHostOverlayRenderer();
                    if (!overlayRenderer || !ctx.commandList) {
                        return;
                    }
                    const uint32_t imageIndex = renderer->GetCurrentImageIndex();
                    we::runtime::uigfx::OverlayRenderContext overlayContext{};
                    overlayContext.cmd = ctx.commandList;
                    overlayContext.colorTarget = swapImage;
                    overlayContext.targetFormat = renderer->GetSwapchainFormat();
                    overlayContext.targetExtent = {
                        renderer->GetSwapchainWidth(), renderer->GetSwapchainHeight()};
                    overlayContext.imageIndex = imageIndex;
                    overlayContext.viewportOffsetX = 0;
                    overlayContext.viewportOffsetY = 0;

                    renderer->RecordUiPresentPath(imageIndex);
                    renderer->InsertOverlayPassBarrier();
                    overlayRenderer->BeginOverlayPass(overlayContext);
                    overlayRenderer->EndOverlayPass(overlayContext);
                    renderer->MarkOverlayPassEnded();
                });
        } else {
            renderer->ClearOverlayRecorder();
        }

        const bool pipelineColorTest =
            we::runtime::kindui::UiColorPipelineDiagnostic::IsEnabled()
            && !we::runtime::kindui::UiColorCompositionDiagnostic::IsEnabled();
        const bool compositionColorTest = we::runtime::kindui::UiColorCompositionDiagnostic::IsEnabled();
        const bool paintOnlyFrame =
            pipelineColorTest
            || (!layoutOrResizeThisFrame
                && m_Host.HostHasRenderedScene()
                && cameraHash == m_Host.HostLastSceneCameraHash());
        if (paintOnlyFrame) {
            renderer->RenderUiPaintOnly();
        } else {
            renderer->RenderScene();
            m_Host.HostLastSceneCameraHash() = cameraHash;
            m_Host.HostHasRenderedScene() = true;
        }
        ::we::editor::services::EditorPerfStats::Get().Mark("scene");

        renderer->SubmitFrame();
        we::runtime::kindui::UiInputLatencyAudit::Get().OnGpuSubmit();
        we::runtime::kindui::UiInputLatencyAudit::Get().OnPresentStart();
        {
            we::runtime::core::LoopExecutionTrace::Scoped presentScope("RenderPipeline.PresentFrame");
            renderer->PresentFrame();
        }
        if (renderer->GetRHIDevice()) {
            we::runtime::kindui::UiInputLatencyAudit::Get().OnPresentComplete(
                &renderer->GetRHIDevice()->GetDiagnostics().lastFrame);
            if (pipelineColorTest) {
                we::runtime::kindui::UiColorPipelineDiagnostic::Get().TryFinalizeAndReport(
                    renderer->GetRHIDevice());
            }
            if (compositionColorTest) {
                we::runtime::kindui::UiColorCompositionDiagnostic::Get().OnFramePresented(
                    renderer->GetRHIDevice());
            }
        } else {
            we::runtime::kindui::UiInputLatencyAudit::Get().OnPresentComplete(nullptr);
        }
        ::we::editor::services::EditorPerfStats::Get().Mark("present");
        renderer->ClearOverlayRecorder();

        if (m_FirstFrame) {
            HE_INFO("[Render] First foundation renderer frame presented.");
            m_Components.PostPresent(m_Host, true);
            m_FirstFrame = false;
        } else {
            m_Components.PostPresent(m_Host, true);
        }
    } else if (!renderer) {
        HE_ERROR("[Render] Renderer is null in render pipeline.");
        m_Components.PostPresent(m_Host, false);
    } else {
        ++m_BeginFrameFailStreak;
        if (m_BeginFrameFailStreak == 1 || (m_BeginFrameFailStreak % 60) == 0) {
            WE_LOG_WARN(we::LogCategory::Renderer.data(),
                "[Render] BeginFrame failed (streak=" + std::to_string(m_BeginFrameFailStreak)
                + ") — frame skipped, retrying next tick.");
        }
        we::runtime::kindui::UIRepaintGate::RequestPaint();
        m_Components.PostPresent(m_Host, false);
    }

    NotifyLoopPulseDiagnostics(
        windowMinimized,
        m_BeganFrame,
        windowFocused,
        m_BeginFrameFailStreak,
        m_PlatformInput.LastEventCount());
}

} // namespace we::programs::editor
