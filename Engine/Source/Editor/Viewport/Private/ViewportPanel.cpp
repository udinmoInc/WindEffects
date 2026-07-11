#include "WindEffects/Editor/EditorSDK.h"
#include "ViewportToolbarState.h"
#include "Widgets/ToolbarBuilder.h"
#include "Widgets/Label.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"

namespace we::programs::editor {
using namespace WindEffects::Editor::UI;

std::shared_ptr<Panel> CreateViewportPanel() {
    std::shared_ptr<ToolButton> cameraSpeedButton;

    auto toolbar = ToolbarBuilder()
        .Height(ResolveThemeMetric(ThemeToken::PanelHeaderHeight))
        .IconSize(ResolveThemeMetric(ThemeToken::IconSizeNavigation))
        .Dropdown(Icons::PerspectiveName, "Perspective", {}, "Viewport Type")
        .Separator()
        .Dropdown(Icons::LitName, "Lit", {}, "Render Mode")
        .Separator()
        .Dropdown(Icons::EyeName, "Show", {}, "Show/Hide Elements")
        .Separator(ToolbarAlignment::Right)
        .Right([&](ToolbarBuilder& right) {
            right.Item(Icons::CursorName, "", [](){}, "Select (Q)");
            right.Item(Icons::MoveName, "", [](){}, "Move (W)");
            right.Item(Icons::RotateName, "", [](){}, "Rotate (E)");
            right.Item(Icons::ScaleName, "", [](){}, "Scale (R)");
            right.Item(Icons::SnapName, "", [](){}, "Snap");
            right.Separator();
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
    WE_PANEL(Viewport).Title("Viewport").Icon("viewport").Zone(DockZone::Center).WindowMenu("Viewport").SortOrder(1),
    CreateViewportPanel)

} // namespace we::programs::editor
