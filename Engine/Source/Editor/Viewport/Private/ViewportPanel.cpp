#include "WindEffects/Editor/EditorSDK.h"
#include "ViewportToolbarState.h"
#include "Widgets/Toolbar.h"
#include "Widgets/ToolButton.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <functional>
#include "KindUI/Tokens/DesignToken.h"

namespace we::programs::editor {

using we::runtime::kindui::ColorToken;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::PaddingToken;

using namespace we::runtime::kindui;
using namespace we::editor::ui;

namespace {

std::shared_ptr<ToolButton> MakeViewportDropdown(
    const char* icon,
    const std::string& label,
    std::function<void()> onClick = {},
    const char* tooltip = nullptr)
{
    auto button = std::make_shared<ToolButton>(icon, label, std::move(onClick), tooltip ? tooltip : label.c_str());
    button->SetButtonStyle(ToolButtonStyle::ViewportChip);
    button->SetIsDropdown(true);
    button->SetVerticalAlignment(VerticalAlignment::Center);
    return button;
}

std::shared_ptr<ToolButton> MakeViewportIconDropdown(
    const char* icon,
    std::function<void()> onClick = {},
    const char* tooltip = nullptr)
{
    auto button = std::make_shared<ToolButton>(icon, "", std::move(onClick), tooltip ? tooltip : icon);
    button->SetButtonStyle(ToolButtonStyle::ViewportChip);
    button->SetIsDropdown(true);
    button->SetVerticalAlignment(VerticalAlignment::Center);
    return button;
}

std::shared_ptr<ToolButton> MakeViewportIconButton(
    const char* icon,
    std::function<void()> onClick = {},
    const char* tooltip = nullptr)
{
    auto button = std::make_shared<ToolButton>(icon, "", std::move(onClick), tooltip ? tooltip : icon);
    button->SetButtonStyle(ToolButtonStyle::ViewportChip);
    button->SetVerticalAlignment(VerticalAlignment::Center);
    return button;
}

} // namespace

std::shared_ptr<Panel> CreateViewportPanel() {
    std::shared_ptr<ToolButton> cameraSpeedButton;

    auto cameraButton = MakeViewportDropdown(
        Icons::CameraName,
        "Camera",
        []() { ShowViewportCameraSpeedPopup(); },
        "Camera Speed");
    cameraButton->SetOnMouseWheel([](float wheelDeltaY) {
        AdjustViewportCameraSpeedFromWheel(wheelDeltaY);
    });
    cameraSpeedButton = cameraButton;

    auto toolbar = std::make_shared<Toolbar>();
    toolbar->SetFloating(true);
    toolbar->SetHeight(we::runtime::kindui::ResolveMetric(MetricToken::PanelHeaderHeight));

    toolbar->AddWidget(MakeViewportDropdown(Icons::PerspectiveName, "Perspective", {}, "Viewport Type"), ToolbarAlignment::Left);
    toolbar->AddWidget(MakeViewportDropdown(Icons::LitName, "Lit", {}, "Render Mode"), ToolbarAlignment::Left);
    toolbar->AddWidget(cameraButton, ToolbarAlignment::Left);

    toolbar->AddWidget(MakeViewportIconDropdown(Icons::EyeName, {}, "Show/Hide Elements"), ToolbarAlignment::Right);
    toolbar->AddWidget(MakeViewportIconButton(Icons::ProfilerName, [](){}, "Toggle Stats"), ToolbarAlignment::Right);
    toolbar->AddWidget(MakeViewportIconDropdown(Icons::MoveName, {}, "Toggle Gizmos"), ToolbarAlignment::Right);
    toolbar->AddWidget(MakeViewportIconDropdown(Icons::GridName, {}, "Viewport Grid"), ToolbarAlignment::Right);

    if (cameraSpeedButton) {
        SetViewportCameraSpeedIndicator(cameraSpeedButton);
    }

    return PanelBuilder("Viewport")
        .TabIcon(Icons::PerspectiveName)
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
