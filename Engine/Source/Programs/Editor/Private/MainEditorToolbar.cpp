#include "MainEditorToolbar.h"

#include "EditorShellBuilder.h"
#include "ViewportNavigationPreferences.h"
#include "Widgets/EditorModeSelector.h"
#include "Environment/EnvironmentEditorApi.h"

#include "Widgets/Toolbar.h"
#include "Widgets/ToolbarBuilder.h"
#include "Widgets/ToolbarItem.h"
#include "Widgets/ToolButton.h"
#include "Widgets/WindowsPanelMenuButton.h"

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
using ::we::editor::toolbar::ToolButton;
using ::we::editor::toolbar::ToolButtonStyle;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;

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

    ToolbarBuilder builder;
    builder.Height(toolbarHeight)
        .IconSize(toolbarIconTier)
        .LeftInset(leftInset)
        .RightInset(rightInset)
        .EdgePadding(edgePadding);

    builder.AddWidget(modeSelector);
    builder.Separator();

    builder.Group(ToolbarAlignment::Left, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& file) {
        if (deps.onCreateNewLevel) {
            file.IconItem(Icons::NewName, "New Level (Ctrl+N)", deps.onCreateNewLevel);
        }
        if (deps.onOpenProject) {
            file.IconItem(Icons::OpenName, "Open (Ctrl+O)", deps.onOpenProject);
        }
    });
    builder.Separator();

    builder.Group(ToolbarAlignment::Left, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& history) {
        if (deps.onUndo) {
            history.IconItem(Icons::UndoName, "Undo (Ctrl+Z)", deps.onUndo);
        }
        if (deps.onRedo) {
            history.IconItem(Icons::RedoName, "Redo (Ctrl+Y)", deps.onRedo);
        }
    });
    builder.Separator();

    builder.Group(ToolbarAlignment::Left, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& play) {
        play.IconItem(Icons::MediaPlayName, "Play (PIE)", []() {}, [](const std::shared_ptr<ToolButton>& btn) {
            btn->SetButtonStyle(ToolButtonStyle::PlayButton);
        });
        play.IconItem(Icons::PauseName, "Pause", []() {}, [](const std::shared_ptr<ToolButton>& btn) {
            btn->SetButtonStyle(ToolButtonStyle::TransportButton);
        });
        play.IconItem(Icons::StopName, "Stop", []() {}, [](const std::shared_ptr<ToolButton>& btn) {
            btn->SetButtonStyle(ToolButtonStyle::TransportButton);
        });
    });
    builder.Separator();

    builder.AddWidget(::we::editor::environment::CreateEnvironmentToolbarMenu());

    builder.Right([&](ToolbarBuilder& right) {
        if (deps.windowsPanelMenu) {
            right.AddWidget(deps.windowsPanelMenu);
        }
        if (deps.onOpenProjectManager) {
            right.DropdownItem(
                Icons::ProjectFolderName,
                "Project",
                deps.onOpenProjectManager,
                "Project Manager");
        }
        right.Separator();
        right.IconItem(Icons::SettingsName, "Editor Settings", []() { ShowViewportNavigationPreferences(); });
    });

    auto toolbar = builder.Build();
    toolbar->SetContext(widgetContext);
    return toolbar;
}

} // namespace we::programs::editor