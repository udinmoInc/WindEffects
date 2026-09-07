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

    // Left group: Editor Mode Selector & File actions (Save, Blueprint, Sequencer)
    builder.Left([&](ToolbarBuilder& left) {
        left.AddWidget(modeSelector);
        left.Separator();
        left.Group(ToolbarAlignment::Left, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& file) {
            file.IconItem(WindIcons::Save24, "Save Level (Ctrl+S)", []() {});
            file.IconItem(WindIcons::Blueprint24, "Open Blueprints", []() {});
            file.IconItem(WindIcons::Clapperboard24, "Cinematics & Sequencer", []() {});
        });
    });

    // Center group: Transport controls (Green Play + Mode Dropdown + Play Settings) at true window center
    builder.Center([&](ToolbarBuilder& center) {
        center.Group(ToolbarAlignment::Center, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& transport) {
            transport.IconItem(WindIcons::Play24, "Play (PIE)", []() {}, [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetButtonStyle(ToolButtonStyle::PlayButton);
            });
            transport.DropdownItem(
                we::runtime::kindui::kWindIconNone,
                "Default (Debug)",
                []() {},
                "Play Mode Options");
            transport.IconItem(WindIcons::SettingsV224, "Play Options", []() { ShowViewportNavigationPreferences(); });
        });
    });

    // Right group: Build & Accessibility dropdown controls
    builder.Right([&](ToolbarBuilder& right) {
        right.Group(ToolbarAlignment::Right, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& tools) {
            tools.DropdownItem(WindIcons::ConstructV224, "", []() {}, "Build Options");
            tools.DropdownItem(WindIcons::AccessibilityV224, "", []() {}, "Accessibility Options");
        });
    });

    auto toolbar = builder.Build();
    toolbar->SetSurfaceRole(we::runtime::kindui::SurfaceRole::Workspace);
    toolbar->SetContext(widgetContext);
    return toolbar;
}

} // namespace we::programs::editor
