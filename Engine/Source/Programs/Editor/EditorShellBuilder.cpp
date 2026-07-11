#include "EditorShellBuilder.h"

#include "Core/Logger.h"
#include "Core/IgniteBTInvoker.h"
#include "EditorModeController.h"
#include "EditorWorkspaceController.h"
#include "Explorer/WorldOutlinerApi.h"
#include "Environment/EnvironmentEditorApi.h"
#include "ViewportNavigationPreferences.h"

#include "Widgets/Panel.h"
#include "Widgets/DockContainer.h"
#include "Widgets/TitleBar.h"
#include "Widgets/WindowShell.h"
#include "Widgets/StatusBar.h"
#include "Widgets/Toolbar.h"
#include "Widgets/MenuBar.h"
#include "Widgets/ViewportWidget.h"
#include "Widgets/EditorModeSelector.h"
#include "Widgets/PropertyEditor.h"
#include "Widgets/TreeView.h"
#include "Layout/Box.h"
#include "Layout/OverlayManager.h"
#include "Core/Icon.h"
#include "Core/Widget.h"
#include "WindEffects/Editor/UI/Core/WidgetContext.h"
#include "Rendering/OverlayRenderer.h"
#include "Rendering/IconRenderer.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"

#include <volk.h>
#include <algorithm>

namespace WindEffects::Editor::UI {
namespace {

void PropagateWidgetContext(const std::shared_ptr<Widget>& widget, const std::shared_ptr<IWidgetContext>& context) {
    if (!widget || !context) {
        return;
    }

    widget->SetContext(context);
    for (const auto& child : widget->GetChildren()) {
        PropagateWidgetContext(child, context);
    }
}

std::string ResolveTabIconName(const DockPanelDescriptor& descriptor) {
    static const std::unordered_map<std::string, const char*> kAliases = {
        {"tools-panel", Icons::LayersName},
        {"viewport", Icons::PerspectiveName},
        {"outliner", Icons::HierarchyName},
        {"details", Icons::PropertiesName},
        {"content-browser", Icons::FolderName},
        {"output-log", Icons::ConsoleName},
    };

    if (descriptor.iconResource.empty()) {
        return {};
    }

    if (const auto it = kAliases.find(descriptor.iconResource); it != kAliases.end()) {
        return it->second;
    }

    return descriptor.iconResource;
}

void ApplyPanelDescriptor(
    const std::shared_ptr<Panel>& panel,
    const DockPanelDescriptor& descriptor) {
    if (!panel) {
        return;
    }

    if (!descriptor.title.empty()) {
        panel->SetTitle(descriptor.title);
    }

    const std::string iconName = ResolveTabIconName(descriptor);
    if (!iconName.empty()) {
        panel->SetTabIcon(iconName);
    }
}

} // namespace

EditorShellResult EditorShellBuilder::Build(
    IEditorApplicationContext& context,
    const EditorShellDependencies& deps) {
    EditorShellResult shellResult;
    const float uiScale = std::max(1.0f, deps.dpiScale);

    auto widgetContext = std::make_shared<WidgetContext>(context, nullptr);

    context.GetExtensionRegistry().PopulateDockManager(context.GetDockManager());

    auto& style = context.GetStyleResolver();
    const auto toolbarStyle = style.Resolve(StyleRole::Toolbar);
    const auto statusStyle = style.Resolve(StyleRole::StatusBar);

    auto menuBar = std::make_shared<MenuBar>();
    menuBar->SetContext(widgetContext);

    std::vector<std::shared_ptr<MenuItem>> fileItems;
    auto newItem = std::make_shared<MenuItem>();
    newItem->label = "New Level";
    newItem->shortcut = "Ctrl+N";
    if (deps.onCreateNewLevel) {
        newItem->onClick = deps.onCreateNewLevel;
    }
    fileItems.push_back(newItem);
    fileItems.push_back([] { auto i = std::make_shared<MenuItem>(); i->label = "Open Scene"; i->shortcut = "Ctrl+O"; return i; }());
    fileItems.push_back([] { auto i = std::make_shared<MenuItem>(); i->label = "Save"; i->shortcut = "Ctrl+S"; return i; }());
    fileItems.push_back([] { auto i = std::make_shared<MenuItem>(); i->label = "Save As..."; i->shortcut = "Ctrl+Shift+S"; return i; }());
    fileItems.push_back([] { auto i = std::make_shared<MenuItem>(); i->label = "Exit"; i->shortcut = "Alt+F4"; return i; }());
    menuBar->AddMenu("File", fileItems);

    std::vector<std::shared_ptr<MenuItem>> editItems;
    editItems.push_back([] { auto i = std::make_shared<MenuItem>(); i->label = "Undo"; i->shortcut = "Ctrl+Z"; return i; }());
    editItems.push_back([] { auto i = std::make_shared<MenuItem>(); i->label = "Redo"; i->shortcut = "Ctrl+Y"; return i; }());
    editItems.push_back([] { auto i = std::make_shared<MenuItem>(); i->label = "Cut"; i->shortcut = "Ctrl+X"; return i; }());
    editItems.push_back([] { auto i = std::make_shared<MenuItem>(); i->label = "Copy"; i->shortcut = "Ctrl+C"; return i; }());
    editItems.push_back([] { auto i = std::make_shared<MenuItem>(); i->label = "Paste"; i->shortcut = "Ctrl+V"; return i; }());
    menuBar->AddMenu("Edit", editItems);

    auto& workspace = we::programs::editor::EditorWorkspaceController::Get();
    std::vector<std::shared_ptr<MenuItem>> windowItems;
    auto makeWindowItem = [&workspace](const std::string& label, const std::string& panelId) {
        auto item = std::make_shared<MenuItem>();
        item->label = label;
        item->checked = true;
        item->onClick = [panelId, &workspace]() {
            workspace.TogglePanelVisibility(panelId);
        };
        return item;
    };

    auto vpItem = makeWindowItem("Viewport", "Viewport");
    auto cbItem = makeWindowItem("Content Browser", "ContentBrowser");
    auto woItem = makeWindowItem("Explorer", "WorldOutliner");
    auto dtItem = makeWindowItem("Details", "Details");
    windowItems.push_back(vpItem);
    windowItems.push_back(cbItem);
    windowItems.push_back(woItem);
    windowItems.push_back(dtItem);
    menuBar->AddMenu("Window", windowItems);

    workspace.SetOnPanelVisibilityChanged([vpItem, cbItem, woItem, dtItem, &workspace]() {
        vpItem->checked = workspace.IsPanelVisible("Viewport");
        cbItem->checked = workspace.IsPanelVisible("ContentBrowser");
        woItem->checked = workspace.IsPanelVisible("WorldOutliner");
        dtItem->checked = workspace.IsPanelVisible("Details");
    });

    menuBar->AddMenu("Tools", {[] { auto i = std::make_shared<MenuItem>(); i->label = "Place Actors"; return i; }()});

    std::vector<std::shared_ptr<MenuItem>> buildItems;
    auto compileItem = std::make_shared<MenuItem>();
    compileItem->label = "Compile";
    compileItem->onClick = []() {
        const auto result = we::core::InvokeIgniteBT({"build", "--config", "Debug"});
        if (!result.launched) {
            HE_ERROR("[Build] " + result.errorMessage);
        }
    };
    buildItems.push_back(compileItem);
    buildItems.push_back([] { auto i = std::make_shared<MenuItem>(); i->label = "Build"; return i; }());
    buildItems.push_back([] { auto i = std::make_shared<MenuItem>(); i->label = "Package"; return i; }());
    buildItems.push_back([] { auto i = std::make_shared<MenuItem>(); i->label = "Cook Content"; return i; }());
    menuBar->AddMenu("Build", buildItems);
    menuBar->AddMenu("Select", {
        [] { auto i = std::make_shared<MenuItem>(); i->label = "Select All"; return i; }(),
        [] { auto i = std::make_shared<MenuItem>(); i->label = "Deselect All"; return i; }()
    });
    menuBar->AddMenu("Help", {
        [] { auto i = std::make_shared<MenuItem>(); i->label = "Documentation"; return i; }(),
        [] { auto i = std::make_shared<MenuItem>(); i->label = "About WindEffects"; return i; }()
    });
    menuBar->SetItemSpacing(0.0f);

    const int logoPx = static_cast<int>(std::round(kTitleBarLogoDisplaySize * uiScale));
    VkDescriptorSet logoSet = VK_NULL_HANDLE;
    if (deps.overlayRenderer && deps.overlayRenderer->GetIconRenderer()) {
        logoSet = deps.overlayRenderer->GetIconRenderer()->GetIcon("Assets/Editor/WindEffects.svg", logoPx);
    }

    auto titleBar = std::make_shared<TitleBar>(deps.window, "WindEffects Editor", logoSet, menuBar);
    titleBar->SetContext(widgetContext);
    titleBar->Construct();

    we::programs::editor::EditorModeController::Get().InitializeFromRegistry();

    auto toolbar = std::make_shared<Toolbar>();
    toolbar->SetContext(widgetContext);
    toolbar->SetHeight(toolbarStyle.height > 0.0f ? toolbarStyle.height : 36.0f * uiScale);
    toolbar->SetLeftInset(style.Scaled(12.0f));
    toolbar->SetIconSize(toolbarStyle.iconSize > 0.0f ? toolbarStyle.iconSize : 18.0f * uiScale);
    toolbar->AddWidget(std::make_shared<we::programs::editor::EditorModeSelector>());
    toolbar->AddSeparator();
    toolbar->AddTool(Icons::NewName, "", deps.onCreateNewLevel ? deps.onCreateNewLevel : [](){}, "New Level (Ctrl+N)");
    toolbar->AddTool(Icons::OpenName, "", [](){}, "Open (Ctrl+O)");
    toolbar->AddTool(Icons::SaveName, "", [](){}, "Save (Ctrl+S)");
    toolbar->AddSeparator();
    toolbar->AddTool(Icons::CursorName, "", [](){}, "Select (Q)");
    toolbar->AddTool(Icons::MoveName, "", [](){}, "Move (W)");
    toolbar->AddTool(Icons::RotateName, "", [](){}, "Rotate (E)");
    toolbar->AddTool(Icons::ScaleName, "", [](){}, "Scale (R)");
    toolbar->AddTool(Icons::SnapName, "", [](){}, "Snap");
    toolbar->AddSeparator();
    toolbar->AddTool(Icons::UndoName, "", [](){}, "Undo (Ctrl+Z)");
    toolbar->AddTool(Icons::RedoName, "", [](){}, "Redo (Ctrl+Y)");
    toolbar->AddSeparator();
    toolbar->AddWidget(we::editor::environment::CreateEnvironmentToolbarMenu());

    auto playBtn = toolbar->AddTool(Icons::PlayName, "", [](){}, "Play (Alt+P)", false, ToolbarAlignment::Center);
    auto pauseBtn = toolbar->AddTool(Icons::PauseName, "", [](){}, "Pause (Alt+P)", false, ToolbarAlignment::Center);
    auto stopBtn = toolbar->AddTool(Icons::StopName, "", [](){}, "Stop", false, ToolbarAlignment::Center);
    playBtn->SetButtonStyle(ToolButtonStyle::TransportButton);
    pauseBtn->SetButtonStyle(ToolButtonStyle::TransportButton);
    stopBtn->SetButtonStyle(ToolButtonStyle::TransportButton);
    toolbar->AddTool(Icons::PackageName, "Platform", [](){}, "Platform", false, ToolbarAlignment::Right)
        ->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    toolbar->AddTool(Icons::SettingsName, "Settings", [](){ we::programs::editor::ShowViewportNavigationPreferences(); }, "Editor Settings", false, ToolbarAlignment::Right)
        ->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    toolbar->SetActiveTool(Icons::CursorName);

    DockLayoutBuilder layoutBuilder;
    shellResult.layout = layoutBuilder.Build(context.GetDockManager().GetLayout(), context.GetExtensionRegistry(), uiScale);

    const auto& panelDescriptors = context.GetDockManager().GetLayout().panels;
    for (auto& [panelId, panel] : shellResult.layout.panels) {
        if (!panel) {
            continue;
        }
        if (const auto descIt = panelDescriptors.find(panelId); descIt != panelDescriptors.end()) {
            ApplyPanelDescriptor(panel, descIt->second);
        }
    }

    if (auto viewportPanel = shellResult.layout.panels["Viewport"]) {
        auto viewportWidget = std::make_shared<ViewportWidget>(
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
            std::shared_ptr<Widget> viewportAsWidget = viewportWidget;
            deps.onViewportCreated(viewportAsWidget);
        }
    }

    if (auto explorerPanel = shellResult.layout.panels["WorldOutliner"]) {
        if (deps.overlayRenderer && deps.overlayRenderer->GetIconRenderer()) {
            const float logoLogical = we::programs::editor::GetExplorerDockTabLogoSize();
            const int explorerLogoPx = static_cast<int>(std::round(logoLogical * uiScale));
            VkDescriptorSet explorerLogo = deps.overlayRenderer->GetIconRenderer()->GetIcon(
                "Assets/Editor/WindEffects.svg", explorerLogoPx);
            explorerPanel->SetTabBrand(explorerLogo, logoLogical);
            we::programs::editor::BindExplorerBrandLogo(explorerLogo, logoLogical);
        }
    }

    if (shellResult.layout.explorerDock) {
        shellResult.layout.explorerDock->SetOnTabClosed([](const std::shared_ptr<Panel>& panel) {
            if (panel && panel->GetTitle() == "Explorer") {
                we::programs::editor::EditorWorkspaceController::Get().SetPanelVisible("WorldOutliner", false);
            }
        });
        shellResult.layout.explorerDock->SetOnTabDragStarted([](const std::shared_ptr<Panel>& panel, const Point&) {
            if (panel && panel->GetTitle() == "Explorer") {
                we::programs::editor::EditorWorkspaceController::Get().FloatPanel("WorldOutliner");
            }
        });
    }

    if (shellResult.layout.toolsDock) {
        shellResult.layout.toolsDock->SetVisible(we::programs::editor::EditorModeController::Get().IsDrawerVisible());
    }

    std::shared_ptr<TreeView> worldOutlinerTree = we::programs::editor::GetExplorerTreeView();
    std::shared_ptr<PropertyEditor> detailsEditor;
    if (auto detailsPanel = shellResult.layout.panels["Details"]) {
        detailsEditor = std::dynamic_pointer_cast<PropertyEditor>(detailsPanel->GetContent());
    }
    we::editor::environment::InitializeEditor(deps.scene, nullptr, worldOutlinerTree, detailsEditor);

    workspace.BindLayout(shellResult.layout);
    workspace.RegisterPanel("Tools", shellResult.layout.panels["Tools"], DockZone::Left);
    workspace.RegisterPanel("Viewport", shellResult.layout.panels["Viewport"], DockZone::Center);
    workspace.RegisterPanel("WorldOutliner", shellResult.layout.panels["WorldOutliner"], DockZone::Right);
    workspace.RegisterPanel("Details", shellResult.layout.panels["Details"], DockZone::Right);
    workspace.RegisterPanel("ContentBrowser", shellResult.layout.panels["ContentBrowser"], DockZone::Bottom);
    workspace.RegisterPanel("OutputLog", shellResult.layout.panels["OutputLog"], DockZone::Floating);

    if (const auto navPanel = context.GetExtensionRegistry().GetPanels().find("ViewportNavigation");
        navPanel != context.GetExtensionRegistry().GetPanels().end()) {
        workspace.RegisterPanel("ViewportNavigation", navPanel->second.factory(), DockZone::Floating);
    }

    workspace.ApplyToolsPanelVisibility(we::programs::editor::EditorModeController::Get().IsDrawerVisible());

    auto statusBar = std::make_shared<StatusBar>();
    statusBar->Construct();
    statusBar->SetHeight(statusStyle.height > 0.0f ? statusStyle.height : 28.0f * uiScale);
    statusBar->SetOnFooterTabChanged([](int index) {
        we::programs::editor::EditorWorkspaceController::Get().SetBottomPanelIndex(index);
    });
    statusBar->SetOnCommandSubmitted([](const std::string& command) {
        HE_INFO("[Command] " + command);
    });
    statusBar->SetOnOutputLogClicked([]() {
        auto& ws = we::programs::editor::EditorWorkspaceController::Get();
        ws.SetPanelVisible("OutputLog", true);
        ws.FocusPanel("OutputLog");
    });
    statusBar->SetOnBuildMenuClicked([]() {
        const auto result = we::core::InvokeIgniteBT({"build", "--config", "Debug"});
        if (!result.launched) {
            HE_ERROR("[Build] " + result.errorMessage);
        }
    });

    auto rootVBox = std::make_shared<VerticalBox>();
    rootVBox->SetSpacing(0.0f);
    rootVBox->AddChild(titleBar);
    rootVBox->AddChild(toolbar);
    if (shellResult.layout.root) {
        rootVBox->AddChild(shellResult.layout.root);
    }
    rootVBox->AddChild(statusBar);

    auto windowShell = std::make_shared<WindowShell>();
    windowShell->SetContent(rootVBox);

    auto overlayHost = std::make_shared<OverlayHost>();
    overlayHost->SetBaseWidget(windowShell);
    workspace.SetPopupHost(overlayHost.get());

    widgetContext->SetPopupHost(overlayHost.get());
    PropagateWidgetContext(overlayHost, widgetContext);

    shellResult.titleBar = titleBar;
    shellResult.statusBar = statusBar;
    shellResult.rootWidget = overlayHost;
    shellResult.overlayHost = overlayHost;

    workspace.LoadLayout();

    vpItem->checked = workspace.IsPanelVisible("Viewport");
    cbItem->checked = workspace.IsPanelVisible("ContentBrowser");
    woItem->checked = workspace.IsPanelVisible("WorldOutliner");
    dtItem->checked = workspace.IsPanelVisible("Details");

    if (deps.onLayoutBuilt) {
        deps.onLayoutBuilt(shellResult.layout);
    }

    HE_INFO("[UIFramework] Editor shell assembled from workspace layout configuration.");
    return shellResult;
}

} // namespace WindEffects::Editor::UI
