#include "WindEffects/Editor/EditorSDK.h"
#include "ViewportToolbarState.h"
#include "Widgets/ToolbarBuilder.h"
#include "Widgets/Label.h"

namespace we::programs::editor {
using namespace WindEffects::Editor::UI;

std::shared_ptr<Panel> CreateViewportPanel() {
    std::shared_ptr<ToolButton> cameraSpeedButton;

    auto toolbar = ToolbarBuilder()
        .Height(28.0f)
        .IconSize(16.0f)
        .Dropdown(Icons::PerspectiveName, "Viewport", {}, "Viewport Type")
        .Separator()
        .Dropdown(Icons::ConsoleName, "Display 1", {}, "Select Display")
        .Separator()
        .Dropdown(Icons::LitName, "Lit", {}, "Render Mode")
        .Separator()
        .Dropdown(Icons::CameraName, "Camera", {}, "Camera Settings")
        .Separator()
        .Dropdown(Icons::EyeName, "Show", {}, "Show/Hide Elements")
        .Separator()
        .Item(Icons::ProfilerName, "Stats", {}, "Toggle Stats")
        .Separator()
        .Dropdown(Icons::GridName, "Gizmos", {}, "Toggle Gizmos")
        .Separator(ToolbarAlignment::Right)
        .Right([&](ToolbarBuilder& right) {
            right.Dropdown(
                Icons::CameraName,
                "4",
                []() { ShowViewportCameraSpeedPopup(); },
                "Camera Speed",
                [&](std::shared_ptr<ToolButton> button) {
                    button->SetOnMouseWheel([](float wheelDeltaY) {
                        AdjustViewportCameraSpeedFromWheel(wheelDeltaY);
                    });
                    cameraSpeedButton = button;
                });
        })
        .Build();

    if (cameraSpeedButton) {
        SetViewportCameraSpeedIndicator(cameraSpeedButton);
    }

    return PanelBuilder("Viewport")
        .TabIcon(Icons::PerspectiveName)
        .Transparent()
        .WithCloseButton()
        .Toolbar(toolbar)
        .Content(std::make_shared<Label>(""));
}

REGISTER_UI_PANEL(Viewport,
    WE_PANEL(Viewport).Title("Viewport 1").Icon("viewport").Zone(DockZone::Center).WindowMenu("Viewport").SortOrder(1),
    CreateViewportPanel)

} // namespace we::programs::editor
