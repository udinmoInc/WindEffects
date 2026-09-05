#include "WindEffects/Editor/EditorSDK.h"
#include "ViewportToolbar.h"
#include "ViewportToolbarState.h"
#include "KindUI/Widgets/Label.h"

namespace we::programs::editor {
using ::we::editor::panels::Panel;
using ::we::editor::panels::PanelBuilder;
using ::we::editor::docking::DockZone;
using ::we::runtime::kindui::Label;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

std::shared_ptr<Panel> CreateViewportPanel() {
    auto toolbar = CreateViewportToolbar();

    return PanelBuilder("Viewport")
        .TabIcon(WindIcons::ToolbarCamera16)
        .Transparent()
        .FloatingToolbar()
        .WithCloseButton()
        .Toolbar(toolbar)
        .Content(std::make_shared<Label>(""));
}

REGISTER_UI_PANEL(Viewport,
    WE_PANEL(Viewport).Title("Viewport").Icon("viewport").Zone(DockZone::Center).WindowMenu("Viewport").SortOrder(1),
    CreateViewportPanel)

} // namespace we::programs::editor
