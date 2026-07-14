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
#include "Core/SwapchainManager.h"
#include "Rendering/IconRenderer.h"
#include "Layout/OverlayManager.h"
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
#include "Core/DPIContext.h"
#include "Core/UIRepaintGate.h"
#include "Core/EditorPerfStats.h"
#include "Debug/FoundationRenderDebug.h"
#include "Runtime/World/DefaultScene/DefaultSceneBuilder.h"
#include "Runtime/World/Environment/EnvironmentSystem.h"
#include "Widgets/RenderInvestigationModal.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>

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

#if defined(_WIN32)
#include "../Windows/Win32WindowChrome.h"
#endif

namespace we::programs::editor {

namespace UI = WindEffects::Editor::UI;

using namespace WindEffects::Editor::UI;
using namespace we::runtime::renderer;
using namespace we::runtime::scene;
using namespace we::runtime::engine;
using namespace we::runtime::world;

Editor::Editor(SDL_Window* window) : m_Window(window) {
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
            detail = m_Renderer->GetDeviceContext() ? "Foundation renderer initialized" : "Renderer missing device context";
            return m_Renderer->GetDeviceContext() != nullptr
                && m_Renderer->GetDeviceContext()->GetDevice() != VK_NULL_HANDLE;
        });
        startup.RunAll();
        if (!startup.AllPassed()) {
            throw std::runtime_error("Startup validation failed!");
        }
    }

    HE_INFO("[Startup] Stage 3/6: Default assets (fonts, shaders, icons, theme)...");
    if (!we::core::AssetRegistry::Get().LoadDefaultEditorAssets()) {
        HE_ERROR("[Startup] Required default assets missing â€” UI rendering may fail.");
    }

    HE_INFO("[Startup] Stage 4/6: OverlayRenderer init...");
    try {
        m_OverlayRenderer = std::make_unique<WindEffects::Editor::UI::OverlayRenderer>();
        if (!m_OverlayRenderer->Init(
                m_Renderer->GetDeviceContext()->GetPhysicalDevice(),
                m_Renderer->GetDeviceContext()->GetDevice(),
                m_Renderer->GetDeviceContext()->GetGraphicsQueue(),
                m_Renderer->GetDeviceContext()->GetGraphicsQueueFamily(),
                m_Renderer->GetSwapchainImageFormat(),
                2,
                m_Renderer->GetDeviceContext(),
                m_Renderer->GetResourceManager())) {
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

    m_UIContext = std::make_unique<WindEffects::Editor::UI::EditorApplicationContext>();
    m_UIContext->Initialize(WindEffects::Editor::UI::DPIContext::GetScale());

    const auto& extensionPanels = m_UIContext->GetExtensionRegistry().GetPanels();
    HE_INFO("[Startup] UI extension panels registered: " + std::to_string(extensionPanels.size()));
    for (const auto& [name, _] : extensionPanels) {
        HE_INFO("[Startup]   - Panel factory: " + name);
    }

    try {
        WindEffects::Editor::UI::EditorShellDependencies shellDeps;
        shellDeps.window = m_Window;
        shellDeps.renderer = m_Renderer.get();
        shellDeps.scene = m_Scene;
        shellDeps.camera = m_Camera;
        shellDeps.overlayRenderer = m_OverlayRenderer.get();
        shellDeps.eventSystem = m_UIEventSystem;
        shellDeps.dpiScale = WindEffects::Editor::UI::DPIContext::GetScale();
        shellDeps.onCreateNewLevel = [this]() { CreateNewLevel(); };
        shellDeps.onViewportCreated = [this](std::shared_ptr<WindEffects::Editor::UI::Widget>& viewportWidget) {
            m_ViewportWidget = viewportWidget;
        };
        shellDeps.onLayoutBuilt = [this](const WindEffects::Editor::UI::DockLayoutBuildResult&) {
            // Title bar reference retained for window hit testing.
        };

        const auto shellResult = WindEffects::Editor::UI::EditorShellBuilder::Build(*m_UIContext, shellDeps);
        m_RootWidget = shellResult.rootWidget;
        m_OverlayHost = shellResult.overlayHost;
        m_TitleBar = shellResult.titleBar;
        m_StatusBar = shellResult.statusBar;

        m_WindowHitTestData.titleBar = m_TitleBar;
        if (!SDL_SetWindowHitTest(m_Window, we::editor::mainframe::EditorWindowHitTest, &m_WindowHitTestData)) {
            HE_WARN("[UI] SDL_SetWindowHitTest failed â€” title bar drag may not work.");
        }
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
    if (m_Window) {
        int logicalW = 0;
        int pixelW = 0;
        SDL_GetWindowSize(m_Window, &logicalW, nullptr);
        if (logicalW > 0 && SDL_GetWindowSizeInPixels(m_Window, &pixelW, nullptr) && pixelW > 0) {
            scale = static_cast<float>(pixelW) / static_cast<float>(logicalW);
        }
    }

    // Keep one canonical, bounded UI scale.
    const float clamped = std::clamp(scale, 1.0f, 3.0f);
    const float previous = WindEffects::Editor::UI::DPIContext::GetScale();
    WindEffects::Editor::UI::DPIContext::SetScale(clamped);
    if (std::abs(clamped - previous) > 0.001f) {
        WindEffects::Editor::UI::UIRepaintGate::Request();
    }
}

void Editor::EnsureVisibleSwapchain() {
    int width = 0;
    int height = 0;
    if (!SDL_GetWindowSizeInPixels(m_Window, &width, &height) || width <= 0 || height <= 0) {
        SDL_GetWindowSize(m_Window, &width, &height);
    }

    HE_INFO("[Render] Ensuring swapchain matches visible window (" + std::to_string(width) + "x" + std::to_string(height) + ")...");
    if (width > 0 && height > 0) {
        if (width != static_cast<int>(m_Renderer->GetSwapchainWidth()) ||
            height != static_cast<int>(m_Renderer->GetSwapchainHeight())) {
            m_Renderer->RecreateSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            HE_INFO("[Render] Swapchain recreated for visible window.");
        }
    } else {
        HE_ERROR("[Render] Window still reports zero size â€” UI layout will be empty until resized.");
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
    const bool needsLayout = sizeChanged || WindEffects::Editor::UI::UIRepaintGate::PeekNeedsRebuild();

    if (needsLayout) {
        // Layout the full editor chrome, then let the viewport panel own offscreen size.
        // Do not resize the offscreen framebuffer to the full window — that breaks aspect
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
        HE_ERROR("[UI] " + indent + name + " has ZERO size â€” will not paint visible content.");
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
        return;
    }

    auto* overlayHost = m_OverlayHost.get();

    auto showEulaPopup = [this, overlayHost]() {
        try {
            auto eulaPopup = std::make_shared<FirstRunAgreementPopup>(LoadFirstRunAgreementEulaContent());
            eulaPopup->SetOnAccepted([this, overlayHost]() {
                overlayHost->CloseAllPopups();

                auto thirdPartyPopup = std::make_shared<FirstRunAgreementPopup>(
                    LoadFirstRunAgreementThirdPartyNoticesContent());
                thirdPartyPopup->SetOnAccepted([this, overlayHost]() {
                    SetAcceptedFirstRunAgreement(true);
                    overlayHost->CloseAllPopups();
                });
                thirdPartyPopup->SetOnDeclined([this, overlayHost]() {
                    SetAcceptedFirstRunAgreement(false);
                    overlayHost->CloseAllPopups();
                    m_Running = false;
                });

                overlayHost->ShowPopup(thirdPartyPopup, Point{ 0.0f, 0.0f });
            });
            eulaPopup->SetOnDeclined([this, overlayHost]() {
                SetAcceptedFirstRunAgreement(false);
                overlayHost->CloseAllPopups();
                m_Running = false;
            });

            overlayHost->CloseAllPopups();
            overlayHost->ShowPopup(eulaPopup, Point{ 0.0f, 0.0f });
            HE_INFO("[Startup] First-run agreement popup (EULA) displayed.");
        } catch (const std::exception& e) {
            HE_ERROR("[Startup] Failed to show first-run agreement: " + std::string(e.what()));
            SetAcceptedFirstRunAgreement(true);
        } catch (...) {
            HE_ERROR("[Startup] Unknown error showing first-run agreement");
            SetAcceptedFirstRunAgreement(true);
        }
    };

    m_FirstRunAgreementPending = false;
    showEulaPopup();
}

void Editor::MainLoop() {
    uint64_t lastTime = SDL_GetPerformanceCounter();
    double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    bool firstFrame = true;
    WindEffects::Editor::UI::UIRepaintGate::Request();

    while (m_Running) {
        WindEffects::Editor::UI::EditorPerfStats::Get().BeginFrame();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            bool requestUiRebuild = false;

            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_Window))) {
                m_Running = false;
                // Execute pending callbacks before shutdown to prevent use-after-free
                if (m_OverlayHost) {
                    m_OverlayHost->ExecutePendingCallbacks();
                }
            }

#if defined(_WIN32)
            if (event.window.windowID == SDL_GetWindowID(m_Window) &&
                (event.type == SDL_EVENT_WINDOW_RESIZED ||
                 event.type == SDL_EVENT_WINDOW_MAXIMIZED ||
                 event.type == SDL_EVENT_WINDOW_RESTORED)) {
                we::programs::windows::UpdateBorderlessWindowShape(m_Window);
                requestUiRebuild = true;
            }
#endif

            if (event.type == SDL_EVENT_WINDOW_RESIZED ||
                event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ||
                event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED) {
                requestUiRebuild = true;
            }

            // Simple event processing
            if (event.type == SDL_EVENT_MOUSE_MOTION ||
                event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                event.type == SDL_EVENT_MOUSE_BUTTON_UP ||
                event.type == SDL_EVENT_MOUSE_WHEEL) {

                UI::MouseEvent mouseEvent{};
                mouseEvent.position = UI::Point{ static_cast<float>(event.motion.x), static_cast<float>(event.motion.y) };

                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    mouseEvent.type = UI::MouseEventType::MouseMove;
                    mouseEvent.deltaX = static_cast<float>(event.motion.xrel);
                    mouseEvent.deltaY = static_cast<float>(event.motion.yrel);
                } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                    mouseEvent.type = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                        ? UI::MouseEventType::MouseDown
                        : UI::MouseEventType::MouseUp;
                    mouseEvent.position = UI::Point{ static_cast<float>(event.button.x), static_cast<float>(event.button.y) };

                    if (event.button.button == SDL_BUTTON_LEFT)   mouseEvent.button = UI::MouseButton::Left;
                    else if (event.button.button == SDL_BUTTON_RIGHT)  mouseEvent.button = UI::MouseButton::Right;
                    else if (event.button.button == SDL_BUTTON_MIDDLE) mouseEvent.button = UI::MouseButton::Middle;
                } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                    mouseEvent.type = UI::MouseEventType::MouseWheel;
                    mouseEvent.wheelDeltaX = static_cast<float>(event.wheel.x);
                    mouseEvent.wheelDeltaY = static_cast<float>(event.wheel.y);
                }

                const SDL_Keymod mods = SDL_GetModState();
                mouseEvent.altDown   = (mods & SDL_KMOD_ALT) != 0;
                mouseEvent.shiftDown = (mods & SDL_KMOD_SHIFT) != 0;
                mouseEvent.ctrlDown  = (mods & SDL_KMOD_CTRL) != 0;

                m_UIEventSystem->ProcessMouseEvent(mouseEvent);

                // Fly-look mouse motion only updates the camera; skip full UI rebuilds.
                bool flyLook = false;
                if (m_ViewportWidget) {
                    if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(m_ViewportWidget)) {
                        flyLook = vp->IsFlyLookActive();
                    }
                }
                if (!(flyLook && event.type == SDL_EVENT_MOUSE_MOTION)) {
                    requestUiRebuild = true;
                }
            } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
#if WE_DEBUG_UI
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F9) {
                    WindEffects::Editor::UI::RenderInvestigationModalHost::Toggle();
                }
#endif
                UI::KeyEvent keyEvent{};
                keyEvent.type = (event.type == SDL_EVENT_KEY_DOWN)
                    ? UI::KeyEventType::KeyDown
                    : UI::KeyEventType::KeyUp;
                keyEvent.keycode = event.key.key;

                const SDL_Keymod mods = event.key.mod;
                keyEvent.altDown   = (mods & SDL_KMOD_ALT) != 0;
                keyEvent.shiftDown = (mods & SDL_KMOD_SHIFT) != 0;
                keyEvent.ctrlDown  = (mods & SDL_KMOD_CTRL) != 0;

                m_UIEventSystem->ProcessKeyEvent(keyEvent);
                requestUiRebuild = true;
            } else if (event.type == SDL_EVENT_TEXT_INPUT) {
                requestUiRebuild = true;
            }

            if (requestUiRebuild) {
                WindEffects::Editor::UI::UIRepaintGate::Request();
            }
        }
        
        // Execute pending callbacks after event processing to avoid use-after-free
        if (m_OverlayHost && m_OverlayHost->HasOpenPopups()) {
            m_OverlayHost->ExecutePendingCallbacks();
        }

        if (!m_Running) break;

        uint64_t now = SDL_GetPerformanceCounter();
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
            we::runtime::world::environment::EnvironmentSystem::Get().SyncFromScene(m_Camera->GetPosition());
        }
        we::editor::environment::TickEditor();
        UpdateUiScaleFromWindow();
        WindEffects::Editor::UI::EditorPerfStats::Get().Mark("tick");

        SyncViewportFramebufferFromLayout();
        WindEffects::Editor::UI::EditorPerfStats::Get().Mark("layout");

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

            // 1. Render scene into the viewport offscreen target (sampled by the UI viewport widget).
            m_Renderer->RenderScene();
            WindEffects::Editor::UI::EditorPerfStats::Get().Mark("scene");

            // 2. Render editor UI (viewport panel composites the 3D target as a textured quad).
            if (m_OverlayRenderer) {
                VkCommandBuffer cmd = m_Renderer->GetCommandBuffer();
                auto* swapchainManager = m_Renderer->GetSwapchainManager();
                const uint32_t imageIndex = m_Renderer->GetCurrentImageIndex();
                const auto& imageViews = swapchainManager ? swapchainManager->GetImageViews() : std::vector<VkImageView>{};
                const bool overlayInputsValid =
                    cmd != VK_NULL_HANDLE &&
                    swapchainManager != nullptr &&
                    imageIndex < imageViews.size() &&
                    imageViews[imageIndex] != VK_NULL_HANDLE;

                if (!overlayInputsValid) {
                    HE_ERROR(
                        "[Render] Skipping overlay pass: invalid swapchain/ui inputs "
                        "(cmd=" + std::to_string(static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(cmd))) +
                        ", imageIndex=" + std::to_string(imageIndex) +
                        ", imageViews=" + std::to_string(imageViews.size()) + ").");
                } else {
                we::editor::rendering::OverlayRenderContext overlayContext{};
                overlayContext.cmd = cmd;
                overlayContext.targetView = imageViews[imageIndex];
                overlayContext.targetFormat = m_Renderer->GetSwapchainImageFormat();
                overlayContext.targetExtent = { m_Renderer->GetSwapchainWidth(), m_Renderer->GetSwapchainHeight() };
                // Editor UI is authored in full swapchain coordinates; do not apply the scene viewport offset.
                overlayContext.viewportOffset = {0, 0};

                const auto& swapchainImages = swapchainManager->GetImages();
                const VkImage swapImage =
                    imageIndex < swapchainImages.size() ? swapchainImages[imageIndex] : VK_NULL_HANDLE;

                m_Renderer->RecordUiPresentPath(imageIndex, cmd);
                m_OverlayRenderer->SetPipelineAuditImageIndex(imageIndex);
                if (PresentAuditEnabled()) {
                    HE_INFO(
                        std::string("[PresentAudit] UI renders image=") + std::to_string(imageIndex) +
                        " | Cmd=0x" + std::to_string(reinterpret_cast<uint64_t>(cmd)) +
                        " | targetView=0x" + std::to_string(reinterpret_cast<uint64_t>(overlayContext.targetView)) +
                        " | swapImage=0x" + std::to_string(reinterpret_cast<uint64_t>(swapImage)) +
                        " | Layout preUI=COLOR_ATTACHMENT_OPTIMAL");
                }

                // Build UI geometry before entering the dynamic render pass.
                m_OverlayRenderer->SetTargetExtent(overlayContext.targetExtent.width, overlayContext.targetExtent.height);
                m_OverlayRenderer->RenderEditorUI(m_RootWidget, m_Renderer->GetCurrentFrameIndex());
                WindEffects::Editor::UI::EditorPerfStats::Get().Mark("ui");

                if (m_ViewportWidget) {
                    if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(m_ViewportWidget)) {
                        vp->FlushPendingResize();
                        vp->SyncRendererViewport();
                    }
                }

                // Keep Vulkan synchronization inside Renderer module where dispatch is initialized.
                m_Renderer->InsertOverlayPassBarrier();
                m_OverlayRenderer->BeginOverlayPass(overlayContext);
                m_OverlayRenderer->EndOverlayPass(overlayContext);
                m_Renderer->MarkOverlayPassEnded();
                }
            }

            // 3. Submit to GPU and Present
            m_Renderer->SubmitAndPresent();
            WindEffects::Editor::UI::EditorPerfStats::Get().Mark("present");

            {
                const auto& stats = m_OverlayRenderer
                    ? m_OverlayRenderer->GetFrameStats()
                    : WindEffects::Editor::UI::UIFrameStats{};
                WindEffects::Editor::UI::EditorPerfStats::Get().EndFrame(stats.vertices, stats.batches);
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
            WindEffects::Editor::UI::EditorPerfStats::Get().EndFrame(0, 0);
        } else {
            WindEffects::Editor::UI::EditorPerfStats::Get().EndFrame(0, 0);
        }
    }
}

void Editor::Shutdown() {
    if (m_OverlayHost) {
        m_OverlayHost->CloseAllPopups();
    }

    EditorWorkspaceController::Get().SaveLayout();

    if (m_Window) {
        SDL_SetWindowRelativeMouseMode(m_Window, false);
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
