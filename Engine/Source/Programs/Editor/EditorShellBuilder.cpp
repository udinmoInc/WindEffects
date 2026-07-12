#include "EditorShellBuilder.h"

#include "Core/Logger.h"
#include "Core/IgniteBTInvoker.h"
#include "EditorModeController.h"
#include "EditorWorkspaceController.h"
#include "Explorer/WorldOutlinerApi.h"
#include "Environment/EnvironmentEditorApi.h"
#include "ViewportNavigationPreferences.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"

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
#include "Rendering/IconMetrics.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"

#include <volk.h>
#include <algorithm>
#include <vector>

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

    struct WindowMenuBinding {
        std::shared_ptr<MenuItem> item;
        std::string panelId;
    };
    std::vector<WindowMenuBinding> windowMenuBindings;
    std::vector<std::shared_ptr<MenuItem>> windowItems;

    std::vector<const PanelRegistration*> windowPanels;
    for (const auto& [panelId, reg] : context.GetExtensionRegistry().GetPanels()) {
        if (reg.descriptor.showInWindowMenu) {
            windowPanels.push_back(&reg);
        }
    }
    std::sort(windowPanels.begin(), windowPanels.end(), [](const PanelRegistration* a, const PanelRegistration* b) {
        if (a->descriptor.sortOrder != b->descriptor.sortOrder) {
            return a->descriptor.sortOrder < b->descriptor.sortOrder;
        }
        return a->descriptor.windowMenuLabel < b->descriptor.windowMenuLabel;
    });

    for (const PanelRegistration* reg : windowPanels) {
        const std::string& panelId = reg->descriptor.id;
        const std::string label = reg->descriptor.windowMenuLabel.empty()
            ? reg->descriptor.title
            : reg->descriptor.windowMenuLabel;

        auto item = std::make_shared<MenuItem>();
        item->label = label;
        item->checked = reg->descriptor.defaultVisible;
        item->onClick = [panelId, &workspace]() {
            workspace.TogglePanelVisibility(panelId);
        };
        windowItems.push_back(item);
        windowMenuBindings.push_back({item, panelId});
    }
    menuBar->AddMenu("Window", windowItems);

    workspace.SetOnPanelVisibilityChanged([&windowMenuBindings, &workspace]() {
        for (const auto& binding : windowMenuBindings) {
            binding.item->checked = workspace.IsPanelVisible(binding.panelId);
        }
    });

    for (const auto& menu : context.GetExtensionRegistry().GetMenus()) {
        menuBar->AddMenu(menu.menuName, menu.factory());
    }

    menuBar->AddMenu("Tools", {[] { auto i = std::make_shared<MenuItem>(); i->label = "Place Actors"; return i; }()});

    std::vector<std::shared_ptr<MenuItem>> buildItems;
    auto compileItem = std::make_shared<MenuItem>();
    compileItem->label = "Compile";
    compileItem->onClick = [&context]() {
        WindEffects::Editor::UI::CommandContext commandContext;
        commandContext.services = &context.GetServices();
        commandContext.sourceId = "BuildMenu";
        context.GetCommandRegistry().Execute("build.compile", commandContext);
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
    toolbar->SetHeight(toolbarStyle.height > 0.0f ? toolbarStyle.height : ResolveThemeMetric(ThemeToken::ToolbarHeight) * uiScale);
    toolbar->SetLeftInset(style.Scaled(ResolveThemeMetric(ThemeToken::Space3)));
    toolbar->SetRightInset(style.Scaled(ResolveThemeMetric(ThemeToken::WindowControlWidth) * kWindowControlCount));
    toolbar->SetEdgePadding(style.Scaled(ResolveThemeMetric(ThemeToken::Space2)));
    const float toolbarIconTier = static_cast<float>(IconMetrics::NativeIconTierPx(
        toolbarStyle.iconSize > 0.0f ? toolbarStyle.iconSize : ResolveThemeMetric(ThemeToken::IconSizeToolbar)));
    toolbar->SetIconSize(toolbarIconTier);

    // Actor / object mode selector
    toolbar->AddWidget(std::make_shared<we::programs::editor::EditorModeSelector>());
    toolbar->AddSeparator();

    // File operations
    toolbar->AddTool(Icons::NewName, "", deps.onCreateNewLevel ? deps.onCreateNewLevel : [](){}, "New Level (Ctrl+N)");
    toolbar->AddTool(Icons::OpenName, "", [](){}, "Open (Ctrl+O)");
    toolbar->AddTool(Icons::SaveName, "", [](){}, "Save (Ctrl+S)");
    toolbar->AddTool(Icons::SaveAllName, "", [](){}, "Save All");
    toolbar->AddSeparator();

    // Transform tools (icon + label)
    auto selectBtn = toolbar->AddTool(Icons::CursorName, "Select", [](){}, "Select (Q)");
    auto moveBtn = toolbar->AddTool(Icons::MoveName, "Move", [](){}, "Move (W)");
    auto rotateBtn = toolbar->AddTool(Icons::RotateName, "Rotate", [](){}, "Rotate (E)");
    auto scaleBtn = toolbar->AddTool(Icons::ScaleName, "Scale", [](){}, "Scale (R)");
    selectBtn->SetButtonStyle(ToolButtonStyle::ToolbarLabeled);
    moveBtn->SetButtonStyle(ToolButtonStyle::ToolbarLabeled);
    rotateBtn->SetButtonStyle(ToolButtonStyle::ToolbarLabeled);
    scaleBtn->SetButtonStyle(ToolButtonStyle::ToolbarLabeled);
    toolbar->AddTool(Icons::SnapName, "", [](){}, "Snap");
    toolbar->AddSeparator();

    // History
    toolbar->AddTool(Icons::UndoName, "", [](){}, "Undo (Ctrl+Z)");
    toolbar->AddTool(Icons::RedoName, "", [](){}, "Redo (Ctrl+Y)");
    toolbar->AddSeparator();

    toolbar->AddWidget(we::editor::environment::CreateEnvironmentToolbarMenu());

    // Transport (centered)
    auto playBtn = toolbar->AddTool(Icons::PlayName, "", [](){}, "Play (Alt+P)", false, ToolbarAlignment::Center);
    auto pauseBtn = toolbar->AddTool(Icons::PauseName, "", [](){}, "Pause (Alt+P)", false, ToolbarAlignment::Center);
    auto stopBtn = toolbar->AddTool(Icons::StopName, "", [](){}, "Stop", false, ToolbarAlignment::Center);
    playBtn->SetButtonStyle(ToolButtonStyle::TransportButton);
    pauseBtn->SetButtonStyle(ToolButtonStyle::TransportButton);
    stopBtn->SetButtonStyle(ToolButtonStyle::TransportButton);

    // Platform, project, settings (right)
    auto windowsBtn = toolbar->AddTool(Icons::MonitorName, "Windows", [](){}, "Windows", false, ToolbarAlignment::Right);
    windowsBtn->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    windowsBtn->SetIsDropdown(true);
    auto projectBtn = toolbar->AddTool(Icons::ProjectFolderName, "MyProject", [](){}, "MyProject", false, ToolbarAlignment::Right);
    projectBtn->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    projectBtn->SetIsDropdown(true);
    toolbar->AddTool(Icons::SettingsName, "", [](){ we::programs::editor::ShowViewportNavigationPreferences(); }, "Editor Settings", false, ToolbarAlignment::Right);

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

    for (const auto& [panelId, reg] : context.GetExtensionRegistry().GetPanels()) {
        const auto panelIt = shellResult.layout.panels.find(panelId);
        if (panelIt != shellResult.layout.panels.end() && panelIt->second) {
            workspace.RegisterPanel(panelId, panelIt->second, reg.descriptor.defaultZone);
        }
    }

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
    statusBar->SetOnCommandSubmitted([&context](const std::string& command) {
        WindEffects::Editor::UI::CommandContext commandContext;
        commandContext.services = &context.GetServices();
        commandContext.sourceId = "StatusBar";
        if (!context.GetCommandRegistry().Execute(command, commandContext)) {
            HE_INFO("[Command] " + command);
        }
    });
    statusBar->SetOnOutputLogClicked([]() {
        auto& ws = we::programs::editor::EditorWorkspaceController::Get();
        ws.SetPanelVisible("OutputLog", true);
        ws.FocusPanel("OutputLog");
    });
    statusBar->SetOnBuildMenuClicked([&context]() {
        WindEffects::Editor::UI::CommandContext commandContext;
        commandContext.services = &context.GetServices();
        commandContext.sourceId = "StatusBar";
        context.GetCommandRegistry().Execute("build.compile", commandContext);
    });

    auto rootVBox = std::make_shared<VerticalBox>();
    rootVBox->SetSpacing(0.0f);
    rootVBox->AddChild(titleBar);
    rootVBox->AddChild(toolbar);
    if (shellResult.layout.root) {
        const float workspaceGap = ResolveThemeMetric(ThemeToken::Space2) * uiScale;
        auto workspaceArea = std::make_shared<VerticalBox>();
        workspaceArea->SetPadding({ workspaceGap, workspaceGap, workspaceGap, workspaceGap });
        workspaceArea->SetBackgroundColor(ResolveThemeColor(ThemeToken::WorkspaceBackground));
        workspaceArea->AddChild(shellResult.layout.root);
        rootVBox->AddChild(workspaceArea);
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

    for (const auto& binding : windowMenuBindings) {
        binding.item->checked = workspace.IsPanelVisible(binding.panelId);
    }

    if (deps.onLayoutBuilt) {
        deps.onLayoutBuilt(shellResult.layout);
    }

    HE_INFO("[UIFramework] Editor shell assembled from workspace layout configuration.");
    return shellResult;
}

} // namespace WindEffects::Editor::UI
