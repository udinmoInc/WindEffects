// ==============================================================================
// WindEffects — Editor — EditorShellBuilder
// Internal implementation for the Editor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "EditorShellBuilder.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Core/IgniteBTInvoker.h"
#include "KindUI/Profiling/ScreenRecorder.h"
#include "Platform/Platform.h"
#include "WindEffects/Editor/UI/Shell/EditorModeController.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "Explorer/WorldOutlinerApi.h"
#include "Environment/EnvironmentEditorApi.h"
#include "MainEditorToolbar.h"
#include "Widgets/WindowsPanelMenuButton.h"
#include "KindUI/Theming/ThemeAccess.h"

#include "KindUI/Panel/Panel.h"
#include "KindUI/Docking/DockContainer.h"
#include "Widgets/TitleBar.h"
#include "Projects/EngineContext.h"
#include "Widgets/WindowShell.h"
#include "Widgets/StatusBar.h"
#include "Widgets/MenuBar.h"
#include "Widgets/ViewportWidget.h"
#include "ContentBrowser/Widgets/TreeView.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/OverlayManager.h"
#include "WindEffects/Editor/UI/Core/PanelIconResolver.h"
#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WidgetContext.h"
#include "KindUI/Rendering/OverlayRenderer.h"
#include "KindUI/Rendering/IconRenderer.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "PropertyEditor/PropertyEditorSession.h"

#include <algorithm>
#include <vector>
#include "KindUI/Tokens/ChromeSeparation.h"
#include "KindUI/Tokens/DesignToken.h"

using we::runtime::kindui::ColorToken;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::Margin;
using we::runtime::kindui::PaddingToken;
using we::runtime::kindui::ResolveColor;

namespace we::programs::editor {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::VerticalAlignment;
using ::we::runtime::kindui::HorizontalAlignment;
using ::we::runtime::kindui::IWidgetContext;
using ::we::runtime::kindui::WidgetContext;
using ::we::runtime::kindui::StyleRole;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Column;
using ::we::runtime::kindui::OverlayHost;
using ::we::editor::viewport::ViewportWidget;
using ::we::editor::contentbrowser::TreeView;
using ::we::runtime::kindui::docking::DockContainer;
using ::we::editor::shell::DockLayoutBuilder;
using ::we::editor::docking::DockPanelDescriptor;
using ::we::editor::docking::DockZone;
using ::we::editor::services::IEditorApplicationContext;
using ::we::runtime::kindui::panels::Panel;
using ::we::editor::extensions::PanelRegistration;
using ::we::editor::services::ResolvePanelTabIcon;
using ::we::editor::menus::MenuBar;
using ::we::editor::menus::MenuItem;
using ::we::editor::shell::TitleBar;
using ::we::editor::shell::StatusBar;
using ::we::editor::shell::WindowShell;
namespace {

void LogMenuStub(const char* label) {
    WE_LOG_INFO(we::LogCategory::Editor.data(),
        std::string("[Menu] ") + label + " clicked (no handler yet)");
    if (we::runtime::kindui::ScreenRecorder::IsRecordingEnabled()) {
        we::runtime::kindui::ScreenRecorder::Get().RecordInput(
            "StubClick", 0.0f, 0.0f, label, "Menu", "empty-handler");
    }
}

std::shared_ptr<MenuItem> MakeStubMenuItem(const char* label, const char* shortcut = nullptr) {
    auto item = std::make_shared<MenuItem>();
    item->label = label;
    if (shortcut && shortcut[0] != '\0') {
        item->shortcut = shortcut;
    }
    item->onClick = [name = std::string(label)]() {
        LogMenuStub(name.c_str());
    };
    return item;
}

void PropagateWidgetContext(const std::shared_ptr<Widget>& widget, const std::shared_ptr<IWidgetContext>& context) {
    if (!widget || !context) {
        return;
    }

    widget->SetContext(context);
    for (const auto& child : widget->GetChildren()) {
        PropagateWidgetContext(child, context);
    }
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

    panel->SetTabIcon(ResolvePanelTabIcon(descriptor.id));
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
    const auto titleStyle = style.Resolve(StyleRole::WindowHeader);

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

    auto openProjectItem = std::make_shared<MenuItem>();
    openProjectItem->label = "Open Project...";
    openProjectItem->shortcut = "Ctrl+Shift+O";
    if (deps.onOpenProject) {
        openProjectItem->onClick = deps.onOpenProject;
    }
    fileItems.push_back(openProjectItem);

    auto projectManagerItem = std::make_shared<MenuItem>();
    projectManagerItem->label = "Open Launcher...";
    if (deps.onOpenProjectManager) {
        projectManagerItem->onClick = deps.onOpenProjectManager;
    }
    fileItems.push_back(projectManagerItem);

    fileItems.push_back(MakeStubMenuItem("Open Scene", "Ctrl+O"));
    fileItems.push_back(MakeStubMenuItem("Save", "Ctrl+S"));
    fileItems.push_back(MakeStubMenuItem("Save As...", "Ctrl+Shift+S"));

    // Exit is handled by the window chrome / Alt+F4 — keep an explicit item.
    auto exitItem = std::make_shared<MenuItem>();
    exitItem->label = "Exit";
    exitItem->shortcut = "Alt+F4";
    exitItem->onClick = []() {
        we::platform::Platform::Get().PostQuit();
    };
    fileItems.push_back(exitItem);
    menuBar->AddMenu("File", fileItems);

    std::vector<std::shared_ptr<MenuItem>> editItems;
    {
        auto undoItem = std::make_shared<MenuItem>();
        undoItem->label = "Undo";
        undoItem->shortcut = "Ctrl+Z";
        if (deps.onUndo) {
            undoItem->onClick = deps.onUndo;
        }
        editItems.push_back(undoItem);

        auto redoItem = std::make_shared<MenuItem>();
        redoItem->label = "Redo";
        redoItem->shortcut = "Ctrl+Y";
        if (deps.onRedo) {
            redoItem->onClick = deps.onRedo;
        }
        editItems.push_back(redoItem);
    }
    editItems.push_back(MakeStubMenuItem("Cut", "Ctrl+X"));
    editItems.push_back(MakeStubMenuItem("Copy", "Ctrl+C"));
    editItems.push_back(MakeStubMenuItem("Paste", "Ctrl+V"));
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

    // NOTE: windowMenuBindings is captured BY VALUE. The workspace outlives
    // this builder function — a reference capture would dangle and crash
    // inside IsPanelVisible on later footer-tab clicks.
    workspace.SetOnPanelVisibilityChanged([windowMenuBindings, &workspace]() {
        for (const auto& binding : windowMenuBindings) {
            binding.item->checked = workspace.IsPanelVisible(binding.panelId);
        }
    });

    for (const auto& menu : context.GetExtensionRegistry().GetMenus()) {
        if (menu.factory) {
            menuBar->AddMenu(menu.menuName, menu.factory());
        }
    }

    menuBar->AddMenu("Tools", { MakeStubMenuItem("Place Actors") });

    std::vector<std::shared_ptr<MenuItem>> buildItems;
    auto compileItem = std::make_shared<MenuItem>();
    compileItem->label = "Compile";
    compileItem->onClick = [&context]() {
        we::runtime::kindui::CommandContext commandContext;
        commandContext.services = &context.GetServices();
        commandContext.sourceId = "BuildMenu";
        context.GetCommandRegistry().Execute("build.compile", commandContext);
    };
    buildItems.push_back(compileItem);
    buildItems.push_back(MakeStubMenuItem("Build"));
    buildItems.push_back(MakeStubMenuItem("Package"));
    buildItems.push_back(MakeStubMenuItem("Cook Content"));
    menuBar->AddMenu("Build", buildItems);
    menuBar->AddMenu("Select", {
        MakeStubMenuItem("Select All"),
        MakeStubMenuItem("Deselect All"),
    });
    menuBar->AddMenu("Help", {
        MakeStubMenuItem("Documentation"),
        MakeStubMenuItem("About WindEffects"),
    });
    menuBar->SetItemSpacing(0.0f);

    const int logoPx = static_cast<int>(std::round(
        (we::runtime::kindui::ResolveMetric(MetricToken::IconSizePrimary)
            + we::runtime::kindui::ResolveMetric(MetricToken::Space1) * 0.5f) * uiScale));
    we::rhi::RHIDescriptorSetHandle logoSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    (void)logoPx;

    std::string titleText = "";
    auto titleBar = std::make_shared<TitleBar>(deps.window, titleText, logoSet, menuBar);
    titleBar->SetContext(widgetContext);
    titleBar->Construct();

    ::we::editor::shell::EditorModeController::Get().InitializeFromRegistry();

    const float toolbarHeight = toolbarStyle.height > 0.0f
        ? toolbarStyle.height
        : we::runtime::kindui::ResolveMetric(MetricToken::ToolbarHeight) * uiScale;
    const float toolbarLeftInset = style.Scaled(we::runtime::kindui::ResolveMetric(MetricToken::Space3));
    const float toolbarRightInset = style.Scaled(we::runtime::kindui::ResolveMetric(MetricToken::Space3));
    const float toolbarEdgePadding = style.Scaled(we::runtime::kindui::ResolveMetric(MetricToken::Space2));

    EditorShellDependencies toolbarDeps = deps;
    toolbarDeps.windowsPanelMenu = ::we::editor::toolbar::WindowsPanelMenuButton::Create(
        context.GetExtensionRegistry(),
        [&workspace](const std::string& panelId) { workspace.TogglePanelVisibility(panelId); },
        [&workspace](const std::string& panelId) { return workspace.IsPanelVisible(panelId); });
    if (toolbarDeps.windowsPanelMenu) {
        toolbarDeps.windowsPanelMenu->SetContext(widgetContext);
    }

    auto toolbar = BuildMainEditorToolbar(
        toolbarDeps,
        widgetContext,
        toolbarHeight,
        toolbarLeftInset,
        toolbarRightInset,
        toolbarEdgePadding);

    DockLayoutBuilder layoutBuilder;
    shellResult.layout = layoutBuilder.Build(context.GetDockManager().GetLayout(), context.GetExtensionRegistry(),
        uiScale);
    if (shellResult.layout.root) {
        shellResult.layout.root->SetContext(widgetContext);
    }

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
            deps.renderer->GetRHIDevice(),
            deps.renderer->GetSwapchainFormat(),
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
        const float logoLogical = we::programs::editor::GetExplorerDockTabLogoSize();
        explorerPanel->SetTabBrand(we::rhi::RHIDescriptorSetHandle::Invalid, logoLogical);
        we::programs::editor::BindExplorerBrandLogo(we::rhi::RHIDescriptorSetHandle::Invalid, logoLogical);
    }

    if (shellResult.layout.viewportDock) {
        shellResult.layout.viewportDock->SetOnTabClosed([](const std::shared_ptr<Panel>& panel) {
            we::programs::editor::EditorWorkspaceController::Get().HidePanelWidget(panel);
        });
        shellResult.layout.viewportDock->SetOnTabDragStarted([](const std::shared_ptr<Panel>& panel, const Point& pos) {
            we::programs::editor::EditorWorkspaceController::Get().FloatPanelWidget(panel, pos);
        });
    }

    if (shellResult.layout.explorerDock) {
        shellResult.layout.explorerDock->SetOnTabClosed([](const std::shared_ptr<Panel>& panel) {
            we::programs::editor::EditorWorkspaceController::Get().HidePanelWidget(panel);
        });
        shellResult.layout.explorerDock->SetOnTabDragStarted([](const std::shared_ptr<Panel>& panel, const Point& pos) {
            we::programs::editor::EditorWorkspaceController::Get().FloatPanelWidget(panel, pos);
        });
    }

    if (shellResult.layout.detailsDock) {
        shellResult.layout.detailsDock->SetOnTabClosed([](const std::shared_ptr<Panel>& panel) {
            we::programs::editor::EditorWorkspaceController::Get().HidePanelWidget(panel);
        });
        shellResult.layout.detailsDock->SetOnTabDragStarted([](const std::shared_ptr<Panel>& panel, const Point& pos) {
            we::programs::editor::EditorWorkspaceController::Get().FloatPanelWidget(panel, pos);
        });
    }

    if (shellResult.layout.toolsDock) {
        shellResult.layout.toolsDock->SetVisible(::we::editor::shell::EditorModeController::Get().IsDrawerVisible());
        shellResult.layout.toolsDock->SetOnTabDragStarted([](const std::shared_ptr<Panel>& panel, const Point& pos) {
            we::programs::editor::EditorWorkspaceController::Get().FloatPanelWidget(panel, pos);
        });
        shellResult.layout.toolsDock->SetOnTabClosed([](const std::shared_ptr<Panel>& panel) {
            we::programs::editor::EditorWorkspaceController::Get().HidePanelWidget(panel);
        });
    }

    if (shellResult.layout.contentBrowserDock) {
        shellResult.layout.contentBrowserDock->SetOnTabDragStarted([](const std::shared_ptr<Panel>& panel, const Point&
            pos) {
            we::programs::editor::EditorWorkspaceController::Get().FloatPanelWidget(panel, pos);
        });
        shellResult.layout.contentBrowserDock->SetOnTabClosed([](const std::shared_ptr<Panel>& panel) {
            we::programs::editor::EditorWorkspaceController::Get().HidePanelWidget(panel);
        });
    }

    std::shared_ptr<TreeView> worldOutlinerTree = we::programs::editor::GetExplorerTreeView();
    ::we::editor::environment::InitializeEditor(
        deps.scene,
        worldOutlinerTree,
        ::we::editor::property::PropertyEditorSession::DetailsShared());

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

    workspace.ApplyToolsPanelVisibility(::we::editor::shell::EditorModeController::Get().IsDrawerVisible());

    auto statusBar = std::make_shared<StatusBar>();
    statusBar->Construct();
    statusBar->SetHeight(statusStyle.height > 0.0f ? statusStyle.height :
        we::runtime::kindui::ResolveMetric(MetricToken::StatusBarHeight) * uiScale);
    statusBar->SetOnFooterTabChanged([](int index) {
        we::programs::editor::EditorWorkspaceController::Get().SetBottomPanelIndex(index);
    });
    statusBar->SetOnCommandSubmitted([&context](const std::string& command) {
        we::runtime::kindui::CommandContext commandContext;
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
        we::runtime::kindui::CommandContext commandContext;
        commandContext.services = &context.GetServices();
        commandContext.sourceId = "StatusBar";
        context.GetCommandRegistry().Execute("build.compile", commandContext);
    });

    auto rootVBox = std::make_shared<Column>();
    rootVBox->Gap(0.0f);
    rootVBox->SetVerticalAlignment(VerticalAlignment::Fill);

    titleBar->SetFlexShrink(0.0f);
    const float titleHeight = titleStyle.height > 0.0f
        ? titleStyle.height
        : we::runtime::kindui::ResolveMetric(MetricToken::TitleBarHeight) * uiScale;
    titleBar->SetMinSize(Size{ 0.0f, titleHeight });
    titleBar->SetMaxSize(Size{ 1.0e9f, titleHeight });
    toolbar->SetFlexShrink(0.0f);
    toolbar->SetMinSize(Size{ 0.0f, toolbarHeight });
    toolbar->SetMaxSize(Size{ 1.0e9f, toolbarHeight });
    statusBar->SetFlexShrink(0.0f);
    const float statusHeight = statusStyle.height > 0.0f ? statusStyle.height :
        we::runtime::kindui::ResolveMetric(MetricToken::StatusBarHeight) * uiScale;
    statusBar->SetMinSize(Size{ 0.0f, statusHeight });
    statusBar->SetMaxSize(Size{ 1.0e9f, statusHeight });

    rootVBox->AddChild(titleBar);
    rootVBox->AddChild(toolbar);
    if (shellResult.layout.root) {
        const float dockGapV = style.Scaled(2.5f);
        const float dockGapH = style.Scaled(2.5f);
        // CSS-like token styling on Flex (imperative twin of UI::Bg / Padding).
        auto workspaceArea = std::make_shared<Column>();
        workspaceArea->Gap(0.0f);
        workspaceArea->Padding(Margin{ dockGapH, dockGapV, dockGapH, dockGapV });
        workspaceArea->Background(ColorToken::WorkspaceBackground);
        workspaceArea->SetFlexGrow(1.0f);
        workspaceArea->SetFlexShrink(0.0f);
        workspaceArea->SetVerticalAlignment(VerticalAlignment::Fill);
        workspaceArea->SetHorizontalAlignment(HorizontalAlignment::Fill);
        shellResult.layout.root->SetFlexGrow(1.0f);
        shellResult.layout.root->SetVerticalAlignment(VerticalAlignment::Fill);
        shellResult.layout.root->SetHorizontalAlignment(HorizontalAlignment::Fill);
        workspaceArea->AddChild(shellResult.layout.root);
        rootVBox->AddChild(workspaceArea);
    }
    rootVBox->AddChild(statusBar);

    auto windowShell = std::make_shared<WindowShell>();
    windowShell->SetContent(rootVBox);

    auto overlayHost = std::make_shared<OverlayHost>();
    overlayHost->SetBaseWidget(windowShell);
    workspace.SetPopupHost(overlayHost.get());
    if (deps.eventSystem) {
        deps.eventSystem->SetPopupHost(overlayHost.get());
    }

    widgetContext->SetPopupHost(overlayHost.get());
    PropagateWidgetContext(overlayHost, widgetContext);

    shellResult.titleBar = titleBar;
    shellResult.statusBar = statusBar;
    shellResult.rootWidget = overlayHost;
    shellResult.overlayHost = overlayHost;

    workspace.LoadLayout();
    workspace.EnsureDefaultDockPlacement();
    workspace.ApplyToolsPanelVisibility(::we::editor::shell::EditorModeController::Get().IsDrawerVisible());

    for (const auto& binding : windowMenuBindings) {
        binding.item->checked = workspace.IsPanelVisible(binding.panelId);
    }

    if (deps.onLayoutBuilt) {
        deps.onLayoutBuilt(shellResult.layout);
    }

    HE_INFO("[EditorShell] Editor shell assembled from KindUI widgets + workspace layout.");
    return shellResult;
}

} // namespace we::programs::editor
