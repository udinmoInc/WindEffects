#include "EditorRegistry.h"
#include "ViewportToolbarState.h"
#include "Widgets/Panel.h"
#include "Core/Icon.h"
#include "Widgets/Label.h"

#include "Widgets/Toolbar.h"
#include "Widgets/ToolButton.h"

namespace we::programs::editor {

std::shared_ptr<we::UI::Panel> CreateViewportPanel() {
    auto panel = std::make_shared<we::UI::Panel>("Viewport");
    panel->SetHeaderHeight(30.0f);
    panel->AddHeaderAction(we::UI::Icons::XName, []() {});

    // Create Level-2 Toolbar for Scene Viewport
    auto toolbar = std::make_shared<we::UI::Toolbar>();
    toolbar->SetHeight(28.0f); // 28-30px toolbar height
    toolbar->SetIconSize(16.0f); // Compact size
    
    using we::UI::ToolButtonStyle;
    using we::UI::ToolbarAlignment;
    namespace Icons = we::UI::Icons;

    // Viewport dropdown
    auto btnViewport = toolbar->AddTool(Icons::PerspectiveName, "Viewport", [](){}, "Viewport Type");
    btnViewport->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnViewport->SetIsDropdown(true);

    toolbar->AddSeparator();

    // Display dropdown
    auto btnDisplay = toolbar->AddTool(Icons::ConsoleName, "Display 1", [](){}, "Select Display");
    btnDisplay->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnDisplay->SetIsDropdown(true);

    toolbar->AddSeparator();

    // Render Mode dropdown
    auto btnLit = toolbar->AddTool(Icons::LitName, "Lit", [](){}, "Render Mode");
    btnLit->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnLit->SetIsDropdown(true);

    toolbar->AddSeparator();

    // Camera dropdown
    auto btnCamera = toolbar->AddTool(Icons::CameraName, "Camera", [](){}, "Camera Settings");
    btnCamera->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnCamera->SetIsDropdown(true);

    toolbar->AddSeparator();

    // Show dropdown
    auto btnShow = toolbar->AddTool(Icons::EyeName, "Show", [](){}, "Show/Hide Elements");
    btnShow->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnShow->SetIsDropdown(true);

    toolbar->AddSeparator();

    // Stats
    auto btnStats = toolbar->AddTool(Icons::ProfilerName, "Stats", [](){}, "Toggle Stats");
    btnStats->SetButtonStyle(ToolButtonStyle::ToolbarInline);

    toolbar->AddSeparator();
    
    // Gizmos
    auto btnGizmos = toolbar->AddTool(Icons::GridName, "Gizmos", [](){}, "Toggle Gizmos");
    btnGizmos->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnGizmos->SetIsDropdown(true);

    toolbar->AddSeparator(ToolbarAlignment::Right);

    // UE5-style camera speed (default 4, max 50).
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

    // ViewportWidget requires Renderer, Camera, Scene, UIRenderer which
    // are not available at static registration time. Set an empty placeholder
    // content for now — the real ViewportWidget will be injected later by
    // the editor application once the runtime services are initialized.
    auto placeholder = std::make_shared<we::UI::Label>("");
    panel->SetContent(placeholder);

    return panel;
}

REGISTER_EDITOR_PANEL(Viewport, CreateViewportPanel)

} // namespace we::programs::editor
