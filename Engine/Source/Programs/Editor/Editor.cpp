#include "Editor.h"
#include "FirstRunAgreementPopup.h"
#include "Core/Logger.h"
#include "Core/DiagnosticMacros.h"
#include "Core/FrameCounter.h"
#include "Core/StartupValidator.h"
#include "Core/LogCategory.h"
#include "Core/IgniteBTInvoker.h"
#include "Runtime/Core/PluginManager.h"
#include "Environment/EnvironmentEditorApi.h"
#include "Renderer/Renderer.h"
#include "KindUI/Rendering/IconRenderer.h"
#include "KindUI/Layout/OverlayManager.h"
#include "EditorWorkspaceController.h"
#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"

#include "EditorWindowHitTest.h"
#include "EditorShellBuilder.h"
#include "Widgets/ViewportWidget.h"
#include "ViewportToolbarState.h"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "ContentBrowserApi.h"
#include "Runtime/Core/AssetRegistry.h"
#include "EditorGridRenderer.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "Core/EditorPerfStats.h"
#include "Debug/FoundationRenderDebug.h"
#include "Runtime/World/DefaultScene/DefaultSceneBuilder.h"
#include "Runtime/World/Environment/EnvironmentSystem.h"
#include "Runtime/World/Environment/EnvironmentLighting.h"
#include "Widgets/RenderInvestigationModal.h"

#include "Platform/PlatformSDK.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <variant>

#ifndef WE_DEBUG_UI
#define WE_DEBUG_UI 0
#endif

namespace {
bool PresentAuditEnabled() {
    static const bool enabled = []() {
        const char* value = std::getenv("WE_PRESENT_AUDIT");
        return value != nullptr && value[0] != '\0' && value[0] != '0';
    }();
    return enabled;
}
} // namespace

namespace we::programs::editor {

namespace UI = we::runtime::kindui;

using namespace we::runtime::kindui;
using namespace we::runtime::renderer;
using namespace we::runtime::scene;
using namespace we::runtime::engine;
using namespace we::runtime::world;

Editor::Editor(we::platform::WindowId window) : m_Window(window) {
    HE_INFO("[Startup] === Editor construction begin ===");

    HE_INFO("[Startup] Stage 1/6: Renderer...");
    m_Renderer = std::make_unique<Renderer>();
    m_Renderer->Init(m_Window);

    HE_INFO("[Startup] Stage 2/6: Scene and camera...");
    m_Camera = std::make_shared<EditorCamera>();
    BindViewportCamera(m_Camera);
    m_Scene = std::make_shared<Scene>();
    we::runtime::world::environment::EnvironmentSystem::Get().BindScene(m_Scene);
    PlaceActorsPlacement::Get().BindScene(m_Scene, m_Camera);
    DefaultSceneBuilder::CreateDefaultScene(*m_Scene);
    we::runtime::world::environment::EnvironmentSystem::Get().UpdateRendering(m_Camera->GetPosition());

    {
        auto& startup = we::runtime::core::StartupValidator::Get();
        startup.RegisterCheck("Renderer", [this](std::string& detail) {
            detail = m_Renderer->IsGpuReady() ? "Foundation renderer initialized" : "Renderer GPU not ready";
            return m_Renderer->IsGpuReady();
        });
        startup.RunAll();
        if (!startup.AllPassed()) {
            throw std::runtime_error("Startup validation failed!");
        }
    }

    HE_INFO("[Startup] Stage 3/6: Default assets (fonts, shaders, icons, theme)...");
    if (!we::core::AssetRegistry::Get().LoadDefaultEditorAssets()) {
        HE_ERROR("[Startup] Required default assets missing - UI rendering may fail.");
    }

    HE_INFO("[Startup] Stage 4/6: OverlayRenderer init...");
    try {
        m_OverlayRenderer = std::make_unique<we::runtime::kindui::OverlayRenderer>();
        if (!m_OverlayRenderer->Init(m_Renderer->GetRHIDevice(), m_Renderer->GetSwapchainFormat(), 2)) {
            throw std::runtime_error("Failed to initialize OverlayRenderer!");
        }
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to initialize OverlayRenderer: " + std::string(e.what()));
        throw;
    }

    // EditorCompositor no longer needed - scene renders directly to swapchain

    try {
        InitializeContentBrowserService(m_OverlayRenderer->GetIconRenderer());
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to initialize content browser service: " + std::string(e.what()));
        throw;
    }
    m_UIEventSystem = std::make_shared<UI::EventSystem>();

    HE_INFO("[Startup] Stage 5/6: Plugins...");
    try {
        we::core::PluginManager::Get().ScanAndLoadPlugins("Plugins");
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to load plugins: " + std::string(e.what()));
        throw;
    }

    HE_INFO("[Startup] Stage 6/6: Widget tree and layout...");
    UpdateUiScaleFromWindow();

    m_UIContext = std::make_unique<we::runtime::kindui::EditorApplicationContext>();
    m_UIContext->Initialize(we::runtime::kindui::DPIContext::GetScale());

    const auto& extensionPanels = m_UIContext->GetExtensionRegistry().GetPanels();
    HE_INFO("[Startup] UI extension panels registered: " + std::to_string(extensionPanels.size()));
    for (const auto& [name, _] : extensionPanels) {
        HE_INFO("[Startup]   - Panel factory: " + name);
    }

    try {
        we::runtime::kindui::EditorShellDependencies shellDeps;
        shellDeps.window = m_Window;
        shellDeps.renderer = m_Renderer.get();
        shellDeps.scene = m_Scene;
        shellDeps.camera = m_Camera;
        shellDeps.overlayRenderer = m_OverlayRenderer.get();
        shellDeps.eventSystem = m_UIEventSystem;
        shellDeps.dpiScale = we::runtime::kindui::DPIContext::GetScale();
        shellDeps.onCreateNewLevel = [this]() { CreateNewLevel(); };
        shellDeps.onViewportCreated = [this](std::shared_ptr<we::runtime::kindui::Widget>& viewportWidget) {
            m_ViewportWidget = viewportWidget;
        };
        shellDeps.onLayoutBuilt = [this](const we::runtime::kindui::DockLayoutBuildResult&) {
            // Title bar reference retained for window hit testing.
        };

        const auto shellResult = we::runtime::kindui::EditorShellBuilder::Build(*m_UIContext, shellDeps);
        m_RootWidget = shellResult.rootWidget;
        m_OverlayHost = shellResult.overlayHost;
        m_TitleBar = shellResult.titleBar;
        m_StatusBar = shellResult.statusBar;

        m_WindowHitTestData.titleBar = m_TitleBar;
        we::platform::Platform::Get().SetWindowHitTest(
            m_Window,
            we::editor::mainframe::EditorWindowHitTest,
            &m_WindowHitTestData);
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to build editor shell: " + std::string(e.what()));
        throw;
    }

    try {
        m_UIEventSystem->SetRootWidget(m_RootWidget);
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to set root widget: " + std::string(e.what()));
        throw;
    }

    EnsureVisibleSwapchain();
    SyncViewportFramebufferFromLayout();
    try {
        LogWidgetTreeLayout(m_RootWidget, "OverlayHost");
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to log widget tree: " + std::string(e.what()));
    }
    m_FirstRunAgreementPending = !HasAcceptedFirstRunAgreement();

    HE_INFO("[Startup] Swapchain: " + std::to_string(m_Renderer->GetSwapchainWidth())
        + "x" + std::to_string(m_Renderer->GetSwapchainHeight()));
    HE_INFO("[Startup] === WindEffects Engine Editor successfully bootstrapped ===");
}

Editor::~Editor() {
    Shutdown();
}

void Editor::UpdateUiScaleFromWindow() {
    float scale = 1.0f;
    if (m_Window != we::platform::WindowId::Invalid) {
        auto& platform = we::platform::Platform::Get();
        const auto logical = platform.GetWindowSize(m_Window);
        const auto pixels = platform.GetWindowPixelSize(m_Window);
        if (logical.x > 0 && pixels.x > 0) {
            scale = static_cast<float>(pixels.x) / static_cast<float>(logical.x);
        } else {
            scale = platform.GetWindowDpiScale(m_Window);
        }
    }

    // Keep one canonical, bounded UI scale.
    const float clamped = std::clamp(scale, 1.0f, 3.0f);
    const float previous = we::runtime::kindui::DPIContext::GetScale();
    we::runtime::kindui::DPIContext::SetScale(clamped);
    if (std::abs(clamped - previous) > 0.001f) {
        we::runtime::kindui::UIRepaintGate::Request();
    }
}

void Editor::EnsureVisibleSwapchain() {
    auto& platform = we::platform::Platform::Get();
    auto pixelSize = platform.GetWindowPixelSize(m_Window);
    if (pixelSize.x == 0 || pixelSize.y == 0) {
        const auto logical = platform.GetWindowSize(m_Window);
        pixelSize = {logical.x, logical.y};
    }

    const int width = static_cast<int>(pixelSize.x);
    const int height = static_cast<int>(pixelSize.y);

    HE_INFO("[Render] Ensuring swapchain matches visible window (" + std::to_string(width) + "x" + std::to_string(height) + ")...");
    if (width > 0 && height > 0) {
        if (width != static_cast<int>(m_Renderer->GetSwapchainWidth()) ||
            height != static_cast<int>(m_Renderer->GetSwapchainHeight())) {
            m_Renderer->RecreateSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            HE_INFO("[Render] Swapchain recreated for visible window.");
        }
    } else {
        HE_ERROR("[Render] Window still reports zero size  UI layout will be empty until resized.");
    }
}

void Editor::SyncViewportFramebufferFromLayout() {
    if (!m_RootWidget || !m_Renderer) {
        return;
    }

    const uint32_t w = m_Renderer->GetSwapchainWidth();
    const uint32_t h = m_Renderer->GetSwapchainHeight();
    if (w == 0 || h == 0) {
        return;
    }

    const bool sizeChanged = w != m_LastLayoutSwapchainW || h != m_LastLayoutSwapchainH;
    const bool needsLayout = sizeChanged || we::runtime::kindui::UIRepaintGate::PeekNeedsRebuild();

    if (needsLayout) {
        // Layout the full editor chrome, then let the viewport panel own offscreen size.
        // Do not resize the offscreen framebuffer to the full window  that breaks aspect
        // ratio and can clip the 3D view inside the docked viewport widget.
        m_RootWidget->Measure(UI::Size{ static_cast<float>(w), static_cast<float>(h) });
        m_RootWidget->Arrange(UI::Rect{ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
        m_LastLayoutSwapchainW = w;
        m_LastLayoutSwapchainH = h;
    }

    if (m_ViewportWidget) {
        if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(m_ViewportWidget)) {
            vp->FlushPendingResize();
            vp->SyncRendererViewport();
        }
    }
}

void Editor::LogWidgetTreeLayout(const std::shared_ptr<UI::Widget>& widget, const std::string& name, int depth) {
    if (!widget) {
        HE_ERROR("[UI] Widget tree node '" + name + "' is null.");
        return;
    }

    const uint32_t w = m_Renderer->GetSwapchainWidth();
    const uint32_t h = m_Renderer->GetSwapchainHeight();
    widget->Measure(Size{ static_cast<float>(w), static_cast<float>(h) });
    widget->Arrange(Rect{ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });

    const Rect geom = widget->GetGeometry();
    std::string indent(depth * 2, ' ');
    HE_INFO("[UI] " + indent + name + " visible=" + (widget->IsVisible() ? "yes" : "no")
        + " geometry=" + std::to_string(static_cast<int>(geom.x)) + ","
        + std::to_string(static_cast<int>(geom.y)) + " "
        + std::to_string(static_cast<int>(geom.width)) + "x"
        + std::to_string(static_cast<int>(geom.height)));

    if (geom.width <= 0.0f || geom.height <= 0.0f) {
        HE_ERROR("[UI] " + indent + name + " has ZERO size - will not paint visible content.");
    }

    for (size_t i = 0; i < widget->GetChildren().size(); ++i) {
        LogWidgetTreeLayout(widget->GetChildren()[i], name + ".child[" + std::to_string(i) + "]", depth + 1);
    }
}

void Editor::Run() {
    MainLoop();
}

void Editor::CreateNewLevel() {
    if (!m_Scene) {
        return;
    }

    m_Scene->Clear();
    if (m_Scene->IsEmpty()) {
        we::runtime::world::environment::EnvironmentSystem::Get().EnsureDefaultEnvironment();
    }
    we::editor::environment::TickEditor();
}

void Editor::MaybeShowFirstRunAgreement() {
    if (HasAcceptedFirstRunAgreement()) {
        m_FirstRunAgreementPending = false;
        return;
    }

    if (!m_OverlayHost) {
        HE_WARN("[Startup] First-run agreement skipped: overlay host unavailable.");
        m_FirstRunAgreementPending = false;
        return;
    }

    // During the RHI UI cutover the agreement dialog can be invisible (missing fonts /
    // incomplete GPU UI). Auto-accept so the editor remains usable.
    HE_WARN("[Startup] First-run agreement auto-accepted (UI path still finishing RHI cutover).");
    SetAcceptedFirstRunAgreement(true);
    m_FirstRunAgreementPending = false;
}

void Editor::MainLoop() {
    auto& platform = we::platform::Platform::Get();
    uint64_t lastTime = platform.GetHighResolutionCounter();
    const double frequency = static_cast<double>(platform.GetHighResolutionFrequency());
    bool firstFrame = true;
    we::runtime::kindui::UIRepaintGate::Request();

    while (m_Running) {
        we::runtime::kindui::EditorPerfStats::Get().BeginFrame();

        if (!platform.PollEvents()) {
            m_Running = false;
            if (m_OverlayHost) {
                m_OverlayHost->ExecutePendingCallbacks();
            }
        }

        for (const auto& event : platform.GetFrameEvents()) {
            bool requestUiRebuild = false;
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
            } else if (std::holds_alternative<we::platform::WindowResizeEvent>(event)
                || std::holds_alternative<we::platform::WindowDpiEvent>(event)
                || std::holds_alternative<we::platform::WindowMaximizeEvent>(event)
                || std::holds_alternative<we::platform::WindowMinimizeEvent>(event)) {
                requestUiRebuild = true;
            } else if (const auto* move = std::get_if<we::platform::MouseMoveEvent>(&event)) {
                UI::MouseEvent mouseEvent{};
                mouseEvent.type = UI::MouseEventType::MouseMove;
                mouseEvent.position = UI::Point{ static_cast<float>(move->position.x), static_cast<float>(move->position.y) };
                mouseEvent.deltaX = move->delta.x;
                mouseEvent.deltaY = move->delta.y;
                const auto mods = platform.GetKeyModifiers();
                mouseEvent.altDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Control);
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);

                bool skipUiRebuild = false;
                if (m_ViewportWidget) {
                    if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(m_ViewportWidget)) {
                        if (vp->IsViewportNavigating()) {
                            skipUiRebuild = true;
                        }
                    }
                }
                if (!skipUiRebuild) {
                    requestUiRebuild = true;
                }
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
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);

                bool skipUiRebuild = false;
                if (m_ViewportWidget) {
                    if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(m_ViewportWidget)) {
                        if (vp->IsViewportNavigating()) {
                            skipUiRebuild = true;
                        }
                    }
                }
                if (!skipUiRebuild) {
                    requestUiRebuild = true;
                }
            } else if (const auto* button = std::get_if<we::platform::MouseButtonEvent>(&event)) {
                UI::MouseEvent mouseEvent{};
                mouseEvent.type = button->pressed ? UI::MouseEventType::MouseDown : UI::MouseEventType::MouseUp;
                mouseEvent.position = UI::Point{ static_cast<float>(button->position.x), static_cast<float>(button->position.y) };
                switch (button->button) {
                case we::platform::MouseButton::Left: mouseEvent.button = UI::MouseButton::Left; break;
                case we::platform::MouseButton::Right: mouseEvent.button = UI::MouseButton::Right; break;
                case we::platform::MouseButton::Middle: mouseEvent.button = UI::MouseButton::Middle; break;
                default: mouseEvent.button = UI::MouseButton::None; break;
                }
                mouseEvent.altDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Control);
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
                requestUiRebuild = true;
            } else if (const auto* wheel = std::get_if<we::platform::MouseWheelEvent>(&event)) {
                UI::MouseEvent mouseEvent{};
                mouseEvent.type = UI::MouseEventType::MouseWheel;
                mouseEvent.position = UI::Point{ static_cast<float>(wheel->position.x), static_cast<float>(wheel->position.y) };
                mouseEvent.wheelDeltaX = wheel->delta.x;
                mouseEvent.wheelDeltaY = wheel->delta.y;
                const auto mods = platform.GetKeyModifiers();
                mouseEvent.altDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Control);
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
                requestUiRebuild = true;
            } else if (const auto* key = std::get_if<we::platform::KeyEvent>(&event)) {
#if WE_DEBUG_UI
                if (key->pressed && key->key == we::platform::KeyCode::F9) {
                    we::runtime::kindui::RenderInvestigationModalHost::Toggle();
                }
#endif
                UI::KeyEvent keyEvent{};
                keyEvent.type = key->pressed ? UI::KeyEventType::KeyDown : UI::KeyEventType::KeyUp;
                keyEvent.key = key->key;
                keyEvent.altDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Alt);
                keyEvent.shiftDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Shift);
                keyEvent.ctrlDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Control);
                m_UIEventSystem->ProcessKeyEvent(keyEvent);
                requestUiRebuild = true;
            } else if (std::holds_alternative<we::platform::TextInputEvent>(event)) {
                requestUiRebuild = true;
            }

            (void)isMotion;
            if (requestUiRebuild) {
                we::runtime::kindui::UIRepaintGate::Request();
            }
        }
        
        // Execute pending callbacks after event processing to avoid use-after-free
        if (m_OverlayHost && m_OverlayHost->HasOpenPopups()) {
            m_OverlayHost->ExecutePendingCallbacks();
        }

        if (!m_Running) break;

        uint64_t now = platform.GetHighResolutionCounter();
        float dt = static_cast<float>((now - lastTime) / frequency);
        lastTime = now;
        if (dt > 0.1f) dt = 0.1f;
        // Input and fly movement update camera targets first; smoothing runs after.
        if (m_RootWidget) {
            m_RootWidget->Tick(dt);
        } else {
            HE_ERROR("[Render] Root widget is null during frame tick; stopping main loop.");
            m_Running = false;
            break;
        }
        if (m_Camera) {
            m_Camera->Update(dt);
        }
        if (m_Scene) {
            m_Scene->Update();
        }
        if (m_Camera) {
            auto& env = we::runtime::world::environment::EnvironmentSystem::Get();
            env.Tick(dt);
            env.SyncFromScene(m_Camera->GetPosition());
        }
        we::editor::environment::TickEditor();
        UpdateUiScaleFromWindow();
        we::runtime::kindui::EditorPerfStats::Get().Mark("tick");

        SyncViewportFramebufferFromLayout();
        we::runtime::kindui::EditorPerfStats::Get().Mark("layout");

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
            int skyDebugMode = 0;
            if (const char* v = std::getenv("WE_SKY_DEBUG")) {
                skyDebugMode = std::atoi(v);
            }
            cameraUBO.padding = static_cast<float>(skyDebugMode);
            we::runtime::renderer::FoundationRenderDebug::MaybeLog(skyDebugMode);
        }



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

            // CPU UI build + viewport sync before the graph (GPU overlay records inside UiOverlayPass).
            if (m_OverlayRenderer) {
                const uint32_t imageIndex = m_Renderer->GetCurrentImageIndex();
                m_OverlayRenderer->SetPipelineAuditImageIndex(imageIndex);
                m_OverlayRenderer->SetTargetExtent(
                    m_Renderer->GetSwapchainWidth(), m_Renderer->GetSwapchainHeight());
                m_OverlayRenderer->RenderEditorUI(m_RootWidget, m_Renderer->GetCurrentFrameIndex());
                we::runtime::kindui::EditorPerfStats::Get().Mark("ui");

                if (m_ViewportWidget) {
                    if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(m_ViewportWidget)) {
                        vp->FlushPendingResize();
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
                        we::editor::rendering::OverlayRenderContext overlayContext{};
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

            m_Renderer->RenderScene();
            we::runtime::kindui::EditorPerfStats::Get().Mark("scene");

            m_Renderer->SubmitAndPresent();
            we::runtime::kindui::EditorPerfStats::Get().Mark("present");
            m_Renderer->ClearOverlayRecorder();

            {
                const auto& stats = m_OverlayRenderer
                    ? m_OverlayRenderer->GetFrameStats()
                    : we::runtime::kindui::UIFrameStats{};
                we::runtime::kindui::EditorPerfStats::Get().EndFrame(stats.vertices, stats.batches);
            }

            if (firstFrame) {
                HE_INFO("[Render] First foundation renderer frame presented.");
                if (m_FirstRunAgreementPending) {
                    MaybeShowFirstRunAgreement();
                }
                firstFrame = false;
            }
        } else if (!m_Renderer) {
            HE_ERROR("[Render] Renderer is null in main loop.");
            we::runtime::kindui::EditorPerfStats::Get().EndFrame(0, 0);
        } else {
            we::runtime::kindui::EditorPerfStats::Get().EndFrame(0, 0);
        }
    }
}

void Editor::Shutdown() {
    if (m_OverlayHost) {
        m_OverlayHost->CloseAllPopups();
    }

    EditorWorkspaceController::Get().SaveLayout();

    if (m_Window != we::platform::WindowId::Invalid) {
        we::platform::Platform::Get().SetRelativeMouseMode(m_Window, false);
    }

    we::core::PluginManager::Get().UnloadAllPlugins();

    m_ViewportWidget.reset();
    m_OverlayHost.reset();
    m_RootWidget.reset();
    ShutdownContentBrowserService();
    if (m_OverlayRenderer) {
        m_OverlayRenderer->Shutdown();
        m_OverlayRenderer.reset();
    }

    // EditorCompositor no longer used
    m_UIEventSystem.reset();

    m_Scene.reset();
    m_Camera.reset();
    we::editor::grid::EditorGridRenderer::Get().Shutdown();
    if (m_Renderer) {
        m_Renderer->Shutdown();
        m_Renderer.reset();
    }
}

} // namespace we::programs::editor
