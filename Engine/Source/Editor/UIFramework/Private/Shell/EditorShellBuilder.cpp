#include "WindEffects/Editor/UI/Shell/EditorShellBuilder.h"

#include "Core/Logger.h"
#include "Core/IgniteBTInvoker.h"
#include "EditorPanelController.h"
#include "EditorLayoutController.h"
#include "EditorLayoutPersistence.h"
#include "EditorModeController.h"
#include "Core/DockTabBrandRegistry.h"
#include "Explorer/WorldOutlinerApi.h"
#include "Environment/EnvironmentEditorApi.h"
#include "ViewportNavigationPreferences.h"

#include "Widgets/TitleBar.h"
#include "Widgets/WindowShell.h"
#include "Widgets/StatusBar.h"
#include "Widgets/Toolbar.h"
#include "Widgets/MenuBar.h"
#include "Widgets/ViewportWidget.h"
#include "Widgets/EditorModeSelector.h"
#include "Widgets/PropertyEditor.h"
#include "Widgets/TreeView.h"
#include "Widgets/Label.h"
#include "Layout/Box.h"
#include "Layout/OverlayManager.h"
#include "Core/Icon.h"
#include "Rendering/OverlayRenderer.h"

#include <volk.h>
#include <algorithm>

namespace WindEffects::Editor::UI {

EditorShellResult EditorShellBuilder::Build(
    IEditorApplicationContext& context,
    const EditorShellDependencies& deps) {
    EditorShellResult shellResult;
    const float uiScale = std::max(1.0f, deps.dpiScale);

    BridgeLegacyEditorRegistry(context.GetExtensionRegistry());
    context.GetExtensionRegistry().PopulateDockManager(context.GetDockManager());

    auto& style = context.GetStyleResolver();
    const auto toolbarStyle = style.Resolve(StyleRole::Toolbar);
    const auto statusStyle = style.Resolve(StyleRole::StatusBar);

    auto menuBar = std::make_shared<we::UI::MenuBar>();

    auto addMenuFromRegistry = [&](const std::string& menuName) {
        for (const auto& menuReg : context.GetExtensionRegistry().GetMenus()) {
            if (menuReg.menuName == menuName && menuReg.factory) {
                menuBar->AddMenu(menuName, menuReg.factory());
                return;
            }
        }
    };

    std::vector<std::shared_ptr<we::UI::MenuItem>> fileItems;
    auto newItem = std::make_shared<we::UI::MenuItem>();
    newItem->label = "New Level";
    newItem->shortcut = "Ctrl+N";
    if (deps.onCreateNewLevel) {
        newItem->onClick = deps.onCreateNewLevel;
    }
    fileItems.push_back(newItem);
    fileItems.push_back([] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Open Scene"; i->shortcut = "Ctrl+O"; return i; }());
    fileItems.push_back([] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Save"; i->shortcut = "Ctrl+S"; return i; }());
    fileItems.push_back([] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Save As..."; i->shortcut = "Ctrl+Shift+S"; return i; }());
    fileItems.push_back([] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Exit"; i->shortcut = "Alt+F4"; return i; }());
    menuBar->AddMenu("File", fileItems);

    std::vector<std::shared_ptr<we::UI::MenuItem>> editItems;
    editItems.push_back([] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Undo"; i->shortcut = "Ctrl+Z"; return i; }());
    editItems.push_back([] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Redo"; i->shortcut = "Ctrl+Y"; return i; }());
    editItems.push_back([] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Cut"; i->shortcut = "Ctrl+X"; return i; }());
    editItems.push_back([] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Copy"; i->shortcut = "Ctrl+C"; return i; }());
    editItems.push_back([] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Paste"; i->shortcut = "Ctrl+V"; return i; }());
    menuBar->AddMenu("Edit", editItems);

    std::vector<std::shared_ptr<we::UI::MenuItem>> windowItems;
    auto makeWindowItem = [](const std::string& label, we::programs::editor::EditorPanelId panelId) {
        auto item = std::make_shared<we::UI::MenuItem>();
        item->label = label;
        item->checked = true;
        item->onClick = [panelId]() {
            we::programs::editor::EditorPanelController::Get().TogglePanelVisibility(panelId);
        };
        return item;
    };
    auto vpItem = makeWindowItem("Viewport", we::programs::editor::EditorPanelId::Viewport);
    auto cbItem = makeWindowItem("Content Browser", we::programs::editor::EditorPanelId::ContentBrowser);
    auto woItem = makeWindowItem("Explorer", we::programs::editor::EditorPanelId::Explorer);
    auto dtItem = makeWindowItem("Details", we::programs::editor::EditorPanelId::Details);
    windowItems.push_back(vpItem);
    windowItems.push_back(cbItem);
    windowItems.push_back(woItem);
    windowItems.push_back(dtItem);
    menuBar->AddMenu("Window", windowItems);

    we::programs::editor::EditorPanelController::Get().SetOnPanelVisibilityChanged([vpItem, cbItem, woItem, dtItem]() {
        vpItem->checked = we::programs::editor::EditorPanelController::Get().IsPanelVisible(we::programs::editor::EditorPanelId::Viewport);
        cbItem->checked = we::programs::editor::EditorPanelController::Get().IsPanelVisible(we::programs::editor::EditorPanelId::ContentBrowser);
        woItem->checked = we::programs::editor::EditorPanelController::Get().IsPanelVisible(we::programs::editor::EditorPanelId::Explorer);
        dtItem->checked = we::programs::editor::EditorPanelController::Get().IsPanelVisible(we::programs::editor::EditorPanelId::Details);
    });

    menuBar->AddMenu("Tools", {[] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Place Actors"; return i; }()});

    std::vector<std::shared_ptr<we::UI::MenuItem>> buildItems;
    auto compileItem = std::make_shared<we::UI::MenuItem>();
    compileItem->label = "Compile";
    compileItem->onClick = []() {
        const auto result = we::core::InvokeIgniteBT({"build", "--config", "Debug"});
        if (!result.launched) {
            HE_ERROR("[Build] " + result.errorMessage);
        }
    };
    buildItems.push_back(compileItem);
    buildItems.push_back([] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Build"; return i; }());
    buildItems.push_back([] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Package"; return i; }());
    buildItems.push_back([] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Cook Content"; return i; }());
    menuBar->AddMenu("Build", buildItems);
    menuBar->AddMenu("Select", {
        [] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Select All"; return i; }(),
        [] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Deselect All"; return i; }()
    });
    menuBar->AddMenu("Help", {
        [] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "Documentation"; return i; }(),
        [] { auto i = std::make_shared<we::UI::MenuItem>(); i->label = "About WindEffects"; return i; }()
    });
    menuBar->SetItemSpacing(0.0f);

    const int logoPx = static_cast<int>(std::round(we::UI::kTitleBarLogoDisplaySize * uiScale));
    VkDescriptorSet logoSet = VK_NULL_HANDLE;
    if (deps.overlayRenderer && deps.overlayRenderer->GetIconRenderer()) {
        logoSet = deps.overlayRenderer->GetIconRenderer()->GetIcon("Assets/Editor/WindEffects.svg", logoPx);
    }

    auto titleBar = std::make_shared<we::UI::TitleBar>(deps.window, "WindEffects Editor", logoSet, menuBar);
    titleBar->Construct();

    we::programs::editor::EditorModeController::Get().InitializeFromRegistry();

    auto toolbar = std::make_shared<we::UI::Toolbar>();
    toolbar->SetHeight(toolbarStyle.height > 0.0f ? toolbarStyle.height : 36.0f * uiScale);
    toolbar->SetLeftInset(style.Scaled(12.0f));
    toolbar->SetIconSize(toolbarStyle.iconSize > 0.0f ? toolbarStyle.iconSize : 18.0f * uiScale);
    toolbar->AddWidget(std::make_shared<we::programs::editor::EditorModeSelector>());
    toolbar->AddSeparator();
    toolbar->AddTool(we::UI::Icons::NewName, "", deps.onCreateNewLevel ? deps.onCreateNewLevel : [](){}, "New Level (Ctrl+N)");
    toolbar->AddTool(we::UI::Icons::OpenName, "", [](){}, "Open (Ctrl+O)");
    toolbar->AddTool(we::UI::Icons::SaveName, "", [](){}, "Save (Ctrl+S)");
    toolbar->AddSeparator();
    toolbar->AddTool(we::UI::Icons::CursorName, "", [](){}, "Select (Q)");
    toolbar->AddTool(we::UI::Icons::MoveName, "", [](){}, "Move (W)");
    toolbar->AddTool(we::UI::Icons::RotateName, "", [](){}, "Rotate (E)");
    toolbar->AddTool(we::UI::Icons::ScaleName, "", [](){}, "Scale (R)");
    toolbar->AddTool(we::UI::Icons::SnapName, "", [](){}, "Snap");
    toolbar->AddSeparator();
    toolbar->AddTool(we::UI::Icons::UndoName, "", [](){}, "Undo (Ctrl+Z)");
    toolbar->AddTool(we::UI::Icons::RedoName, "", [](){}, "Redo (Ctrl+Y)");
    toolbar->AddSeparator();
    toolbar->AddWidget(we::editor::environment::CreateEnvironmentToolbarMenu());

    auto playBtn = toolbar->AddTool(we::UI::Icons::PlayName, "", [](){}, "Play (Alt+P)", false, we::UI::ToolbarAlignment::Center);
    auto pauseBtn = toolbar->AddTool(we::UI::Icons::PauseName, "", [](){}, "Pause (Alt+P)", false, we::UI::ToolbarAlignment::Center);
    auto stopBtn = toolbar->AddTool(we::UI::Icons::StopName, "", [](){}, "Stop", false, we::UI::ToolbarAlignment::Center);
    playBtn->SetButtonStyle(we::UI::ToolButtonStyle::TransportButton);
    pauseBtn->SetButtonStyle(we::UI::ToolButtonStyle::TransportButton);
    stopBtn->SetButtonStyle(we::UI::ToolButtonStyle::TransportButton);
    toolbar->AddTool(we::UI::Icons::PackageName, "Platform", [](){}, "Platform", false, we::UI::ToolbarAlignment::Right)
        ->SetButtonStyle(we::UI::ToolButtonStyle::ToolbarInline);
    toolbar->AddTool(we::UI::Icons::SettingsName, "Settings", [](){ ShowViewportNavigationPreferences(); }, "Editor Settings", false, we::UI::ToolbarAlignment::Right)
        ->SetButtonStyle(we::UI::ToolButtonStyle::ToolbarInline);
    toolbar->SetActiveTool(we::UI::Icons::CursorName);

    DockLayoutBuilder layoutBuilder;
    shellResult.layout = layoutBuilder.Build(context.GetDockManager().GetLayout(), context.GetExtensionRegistry(), uiScale);

    if (auto viewportPanel = shellResult.layout.panels["Viewport"]) {
        auto viewportWidget = std::make_shared<we::UI::ViewportWidget>(
            static_cast<we::runtime::renderer::ISceneViewportController*>(deps.renderer),
            deps.renderer->GetDeviceContext(),
            deps.renderer->GetResourceManager(),
            deps.renderer->GetSwapchainImageFormat(),
            deps.camera,
            deps.scene,
            deps.overlayRenderer);
        viewportWidget->Construct();
        viewportWidget->SetWindow(deps.window);
        viewportPanel->SetContent(viewportWidget);
        if (deps.onViewportCreated) {
            deps.onViewportCreated(viewportWidget);
        }
    }

    if (deps.overlayRenderer && deps.overlayRenderer->GetIconRenderer()) {
        const float logoLogical = we::programs::editor::GetExplorerDockTabLogoSize();
        const int explorerLogoPx = static_cast<int>(std::round(logoLogical * uiScale));
        VkDescriptorSet explorerLogo = deps.overlayRenderer->GetIconRenderer()->GetIcon("Assets/Editor/WindEffects.svg", explorerLogoPx);
        we::UI::DockTabBrandRegistry::Get().RegisterBrand("Explorer", explorerLogo, logoLogical);
        we::programs::editor::BindExplorerBrandLogo(explorerLogo, logoLogical);
    }

    if (shellResult.layout.explorerDock) {
        shellResult.layout.explorerDock->SetOnTabClosed([](const std::shared_ptr<we::UI::Panel>& panel) {
            if (panel && panel->GetTitle() == "Explorer") {
                we::programs::editor::EditorPanelController::Get().SetPanelVisible(we::programs::editor::EditorPanelId::Explorer, false);
            }
        });
        shellResult.layout.explorerDock->SetOnTabDragStarted([](const std::shared_ptr<we::UI::Panel>& panel, const we::UI::Point&) {
            if (panel && panel->GetTitle() == "Explorer") {
                we::programs::editor::EditorPanelController::Get().FloatPanel(we::programs::editor::EditorPanelId::Explorer);
            }
        });
    }

    if (shellResult.layout.toolsDock) {
        shellResult.layout.toolsDock->SetVisible(we::programs::editor::EditorModeController::Get().IsDrawerVisible());
        we::programs::editor::EditorLayoutController::Get().SetToolsPanelRoot(shellResult.layout.toolsDock);
    }

    std::shared_ptr<we::UI::TreeView> worldOutlinerTree = we::programs::editor::GetExplorerTreeView();
    std::shared_ptr<we::UI::PropertyEditor> detailsEditor;
    if (auto detailsPanel = shellResult.layout.panels["Details"]) {
        detailsEditor = std::dynamic_pointer_cast<we::UI::PropertyEditor>(detailsPanel->GetContent());
    }
    we::editor::environment::InitializeEditor(deps.scene, nullptr, worldOutlinerTree, detailsEditor);

    if (shellResult.layout.toolsDock) {
        we::programs::editor::EditorPanelController::Get().RegisterDockZone(we::programs::editor::EditorDockZone::Left, shellResult.layout.toolsDock);
    }
    if (shellResult.layout.viewportDock) {
        we::programs::editor::EditorPanelController::Get().RegisterDockZone(we::programs::editor::EditorDockZone::Center, shellResult.layout.viewportDock);
    }
    if (shellResult.layout.explorerDock) {
        we::programs::editor::EditorPanelController::Get().RegisterDockZone(we::programs::editor::EditorDockZone::Right, shellResult.layout.explorerDock);
    }

    auto registerPanel = [&](we::programs::editor::EditorPanelId id, const char* key, we::programs::editor::EditorDockZone zone) {
        if (auto panel = shellResult.layout.panels[key]) {
            we::programs::editor::EditorPanelController::Get().RegisterPanel(id, key, panel, zone);
        }
    };
    registerPanel(we::programs::editor::EditorPanelId::Tools, "Tools", we::programs::editor::EditorDockZone::Left);
    registerPanel(we::programs::editor::EditorPanelId::Viewport, "Viewport", we::programs::editor::EditorDockZone::Center);
    registerPanel(we::programs::editor::EditorPanelId::Explorer, "WorldOutliner", we::programs::editor::EditorDockZone::Right);
    registerPanel(we::programs::editor::EditorPanelId::Details, "Details", we::programs::editor::EditorDockZone::Right);
    registerPanel(we::programs::editor::EditorPanelId::ContentBrowser, "ContentBrowser", we::programs::editor::EditorDockZone::Floating);
    registerPanel(we::programs::editor::EditorPanelId::OutputLog, "OutputLog", we::programs::editor::EditorDockZone::Floating);

    if (shellResult.layout.toolsViewportSplitter) {
        we::programs::editor::EditorLayoutController::Get().SetToolsPanelSplitter(shellResult.layout.toolsViewportSplitter);
        we::programs::editor::EditorLayoutController::Get().ApplyToolsPanelVisibility(
            we::programs::editor::EditorModeController::Get().IsDrawerVisible());
    }
    if (shellResult.layout.leftCenterSplitter && shellResult.layout.panels["ContentBrowser"]) {
        we::programs::editor::EditorLayoutController::Get().SetContentBrowserSplitter(shellResult.layout.leftCenterSplitter);
        we::programs::editor::EditorLayoutController::Get().SetBottomPanels(shellResult.layout.panels["ContentBrowser"], nullptr);
    }

    if (shellResult.layout.mainHorizontalSplitter && shellResult.layout.leftCenterSplitter && shellResult.layout.toolsViewportSplitter && shellResult.layout.rightVerticalSplitter) {
        we::programs::editor::EditorLayoutPersistence::Get().BindLayout(
            shellResult.layout.mainHorizontalSplitter,
            shellResult.layout.leftCenterSplitter,
            shellResult.layout.toolsViewportSplitter,
            shellResult.layout.rightVerticalSplitter,
            shellResult.layout.explorerDock,
            shellResult.layout.viewportDock);
    }

    auto statusBar = std::make_shared<we::UI::StatusBar>();
    statusBar->Construct();
    statusBar->SetHeight(statusStyle.height > 0.0f ? statusStyle.height : 28.0f * uiScale);
    statusBar->SetOnFooterTabChanged([](int index) {
        we::programs::editor::EditorLayoutController::Get().SetBottomPanelIndex(index);
    });
    statusBar->SetOnCommandSubmitted([](const std::string& command) {
        HE_INFO("[Command] " + command);
    });
    statusBar->SetOnOutputLogClicked([]() {
        we::programs::editor::EditorPanelController::Get().SetPanelVisible(we::programs::editor::EditorPanelId::OutputLog, true);
        we::programs::editor::EditorPanelController::Get().FocusPanel(we::programs::editor::EditorPanelId::OutputLog);
    });
    statusBar->SetOnBuildMenuClicked([]() {
        const auto result = we::core::InvokeIgniteBT({"build", "--config", "Debug"});
        if (!result.launched) {
            HE_ERROR("[Build] " + result.errorMessage);
        }
    });

    auto rootVBox = std::make_shared<we::UI::VerticalBox>();
    rootVBox->SetSpacing(0.0f);
    rootVBox->AddChild(titleBar);
    rootVBox->AddChild(toolbar);
    if (shellResult.layout.root) {
        rootVBox->AddChild(shellResult.layout.root);
    }
    rootVBox->AddChild(statusBar);

    auto windowShell = std::make_shared<we::UI::WindowShell>();
    windowShell->SetContent(rootVBox);

    auto overlayManager = std::make_shared<we::UI::OverlayManager>();
    overlayManager->SetBaseWidget(windowShell);
    shellResult.titleBar = titleBar;
    shellResult.statusBar = statusBar;
    shellResult.rootWidget = overlayManager;

    we::programs::editor::EditorLayoutPersistence::Get().Load();

    if (deps.onLayoutBuilt) {
        deps.onLayoutBuilt(shellResult.layout);
    }

    HE_INFO("[UIFramework] Editor shell assembled from workspace layout configuration.");
    return shellResult;
}

} // namespace WindEffects::Editor::UI
