#include "MainEditorToolbar.h"

#include "EditorShellBuilder.h"
#include "ViewportNavigationPreferences.h"
#include "Widgets/EditorModeSelector.h"

#include "Widgets/Toolbar.h"
#include "Widgets/ToolbarBuilder.h"
#include "Widgets/ToolbarItem.h"
#include "Widgets/ToolButton.h"
#include "Widgets/WindowsPanelMenuButton.h"

#include "KindUI/Core/WindIcon.h"
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
using ::we::runtime::kindui::kWindIconNone;
namespace WindIcons = ::we::runtime::kindui::WindIcons;

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

    ToolbarBuilder builder;
    builder.Height(toolbarHeight)
        .LeftInset(leftInset)
        .RightInset(rightInset)
        .EdgePadding(edgePadding);

    // Left group: Save (with dropdown) first in execution cluster, then Mode Selector & File actions (Blueprint, Sequencer)
    builder.Left([&](ToolbarBuilder& left) {
        left.Group(ToolbarAlignment::Left, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& file) {
            file.IconItem(WindIcons::Save16, "Save Level (Ctrl+S)", []() {});
        });
        left.AddWidget(modeSelector);
        left.Separator();
        left.Group(ToolbarAlignment::Left, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& file) {
            file.IconItem(WindIcons::Blueprint16, "Open Blueprints", []() {});
            file.IconItem(WindIcons::Clapperboard16, "Cinematics & Sequencer", []() {});
        });
    });

    // Center group: Transport controls (Play + Pause + Mode Dropdown) at true window center
    builder.Center([&](ToolbarBuilder& center) {
        center.Group(ToolbarAlignment::Center, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& transport) {
            transport.IconItem(WindIcons::Play16, "Play (PIE)", []() {}, [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetButtonStyle(ToolButtonStyle::PlayButton);
            });
            transport.IconItem(WindIcons::Pause16, "Pause (PIE)", []() {});
            transport.DropdownItem(
                we::runtime::kindui::kWindIconNone,
                "Default (Debug)",
                []() {},
                "Play Mode Options");
        });
    });

    // Right group: Build & Settings dropdown controls
    builder.Right([&](ToolbarBuilder& right) {
        right.Group(ToolbarAlignment::Right, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& tools) {
            tools.DropdownItem(WindIcons::Construct16, "Build", []() {}, "Build Options");
            tools.DropdownItem(WindIcons::Settings16, "Settings", []() {}, "Settings Options");
        });
    });

    auto toolbar = builder.Build();
    toolbar->SetSurfaceRole(we::runtime::kindui::SurfaceRole::Workspace);
    toolbar->SetContext(widgetContext);
    return toolbar;
}

} // namespace we::programs::editor
 
