#include "MainEditorToolbar.h"

#include "EditorShellBuilder.h"
#include "ViewportNavigationPreferences.h"
#include "Widgets/EditorModeSelector.h"
#include "Environment/EnvironmentEditorApi.h"
#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"

#include "Widgets/Toolbar.h"
#include "Widgets/ToolbarBuilder.h"
#include "Widgets/ToolbarItem.h"

#include "KindUI/Core/Icon.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace we::programs::editor {
namespace {

using ::we::editor::toolbar::Toolbar;
using ::we::editor::toolbar::ToolbarAlignment;
using ::we::editor::toolbar::ToolbarBuilder;
using ::we::editor::toolbar::ToolbarGroupStyle;
using ::we::editor::toolspanel::EditorToolsRegistry;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;

std::function<void()> ToolAction(std::string_view toolId) {
    return [toolId = std::string(toolId)]() {
        if (const auto* tool = EditorToolsRegistry::Get().FindTool(toolId)) {
            if (tool->onExecute) {
                tool->onExecute();
            }
        }
    };
}

std::function<void()> TransformToolAction(
    const std::shared_ptr<std::shared_ptr<Toolbar>>& toolbarHolder,
    std::string_view iconName,
    std::string_view toolId)
{
    return [toolbarHolder, icon = std::string(iconName), toolId = std::string(toolId)]() {
        if (toolbarHolder && *toolbarHolder) {
            (*toolbarHolder)->SetActiveTool(icon);
        }
        ToolAction(toolId)();
    };
}

} // namespace

std::shared_ptr<::we::runtime::kindui::Widget> BuildMainEditorToolbar(
    const EditorShellDependencies& deps,
    const std::shared_ptr<::we::runtime::kindui::IWidgetContext>& widgetContext,
    float toolbarHeight,
    float leftInset,
    float rightInset,
    float edgePadding)
{
    auto modeSelector = std::make_shared<EditorModeSelector>();
    modeSelector->SetContext(widgetContext);
    modeSelector->InitializeCallbacks(modeSelector);
    modeSelector->Refresh();

    const float uiScale = std::max(1.0f, deps.dpiScale);
    const float toolbarIconTier = static_cast<float>(IconMetrics::NativeIconTierPx(
        we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::IconSizeToolbar)));

    auto toolbarHolder = std::make_shared<std::shared_ptr<Toolbar>>();

    ToolbarBuilder builder;
    builder.Height(toolbarHeight)
        .IconSize(toolbarIconTier)
        .LeftInset(leftInset)
        .RightInset(rightInset)
        .EdgePadding(edgePadding);

    builder.AddWidget(modeSelector);
    builder.Separator();

    builder.Group(ToolbarAlignment::Left, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& file) {
        file.IconItem(Icons::NewName, "New Level (Ctrl+N)", deps.onCreateNewLevel ? deps.onCreateNewLevel : std::function<void()>{});
        file.IconItem(Icons::OpenName, "Open (Ctrl+O)", deps.onOpenProject ? deps.onOpenProject : std::function<void()>{});
        file.IconItem(Icons::SaveName, "Save (Ctrl+S)");
        file.IconItem(Icons::SaveAllName, "Save All");
    });
    builder.Separator();

    builder.Group(ToolbarAlignment::Left, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& transform) {
        transform.IconItem(Icons::CursorName, "Select (Q)", TransformToolAction(toolbarHolder, Icons::CursorName, "SelectTool"));
        transform.IconItem(Icons::MoveName, "Move (W)", TransformToolAction(toolbarHolder, Icons::MoveName, "MoveTool"));
        transform.IconItem(Icons::RotateName, "Rotate (E)", TransformToolAction(toolbarHolder, Icons::RotateName, "RotateTool"));
        transform.IconItem(Icons::ScaleName, "Scale (R)", TransformToolAction(toolbarHolder, Icons::ScaleName, "ScaleTool"));
        transform.IconItem(Icons::SnapName, "Snap");
    });
    builder.Separator();

    builder.Group(ToolbarAlignment::Left, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& history) {
        history.IconItem(Icons::UndoName, "Undo (Ctrl+Z)", deps.onUndo ? deps.onUndo : std::function<void()>{});
        history.IconItem(Icons::RedoName, "Redo (Ctrl+Y)", deps.onRedo ? deps.onRedo : std::function<void()>{});
    });
    builder.Separator();

    builder.AddWidget(::we::editor::environment::CreateEnvironmentToolbarMenu());

    builder.Group(ToolbarAlignment::Center, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& transport) {
        transport.TransportItem(Icons::MediaPlayName, "Play (Alt+P)", {}, true);
        transport.TransportItem(Icons::PauseName, "Pause (Alt+P)");
        transport.TransportItem(Icons::StopName, "Stop");
    });

    builder.Right([&](ToolbarBuilder& right) {
        right.DropdownItem(Icons::MonitorName, "Windows", {}, "Windows");
        right.DropdownItem(Icons::ProjectFolderName, "MyProject", {}, "MyProject");
        right.Separator();
        right.IconItem(Icons::SettingsName, "Editor Settings", []() { ShowViewportNavigationPreferences(); });
    });

    auto toolbar = builder.Build();
    *toolbarHolder = toolbar;
    toolbar->SetContext(widgetContext);
    toolbar->SetActiveTool(Icons::CursorName);
    return toolbar;
}

} // namespace we::programs::editor
