#include "WindEffects/Editor/EditorSDK.h"
#include "Widgets/ToolbarBuilder.h"
#include "KindUI/Widgets/Label.h"

namespace we::programs::editor {
using namespace we::runtime::kindui;
using namespace we::editor::ui;

std::shared_ptr<Panel> CreateGamePanel() {
    auto toolbar = ToolbarBuilder()
        .Height(28.0f)
        .IconSize(16.0f)
        .Dropdown(Icons::PlayName, "Game", {}, "Game View Options")
        .Separator()
        .Dropdown(Icons::ConsoleName, "Display 1", {}, "Select Display")
        .Separator()
        .Dropdown(Icons::MaximizeName, "Auto Resolution", {}, "Select Resolution")
        .Separator()
        .Dropdown(Icons::CameraName, "Free Aspect", {}, "Aspect Ratio")
        .Separator()
        .Dropdown(Icons::ScaleName, "1x", {}, "View Scale")
        .Separator()
        .Dropdown(Icons::PlayName, "Play Focus", {}, "Play Focus Mode")
        .Separator()
        .Item(Icons::ProfilerName, "Stats", {}, "Toggle Stats")
        .Separator()
        .Dropdown(Icons::GridName, "Gizmos", {}, "Toggle Gizmos")
        .Build();

    return PanelBuilder("Game")
        .WithCloseButton()
        .Toolbar(toolbar)
        .Content(std::make_shared<Label>(""));
}

REGISTER_UI_PANEL(Game,
    WE_PANEL(Game).Title("Game").Zone(DockZone::Floating).Hidden(),
    CreateGamePanel)

} // namespace we::programs::editor
