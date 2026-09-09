#include "Editor.h"
#include "Core/FrameCounter.h"
#include "Core/Logger.h"
#include "Debug/FoundationRenderDebug.h"
#include "Environment/EnvironmentLighting.h"
#include "Environment/EnvironmentSystem.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Profiling/UiColorCompositionDiagnostic.h"
#include "KindUI/Profiling/UiColorPipelineDiagnostic.h"
#include "KindUI/Profiling/UiInputLatencyAudit.h"
#include "KindUI/Profiling/UiPathDiagnostics.h"
#include "Platform/Events.h"
#include "Platform/InputTypes.h"
#include "Platform/PlatformSDK.h"
#include "Widgets/ViewportWidget.h"
#include "WindEffects/Editor/UI/Core/EditorPerfStats.h"
#include "KindUI/Profiling/ScreenRecorder.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "WindEffects/Editor/UI/Widgets/RenderInvestigationModal.h"

#include <cstdlib>
#include <variant>

#include "Platform/UndefWin32Macros.h"

#ifndef WE_DEBUG_UI
#define WE_DEBUG_UI 0
#endif

namespace {
we::runtime::kindui::UiInteractionKind InteractionKindForMouse(const we::runtime::kindui::MouseEvent& event) {
    switch (event.type) {
    case we::runtime::kindui::MouseEventType::MouseMove: return we::runtime::kindui::UiInteractionKind::MouseMove;
    case we::runtime::kindui::MouseEventType::MouseWheel: return we::runtime::kindui::UiInteractionKind::Wheel;
    case we::runtime::kindui::MouseEventType::MouseDown: return we::runtime::kindui::UiInteractionKind::Click;
    case we::runtime::kindui::MouseEventType::MouseUp: return we::runtime::kindui::UiInteractionKind::ButtonPress;
    default: return we::runtime::kindui::UiInteractionKind::Unknown;
    }
}

we::runtime::kindui::UiInteractionKind InteractionKindForKey(bool pressed) {
    (void)pressed;
    return we::runtime::kindui::UiInteractionKind::Keyboard;
}

void MarkOsInput(we::runtime::kindui::UiInteractionKind kind) {
    if (we::runtime::kindui::UiInputLatencyAudit::IsEnabled()) {
        we::runtime::kindui::UiInputLatencyAudit::Get().OnOsEvent(kind);
    }
}

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

namespace we::programs::editor {

using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::MouseEventType;
using ::we::editor::viewport::ViewportWidget;

namespace UI = we::runtime::kindui;

void Editor::MainLoop() {
    auto& platform = we::platform::Platform::Get();
    uint64_t lastTime = platform.GetHighResolutionCounter();
    const double frequency = static_cast<double>(platform.GetHighResolutionFrequency());
    bool firstFrame = true;
    we::runtime::kindui::UIRepaintGate::Request();

    while (m_Running) {
        we::runtime::kindui::UIRepaintGate::BeginFrame();
        we::runtime::kindui::UiPathDiagnostics::Get().BeginFrame();
        ::we::editor::services::EditorPerfStats::Get().BeginFrame();

        uint64_t now = platform.GetHighResolutionCounter();
        float dt = static_cast<float>((now - lastTime) / frequency);
        lastTime = now;
        if (dt > 0.1f) dt = 0.1f;

        if (!platform.PollEvents()) {
            m_Running = false;
            if (m_OverlayHost) {
                m_OverlayHost->ExecutePendingCallbacks();
            }
        }

        const auto frameEvents = platform.GetFrameEvents();
        we::runtime::kindui::UiInputLatencyAudit::Get().BeginFrame(
            static_cast<uint32_t>(frameEvents.size()));

        // Keep hit-test geometry current before routing pointer/keyboard input.
        UpdateUiScaleFromWindow();
        SyncViewportFramebufferFromLayout();

        for (const auto& event : frameEvents) {
            bool requestUiLayout = false;
            bool requestUiPaint = false;
            const bool isMotion = std::holds_alternative<we::platform::MouseMoveEvent>(event)
                || std::holds_alternative<we::platform::RawMouseEvent>(event);

            if (std::holds_alternative<we::platform::QuitEvent>(event)) {
                m_Running = false;
                if (m_OverlayHost) {
                    m_OverlayHost->ExecutePendingCallbacks();
                }
            } else if (const auto* close = std::get_if<we::platform::WindowCloseEvent>(&event)) {
                if (close->window == m_Window) {
                    m_Running = false;
                    if (m_OverlayHost) {
                        m_OverlayHost->ExecutePendingCallbacks();
                    }
                }
            } else if (const auto* minEv = std::get_if<we::platform::WindowMinimizeEvent>(&event)) {
                if (minEv->window == m_Window && minEv->minimized) {
                    m_UIEventSystem->ClearAllInputState();
                }
                requestUiLayout = true;
                UpdateUiScaleFromWindow();
                SyncViewportFramebufferFromLayout();
            } else if (std::holds_alternative<we::platform::WindowResizeEvent>(event)
                || std::holds_alternative<we::platform::WindowDpiEvent>(event)
                || std::holds_alternative<we::platform::WindowMaximizeEvent>(event)) {
                requestUiLayout = true;
                UpdateUiScaleFromWindow();
                SyncViewportFramebufferFromLayout();
            } else if (const auto* cursor = std::get_if<we::platform::CursorEnterEvent>(&event)) {
                if (cursor->window == m_Window && !cursor->entered) {
                    m_UIEventSystem->ClearAllInputState();
                    requestUiPaint = true;
                }
            } else if (const auto* focus = std::get_if<we::platform::WindowFocusEvent>(&event)) {
                // Alt-tab / focus steal: release hover, capture, and focus explicitly.
                if (focus->window == m_Window && !focus->focused) {
                    m_UIEventSystem->ClearAllInputState();
                }
            } else if (const auto* move = std::get_if<we::platform::MouseMoveEvent>(&event)) {
                if (we::runtime::kindui::UIRepaintGate::PeekNeedsLayout()) {
                    SyncViewportFramebufferFromLayout();
                }
                UI::MouseEvent mouseEvent{};
                mouseEvent.type = UI::MouseEventType::MouseMove;
                mouseEvent.position = UI::Point{ static_cast<float>(move->position.x), static_cast<float>(move->position.y) };
                mouseEvent.deltaX = move->delta.x;
                mouseEvent.deltaY = move->delta.y;
                const auto mods = platform.GetKeyModifiers();
                mouseEvent.altDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Control);
                MarkOsInput(InteractionKindForMouse(mouseEvent));
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
                m_LastSampledMousePos = move->position;
                // Do not force a full UI rebuild on every move — hover/press
                // widgets already InvalidatePaint when their state actually changes.
            } else if (const auto* raw = std::get_if<we::platform::RawMouseEvent>(&event)) {
                UI::MouseEvent mouseEvent{};
                mouseEvent.type = UI::MouseEventType::MouseMove;
                const auto pos = platform.GetMousePosition(m_Window);
                mouseEvent.position = UI::Point{ static_cast<float>(pos.x), static_cast<float>(pos.y) };
                mouseEvent.deltaX = raw->delta.x;
                mouseEvent.deltaY = raw->delta.y;
                const auto mods = platform.GetKeyModifiers();
                mouseEvent.altDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Control);
                MarkOsInput(UI::UiInteractionKind::MouseMove);
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
                m_LastSampledMousePos = pos;
            } else if (const auto* button = std::get_if<we::platform::MouseButtonEvent>(&event)) {
                if (we::runtime::kindui::UIRepaintGate::PeekNeedsLayout()) {
                    SyncViewportFramebufferFromLayout();
                }
                UI::MouseEvent mouseEvent{};
                mouseEvent.type = button->pressed ? UI::MouseEventType::MouseDown : UI::MouseEventType::MouseUp;
                mouseEvent.position = UI::Point{ static_cast<float>(button->position.x), static_cast<float>(button->position.y) };
                switch (button->button) {
                case we::platform::MouseButton::Left: mouseEvent.button = MouseButton::Left; break;
                case we::platform::MouseButton::Right: mouseEvent.button = MouseButton::Right; break;
                case we::platform::MouseButton::Middle: mouseEvent.button = MouseButton::Middle; break;
                default: mouseEvent.button = MouseButton::None; break;
                }
                mouseEvent.altDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Control);
                MarkOsInput(InteractionKindForMouse(mouseEvent));
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
                requestUiPaint = true;
            } else if (const auto* wheel = std::get_if<we::platform::MouseWheelEvent>(&event)) {
                if (we::runtime::kindui::UIRepaintGate::PeekNeedsLayout()) {
                    SyncViewportFramebufferFromLayout();
                }
                UI::MouseEvent mouseEvent{};
                mouseEvent.type = UI::MouseEventType::MouseWheel;
                mouseEvent.position = UI::Point{ static_cast<float>(wheel->position.x), static_cast<float>(wheel->position.y) };
                mouseEvent.wheelDeltaX = wheel->delta.x;
                mouseEvent.wheelDeltaY = wheel->delta.y;
                const auto mods = platform.GetKeyModifiers();
                mouseEvent.altDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Control);
                MarkOsInput(UI::UiInteractionKind::Wheel);
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
                requestUiPaint = true;
            } else if (const auto* key = std::get_if<we::platform::KeyEvent>(&event)) {
#if WE_DEBUG_UI
                if (key->pressed && key->key == we::platform::KeyCode::F9) {
                    ::we::editor::panels::RenderInvestigationModalHost::Toggle();
                }
#endif
                UI::KeyEvent keyEvent{};
                keyEvent.type = key->pressed ? UI::KeyEventType::KeyDown : UI::KeyEventType::KeyUp;
                keyEvent.key = key->key;
                keyEvent.altDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Alt);
                keyEvent.shiftDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Shift);
                keyEvent.ctrlDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Control);
                MarkOsInput(InteractionKindForKey(key->pressed));
                m_UIEventSystem->ProcessKeyEvent(keyEvent);
                requestUiPaint = true;
            } else if (const auto* text = std::get_if<we::platform::TextInputEvent>(&event)) {
                if (m_UIEventSystem) {
                    m_UIEventSystem->ProcessTextInput(text->codepoint);
                }
                requestUiPaint = true;
            }

            (void)isMotion;
            if (requestUiLayout) {
                we::runtime::kindui::UIRepaintGate::Request();
            } else if (requestUiPaint) {
                we::runtime::kindui::UIRepaintGate::RequestPaint();
            }
        }
        
        // Execute pending callbacks after event processing to avoid use-after-free
        if (m_OverlayHost && m_OverlayHost->HasOpenPopups()) {
            m_OverlayHost->ExecutePendingCallbacks();
        }
        EditorWorkspaceController::Get().FlushPendingDockActions();

        if (!m_Running) break;

        // Tick before render so hover/press damping and camera settle apply to this frame's UI
        // (previously Tick ran after Present → one-frame interaction lag).
        if (!m_RootWidget) {
            HE_ERROR("[Render] Root widget is null during frame tick; stopping main loop.");
            m_Running = false;
            break;
        }
        ProcessLateInputMouse();
        TickSimulation(dt);
        ::we::editor::services::EditorPerfStats::Get().Mark("tick");

        // Layout immediately after input/tick so interaction geometry is current before render.
        UpdateUiScaleFromWindow();
        bool layoutOrResizeThisFrame = SyncViewportFramebufferFromLayout();
        if (we::runtime::kindui::UIRepaintGate::PeekNeedsLayout()) {
            layoutOrResizeThisFrame = SyncViewportFramebufferFromLayout() || layoutOrResizeThisFrame;
        }
        ::we::editor::services::EditorPerfStats::Get().Mark("layout");

        we::runtime::core::FrameCounter::Advance();

        if (m_UIEventSystem && m_ViewportWidget) {
            if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(m_ViewportWidget)) {
                m_UIEventSystem->SetSuppressSystemCursor(vp->IsFlyLookActive());
            }
        }

        we::runtime::renderer::CameraUniform cameraUBO{};
        cameraUBO.view = m_Camera->GetViewMatrix();
        cameraUBO.proj = m_Camera->GetProjectionMatrix();
        cameraUBO.position = m_Camera->GetPosition();
        {
            // WE_SKY_DEBUG: 0 final, 1 sky only, 2 sun mask, 3 luminance, 4 no sun, 5 linear HDR.
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



        if (m_Renderer && m_Renderer->BeginFrame()) {
            m_Renderer->UploadCameraUniform(cameraUBO);
            {
                auto& env = we::runtime::world::environment::EnvironmentSystem::Get();
                const auto envUBO = we::runtime::world::environment::BuildSceneEnvironmentUniform(
                    env.GetSun(),
                    env.GetSkyLight(),
                    env.GetSkyAtmosphere(),
                    env.GetHeightFog(),
                    env.GetVolumetricClouds(),
                    env.GetExposureController(),
                    m_Camera->GetPosition());
                m_Renderer->UploadEnvironmentUniform(envUBO);
            }

            // Forward ECS extract packet  Renderer never queries World/entities.
            m_Renderer->SetExtractedFrame(m_Scene ? m_Scene->GetExtractedFrame() : nullptr);

            ::we::editor::services::EditorPerfStats::Get().Mark("rhi");

            // CPU UI build + viewport sync before the graph (GPU overlay records inside UiOverlayPass).
            if (m_OverlayRenderer) {
                const uint32_t imageIndex = m_Renderer->GetCurrentImageIndex();
                // Must match the RHI frame slot — Renderer FIF and UI buffer count must stay in sync
                // or every other frame skips UI draw after Clear (whole-chrome flicker).
                const uint32_t frameSlot = m_Renderer->GetRHIDevice()
                    ? m_Renderer->GetRHIDevice()->GetCurrentFrameSlot()
                    : m_Renderer->GetCurrentFrameIndex();
                m_OverlayRenderer->SetPipelineAuditImageIndex(imageIndex);
                m_OverlayRenderer->SetTargetExtent(
                    m_Renderer->GetSwapchainWidth(), m_Renderer->GetSwapchainHeight());
                m_OverlayRenderer->RenderUI(m_RootWidget, frameSlot);
                ::we::editor::services::EditorPerfStats::Get().Mark("ui");

                if (m_ViewportWidget) {
                    if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(m_ViewportWidget)) {
                        if (vp->FlushPendingResize()) {
                            m_HasRenderedScene = false;
                            layoutOrResizeThisFrame = true;
                        }
                        vp->SyncRendererViewport();
                    }
                }

                m_Renderer->SetOverlayRecorder(
                    [this](const we::runtime::renderer::GraphPassContext& ctx,
                        we::rhi::RHITextureHandle swapImage) {
                        if (!m_OverlayRenderer || !ctx.commandList) {
                            return;
                        }
                        const uint32_t imageIndex = m_Renderer->GetCurrentImageIndex();
                        we::runtime::uigfx::OverlayRenderContext overlayContext{};
                        overlayContext.cmd = ctx.commandList;
                        overlayContext.colorTarget = swapImage;
                        overlayContext.targetFormat = m_Renderer->GetSwapchainFormat();
                        overlayContext.targetExtent = {
                            m_Renderer->GetSwapchainWidth(), m_Renderer->GetSwapchainHeight()};
                        overlayContext.imageIndex = imageIndex;
                        overlayContext.viewportOffsetX = 0;
                        overlayContext.viewportOffsetY = 0;

                        m_Renderer->RecordUiPresentPath(imageIndex);
                        m_Renderer->InsertOverlayPassBarrier();
                        m_OverlayRenderer->BeginOverlayPass(overlayContext);
                        m_OverlayRenderer->EndOverlayPass(overlayContext);
                        m_Renderer->MarkOverlayPassEnded();
                    });
            } else {
                m_Renderer->ClearOverlayRecorder();
            }

            const bool pipelineColorTest =
                we::runtime::kindui::UiColorPipelineDiagnostic::IsEnabled()
                && !we::runtime::kindui::UiColorCompositionDiagnostic::IsEnabled();
            const bool compositionColorTest = we::runtime::kindui::UiColorCompositionDiagnostic::IsEnabled();
            // Hover/click only need UI. Reuse the last 3D viewport when the camera is stable and
            // the viewport RT was not just recreated (FIF UI buffers are sized to match RHI).
            const bool paintOnlyFrame =
                pipelineColorTest
                || (!layoutOrResizeThisFrame
                    && m_HasRenderedScene
                    && cameraHash == m_LastSceneCameraHash);
            if (paintOnlyFrame) {
                m_Renderer->RenderUiPaintOnly();
            } else {
                m_Renderer->RenderScene();
                m_LastSceneCameraHash = cameraHash;
                m_HasRenderedScene = true;
            }
            ::we::editor::services::EditorPerfStats::Get().Mark("scene");

            m_Renderer->SubmitFrame();
            we::runtime::kindui::UiInputLatencyAudit::Get().OnGpuSubmit();
            we::runtime::kindui::UiInputLatencyAudit::Get().OnPresentStart();
            m_Renderer->PresentFrame();
            if (m_Renderer->GetRHIDevice()) {
                we::runtime::kindui::UiInputLatencyAudit::Get().OnPresentComplete(
                    &m_Renderer->GetRHIDevice()->GetDiagnostics().lastFrame);
                if (pipelineColorTest) {
                    we::runtime::kindui::UiColorPipelineDiagnostic::Get().TryFinalizeAndReport(
                        m_Renderer->GetRHIDevice());
                }
                if (compositionColorTest) {
                    we::runtime::kindui::UiColorCompositionDiagnostic::Get().OnFramePresented(
                        m_Renderer->GetRHIDevice());
                }
            } else {
                we::runtime::kindui::UiInputLatencyAudit::Get().OnPresentComplete(nullptr);
            }
            ::we::editor::services::EditorPerfStats::Get().Mark("present");
            m_Renderer->ClearOverlayRecorder();

            if (firstFrame) {
                HE_INFO("[Render] First foundation renderer frame presented.");
                if (m_FirstRunAgreementPending) {
                    MaybeShowFirstRunAgreement();
                }
                firstFrame = false;
            }
        } else if (!m_Renderer) {
            HE_ERROR("[Render] Renderer is null in main loop.");
        }

        {
            const auto& stats = m_OverlayRenderer
                ? m_OverlayRenderer->GetFrameStats()
                : we::runtime::kindui::UIFrameStats{};
            ::we::editor::services::EditorPerfStats::Get().EndFrame(
                stats.vertices,
                stats.batches,
                stats.opaqueBatches,
                stats.alphaBatches,
                stats.opaqueIndices,
                stats.alphaIndices);
            we::runtime::kindui::UiPathDiagnostics::Get().SetGeometryVertices(stats.vertices);
            we::runtime::kindui::UiPathDiagnostics::Get().EndFrame();
            ::we::runtime::kindui::ScreenRecorder::Get().RecordFrame();
        }

        if (we::runtime::kindui::UiInputLatencyAudit::IsEnabled()) {
            ++m_LatencyAuditFrameCounter;
            if (m_LatencyAuditFrameCounter % 300 == 0) {
    we::runtime::kindui::UiInputLatencyAudit::Get().FlushPendingReport();
    ::we::runtime::kindui::ScreenRecorder::Get().Shutdown();
            }
        }
    }
}

} // namespace we::programs::editor
