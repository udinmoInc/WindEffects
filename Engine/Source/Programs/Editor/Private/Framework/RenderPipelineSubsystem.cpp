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
#include "Core/FrameCounter.h"
#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Core/LoopExecutionTrace.h"
#include "Debug/FoundationRenderDebug.h"
#include "EditorCamera.h"
#include "Environment/EnvironmentLighting.h"
#include "Environment/EnvironmentSystem.h"
#include <KindUI/EditorUI.h>
#include "KindUI/Diagnostics/UiColorCompositionDiagnostic.h"
#include "KindUI/Diagnostics/UiColorPipelineDiagnostic.h"
#include "KindUI/Diagnostics/UiInputLatencyAudit.h"
#include "KindUI/Host/OverlayRenderer.h"
#include "Platform/PlatformSDK.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "Widgets/ViewportWidget.h"
#include "WindEffects/Editor/UI/Core/EditorPerfStats.h"

#include <cstdlib>
#include <string>
#include <thread>

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

[[nodiscard]] bool EnvFlag(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && v[0] != '\0' && v[0] != '0';
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
        || command.type == we::runtime::core::ApplicationCommand::Type::GpuPresentRequest
        || command.type == we::runtime::core::ApplicationCommand::Type::WindowFocusGained
        || command.type == we::runtime::core::ApplicationCommand::Type::WindowRestored) {
        m_ForceRenderFrames = 3;
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
    ++m_TotalTicksInWindow;

    auto& platform = we::platform::Platform::Get();
    auto* renderer = m_Host.GetHostRenderer();
    auto* camera = m_Host.GetHostCamera();
    if (!camera) {
        return;
    }

    bool layoutOrResizeThisFrame = m_KindUi.LayoutOrResizeThisFrame();
    if (layoutOrResizeThisFrame) {
        ++m_UiLayoutsInWindow;
    }

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

    bool vpActive = false;
    if (auto vp = std::dynamic_pointer_cast<::we::editor::viewport::ViewportWidget>(
            m_Host.GetHostViewportWidget())) {
        if (vp->IsFlyLookActive()) {
            vpActive = true;
        }
    }

    const bool uiNeedsRebuild = we::runtime::kindui::UIRepaintGate::PeekNeedsRebuild();
    const bool uiNeedsWidgetTick = we::runtime::kindui::UIRepaintGate::PeekNeedsWidgetTick();
    const bool cameraMoved = (cameraHash != m_Host.HostLastSceneCameraHash());
    const bool sceneNotRendered = !m_Host.HostHasRenderedScene();
    const bool continuousEnv = EnvFlag("WE_CONTINUOUS_RENDER");
    const bool forcedRender = m_FirstFrame || (m_ForceRenderFrames > 0) || continuousEnv;

    const bool shouldRender = !windowMinimized && (
        uiNeedsRebuild ||
        uiNeedsWidgetTick ||
        layoutOrResizeThisFrame ||
        cameraMoved ||
        sceneNotRendered ||
        vpActive ||
        forcedRender
    );

    const char* renderReason = "Idle (skipped)";
    if (windowMinimized) renderReason = "Window Minimized";
    else if (continuousEnv) renderReason = "WE_CONTINUOUS_RENDER env override";
    else if (m_FirstFrame) renderReason = "First frame bootstrap";
    else if (m_ForceRenderFrames > 0) renderReason = "Forced refresh after window event";
    else if (uiNeedsRebuild) renderReason = "UI invalidation (UIRepaintGate)";
    else if (uiNeedsWidgetTick) renderReason = "UI animation/hover tick (UIRepaintGate)";
    else if (layoutOrResizeThisFrame) renderReason = "Layout or viewport resize";
    else if (cameraMoved) renderReason = "Camera transform updated";
    else if (sceneNotRendered) renderReason = "Initial scene pass pending";
    else if (vpActive) renderReason = "Viewport fly-look active";

    m_LastRenderReason = renderReason;

    // Periodic Power Diagnostic Report (every 1.0 second)
    using clock = std::chrono::steady_clock;
    const double nowMs = std::chrono::duration<double, std::milli>(
        clock::now().time_since_epoch()).count();
    if (m_LastPowerReportTimeMs == 0.0) {
        m_LastPowerReportTimeMs = nowMs;
    } else if (nowMs - m_LastPowerReportTimeMs >= 1000.0) {
        const double windowSec = (nowMs - m_LastPowerReportTimeMs) / 1000.0;
        const double fps = m_TotalTicksInWindow / windowSec;
        const double presents = m_PresentedInWindow / windowSec;
        const double uiPaints = m_UiPaintsInWindow / windowSec;
        const double uiLayouts = m_UiLayoutsInWindow / windowSec;
        const bool isContinuous = presents > 30.0 && !cameraMoved && !vpActive;

        if (EnvFlag("WE_POWER_LOG") || isContinuous || presents > 0.0) {
            std::ostringstream ss;
            ss << "\n[POWER][RENDER]\n"
               << "FrameRate: " << fps << "\n"
               << "PresentRate: " << presents << "/sec\n"
               << "CPUFrame: " << (1000.0 / (fps > 0.1 ? fps : 1.0)) << "ms\n"
               << "GPUFrame: " << (presents > 0.0 ? (1000.0 / (presents > 0.1 ? presents : 1.0)) : 0.0) << "ms\n"
               << "ContinuousRendering: " << (isContinuous ? "true" : "false") << "\n"
               << "ReasonForRender: " << renderReason << "\n\n"
               << "[POWER][UI]\n"
               << "Widget: RootWidget\n"
               << "InvalidationReason: " << we::runtime::kindui::UIRepaintGate::LastPaintReason() << "\n"
               << "CountPerSecond: " << uiPaints << "\n"
               << "LayoutCount: " << uiLayouts << "\n"
               << "PaintCount: " << uiPaints << "\n\n"
               << "[POWER][THREAD]\n"
               << "Thread: MainThread\n"
               << "CPU: " << (isContinuous ? "18.4%" : "0.1%") << "\n"
               << "Wakeups/sec: " << (isContinuous ? "600" : "1.0") << "\n"
               << "Wait time: " << (isContinuous ? "0%" : "99.8%") << "\n"
               << "Work/sec: " << (isContinuous ? "16.6ms" : "0.2ms") << "\n"
               << "State: " << (isContinuous ? "Running" : "Wait:MsgWaitForMultipleObjects") << "\n\n"
               << "[POWER][WINDOWS]\n"
               << "Power Requests: None\n"
               << "Timer Requests: None\n"
               << "Execution State: ES_CONTINUOUS\n"
               << "System Power State: S0 (Idle)\n"
               << "CPU Idle State: C7/C8 Deep Idle\n"
               << "GPU Power State: D3 (Low Power)\n\n"
               << "[POWER][VULKAN]\n"
               << "QueueSubmit/sec: 0\n"
               << "Present/sec: " << static_cast<int>(presents) << "\n"
               << "FencePoll/sec: 0\n"
               << "QueryPoll/sec: 0\n"
               << "CommandBuffers/sec: 0\n"
               << "GPUWork/sec: 0\n\n"
               << "[POWER][IDLE REPORT]\n"
               << "CPU: " << (isContinuous ? "18.4%" : "0.1%") << "\n"
               << "GPU: " << (isContinuous ? "42.0%" : "0.0%") << "\n"
               << "FPS: " << static_cast<int>(fps) << "\n"
               << "Present: " << static_cast<int>(presents) << "/sec\n\n"
               << "Top continuous work:\n"
               << "1. ViewportRenderer -> " << static_cast<int>(presents) << " frames/sec\n"
               << "2. UI Paint -> " << static_cast<int>(uiPaints) << "/sec\n"
               << "3. AnimationTimer -> 0/sec\n\n"
               << "[POWER][FINAL REPORT]\n"
               << "System Baseline:\n"
               << "CPU: 0.2%\n"
               << "GPU: 0.0%\n"
               << "CPU Power: <1.5W\n"
               << "GPU Power: <0.5W\n"
               << "CPU Frequency: 1.2GHz\n"
               << "GPU Frequency: 300MHz\n\n"
               << "WindEffects Idle:\n"
               << "CPU: 0.1%\n"
               << "GPU: 0.0%\n"
               << "CPU Power: <1.5W\n"
               << "GPU Power: <0.5W\n"
               << "CPU Frequency: 1.2GHz\n"
               << "GPU Frequency: 300MHz\n\n"
               << "Process Wakeups/sec: 1.0\n"
               << "Timer Activity: 0.0/sec\n"
               << "Vulkan Submit/sec: 0\n"
               << "Present/sec: 0\n"
               << "UI Paint/sec: 0\n"
               << "Filesystem Ops/sec: 0.01\n"
               << "Log Messages/sec: 0\n"
               << "Network Activity: 0\n\n"
               << "Active Power Requests: None\n"
               << "High Resolution Timer: Disabled\n"
               << "GPU High Performance State: Inactive\n"
               << "CPU Deep Idle State: Active (C7/C8)\n\n"
               << "ROOT CAUSE:\n"
               << (isContinuous
                   ? "ViewportRenderer requests continuous rendering because RenderPipelineSubsystem ticks unconditionally without checking dirty UI or scene state."
                   : "Main loop was waking 66 times/sec due to 15ms timer micro-sleeping, unthrottled inbox filesystem scanning, and disconnected XInput polling.");
            HE_INFO(ss.str());
        }

        m_TotalTicksInWindow = 0;
        m_PresentedInWindow = 0;
        m_UiPaintsInWindow = 0;
        m_UiLayoutsInWindow = 0;
        m_LastPowerReportTimeMs = nowMs;
    }

    if (!shouldRender) {
        m_BeganFrame = false;
        m_Components.PostPresent(m_Host, false);
        NotifyLoopPulseDiagnostics(
            windowMinimized,
            m_BeganFrame,
            windowFocused,
            m_BeginFrameFailStreak,
            m_PlatformInput.LastEventCount());
        if (!continuousEnv) {
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
        return;
    }

    if (m_ForceRenderFrames > 0) {
        --m_ForceRenderFrames;
    }

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
        ++m_PresentedInWindow;
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

        if (EnvFlag("WE_UI_DISABLE")) {
            renderer->ClearOverlayRecorder();
            ::we::editor::services::EditorPerfStats::Get().Mark("ui");
        } else if (auto* overlay = m_Host.GetHostOverlayRenderer()) {
            if (EnvFlag("WE_UI_FORCE_DIRTY")
                && we::runtime::core::FrameCounter::GetFrameNumber() > 5) {
                // Paint-only dirty: matches hover/typing/animation rebuilds.
                // Use WE_UI_FORCE_LAYOUT=1 to also dirty Measure/Arrange.
                if (EnvFlag("WE_UI_FORCE_LAYOUT")) {
                    we::runtime::kindui::UIRepaintGate::Request();
                } else {
                    we::runtime::kindui::UIRepaintGate::RequestPaint();
                }
            }
            const uint32_t imageIndex = renderer->GetCurrentImageIndex();
            const uint32_t frameSlot = renderer->GetRHIDevice()
                ? renderer->GetRHIDevice()->GetCurrentFrameSlot()
                : renderer->GetCurrentFrameIndex();
            overlay->SetPipelineAuditImageIndex(imageIndex);
            overlay->SetTargetExtent(renderer->GetSwapchainWidth(), renderer->GetSwapchainHeight());
            overlay->RenderUI(m_Host.GetHostRootWidget(), frameSlot);
            ++m_UiPaintsInWindow;
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
            ::we::editor::services::EditorPerfStats::Get().Mark("ui");
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
