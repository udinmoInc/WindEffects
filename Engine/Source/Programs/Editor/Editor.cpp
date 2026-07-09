#include "Editor.h"
#include "FirstRunAgreementPopup.h"
#include "Core/Logger.h"
#include "Core/DiagnosticMacros.h"
#include "Core/FrameCounter.h"
#include "Core/StartupValidator.h"
#include "Core/LogCategory.h"
#include "Shader/ShaderLibrary.h"
#include "Core/IgniteBTInvoker.h"
#include "Runtime/Core/ModuleManager.h"
#include "Runtime/Core/PluginManager.h"
#include "EditorRegistry.h"
#include "Environment/EnvironmentEditorApi.h"
#include "Core/DeviceContext.h"
#include "Renderer/Renderer.h"
#include "Core/SwapchainManager.h"
#include "Rendering/IconRenderer.h"
#include "Widgets/Panel.h"
#include "Layout/Box.h"
#include "Layout/Splitter.h"
#include "Layout/OverlayManager.h"
#include "Rendering/EditorCompositor.h"
#include "Core/Icon.h"

#include <cstring>
#include <cstdlib>

// Feature-module widgets (linked via delay-load)
#include "Widgets/TitleBar.h"
#include "Widgets/WindowShell.h"
#include "EditorWindowHitTest.h"
#include "Widgets/StatusBar.h"
#include "Widgets/Toolbar.h"
#include "Widgets/MenuBar.h"
#include "Widgets/StatusBar.h"
#include "Widgets/ViewportWidget.h"
#include "ViewportToolbarState.h"
#include "ViewportNavigationPreferences.h"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "Widgets/ToolsPanel.h"
#include "Widgets/DockContainer.h"
#include "Widgets/EditorModeSelector.h"
#include "Widgets/Label.h"
#include "Widgets/RenderDebuggerPanel.h"
#include "Widgets/RenderInvestigationModal.h"
#include "EditorModeController.h"
#include "EditorLayoutController.h"
#include "EditorPanelController.h"
#include "ContentBrowserApi.h"
#include "EditorLayoutPersistence.h"
#include "Core/DockTabBrandRegistry.h"
#include "Runtime/Core/AssetRegistry.h"
#include "EditorGridRenderer.h"
#include "Core/Theme.h"
#include "Core/DPIContext.h"
#include "Explorer/WorldOutlinerApi.h"
#include "Runtime/World/DefaultScene/DefaultSceneBuilder.h"
#include "Runtime/World/Environment/EnvironmentSystem.h"
#include "Widgets/PropertyEditor.h"
#include "Widgets/TreeView.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <filesystem>

#ifndef WE_DEBUG_UI
#define WE_DEBUG_UI 0
#endif

#if defined(_WIN32)
#include "../Windows/Win32WindowChrome.h"
#endif

namespace we::programs::editor {

using namespace we::UI;
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
        HE_ERROR("[Startup] Required default assets missing — UI rendering may fail.");
    }

    HE_INFO("[Startup] Stage 4/6: OverlayRenderer init...");
    try {
        m_OverlayRenderer = std::make_unique<we::UI::OverlayRenderer>();
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

  HE_INFO("[Startup] Stage 5/6: Modules and plugins...");
    try {
        we::core::ModuleManager::Get().StartupAllModules();
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to startup modules: " + std::string(e.what()));
        throw;
    }
    try {
        we::core::PluginManager::Get().ScanAndLoadPlugins("Plugins");
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to load plugins: " + std::string(e.what()));
        throw;
    }

    auto& panels = EditorRegistry::Get().GetPanels();
    HE_INFO("[Startup] EditorRegistry panels registered: " + std::to_string(panels.size()));
    for (const auto& [name, _] : panels) {
        HE_INFO("[Startup]   - Panel factory: " + name);
    }
    try {
        ValidateEditorPanels(panels);
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to validate editor panels: " + std::string(e.what()));
        throw;
    }

    auto& menus = EditorRegistry::Get().GetMenus();
    HE_INFO("[Startup] EditorRegistry menus registered: " + std::to_string(menus.size()));

    HE_INFO("[Startup] Stage 6/6: Widget tree and layout...");
    UpdateUiScaleFromWindow();
    try {
        BuildDynamicEditorUI();
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to build dynamic UI: " + std::string(e.what()));
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
        LogWidgetTreeLayout(m_RootWidget, "OverlayManager");
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

// LoadSvgIcon has been moved to IconRenderer.cpp

void Editor::BuildDynamicEditorUI() {
    HE_INFO("[UI] Building editor layout...");
    const float uiScale = (std::max)(1.0f, we::UI::DPIContext::GetScale());

    auto& panelFactories = EditorRegistry::Get().GetPanels();

    // ===== 1. Create Menu Bar =====
    auto menuBar = std::make_shared<MenuBar>();
    
    // File menu
    std::vector<std::shared_ptr<MenuItem>> fileItems;
    auto newItem = std::make_shared<MenuItem>(); newItem->label = "New Level"; newItem->shortcut = "Ctrl+N"; newItem->onClick = [this]() { CreateNewLevel(); };
    auto openItem = std::make_shared<MenuItem>(); openItem->label = "Open Scene"; openItem->shortcut = "Ctrl+O";
    auto saveItem = std::make_shared<MenuItem>(); saveItem->label = "Save"; saveItem->shortcut = "Ctrl+S";
    auto saveAsItem = std::make_shared<MenuItem>(); saveAsItem->label = "Save As..."; saveAsItem->shortcut = "Ctrl+Shift+S";
    auto exitItem = std::make_shared<MenuItem>(); exitItem->label = "Exit"; exitItem->shortcut = "Alt+F4";
    fileItems.push_back(newItem); fileItems.push_back(openItem); 
    fileItems.push_back(saveItem); fileItems.push_back(saveAsItem); fileItems.push_back(exitItem);
    menuBar->AddMenu("File", fileItems);

    // Edit menu
    std::vector<std::shared_ptr<MenuItem>> editItems;
    auto undoItem = std::make_shared<MenuItem>(); undoItem->label = "Undo"; undoItem->shortcut = "Ctrl+Z";
    auto redoItem = std::make_shared<MenuItem>(); redoItem->label = "Redo"; redoItem->shortcut = "Ctrl+Y";
    auto cutItem = std::make_shared<MenuItem>(); cutItem->label = "Cut"; cutItem->shortcut = "Ctrl+X";
    auto copyItem = std::make_shared<MenuItem>(); copyItem->label = "Copy"; copyItem->shortcut = "Ctrl+C";
    auto pasteItem = std::make_shared<MenuItem>(); pasteItem->label = "Paste"; pasteItem->shortcut = "Ctrl+V";
    editItems.push_back(undoItem); editItems.push_back(redoItem);
    editItems.push_back(cutItem); editItems.push_back(copyItem); editItems.push_back(pasteItem);
    menuBar->AddMenu("Edit", editItems);

    // Window menu — wired to dockable panel visibility
    std::vector<std::shared_ptr<MenuItem>> windowItems;
    auto makeWindowItem = [](const std::string& label, EditorPanelId panelId) {
        auto item = std::make_shared<MenuItem>();
        item->label = label;
        item->checked = true;
        item->onClick = [panelId]() {
            EditorPanelController::Get().TogglePanelVisibility(panelId);
        };
        return item;
    };

    auto vpItem = makeWindowItem("Viewport", EditorPanelId::Viewport);
    auto cbItem = makeWindowItem("Content Browser", EditorPanelId::ContentBrowser);
    auto woItem = makeWindowItem("Explorer", EditorPanelId::Explorer);
    auto dtItem = makeWindowItem("Details", EditorPanelId::Details);
    windowItems.push_back(vpItem);
    windowItems.push_back(cbItem);
    windowItems.push_back(woItem);
    windowItems.push_back(dtItem);
#if WE_DEBUG_UI
    auto renderInvItem = std::make_shared<MenuItem>();
    renderInvItem->label = "Render Investigation";
    renderInvItem->onClick = []() { we::UI::RenderInvestigationModalHost::Toggle(); };
    windowItems.push_back(renderInvItem);
#endif
    menuBar->AddMenu("Window", windowItems);

    EditorPanelController::Get().SetOnPanelVisibilityChanged([vpItem, cbItem, woItem, dtItem]() {
        vpItem->checked = EditorPanelController::Get().IsPanelVisible(EditorPanelId::Viewport);
        cbItem->checked = EditorPanelController::Get().IsPanelVisible(EditorPanelId::ContentBrowser);
        woItem->checked = EditorPanelController::Get().IsPanelVisible(EditorPanelId::Explorer);
        dtItem->checked = EditorPanelController::Get().IsPanelVisible(EditorPanelId::Details);
    });

    // Tools menu
    std::vector<std::shared_ptr<MenuItem>> toolsItems;
    auto placeActorsItem = std::make_shared<MenuItem>(); placeActorsItem->label = "Place Actors";
    toolsItems.push_back(placeActorsItem);
#if WE_DEBUG_UI
    auto gpuInvItem = std::make_shared<MenuItem>();
    gpuInvItem->label = "GPU Investigation";
    gpuInvItem->onClick = []() { we::UI::RenderInvestigationModalHost::Toggle(); };
    toolsItems.push_back(gpuInvItem);
#endif
    menuBar->AddMenu("Tools", toolsItems);

    // Build menu
    std::vector<std::shared_ptr<MenuItem>> buildItems;
    auto compileItem = std::make_shared<MenuItem>();
    compileItem->label = "Compile";
    compileItem->onClick = []() {
        const auto result = we::core::InvokeIgniteBT({ "build", "--config", "Debug" });
        if (!result.launched) {
            HE_ERROR("[Build] " + result.errorMessage);
        } else if (result.exitCode != 0) {
            HE_ERROR("[Build] Compile failed with exit code " + std::to_string(result.exitCode));
        } else {
            HE_INFO("[Build] Compile completed successfully.");
        }
    };

    auto buildProjectItem = std::make_shared<MenuItem>();
    buildProjectItem->label = "Build";
    buildProjectItem->onClick = []() {
        const auto result = we::core::InvokeIgniteBT({ "build", "--config", "Debug" });
        if (!result.launched) {
            HE_ERROR("[Build] " + result.errorMessage);
        }
    };

    auto packageItem = std::make_shared<MenuItem>();
    packageItem->label = "Package";
    packageItem->onClick = []() {
        const auto result = we::core::InvokeIgniteBT({ "package" });
        if (!result.launched) {
            HE_ERROR("[Build] " + result.errorMessage);
        }
    };

    auto cookItem = std::make_shared<MenuItem>();
    cookItem->label = "Cook Content";
    cookItem->onClick = []() {
        const auto result = we::core::InvokeIgniteBT({ "package", "--cook" });
        if (!result.launched) {
            HE_ERROR("[Build] " + result.errorMessage);
        }
    };

    buildItems.push_back(compileItem);
    buildItems.push_back(buildProjectItem);
    buildItems.push_back(packageItem);
    buildItems.push_back(cookItem);
    menuBar->AddMenu("Build", buildItems);

    // Select menu
    std::vector<std::shared_ptr<MenuItem>> selectItems;
    auto selectAllItem = std::make_shared<MenuItem>(); selectAllItem->label = "Select All";
    auto deselectItem = std::make_shared<MenuItem>(); deselectItem->label = "Deselect All";
    selectItems.push_back(selectAllItem); selectItems.push_back(deselectItem);
    menuBar->AddMenu("Select", selectItems);

    menuBar->SetItemSpacing(0.0f);
    HE_INFO("[UI] Menu bar created with File, Edit, Window, Tools, Build, Select menus.");

    // ===== 2. Create Title Bar =====
    const int finalPxSize = static_cast<int>(std::round(we::UI::kTitleBarLogoDisplaySize * uiScale));

    VkDescriptorSet logoSet = VK_NULL_HANDLE;
    if (m_OverlayRenderer && m_OverlayRenderer->GetIconRenderer()) {
        logoSet = m_OverlayRenderer->GetIconRenderer()->GetIcon("Assets/Editor/WindEffects.svg", finalPxSize);
    }

    auto titleBar = std::make_shared<TitleBar>(m_Window, "WindEffects Editor", logoSet, menuBar);
    titleBar->Construct();
    m_TitleBar = titleBar;
    HE_INFO("[UI] Title bar created.");

    // ===== 3. Create Toolbar =====
    EditorModeController::Get().InitializeFromRegistry();

    auto toolbar = std::make_shared<Toolbar>();
    toolbar->SetHeight(36.0f * uiScale);
    toolbar->SetLeftInset(12.0f * uiScale);
    toolbar->SetIconSize(18.0f * uiScale);

    // Group 1: Actors Dropdown
    auto modeSelector = std::make_shared<we::programs::editor::EditorModeSelector>();
    toolbar->AddWidget(modeSelector);
    toolbar->AddSeparator();

    // Group 2: File operations
    toolbar->AddTool(Icons::NewName, "", [this](){ CreateNewLevel(); }, "New Level (Ctrl+N)");
    toolbar->AddTool(Icons::OpenName, "", [](){}, "Open (Ctrl+O)");
    toolbar->AddTool(Icons::SaveName, "", [](){}, "Save (Ctrl+S)");
    toolbar->AddSeparator();

    // Group 3: Transform tools
    toolbar->AddTool(Icons::CursorName, "", [](){}, "Select (Q)");
    toolbar->AddTool(Icons::MoveName, "", [](){}, "Move (W)");
    toolbar->AddTool(Icons::RotateName, "", [](){}, "Rotate (E)");
    toolbar->AddTool(Icons::ScaleName, "", [](){}, "Scale (R)");
    toolbar->AddTool(Icons::SnapName, "", [](){}, "Snap");
    toolbar->AddSeparator();

    // Group 4: Undo / Redo
    toolbar->AddTool(Icons::UndoName, "", [](){}, "Undo (Ctrl+Z)");
    toolbar->AddTool(Icons::RedoName, "", [](){}, "Redo (Ctrl+Y)");
    toolbar->AddSeparator();

    // Group 5: Environment Menu
    toolbar->AddWidget(we::editor::environment::CreateEnvironmentToolbarMenu());

    // Center Group: Play / Pause / Stop
    auto playBtn  = toolbar->AddTool(Icons::PlayName,  "", [](){}, "Play (Alt+P)",  false, ToolbarAlignment::Center);
    auto pauseBtn = toolbar->AddTool(Icons::PauseName, "", [](){}, "Pause (Alt+P)", false, ToolbarAlignment::Center);
    auto stopBtn  = toolbar->AddTool(Icons::StopName,  "", [](){}, "Stop",          false, ToolbarAlignment::Center);
    playBtn->SetButtonStyle(ToolButtonStyle::TransportButton);
    pauseBtn->SetButtonStyle(ToolButtonStyle::TransportButton);
    stopBtn->SetButtonStyle(ToolButtonStyle::TransportButton);

    // Right Group: Settings, Platform
    // Note: Items added with Right alignment stack from right-to-left
    auto platformBtn = toolbar->AddTool(Icons::PackageName, "Platform", [](){}, "Platform", false, ToolbarAlignment::Right);
    platformBtn->SetButtonStyle(ToolButtonStyle::ToolbarInline);

    auto settingsBtn = toolbar->AddTool(Icons::SettingsName, "Settings", [](){
        ShowViewportNavigationPreferences();
    }, "Editor Settings", false, ToolbarAlignment::Right);
    settingsBtn->SetButtonStyle(ToolButtonStyle::ToolbarInline);

    toolbar->SetActiveTool(Icons::CursorName);
    HE_INFO("[UI] Toolbar created with mode selector and tools.");

    // ===== 4. Create Viewports =====
    auto viewportWidget = std::make_shared<ViewportWidget>(
        static_cast<we::runtime::renderer::ISceneViewportController*>(m_Renderer.get()),
        m_Renderer->GetDeviceContext(),
        m_Renderer->GetResourceManager(),
        m_Renderer->GetSwapchainImageFormat(),
        m_Camera,
        m_Scene,
        m_OverlayRenderer.get());
    viewportWidget->Construct();
    viewportWidget->SetWindow(m_Window);
    m_ViewportWidget = viewportWidget;

    std::shared_ptr<Panel> toolsPanel;
    if (panelFactories.count("Tools")) {
        toolsPanel = panelFactories.at("Tools")();
        HE_INFO("[UI] Tools panel created from registry.");
    } else {
        toolsPanel = std::make_shared<Panel>("Tools");
        toolsPanel->SetHeaderHeight(30.0f * uiScale);
        auto toolsContent = std::make_shared<we::programs::editor::ToolsPanel>();
        toolsContent->InitializeFromRegistry();
        toolsPanel->SetContent(toolsContent);
        HE_INFO("[UI] Tools panel created (fallback).");
    }

    auto toolsDock = std::make_shared<DockContainer>();
    toolsDock->AddPanel(toolsPanel);
    toolsDock->SetVisible(EditorModeController::Get().IsDrawerVisible());
    EditorLayoutController::Get().SetToolsPanelRoot(toolsDock);
    
    std::shared_ptr<Panel> viewportPanel;
    if (panelFactories.count("Viewport")) {
        viewportPanel = panelFactories.at("Viewport")();
        viewportPanel->SetContent(viewportWidget);
        HE_INFO("[UI] Viewport panel created from registry with 3D ViewportWidget.");
    } else {
        viewportPanel = std::make_shared<Panel>("Viewport");
        viewportPanel->SetHeaderHeight(30.0f * uiScale);
        viewportPanel->SetContent(viewportWidget);
        HE_INFO("[UI] Viewport panel created (fallback).");
    }

    std::shared_ptr<Panel> gamePanel;
    if (panelFactories.count("Game")) {
        gamePanel = panelFactories.at("Game")();
        HE_INFO("[UI] Game panel created from registry.");
    } else {
        gamePanel = std::make_shared<Panel>("Game");
        gamePanel->SetHeaderHeight(30.0f * uiScale);
        HE_INFO("[UI] Game panel created (fallback).");
    }

    // Group Viewport and Game into a single tabbed dock container
    auto centralDock = std::make_shared<DockContainer>();
    centralDock->AddPanel(viewportPanel);
    centralDock->AddPanel(gamePanel);

    // ===== 5. Create World Outliner (Explorer) =====
    std::shared_ptr<Panel> worldOutlinerPanel;
    if (panelFactories.count("WorldOutliner")) {
        worldOutlinerPanel = panelFactories.at("WorldOutliner")();
        HE_INFO("[UI] Explorer panel created from registry.");
    } else {
        worldOutlinerPanel = std::make_shared<Panel>("Explorer");
        worldOutlinerPanel->SetHeaderHeight(0.0f);
        HE_INFO("[UI] Explorer panel created (fallback).");
    }

    if (m_OverlayRenderer && m_OverlayRenderer->GetIconRenderer()) {
        const float logoLogical = we::programs::editor::GetExplorerDockTabLogoSize();
        const int logoPx = static_cast<int>(std::round(logoLogical * uiScale));
        VkDescriptorSet explorerLogo = m_OverlayRenderer->GetIconRenderer()->GetIcon("Assets/Editor/WindEffects.svg", logoPx);
        we::UI::DockTabBrandRegistry::Get().RegisterBrand("Explorer", explorerLogo, logoLogical);
        we::programs::editor::BindExplorerBrandLogo(explorerLogo, logoLogical);
    }

    // ===== 6. Create Details Panel =====
    std::shared_ptr<Panel> detailsPanel;
    if (panelFactories.count("Details")) {
        detailsPanel = panelFactories.at("Details")();
        HE_INFO("[UI] Details panel created from registry.");
    } else {
        detailsPanel = std::make_shared<Panel>("Details");
        detailsPanel->SetHeaderHeight(30.0f * uiScale);
        HE_INFO("[UI] Details panel created (fallback).");
    }

    std::shared_ptr<Panel> viewportNavigationPanel;
    if (panelFactories.count("ViewportNavigation")) {
        viewportNavigationPanel = panelFactories.at("ViewportNavigation")();
        HE_INFO("[UI] Viewport Navigation panel created from registry.");
    } else {
        viewportNavigationPanel = std::make_shared<Panel>("Viewport Navigation");
        viewportNavigationPanel->SetHeaderHeight(30.0f * uiScale);
        HE_INFO("[UI] Viewport Navigation panel created (fallback).");
    }

    auto explorerDock = std::make_shared<DockContainer>();
    explorerDock->AddPanel(worldOutlinerPanel);
    explorerDock->SetOnTabClosed([](const std::shared_ptr<Panel>& panel) {
        if (panel && panel->GetTitle() == "Explorer") {
            EditorPanelController::Get().SetPanelVisible(EditorPanelId::Explorer, false);
        }
    });
    explorerDock->SetOnTabDragStarted([](const std::shared_ptr<Panel>& panel, const Point&) {
        if (panel && panel->GetTitle() == "Explorer") {
            EditorPanelController::Get().FloatPanel(EditorPanelId::Explorer);
        }
    });

    std::shared_ptr<TreeView> worldOutlinerTree = we::programs::editor::GetExplorerTreeView();
    std::shared_ptr<PropertyEditor> detailsEditor;
    if (detailsPanel) {
        detailsEditor = std::dynamic_pointer_cast<PropertyEditor>(detailsPanel->GetContent());
    }
    we::editor::environment::InitializeEditor(m_Scene, nullptr, worldOutlinerTree, detailsEditor);

    // ===== 7. Create Content Browser =====
    std::shared_ptr<Panel> contentBrowserPanel;
    if (panelFactories.count("ContentBrowser")) {
        contentBrowserPanel = panelFactories.at("ContentBrowser")();
        HE_INFO("[UI] ContentBrowser panel created from registry.");
    } else {
        contentBrowserPanel = std::make_shared<Panel>("Content Browser");
        contentBrowserPanel->SetHeaderHeight(30.0f * uiScale);
        HE_INFO("[UI] ContentBrowser panel created (fallback).");
    }

    // ===== 7b. Create Output Log Panel =====
    std::shared_ptr<Panel> outputLogPanel;
    if (panelFactories.count("OutputLog")) {
        outputLogPanel = panelFactories.at("OutputLog")();
        HE_INFO("[UI] Output Log panel created from registry.");
    } else {
        outputLogPanel = std::make_shared<Panel>("Output Log");
        outputLogPanel->SetHeaderHeight(30.0f * uiScale);
        outputLogPanel->SetContent(std::make_shared<Label>("Output log unavailable."));
        HE_WARN("[UI] OutputLog panel factory missing.");
    }

#if WE_DEBUG_UI
    // ===== 7c. Create Render Debugger Panel =====
    auto debugPanel = std::make_shared<Panel>("Render Debugger");
    debugPanel->SetHeaderHeight(30.0f * uiScale);
    m_RenderDebuggerPanel = std::make_shared<RenderDebuggerPanel>();
    m_RenderDebuggerPanel->Construct();
    debugPanel->SetContent(m_RenderDebuggerPanel);
    HE_INFO("[UI] Render Debugger panel created.");
#endif

    EditorPanelController::Get().RegisterDockZone(EditorDockZone::Left, toolsDock);
    EditorPanelController::Get().RegisterDockZone(EditorDockZone::Center, centralDock);
    EditorPanelController::Get().RegisterDockZone(EditorDockZone::Right, explorerDock);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::Tools, "Tools", toolsPanel, EditorDockZone::Left);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::Viewport, "Viewport", viewportPanel, EditorDockZone::Center);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::Game, "Game", gamePanel, EditorDockZone::Center);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::Explorer, "Explorer", worldOutlinerPanel, EditorDockZone::Right);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::Details, "Details", detailsPanel, EditorDockZone::Right);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::ViewportNavigation, "Viewport Navigation", viewportNavigationPanel, EditorDockZone::Floating);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::ContentBrowser, "Content Browser", contentBrowserPanel, EditorDockZone::Floating);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::OutputLog, "Output Log", outputLogPanel, EditorDockZone::Floating);
#if WE_DEBUG_UI
    EditorPanelController::Get().RegisterPanel(EditorPanelId::Debug, "Render Debugger", debugPanel, EditorDockZone::Floating);
#endif

    vpItem->checked = EditorPanelController::Get().IsPanelVisible(EditorPanelId::Viewport);
    cbItem->checked = EditorPanelController::Get().IsPanelVisible(EditorPanelId::ContentBrowser);
    woItem->checked = EditorPanelController::Get().IsPanelVisible(EditorPanelId::Explorer);
    dtItem->checked = EditorPanelController::Get().IsPanelVisible(EditorPanelId::Details);

    // ===== 8. Create Status Bar =====
    m_StatusBar = std::make_shared<StatusBar>();
    m_StatusBar->Construct();
    m_StatusBar->SetHeight(28.0f * uiScale);
    m_StatusBar->SetOnFooterTabChanged([](int index) {
        EditorLayoutController::Get().SetBottomPanelIndex(index);
    });
    m_StatusBar->SetOnCommandSubmitted([](const std::string& command) {
        HE_INFO("[Command] " + command);
    });
    m_StatusBar->SetOnOutputLogClicked([]() {
        EditorPanelController::Get().SetPanelVisible(EditorPanelId::OutputLog, true);
        EditorPanelController::Get().FocusPanel(EditorPanelId::OutputLog);
    });
    m_StatusBar->SetOnBuildMenuClicked([]() {
        const auto result = we::core::InvokeIgniteBT({ "build", "--config", "Debug" });
        if (!result.launched) {
            HE_ERROR("[Build] " + result.errorMessage);
        }
    });
    m_StatusBar->SetOnTraceClicked([]() {
        HE_INFO("[Footer] Trace clicked.");
    });
    m_StatusBar->SetOnQualityMenuClicked([]() {
        HE_INFO("[Footer] Quality menu clicked.");
    });
    HE_INFO("[UI] Status bar created.");

    // ===== 9. Assemble Layout =====
    //
    // VerticalBox (root)
    // ├── TitleBar (32px)
    // ├── Toolbar (32px)
    // ├── Splitter (Horizontal)
    // │   ├── Splitter (Vertical) — left + center column
    // │   │   ├── Splitter (Horizontal) — top row only
    // │   │   │   ├── Tools / Actors panel
    // │   │   │   └── Viewport dock
    // │   │   └── Content Browser (full width of left+center column)
    // │   └── Splitter (Vertical) — Explorer dock over Details panel
    // └── StatusBar (28px)

    // Right sidebar: Explorer (tabbed) stacked above Details (always visible, not a tab)
    auto rightSideSplitter = std::make_shared<Splitter>(Orientation::Vertical, 0.40f);
    rightSideSplitter->SetFirstChild(explorerDock);
    rightSideSplitter->SetSecondChild(detailsPanel);
    HE_INFO("[UI] Right sidebar: Explorer dock over Details panel.");

    // Top row inside left+center: Tools | Viewport (content browser sits below both)
    auto editorTopRow = std::make_shared<Splitter>(Orientation::Horizontal, 0.18f);
    editorTopRow->SetFirstChild(toolsDock);
    editorTopRow->SetSecondChild(centralDock);
    EditorLayoutController::Get().SetToolsPanelSplitter(editorTopRow);
    EditorLayoutController::Get().ApplyToolsPanelVisibility(EditorModeController::Get().IsDrawerVisible());
    HE_INFO("[UI] Top row splitter: Tools panel (18%) | Viewport.");

    // Left + center column: top row + content browser spanning full column width
    auto leftCenterColumn = std::make_shared<Splitter>(Orientation::Vertical, 0.7f);
    leftCenterColumn->SetFirstChild(editorTopRow);
    leftCenterColumn->SetSecondChild(contentBrowserPanel);
    EditorLayoutController::Get().SetContentBrowserSplitter(leftCenterColumn);
#if WE_DEBUG_UI
    EditorLayoutController::Get().SetBottomPanels(contentBrowserPanel, debugPanel);
#else
    EditorLayoutController::Get().SetBottomPanels(contentBrowserPanel, nullptr);
#endif
    HE_INFO("[UI] Left/center column: [Tools | Viewport] over full-width Content Browser.");

    // Main body: left+center column | right sidebar (narrow)
    auto mainHSplitter = std::make_shared<Splitter>(Orientation::Horizontal, 0.78f);
    mainHSplitter->SetFirstChild(leftCenterColumn);
    mainHSplitter->SetSecondChild(rightSideSplitter);
    HE_INFO("[UI] Horizontal splitter: Left/Center (78%) | Right sidebar (22%).");

    EditorLayoutPersistence::Get().BindLayout(mainHSplitter, leftCenterColumn, editorTopRow, rightSideSplitter, explorerDock, centralDock);

    // Root VBox
    auto rootVBox = std::make_shared<VerticalBox>();
    rootVBox->SetSpacing(0.0f);
    rootVBox->AddChild(titleBar);
    rootVBox->AddChild(toolbar);
    rootVBox->AddChild(mainHSplitter);
    rootVBox->AddChild(m_StatusBar);

    auto windowShell = std::make_shared<WindowShell>();
    windowShell->SetContent(rootVBox);

    // Wrap in OverlayManager for popups (dropdown menus, etc.)
    auto overlayManager = std::make_shared<OverlayManager>();
    overlayManager->SetBaseWidget(windowShell);
    m_RootWidget = overlayManager;

    // Restore saved layout after OverlayManager exists so floating panels can attach.
    EditorLayoutPersistence::Get().Load();

    m_WindowHitTestData.titleBar = m_TitleBar;
    if (!SDL_SetWindowHitTest(m_Window, we::editor::mainframe::EditorWindowHitTest, &m_WindowHitTestData)) {
        HE_WARN("[UI] SDL_SetWindowHitTest failed — title bar drag may not work.");
    }

    HE_INFO("[UI] Root widget tree assembled: TitleBar -> Toolbar -> [Viewport | Outliner | Details | ContentBrowser] -> StatusBar");
    HE_INFO("[UI] Root VBox children attached: " + std::to_string(rootVBox->GetChildren().size()));
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
    we::UI::DPIContext::SetScale(std::clamp(scale, 1.0f, 3.0f));
}

void Editor::ValidateEditorPanels(const std::unordered_map<std::string, std::function<std::shared_ptr<UI::Panel>()>>& factories) {
    static const char* kExpectedPanels[] = {
        "ContentBrowser", "Details", "WorldOutliner", "Viewport", "MenuBar", "Toolbar"
    };

    for (const char* panelName : kExpectedPanels) {
        // These panels are built inline in Editor — skip calling their factory to
        // avoid creating ghost widgets that render at wrong positions.
        if (std::string(panelName) == "Viewport" || std::string(panelName) == "MenuBar" || std::string(panelName) == "Toolbar") {
            HE_INFO(std::string("[UI] Panel '") + panelName + "' built inline in Editor (not from registry).");
            continue;
        }
        if (factories.count(panelName)) {
            auto panel = factories.at(panelName)();
            HE_INFO(std::string("[UI] Panel '") + panelName + "' factory OK (instance=" + (panel ? "yes" : "no") + ")");
        } else {
            HE_ERROR(std::string("[UI] Panel '") + panelName + "' NOT registered — fallback panel will be used.");
        }
    }
    HE_INFO("[UI] MainFrame shell: TitleBar + StatusBar built inline.");
    HE_INFO("[UI] DockManager: Explorer is a branded dock tab with layout persistence.");
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
        HE_ERROR("[Render] Window still reports zero size — UI layout will be empty until resized.");
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

    // Layout the full editor chrome, then let the viewport panel own offscreen size.
    // Do not resize the offscreen framebuffer to the full window — that breaks aspect
    // ratio and can clip the 3D view inside the docked viewport widget.
    m_RootWidget->Measure(UI::Size{ static_cast<float>(w), static_cast<float>(h) });
    m_RootWidget->Arrange(UI::Rect{ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });

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
        HE_ERROR("[UI] " + indent + name + " has ZERO size — will not paint visible content.");
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

    auto overlay = std::dynamic_pointer_cast<OverlayManager>(m_RootWidget);
    if (!overlay) {
        HE_WARN("[Startup] First-run agreement skipped: OverlayManager unavailable.");
        return;
    }

    const float viewportW = static_cast<float>(m_Renderer->GetSwapchainWidth());
    const float viewportH = static_cast<float>(m_Renderer->GetSwapchainHeight());

    auto* overlayMgr = OverlayManager::Get();
    if (!overlayMgr) {
        HE_WARN("[Startup] First-run agreement skipped: OverlayManager singleton unavailable.");
        return;
    }

    auto showEulaPopup = [this, overlayMgr]() {
        try {
            auto eulaPopup = std::make_shared<FirstRunAgreementPopup>(LoadFirstRunAgreementEulaContent());
            eulaPopup->SetOnAccepted([this, overlayMgr]() {
                overlayMgr->CloseAllPopups();

                auto thirdPartyPopup = std::make_shared<FirstRunAgreementPopup>(
                    LoadFirstRunAgreementThirdPartyNoticesContent());
                thirdPartyPopup->SetOnAccepted([this, overlayMgr]() {
                    SetAcceptedFirstRunAgreement(true);
                    overlayMgr->CloseAllPopups();
                });
                thirdPartyPopup->SetOnDeclined([this, overlayMgr]() {
                    SetAcceptedFirstRunAgreement(false);
                    overlayMgr->CloseAllPopups();
                    m_Running = false;
                });

                overlayMgr->ShowPopup(thirdPartyPopup, Point{ 0.0f, 0.0f });
            });
            eulaPopup->SetOnDeclined([this, overlayMgr]() {
                SetAcceptedFirstRunAgreement(false);
                overlayMgr->CloseAllPopups();
                m_Running = false;
            });

            overlayMgr->CloseAllPopups();
            overlayMgr->ShowPopup(eulaPopup, Point{ 0.0f, 0.0f });
            HE_INFO("[Startup] First-run agreement popup (EULA) displayed.");
        } catch (const std::exception& e) {
            HE_ERROR("[Startup] Failed to show first-run agreement: " + std::string(e.what()));
            SetAcceptedFirstRunAgreement(true); // Skip on error
        } catch (...) {
            HE_ERROR("[Startup] Unknown error showing first-run agreement");
            SetAcceptedFirstRunAgreement(true); // Skip on error
        }
    };

    m_FirstRunAgreementPending = false;
    showEulaPopup();
}

void Editor::MainLoop() {
    uint64_t lastTime = SDL_GetPerformanceCounter();
    double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    bool firstFrame = true;
    while (m_Running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_Window))) {
                m_Running = false;
                // Execute pending callbacks before shutdown to prevent use-after-free
                if (auto* overlayMgr = OverlayManager::Get()) {
                    overlayMgr->ExecutePendingCallbacks();
                }
            }

#if defined(_WIN32)
            if (event.window.windowID == SDL_GetWindowID(m_Window) &&
                (event.type == SDL_EVENT_WINDOW_RESIZED ||
                 event.type == SDL_EVENT_WINDOW_MAXIMIZED ||
                 event.type == SDL_EVENT_WINDOW_RESTORED)) {
                we::programs::windows::UpdateBorderlessWindowShape(m_Window);
            }
#endif

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
            } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
#if WE_DEBUG_UI
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F9) {
                    we::UI::RenderInvestigationModalHost::Toggle();
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
            }
        }
        
        // Execute pending callbacks after event processing to avoid use-after-free
        if (auto* overlayMgr = OverlayManager::Get()) {
            if (overlayMgr->HasOpenPopups()) {
                overlayMgr->ExecutePendingCallbacks();
            }
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

        SyncViewportFramebufferFromLayout();

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
        cameraUBO.padding = 0.0f;



        if (m_Renderer && m_Renderer->BeginFrame()) {
            m_Renderer->UploadCameraUniform(cameraUBO);

            // 1. Render Scene (directly to swapchain at viewport rect)
            m_Renderer->RenderScene();

            // 2. No compositor needed - scene renders directly to swapchain

            // 3. Render Independent UI (LDR) on top of compositor output
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
                HE_INFO(
                    std::string("[PresentAudit] UI renders image=") + std::to_string(imageIndex) +
                    " | Cmd=0x" + std::to_string(reinterpret_cast<uint64_t>(cmd)) +
                    " | targetView=0x" + std::to_string(reinterpret_cast<uint64_t>(overlayContext.targetView)) +
                    " | swapImage=0x" + std::to_string(reinterpret_cast<uint64_t>(swapImage)) +
                    " | Layout preUI=COLOR_ATTACHMENT_OPTIMAL");

                // Build UI geometry before entering the dynamic render pass.
                m_OverlayRenderer->SetTargetExtent(overlayContext.targetExtent.width, overlayContext.targetExtent.height);
                m_OverlayRenderer->RenderEditorUI(m_RootWidget, m_Renderer->GetCurrentFrameIndex());

                // Keep Vulkan synchronization inside Renderer module where dispatch is initialized.
                m_Renderer->InsertOverlayPassBarrier();
                m_OverlayRenderer->BeginOverlayPass(overlayContext);
                m_OverlayRenderer->EndOverlayPass(overlayContext);
                m_Renderer->MarkOverlayPassEnded();
                }
            }

            // 4. Submit to GPU and Present
            m_Renderer->SubmitAndPresent();

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
    }
}

void Editor::Shutdown() {
    // Clear any pending callbacks before shutdown to prevent use-after-free
    if (auto* overlayMgr = OverlayManager::Get()) {
        overlayMgr->CloseAllPopups();
    }

    EditorLayoutPersistence::Get().Save();

    if (m_Window) {
        SDL_SetWindowRelativeMouseMode(m_Window, false);
    }

    we::core::PluginManager::Get().UnloadAllPlugins();
    we::core::ModuleManager::Get().ShutdownAllModules();

    m_ViewportWidget.reset();
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
