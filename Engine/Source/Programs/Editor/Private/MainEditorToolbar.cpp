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

    // 1) Editor mode
    builder.AddWidget(modeSelector);
    builder.Separator();

    // 2) File — authored document / folder / save / blueprint / clapperboard glyphs inside segmented cluster
    builder.Group(ToolbarAlignment::Left, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& file) {
        if (deps.onCreateNewLevel) {
            file.IconItem(WindIcons::Document16, "New Level (Ctrl+N)", deps.onCreateNewLevel);
        }
        if (deps.onOpenProject) {
            file.IconItem(WindIcons::FolderOpen16, "Open Project (Ctrl+O)", deps.onOpenProject);
        }
        file.IconItem(WindIcons::Save16, "Save Level (Ctrl+S)", []() {});
        file.IconItem(WindIcons::Blueprint16, "Open Blueprints", []() {});
        file.IconItem(WindIcons::Clapperboard16, "Cinematics & Sequencer", []() {});
    });
    builder.Separator();

    // 3) Centered execution controls — Green Play + Mode Dropdown + Play Settings (segmented cluster)
    builder.Group(ToolbarAlignment::Center, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& transport) {
        transport.IconItem(WindIcons::Play16, "Play (PIE)", []() {}, [](const std::shared_ptr<ToolButton>& btn) {
            btn->SetButtonStyle(ToolButtonStyle::PlayButton);
        });
        transport.DropdownItem(
            we::runtime::kindui::kWindIconNone,
            "Default (Debug)",
            []() {},
            "Play Mode Options");
        transport.IconItem(WindIcons::Settings16, "Play Options", []() { ShowViewportNavigationPreferences(); });
    });

    // 4) Right-aligned controls — standalone (unconnected) buttons with 16px icons, dark background card chip chrome (32px height) and text label
    builder.Right([&](ToolbarBuilder& right) {
        right.DropdownItem(WindIcons::Construct16, "Build", []() {}, "Build Options", [](const std::shared_ptr<ToolButton>& btn) {
            btn->SetButtonStyle(ToolButtonStyle::ViewportChip);
        });
        right.DropdownItem(WindIcons::Accessibility16, "Accessibility", []() {}, "Accessibility Options", [](const std::shared_ptr<ToolButton>& btn) {
            btn->SetButtonStyle(ToolButtonStyle::ViewportChip);
        });
    });

    auto toolbar = builder.Build();
    toolbar->SetSurfaceRole(we::runtime::kindui::SurfaceRole::Workspace);
    toolbar->SetContext(widgetContext);
    return toolbar;
}

} // namespace we::programs::editor
