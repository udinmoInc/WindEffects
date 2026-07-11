#include "WindEffects/Editor/UI/Extensions/ExtensionBootstrap.h"
#include "ViewportToolbarState.h"
#include "Widgets/Panel.h"
#include "Core/Icon.h"
#include "Widgets/Label.h"
#include "Widgets/Toolbar.h"
#include "Widgets/ToolButton.h"

namespace we::programs::editor {

std::shared_ptr<WindEffects::Editor::UI::Panel> CreateViewportPanel() {
    auto panel = std::make_shared<WindEffects::Editor::UI::Panel>("Viewport");
    panel->SetHeaderHeight(30.0f);
    panel->SetTabIcon(WindEffects::Editor::UI::Icons::PerspectiveName);
    panel->AddHeaderAction(WindEffects::Editor::UI::Icons::XName, []() {});
    panel->SetTransparentBackground(true);

    auto toolbar = std::make_shared<WindEffects::Editor::UI::Toolbar>();
    toolbar->SetHeight(28.0f);
    toolbar->SetIconSize(16.0f);

    using WindEffects::Editor::UI::ToolButtonStyle;
    using WindEffects::Editor::UI::ToolbarAlignment;
    namespace Icons = WindEffects::Editor::UI::Icons;

    auto btnViewport = toolbar->AddTool(Icons::PerspectiveName, "Viewport", [](){}, "Viewport Type");
    btnViewport->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnViewport->SetIsDropdown(true);

    toolbar->AddSeparator();

    auto btnDisplay = toolbar->AddTool(Icons::ConsoleName, "Display 1", [](){}, "Select Display");
    btnDisplay->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnDisplay->SetIsDropdown(true);

    toolbar->AddSeparator();

    auto btnLit = toolbar->AddTool(Icons::LitName, "Lit", [](){}, "Render Mode");
    btnLit->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnLit->SetIsDropdown(true);

    toolbar->AddSeparator();

    auto btnCamera = toolbar->AddTool(Icons::CameraName, "Camera", [](){}, "Camera Settings");
    btnCamera->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnCamera->SetIsDropdown(true);

    toolbar->AddSeparator();

    auto btnShow = toolbar->AddTool(Icons::EyeName, "Show", [](){}, "Show/Hide Elements");
    btnShow->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnShow->SetIsDropdown(true);

    toolbar->AddSeparator();

    auto btnStats = toolbar->AddTool(Icons::ProfilerName, "Stats", [](){}, "Toggle Stats");
    btnStats->SetButtonStyle(ToolButtonStyle::ToolbarInline);

    toolbar->AddSeparator();

    auto btnGizmos = toolbar->AddTool(Icons::GridName, "Gizmos", [](){}, "Toggle Gizmos");
    btnGizmos->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnGizmos->SetIsDropdown(true);

    toolbar->AddSeparator(ToolbarAlignment::Right);

    auto btnCameraSpeed = toolbar->AddTool(
        Icons::CameraName,
        "4",
        []() { ShowViewportCameraSpeedPopup(); },
        "Camera Speed",
        false,
        ToolbarAlignment::Right);
    btnCameraSpeed->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnCameraSpeed->SetIsDropdown(true);
    btnCameraSpeed->SetOnMouseWheel([](float wheelDeltaY) {
        AdjustViewportCameraSpeedFromWheel(wheelDeltaY);
    });
    SetViewportCameraSpeedIndicator(btnCameraSpeed);

    panel->SetToolbar(toolbar);

    auto placeholder = std::make_shared<WindEffects::Editor::UI::Label>("");
    panel->SetContent(placeholder);

    return panel;
}

REGISTER_UI_PANEL(Viewport,
    (WindEffects::Editor::UI::DockPanelDescriptor{.title = "Viewport 1", .iconResource = "viewport", .defaultZone = WindEffects::Editor::UI::DockZone::Center, .defaultVisible = true}),
    CreateViewportPanel)

} // namespace we::programs::editor
