#include "Editor.hpp"
#include "FirstRunAgreementPopup.hpp"
#include "Core/Logger.hpp"
#include "Runtime/Core/ModuleManager.hpp"
#include "Runtime/Core/PluginManager.hpp"
#include "EditorRegistry.hpp"

// HouseUI Layout & Widgets
#include "Widgets/Panel.hpp"
#include "Layout/Box.hpp"
#include "Layout/Splitter.hpp"
#include "Layout/OverlayManager.hpp"
#include "Core/Icon.hpp"

// Feature-module widgets (linked via delay-load)
#include "Widgets/TitleBar.hpp"
#include "Widgets/WindowShell.hpp"
#include "EditorWindowHitTest.hpp"
#include "Widgets/StatusBar.hpp"
#include "Widgets/Toolbar.hpp"
#include "Widgets/MenuBar.hpp"
#include "Widgets/StatusBar.hpp"
#include "Widgets/ViewportWidget.hpp"
#include "ViewportToolbarState.hpp"
#include "ViewportNavigationPreferences.hpp"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "Widgets/ToolsPanel.hpp"
#include "Widgets/DockContainer.hpp"
#include "Widgets/EditorModeSelector.hpp"
#include "Widgets/Label.hpp"
#include "EditorModeController.hpp"
#include "EditorLayoutController.hpp"
#include "EditorPanelController.hpp"
#include "ContentBrowserApi.h"
#include "EditorLayoutPersistence.hpp"
#include "Core/DockTabBrandRegistry.hpp"
#include "Runtime/Core/AssetRegistry.hpp"
#include "EditorPreferences.hpp"
#include "EditorGridRenderer.hpp"
#include "Core/Theme.hpp"
#include "Renderer/Shader/ShaderLibrary.hpp"
#include "Environment/EnvironmentEditorApi.h"
#include "Explorer/WorldOutlinerApi.h"
#include "Runtime/World/DefaultScene/DefaultSceneBuilder.h"
#include "Runtime/World/Environment/EnvironmentSystem.h"
#include "Widgets/PropertyEditor.hpp"
#include "Widgets/TreeView.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <filesystem>

#if defined(_WIN32)
#include "../Windows/Win32WindowChrome.hpp"
#endif

namespace we::programs::editor {

using namespace we::UI;
using namespace we::runtime::renderer;
using namespace we::runtime::scene;
using namespace we::runtime::engine;
using namespace we::runtime::world;

Editor::Editor(SDL_Window* window) : m_Window(window) {
    HE_INFO("[Startup] === Editor construction begin ===");

    HE_INFO("[Startup] Stage 1/6: Vulkan context...");
    m_Context = std::make_shared<VulkanContext>(m_Window);

    volkInitialize();
    volkLoadInstance(m_Context->GetInstance());
    volkLoadDevice(m_Context->GetDevice());

    {
        std::string shaderRoot = "Engine/Shaders";
        std::string bytecodeRoot = "Assets/Shaders";
        for (const char* candidate : {"Engine/Shaders", "../Engine/Shaders", "../../Engine/Shaders"})
        {
            if (std::filesystem::exists(candidate))
            {
                shaderRoot = candidate;
                break;
            }
        }
        for (const char* candidate : {"Assets/Shaders", "../Assets/Shaders"})
        {
            if (std::filesystem::exists(candidate))
            {
                bytecodeRoot = candidate;
                break;
            }
        }
        ShaderLibrary::Get().Initialize(shaderRoot, bytecodeRoot);
    }
    
    HE_INFO("[Startup] Stage 2/6: Renderer, scene, and camera...");
    m_Renderer = std::make_shared<Renderer>(m_Context, m_Window);
    m_RenderGraph = std::make_shared<RenderGraph>(m_Renderer);
    m_SceneRenderer = std::make_shared<SceneRenderer>(m_Context, m_Renderer->GetOffscreenRenderPass(), m_Renderer->GetCameraDescLayout());
    m_SceneRenderer->SetEditorBackgroundEnabled(true);
    EditorPreferences::Get().ApplyEditorViewportIfDirty(m_SceneRenderer);

    we::editor::grid::EditorGridRenderer::Get().Initialize(
        m_Context,
        m_Renderer->GetOffscreenRenderPass(),
        m_Renderer->GetCameraDescLayout());

    m_Camera = std::make_shared<EditorCamera>();
    BindViewportCamera(m_Camera);
    m_Scene = std::make_shared<Scene>(m_Context, m_SceneRenderer);
    m_Scene->SetCameraBuffer(m_Renderer->GetCameraBuffer());
    we::runtime::world::environment::EnvironmentSystem::Get().BindScene(m_Scene);
    we::runtime::world::environment::EnvironmentSystem::Get().BindRenderer(m_SceneRenderer);
    PlaceActorsPlacement::Get().BindScene(m_Scene, m_Camera);
    DefaultSceneBuilder::CreateDefaultScene(*m_Scene);

    HE_INFO("[Startup] Stage 3/6: Default assets (fonts, shaders, icons, theme)...");
    if (!we::core::AssetRegistry::Get().LoadDefaultEditorAssets()) {
        HE_ERROR("[Startup] Required default assets missing — UI rendering may fail.");
    }

    HE_INFO("[Startup] Stage 4/6: UIRenderer init...");
    m_UIRenderer = std::make_unique<UI::UIRenderer>();
    if (!m_UIRenderer->Init(m_Context, m_Renderer->GetSwapchainRenderPass())) {
        throw std::runtime_error("Failed to initialize HouseUI Renderer!");
    }
    InitializeContentBrowserService(m_UIRenderer->GetIconRenderer().get());
    m_UIEventSystem = std::make_shared<UI::EventSystem>();

  HE_INFO("[Startup] Stage 5/6: Modules and plugins...");
    we::core::ModuleManager::Get().StartupAllModules();
    we::core::PluginManager::Get().ScanAndLoadPlugins("Plugins");

    auto& panels = EditorRegistry::Get().GetPanels();
    HE_INFO("[Startup] EditorRegistry panels registered: " + std::to_string(panels.size()));
    for (const auto& [name, _] : panels) {
        HE_INFO("[Startup]   - Panel factory: " + name);
    }
    ValidateEditorPanels(panels);

    auto& menus = EditorRegistry::Get().GetMenus();
    HE_INFO("[Startup] EditorRegistry menus registered: " + std::to_string(menus.size()));

    HE_INFO("[Startup] Stage 6/6: Widget tree and layout...");
    BuildDynamicEditorUI();

    m_UIEventSystem->SetRootWidget(m_RootWidget);

    EnsureVisibleSwapchain();
    LogWidgetTreeLayout(m_RootWidget, "OverlayManager");
    MaybeShowFirstRunAgreement();

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
    menuBar->AddMenu("Tools", toolsItems);

    // Build menu
    std::vector<std::shared_ptr<MenuItem>> buildItems;
    auto compileItem = std::make_shared<MenuItem>(); compileItem->label = "Compile";
    auto cookItem = std::make_shared<MenuItem>(); cookItem->label = "Cook Content";
    buildItems.push_back(compileItem); buildItems.push_back(cookItem);
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
    // Scale logical size to physical pixels
    float displayScale = 1.0f;
    if (m_Window) {
        int logicalW = 0;
        int pixelW = 0;
        SDL_GetWindowSize(m_Window, &logicalW, nullptr);
        if (SDL_GetWindowSizeInPixels(m_Window, &pixelW, nullptr) && logicalW > 0) {
            displayScale = static_cast<float>(pixelW) / static_cast<float>(logicalW);
        }
    }
    
    int finalPxSize = static_cast<int>(std::round(we::UI::TitleBar::LogoDisplaySize * displayScale));

    VkDescriptorSet logoSet = VK_NULL_HANDLE;
    if (m_UIRenderer && m_UIRenderer->GetIconRenderer()) {
        logoSet = m_UIRenderer->GetIconRenderer()->GetIcon("Assets/Editor/windeffects.svg", finalPxSize);
    }

    auto titleBar = std::make_shared<TitleBar>(m_Window, "WindEffects Editor", logoSet, menuBar);
    titleBar->Construct();
    m_TitleBar = titleBar;
    HE_INFO("[UI] Title bar created.");

    // ===== 3. Create Toolbar =====
    EditorModeController::Get().InitializeFromRegistry();

    auto toolbar = std::make_shared<Toolbar>();
    toolbar->SetHeight(30.0f);
    toolbar->SetLeftInset(0.0f);   // Flush to border: no leading gap
    toolbar->SetIconSize(16.0f);

    auto modeSelector = std::make_shared<we::programs::editor::EditorModeSelector>();
    toolbar->AddWidget(modeSelector);

    // Group 1: File operations (Left)
    toolbar->AddTool(Icons::NewName, "", [this](){ CreateNewLevel(); }, "New Level (Ctrl+N)");
    toolbar->AddTool(Icons::OpenName, "", [](){}, "Open (Ctrl+O)");
    toolbar->AddTool(Icons::SaveName, "", [](){}, "Save (Ctrl+S)");
    toolbar->AddSeparator();


    // Group 2: Transform tools (Left)
    toolbar->AddTool(Icons::CursorName, "", [](){}, "Select (Q)");
    toolbar->AddTool(Icons::MoveName, "", [](){}, "Move (W)");
    toolbar->AddTool(Icons::RotateName, "", [](){}, "Rotate (E)");
    toolbar->AddTool(Icons::ScaleName, "", [](){}, "Scale (R)");
    toolbar->AddTool(Icons::SnapName, "", [](){}, "Snap");
    toolbar->AddSeparator();

    toolbar->AddWidget(we::editor::environment::CreateEnvironmentToolbarMenu());

    // Group 3: Transport controls (Center) – Play / Pause / Stop
    auto playBtn  = toolbar->AddTool(Icons::PlayName,  "", [](){}, "Play (Alt+P)",  false, ToolbarAlignment::Center);
    auto pauseBtn = toolbar->AddTool(Icons::PauseName, "", [](){}, "Pause (Alt+P)", false, ToolbarAlignment::Center);
    auto stopBtn  = toolbar->AddTool(Icons::StopName,  "", [](){}, "Stop",          false, ToolbarAlignment::Center);
    playBtn->SetButtonStyle(ToolButtonStyle::TransportButton);
    pauseBtn->SetButtonStyle(ToolButtonStyle::TransportButton);
    stopBtn->SetButtonStyle(ToolButtonStyle::TransportButton);

    // Group 4: Platform + Settings (Right) - added right-to-left order
    toolbar->AddSeparator(ToolbarAlignment::Right);

    auto settingsBtn = toolbar->AddTool(Icons::SettingsName, "Settings", [](){
        ShowViewportNavigationPreferences();
    }, "Editor Settings", false, ToolbarAlignment::Right);
    settingsBtn->SetButtonStyle(ToolButtonStyle::ToolbarInline);

    auto platformBtn = toolbar->AddTool(Icons::PackageName, "Platform", [](){}, "Platform", false, ToolbarAlignment::Right);
    platformBtn->SetButtonStyle(ToolButtonStyle::ToolbarInline);

    toolbar->SetActiveTool(Icons::CursorName);
    HE_INFO("[UI] Toolbar created with mode selector and tools.");

    // ===== 4. Create Viewports =====
    auto viewportWidget = std::make_shared<ViewportWidget>(m_Renderer, m_Camera, m_Scene, m_UIRenderer.get());
    viewportWidget->SetWindow(m_Window);
    m_ViewportWidget = viewportWidget;

    std::shared_ptr<Panel> toolsPanel;
    if (panelFactories.count("Tools")) {
        toolsPanel = panelFactories.at("Tools")();
        HE_INFO("[UI] Tools panel created from registry.");
    } else {
        toolsPanel = std::make_shared<Panel>("Tools");
        toolsPanel->SetHeaderHeight(30.0f);
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
        viewportPanel->SetHeaderHeight(30.0f);
        viewportPanel->SetContent(viewportWidget);
        HE_INFO("[UI] Viewport panel created (fallback).");
    }

    std::shared_ptr<Panel> gamePanel;
    if (panelFactories.count("Game")) {
        gamePanel = panelFactories.at("Game")();
        HE_INFO("[UI] Game panel created from registry.");
    } else {
        gamePanel = std::make_shared<Panel>("Game");
        gamePanel->SetHeaderHeight(30.0f);
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

    if (m_UIRenderer && m_UIRenderer->GetIconRenderer()) {
        float displayScale = 1.0f;
        if (m_Window) {
            int logicalW = 0;
            int pixelW = 0;
            SDL_GetWindowSize(m_Window, &logicalW, nullptr);
            if (SDL_GetWindowSizeInPixels(m_Window, &pixelW, nullptr) && logicalW > 0) {
                displayScale = static_cast<float>(pixelW) / static_cast<float>(logicalW);
            }
        }
        const float logoLogical = we::programs::editor::GetExplorerDockTabLogoSize();
        const int logoPx = static_cast<int>(std::round(logoLogical * displayScale));
        VkDescriptorSet explorerLogo = m_UIRenderer->GetIconRenderer()->GetIcon("Assets/Editor/windeffects.svg", logoPx);
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
        detailsPanel->SetHeaderHeight(30.0f);
        HE_INFO("[UI] Details panel created (fallback).");
    }

    std::shared_ptr<Panel> viewportNavigationPanel;
    if (panelFactories.count("ViewportNavigation")) {
        viewportNavigationPanel = panelFactories.at("ViewportNavigation")();
        HE_INFO("[UI] Viewport Navigation panel created from registry.");
    } else {
        viewportNavigationPanel = std::make_shared<Panel>("Viewport Navigation");
        viewportNavigationPanel->SetHeaderHeight(30.0f);
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

    auto inspectorDock = std::make_shared<DockContainer>();
    inspectorDock->AddPanel(viewportNavigationPanel);
    inspectorDock->SetVisible(false);
    inspectorDock->SetOnTabClosed([](const std::shared_ptr<Panel>& panel) {
        if (panel && panel->GetTitle() == "Viewport Navigation") {
            EditorPanelController::Get().SetPanelVisible(EditorPanelId::ViewportNavigation, false);
        }
    });
    inspectorDock->SetOnTabDragStarted([](const std::shared_ptr<Panel>& panel, const Point&) {
        if (panel && panel->GetTitle() == "Viewport Navigation") {
            EditorPanelController::Get().FloatPanel(EditorPanelId::ViewportNavigation);
        }
    });

    EditorLayoutController::Get().SetRightBottomDock(inspectorDock);
    EditorLayoutController::Get().SetViewportNavigationTabIndex(0);

    std::shared_ptr<TreeView> worldOutlinerTree = we::programs::editor::GetExplorerTreeView();
    std::shared_ptr<PropertyEditor> detailsEditor;
    if (detailsPanel) {
        detailsEditor = std::dynamic_pointer_cast<PropertyEditor>(detailsPanel->GetContent());
    }
    we::editor::environment::InitializeEditor(m_Scene, m_SceneRenderer, worldOutlinerTree, detailsEditor);

    // ===== 7. Create Content Browser =====
    std::shared_ptr<Panel> contentBrowserPanel;
    if (panelFactories.count("ContentBrowser")) {
        contentBrowserPanel = panelFactories.at("ContentBrowser")();
        HE_INFO("[UI] ContentBrowser panel created from registry.");
    } else {
        contentBrowserPanel = std::make_shared<Panel>("Content Browser");
        contentBrowserPanel->SetHeaderHeight(30.0f);
        HE_INFO("[UI] ContentBrowser panel created (fallback).");
    }

    // ===== 7b. Create Debug Panel =====
    auto debugPanel = std::make_shared<Panel>("Debug");
    debugPanel->SetHeaderHeight(30.0f);
    auto debugContent = std::make_shared<Label>("Debug output will appear here.");
    TextStyle debugStyle;
    debugStyle.size = Theme::Get().TextSizeProperty;
    debugStyle.color = Theme::Get().TextSecondary;
    debugContent->SetStyle(debugStyle);
    debugPanel->SetContent(debugContent);
    HE_INFO("[UI] Debug panel created.");

    EditorPanelController::Get().RegisterDockZone(EditorDockZone::Left, toolsDock);
    EditorPanelController::Get().RegisterDockZone(EditorDockZone::Center, centralDock);
    EditorPanelController::Get().RegisterDockZone(EditorDockZone::Right, explorerDock);
    EditorPanelController::Get().RegisterDockZone(EditorDockZone::RightInspector, inspectorDock);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::Tools, "Tools", toolsPanel, EditorDockZone::Left);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::Viewport, "Viewport", viewportPanel, EditorDockZone::Center);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::Game, "Game", gamePanel, EditorDockZone::Center);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::Explorer, "Explorer", worldOutlinerPanel, EditorDockZone::Right);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::Details, "Details", detailsPanel, EditorDockZone::Right);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::ViewportNavigation, "Viewport Navigation", viewportNavigationPanel, EditorDockZone::RightInspector);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::ContentBrowser, "Content Browser", contentBrowserPanel, EditorDockZone::Bottom);
    EditorPanelController::Get().RegisterPanel(EditorPanelId::Debug, "Debug", debugPanel, EditorDockZone::Bottom);

    vpItem->checked = EditorPanelController::Get().IsPanelVisible(EditorPanelId::Viewport);
    cbItem->checked = EditorPanelController::Get().IsPanelVisible(EditorPanelId::ContentBrowser);
    woItem->checked = EditorPanelController::Get().IsPanelVisible(EditorPanelId::Explorer);
    dtItem->checked = EditorPanelController::Get().IsPanelVisible(EditorPanelId::Details);

    // ===== 8. Create Status Bar =====
    m_StatusBar = std::make_shared<StatusBar>();
    m_StatusBar->Construct();
    m_StatusBar->SetOnFooterTabChanged([](int index) {
        EditorLayoutController::Get().SetBottomPanelIndex(index);
    });
    m_StatusBar->SetOnCommandSubmitted([](const std::string& command) {
        HE_INFO("[Command] " + command);
    });
    m_StatusBar->SetOnOutputLogClicked([]() {
        HE_INFO("[Footer] Output log clicked.");
    });
    m_StatusBar->SetOnBuildMenuClicked([]() {
        HE_INFO("[Footer] Build menu clicked.");
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
    EditorLayoutController::Get().SetBottomPanels(contentBrowserPanel, debugPanel);
    HE_INFO("[UI] Left/center column: [Tools | Viewport] over full-width Content Browser.");

    // Main body: left+center column | right sidebar (narrow)
    auto mainHSplitter = std::make_shared<Splitter>(Orientation::Horizontal, 0.78f);
    mainHSplitter->SetFirstChild(leftCenterColumn);
    mainHSplitter->SetSecondChild(rightSideSplitter);
    HE_INFO("[UI] Horizontal splitter: Left/Center (78%) | Right sidebar (22%).");

    EditorLayoutPersistence::Get().BindLayout(mainHSplitter, leftCenterColumn, editorTopRow, rightSideSplitter, explorerDock, centralDock);
    EditorLayoutPersistence::Get().Load();

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

    m_WindowHitTestData.titleBar = m_TitleBar;
    if (!SDL_SetWindowHitTest(m_Window, we::editor::mainframe::EditorWindowHitTest, &m_WindowHitTestData)) {
        HE_WARN("[UI] SDL_SetWindowHitTest failed — title bar drag may not work.");
    }

    HE_INFO("[UI] Root widget tree assembled: TitleBar -> Toolbar -> [Viewport | Outliner | Details | ContentBrowser] -> StatusBar");
    HE_INFO("[UI] Root VBox children attached: " + std::to_string(rootVBox->GetChildren().size()));
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
        m_Renderer->GetOffscreenFramebuffer().Resize(
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            m_Renderer->GetOffscreenRenderPass()
        );
    } else {
        HE_ERROR("[Render] Window still reports zero size — UI layout will be empty until resized.");
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

    auto showEulaPopup = [this, overlayMgr, viewportW, viewportH]() {
        auto eulaPopup = std::make_shared<FirstRunAgreementPopup>(LoadFirstRunAgreementEulaContent());
        eulaPopup->SetViewportSize(viewportW, viewportH);
        eulaPopup->SetOnAccepted([this, overlayMgr, viewportW, viewportH]() {
            overlayMgr->CloseAllPopups();

            auto thirdPartyPopup = std::make_shared<FirstRunAgreementPopup>(
                LoadFirstRunAgreementThirdPartyNoticesContent());
            thirdPartyPopup->SetViewportSize(viewportW, viewportH);
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
    };

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

        if (!m_Running) break;

        uint64_t now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>((now - lastTime) / frequency);
        lastTime = now;
        if (dt > 0.1f) dt = 0.1f;

        m_Camera->Update(dt);
        m_Scene->Update();
        we::runtime::world::environment::EnvironmentSystem::Get().SyncFromScene();
        we::editor::environment::TickEditor();

        // Flush viewport resize before GPU work
        if (m_ViewportWidget) {
            auto vp = std::dynamic_pointer_cast<ViewportWidget>(m_ViewportWidget);
            if (vp) vp->FlushPendingResize();
        }

        m_RootWidget->Tick(dt);

        if (m_UIEventSystem && m_ViewportWidget) {
            if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(m_ViewportWidget)) {
                m_UIEventSystem->SetSuppressSystemCursor(vp->IsFlyLookActive());
            }
        }

        we::runtime::renderer::Renderer::CameraUniform cameraUBO{};
        cameraUBO.view = m_Camera->GetViewMatrix();
        cameraUBO.proj = m_Camera->GetProjectionMatrix();
        cameraUBO.pos = m_Camera->GetPosition();
        cameraUBO.padding = 0.0f;
        m_Renderer->UpdateCameraBuffer(cameraUBO);

        if (m_Renderer->BeginFrame()) {
            VkCommandBuffer cmd = m_Renderer->GetCommandBuffer();

            if (firstFrame) {
                HE_INFO("[Render] First frame: offscreen 3D pass -> swapchain UI pass -> present");
            }

            m_RenderGraph->BeginOffscreenPass(cmd);
            m_SceneRenderer->SetEditorBackgroundEnabled(true);
            EditorPreferences::Get().ApplyEditorViewportIfDirty(m_SceneRenderer);
            m_SceneRenderer->DrawEditorBackground(cmd, m_Renderer->GetCameraDescSet());
            m_Scene->Draw(cmd);
            we::editor::grid::EditorGridRenderer::Get().Render(cmd, m_Renderer->GetCameraDescSet(), *m_Camera);
            we::programs::editor::UpdateViewportCameraSpeedIndicator();
            m_RenderGraph->EndOffscreenPass(cmd);

            m_RenderGraph->BeginSwapchainPass(cmd);
            m_UIRenderer->Render(cmd, m_Renderer->GetSwapchainWidth(), m_Renderer->GetSwapchainHeight(), m_RootWidget);
            m_RenderGraph->EndSwapchainPass(cmd);

            m_Renderer->EndFrame();

            if (firstFrame) {
                const auto& stats = m_UIRenderer->GetLastFrameStats();
                HE_INFO("[Render] First frame presented. UI commands=" + std::to_string(stats.drawCommands)
                    + " vertices=" + std::to_string(stats.vertices)
                    + " batches=" + std::to_string(stats.batches));
                if (stats.vertices == 0) {
                    HE_ERROR("[Render] First frame had EMPTY UI geometry — window will appear blank.");
                }
                firstFrame = false;
            }
        }
    }
}

void Editor::Shutdown() {
    EditorLayoutPersistence::Get().Save();

    if (m_Window) {
        SDL_SetWindowRelativeMouseMode(m_Window, false);
    }
    vkDeviceWaitIdle(m_Context->GetDevice());

    we::core::PluginManager::Get().UnloadAllPlugins();
    we::core::ModuleManager::Get().ShutdownAllModules();

    m_ViewportWidget.reset();
    m_RootWidget.reset();
    ShutdownContentBrowserService();
    if (m_UIRenderer) {
        m_UIRenderer->Shutdown();
        m_UIRenderer.reset();
    }
    m_UIEventSystem.reset();

    m_Scene.reset();
    m_Camera.reset();
    we::editor::grid::EditorGridRenderer::Get().Shutdown();
    m_SceneRenderer.reset();
    m_RenderGraph.reset();
    m_Renderer.reset();
}

} // namespace we::programs::editor
