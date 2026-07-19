#include "Editor.h"
#include "FirstRunAgreementPopup.h"
#include "Core/Logger.h"
#include "Core/DiagnosticMacros.h"
#include "Core/FrameCounter.h"
#include "Core/StartupValidator.h"
#include "Core/LogCategory.h"
#include "Core/IgniteBTInvoker.h"
#include "Core/PluginManager.h"
#include "Environment/EnvironmentEditorApi.h"
#include "Renderer/Renderer.h"
#include "KindUI/Rendering/IconRenderer.h"
#include "KindUI/Layout/OverlayManager.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"

#include "EditorWindowHitTest.h"
#include "EditorShellBuilder.h"
#include "Undo/Undo.h"
#include "PropertyEditor/IPropertyEditorRuntime.h"
#include "PropertyEditor/PropertyEditorSession.h"
#include "ViewportEdit/ViewportEdit.h"
#include "WorldOutliner/WorldOutliner.h"
#include "Serialization/ISerializer.h"
#include "Reflection/ITypeRegistry.h"
#include "Widgets/ViewportWidget.h"
#include "ViewportToolbarState.h"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "ContentBrowser/ContentBrowserApi.h"
#include "Core/AssetRegistry.h"
#include "EditorGridRenderer.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Theming/ThemeManager.h"
#include "WindEffects/Editor/UI/Core/EditorPerfStats.h"
#include "Debug/FoundationRenderDebug.h"
#include "DefaultScene/DefaultSceneBuilder.h"
#include "Environment/EnvironmentSystem.h"
#include "Environment/EnvironmentLighting.h"
#include "WindEffects/Editor/UI/Widgets/RenderInvestigationModal.h"
#include "WindEffects/Editor/UI/Builders/PanelBuilder.h"

#include "Projects/EngineContext.h"
#include "Projects/ProjectContext.h"
#include "Projects/ProjectLifecycle.h"
#include "Projects/RecentProjectsStore.h"

#include "Platform/PlatformSDK.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
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
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::UIRepaintGate;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::MouseEventType;

namespace UI = we::runtime::kindui;

using namespace we::runtime::kindui;
using namespace we::runtime::kindui;
using ::we::editor::panels::Panel;
using ::we::editor::panels::PanelBuilder;
using ::we::editor::docking::DockZone;
using ::we::editor::viewport::ViewportWidget;
using namespace we::runtime::renderer;
using namespace we::runtime::scene;
using namespace we::runtime::engine;
using namespace we::runtime::world;

Editor::Editor(we::platform::WindowId window, const we::projects::EditorCommandLine& commandLine)
    : m_Window(window)
    , m_CommandLine(commandLine) {
    HE_INFO("[Startup] === Editor construction begin ===");
    InitializeEngine();

    we::projects::RecentProjectsStore::Get().Load();

    if (!m_CommandLine.projectPath) {
        throw std::runtime_error("Editor requires a .weproj path. Launch WeLauncher to select a project.");
    }
    EnterProjectWorkspace(*m_CommandLine.projectPath);

    HE_INFO("[Startup] Swapchain: " + std::to_string(m_Renderer->GetSwapchainWidth())
        + "x" + std::to_string(m_Renderer->GetSwapchainHeight()));
    HE_INFO("[Startup] === WindEffects Engine Editor successfully bootstrapped ===");
}

bool Editor::LaunchWeLauncher(const std::vector<std::string>& extraArgs) {
    auto& platform = we::platform::Platform::Get();
    const std::filesystem::path exeDir = platform.GetExecutableDirectory();
    const std::filesystem::path candidates[] = {
        exeDir / "WeLauncher.exe",
        exeDir / "WELauncher.exe",
    };

    std::filesystem::path launcher;
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            launcher = candidate;
            break;
        }
    }
    if (launcher.empty()) {
        HE_ERROR("[Startup] WeLauncher.exe not found in: " + exeDir.string());
        return false;
    }

    std::string exeUtf8 = launcher.string();
    const std::string workDir = exeDir.string();
    we::platform::ProcessLaunchDesc desc{};
    desc.executable = exeUtf8.c_str();
    desc.arguments = extraArgs;
    desc.workingDirectory = workDir.c_str();
    desc.waitForExit = false;
    desc.detach = true;

    const auto result = platform.LaunchProcess(desc);
    if (!result.Ok()) {
        HE_ERROR("[Startup] Failed to launch WeLauncher: "
            + (result.error.message.empty() ? "unknown error" : result.error.message));
        return false;
    }

    HE_INFO("[Startup] Launched WeLauncher: " + exeUtf8);
    return true;
}

void Editor::InitializeEngine() {
    HE_INFO("[Startup] Stage 1/6: Renderer...");
    m_Renderer = std::make_unique<Renderer>();
    m_Renderer->Init(m_Window);

    HE_INFO("[Startup] Stage 2/6: Scene and camera (engine-owned, empty until project loads)...");
    m_Camera = std::make_shared<EditorCamera>();
    BindViewportCamera(m_Camera);
    m_Scene = std::make_shared<Scene>();
    we::runtime::world::environment::EnvironmentSystem::Get().BindScene(m_Scene);
    PlaceActorsPlacement::Get().BindScene(m_Scene, m_Camera);

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

    HE_INFO("[Startup] Stage 3/6: Engine default assets (fonts, shaders, icons, theme)...");
    if (!we::core::AssetRegistry::Get().LoadDefaultEditorAssets()) {
        HE_ERROR("[Startup] Required default assets missing - UI rendering may fail.");
    }

    HE_INFO("[Startup] Stage 4/6: OverlayRenderer init...");
    m_OverlayRenderer = std::make_unique<we::runtime::kindui::OverlayRenderer>();
    if (!m_OverlayRenderer->Init(m_Renderer->GetRHIDevice(), m_Renderer->GetSwapchainFormat(), 2)) {
        throw std::runtime_error("Failed to initialize OverlayRenderer!");
    }

    m_UIEventSystem = std::make_shared<UI::EventSystem>();

    UpdateUiScaleFromWindow();
    m_UIContext = std::make_unique<::we::editor::services::EditorApplicationContext>();
    m_UIContext->Initialize(we::runtime::kindui::DPIContext::GetScale());
    UpdateUiScaleFromWindow();

    m_FirstRunAgreementPending = !HasAcceptedFirstRunAgreement();
    HE_INFO("[Startup] Engine context ready (project not loaded yet).");
}

void Editor::SetRootWidget(const std::shared_ptr<we::runtime::kindui::Widget>& root) {
    m_RootWidget = root;
    if (m_UIEventSystem) {
        m_UIEventSystem->SetRootWidget(m_RootWidget);
    }
    m_LastLayoutSwapchainW = 0;
    m_LastLayoutSwapchainH = 0;
    EnsureVisibleSwapchain();
    SyncViewportFramebufferFromLayout();
    we::runtime::kindui::UIRepaintGate::Request();
}

void Editor::UpdateWindowTitle() {
    auto& platform = we::platform::Platform::Get();
    if (we::projects::ProjectContext::Get().IsLoaded()) {
        const auto& desc = we::projects::ProjectContext::Get().Descriptor();
        const std::string name = desc.displayName.empty() ? desc.projectName : desc.displayName;
        platform.SetWindowTitle(m_Window, name + " - WindEffects Editor");
    } else {
        platform.SetWindowTitle(m_Window, "WindEffects Editor");
    }
}

void Editor::OpenWeLauncher() {
    if (!LaunchWeLauncher()) {
        m_StatusMessage = "WeLauncher.exe not found. Build the WeLauncher target.";
        HE_ERROR("[Startup] " + m_StatusMessage);
    }
}

void Editor::UnloadProjectWorkspace() {
    HE_INFO("[Startup] Unloading current project workspace...");

    if (m_OverlayHost) {
        m_OverlayHost->CloseAllPopups();
    }
    EditorWorkspaceController::Get().SaveLayout();

    we::core::PluginManager::Get().UnloadAllPlugins();
    ShutdownContentBrowserService();

    if (m_Scene) {
        m_Scene->Clear();
    }

    m_ViewportWidget.reset();
    m_OverlayHost.reset();
    m_StatusBar.reset();
    m_TitleBar.reset();

    if (m_UndoRuntime) {
        m_UndoRuntime->Shutdown();
        m_UndoRuntime.reset();
    }
    m_ViewportEdit.reset();
    ::we::editor::viewportedit::ViewportEditSession::Clear();
    ::we::editor::property::PropertyEditorSession::Clear();
    m_Serializer.reset();

    if (we::projects::ProjectContext::Get().IsLoaded()) {
        we::projects::ProjectContext::Get().Unload();
    }
}

void Editor::EnterProjectWorkspace(const std::filesystem::path& weprojPath) {
    HE_INFO("[Startup] Loading project: " + weprojPath.string());

    if (we::projects::ProjectContext::Get().IsLoaded()
        || m_ViewportWidget
        || m_StatusBar) {
        UnloadProjectWorkspace();
    }

    auto validation = we::projects::ProjectLifecycle::ValidateProjectPath(
        weprojPath,
        we::projects::EngineContext::Get().EngineVersion());
    if (validation.needsUpgrade) {
        const auto upgrade = we::projects::ProjectLifecycle::UpgradeProject(
            weprojPath,
            we::projects::EngineContext::Get().EngineVersion(),
            we::projects::EngineContext::Get().EngineRoot().string());
        HE_INFO("[Startup] Project upgrade: " + upgrade.message);
        m_StatusMessage = upgrade.message;
        validation = we::projects::ProjectLifecycle::ValidateProjectPath(
            weprojPath,
            we::projects::EngineContext::Get().EngineVersion());
    }

    if (!validation.ok) {
        HE_ERROR("[Startup] Project validation failed: " + validation.message);
        m_StatusMessage = validation.message;
        OpenWeLauncher();
        m_Running = false;
        return;
    }
    if (validation.missingSdk) {
        HE_WARN("[Startup] Missing SDK / config warning: " + validation.message);
        m_StatusMessage = validation.message;
    }

    const auto loadResult = we::projects::ProjectContext::Get().Load(weprojPath);
    if (!loadResult.ok) {
        HE_ERROR("[Startup] Failed to load ProjectContext: " + loadResult.message);
        m_StatusMessage = loadResult.message;
        OpenWeLauncher();
        m_Running = false;
        return;
    }

    auto& project = we::projects::ProjectContext::Get();

    // Content / asset registry from ProjectContext only.
    try {
        InitializeContentBrowserService(
            m_OverlayRenderer->GetIconRenderer(),
            project.ContentRoot().string());
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to initialize content browser: " + std::string(e.what()));
        project.Unload();
        OpenWeLauncher();
        m_Running = false;
        return;
    }

    // Project plugins (never a hardcoded path).
    if (!m_CommandLine.safeMode) {
        try {
            we::core::PluginManager::Get().ScanAndLoadPlugins(project.PluginsRoot().string());
        } catch (const std::exception& e) {
            HE_ERROR("[Startup] Failed to load project plugins: " + std::string(e.what()));
        }
    } else {
        HE_INFO("[Startup] Safe mode: skipping project plugins.");
    }

    // Editor world — empty default environment; startup map path recorded on context.
    if (m_Scene) {
        m_Scene->Clear();
    }
    DefaultSceneBuilder::CreateDefaultScene(*m_Scene);
    if (!project.Descriptor().startupMap.empty()) {
        project.SetCurrentMap(project.Descriptor().startupMap);
        HE_INFO("[Startup] Startup map from .weproj: " + project.Descriptor().startupMap
            + " (scene file load pipeline reserved)");
    }
    we::runtime::world::environment::EnvironmentSystem::Get().UpdateRendering(m_Camera->GetPosition());

    HE_INFO("[Startup] Building editor shell for project...");
    try {
        we::editor::undo::UndoDependencies undoDeps;
        undoDeps.onLog = [](std::string_view msg) {
            HE_INFO(std::string(msg));
        };
        m_UndoRuntime = we::editor::undo::CreateUndoRuntime(std::move(undoDeps));

        m_Serializer = we::runtime::serialization::CreateSerializer({});

        we::editor::property::PropertyEditorDependencies peDeps;
        peDeps.typeRegistry = &we::runtime::reflection::GetTypeRegistry();
        peDeps.serializer = m_Serializer.get();
        if (m_UndoRuntime) {
            peDeps.transactionHook = m_UndoRuntime->MakePropertyTransactionHook();
        }
        peDeps.onLog = [](std::string_view msg) {
            HE_INFO(std::string(msg));
        };
        auto peRuntime = we::editor::property::CreatePropertyEditorRuntime(std::move(peDeps));
        auto detailsView = peRuntime ? peRuntime->MakeDetailsView() : nullptr;
        we::editor::property::PropertyEditorSession::Install(
            std::shared_ptr<we::editor::property::IPropertyEditorRuntime>(std::move(peRuntime)),
            std::shared_ptr<we::editor::property::IDetailsView>(std::move(detailsView)));

        we::editor::viewportedit::ViewportEditDependencies veDeps;
        veDeps.undo = m_UndoRuntime.get();
        veDeps.propertyEditor = we::editor::property::PropertyEditorSession::Runtime();
        veDeps.scene = m_Scene.get();
        veDeps.editorCamera = m_Camera.get();
        m_ViewportEdit = we::editor::viewportedit::CreateViewportEditRuntime(veDeps);
        we::editor::viewportedit::ViewportEditSession::Install(m_ViewportEdit);

        we::programs::editor::EditorShellDependencies shellDeps;
        shellDeps.window = m_Window;
        shellDeps.renderer = m_Renderer.get();
        shellDeps.scene = m_Scene;
        shellDeps.camera = m_Camera;
        shellDeps.overlayRenderer = m_OverlayRenderer.get();
        shellDeps.eventSystem = m_UIEventSystem;
        shellDeps.dpiScale = we::runtime::kindui::DPIContext::GetScale();
        shellDeps.onCreateNewLevel = [this]() { CreateNewLevel(); };
        shellDeps.onOpenProject = [this]() { OpenProjectDialog(); };
        shellDeps.onOpenProjectManager = [this]() { OpenWeLauncher(); };
        shellDeps.onUndo = [this]() {
            if (m_UndoRuntime) {
                (void)m_UndoRuntime->Manager().Undo();
            }
        };
        shellDeps.onRedo = [this]() {
            if (m_UndoRuntime) {
                (void)m_UndoRuntime->Manager().Redo();
            }
        };
        shellDeps.onViewportCreated = [this](std::shared_ptr<we::runtime::kindui::Widget>& viewportWidget) {
            m_ViewportWidget = viewportWidget;
            if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(viewportWidget)) {
                vp->SetEditInputHandler([this, vp](const we::runtime::kindui::MouseEvent& event, float localX, float localY) {
                    if (!m_ViewportEdit) {
                        return false;
                    }
                    const auto geom = vp->GetGeometry();
                    m_ViewportEdit->SetViewportSize(std::max(1.f, geom.width), std::max(1.f, geom.height));

                    we::editor::viewportedit::ViewportInputEvent ve;
                    ve.x = localX;
                    ve.y = localY;
                    ve.deltaX = event.deltaX;
                    ve.deltaY = event.deltaY;
                    ve.scroll = event.wheelDeltaY;
                    ve.shift = event.shiftDown;
                    ve.ctrl = event.ctrlDown;
                    ve.alt = event.altDown;
                    switch (event.button) {
                    case we::runtime::kindui::MouseButton::Left:
                        ve.button = we::editor::viewportedit::ViewportMouseButton::Left;
                        break;
                    case we::runtime::kindui::MouseButton::Right:
                        ve.button = we::editor::viewportedit::ViewportMouseButton::Right;
                        break;
                    case we::runtime::kindui::MouseButton::Middle:
                        ve.button = we::editor::viewportedit::ViewportMouseButton::Middle;
                        break;
                    default:
                        ve.button = we::editor::viewportedit::ViewportMouseButton::None;
                        break;
                    }

                    auto& interaction = m_ViewportEdit->Interaction();
                    switch (event.type) {
                    case we::runtime::kindui::MouseEventType::MouseDown:
                        return interaction.HandleMouseDown(ve);
                    case we::runtime::kindui::MouseEventType::MouseUp:
                        return interaction.HandleMouseUp(ve);
                    case we::runtime::kindui::MouseEventType::MouseMove:
                        return interaction.HandleMouseMove(ve);
                    case we::runtime::kindui::MouseEventType::MouseWheel:
                        return interaction.HandleScroll(ve);
                    }
                    return false;
                });
            }
        };
        shellDeps.onLayoutBuilt = [this](const ::we::editor::shell::DockLayoutBuildResult&) {};

        const auto shellResult = we::programs::editor::EditorShellBuilder::Build(*m_UIContext, shellDeps);
        m_OverlayHost = shellResult.overlayHost;
        m_TitleBar = shellResult.titleBar;
        m_StatusBar = shellResult.statusBar;

        m_WindowHitTestData.titleBar = m_TitleBar;
        we::platform::Platform::Get().SetWindowHitTest(
            m_Window,
            ::we::editor::mainframe::EditorWindowHitTest,
            &m_WindowHitTestData);

        SetRootWidget(shellResult.rootWidget);
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to build editor shell: " + std::string(e.what()));
        UnloadProjectWorkspace();
        OpenWeLauncher();
        m_Running = false;
        return;
    }

    UpdateWindowTitle();
    try {
        LogWidgetTreeLayout(m_RootWidget, "EditorShell");
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to log widget tree: " + std::string(e.what()));
    }
}

void Editor::OpenProjectDialog() {
    we::platform::FileDialogDesc desc{};
    desc.mode = we::platform::FileDialogMode::OpenFile;
    desc.title = "Open Project";
    desc.filters = {
        { "WindEffects Project", "*.weproj" },
        { "All Files", "*.*" },
    };
    const auto files = we::platform::Platform::Get().ShowFileDialog(desc);
    if (!files.empty()) {
        EnterProjectWorkspace(files.front());
    }
}

Editor::~Editor() {
    Shutdown();
}

void Editor::UpdateUiScaleFromWindow() {
    float scale = 1.0f;
    if (m_Window != we::platform::WindowId::Invalid) {
        auto& platform = we::platform::Platform::Get();
        // Layout uses client-area pixels (GetClientRect). DPI scale is independent of
        // outer vs client size — never derive scale from pixel/logical size ratios on Windows
        // where both APIs return the client rect.
        scale = platform.GetWindowDpiScale(m_Window);
        if (scale <= 0.0f) {
            scale = 1.0f;
        }
    }

    const float clamped = std::clamp(scale, 1.0f, 3.0f);
    const float previous = we::runtime::kindui::DPIContext::GetScale();
    we::runtime::kindui::DPIContext::SetScale(clamped);
    if (we::runtime::kindui::ThemeManager::Get().IsInitialized()) {
        we::runtime::kindui::ThemeManager::Get().SetDpiScale(clamped);
    }
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
        // Root Measure/Arrange uses the swapchain = Windows CLIENT RECT only
        // (GetClientRect). Title/menu/toolbar/status are flex chrome rows; the
        // workspace Column FlexGrow(1) receives the remaining client area.
        const UI::Rect clientRect{ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) };
        m_RootWidget->Measure(UI::Size{ clientRect.width, clientRect.height });
        m_RootWidget->Arrange(clientRect);
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
    ::we::editor::environment::TickEditor();
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
        ::we::editor::services::EditorPerfStats::Get().BeginFrame();

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
                    ::we::editor::panels::RenderInvestigationModalHost::Toggle();
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
        ::we::editor::environment::TickEditor();
        if (m_ViewportEdit) {
            m_ViewportEdit->SyncSelectionFromScene();
            m_ViewportEdit->Tick(dt);
        }
        UpdateUiScaleFromWindow();
        ::we::editor::services::EditorPerfStats::Get().Mark("tick");

        SyncViewportFramebufferFromLayout();
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
                m_OverlayRenderer->RenderUI(m_RootWidget, m_Renderer->GetCurrentFrameIndex());
                ::we::editor::services::EditorPerfStats::Get().Mark("ui");

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

            m_Renderer->RenderScene();
            ::we::editor::services::EditorPerfStats::Get().Mark("scene");

            m_Renderer->SubmitAndPresent();
            ::we::editor::services::EditorPerfStats::Get().Mark("present");
            m_Renderer->ClearOverlayRecorder();

            {
                const auto& stats = m_OverlayRenderer
                    ? m_OverlayRenderer->GetFrameStats()
                    : we::runtime::kindui::UIFrameStats{};
                ::we::editor::services::EditorPerfStats::Get().EndFrame(stats.vertices, stats.batches);
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
            ::we::editor::services::EditorPerfStats::Get().EndFrame(0, 0);
        } else {
            ::we::editor::services::EditorPerfStats::Get().EndFrame(0, 0);
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
    if (we::projects::ProjectContext::Get().IsLoaded()) {
        we::projects::ProjectContext::Get().Unload();
    }
    if (m_OverlayRenderer) {
        m_OverlayRenderer->Shutdown();
        m_OverlayRenderer.reset();
    }

    // EditorCompositor no longer used
    m_UIEventSystem.reset();

    m_Scene.reset();
    m_Camera.reset();
    ::we::editor::grid::EditorGridRenderer::Get().Shutdown();
    if (m_Renderer) {
        m_Renderer->Shutdown();
        m_Renderer.reset();
    }
}

} // namespace we::programs::editor
