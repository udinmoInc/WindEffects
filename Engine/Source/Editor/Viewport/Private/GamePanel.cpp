#include "EditorRegistry.hpp"
#include "Widgets/Panel.hpp"
#include "Core/Icon.hpp"
#include "Widgets/Label.hpp"
#include "Widgets/Toolbar.hpp"
#include "Widgets/ToolButton.hpp"

namespace we::programs::editor {

std::shared_ptr<we::UI::Panel> CreateGamePanel() {
    auto panel = std::make_shared<we::UI::Panel>("Game");
    panel->SetHeaderHeight(30.0f);
    panel->AddHeaderAction(we::UI::Icons::XName, []() {});

    // Create Level-2 Toolbar for Game Viewport
    auto toolbar = std::make_shared<we::UI::Toolbar>();
    toolbar->SetHeight(28.0f); // 28-30px toolbar height
    toolbar->SetIconSize(16.0f); // Compact size
    
    using we::UI::ToolButtonStyle;
    using we::UI::ToolbarAlignment;
    namespace Icons = we::UI::Icons;

    // Game dropdown
    auto btnGame = toolbar->AddTool(Icons::PlayName, "Game", [](){}, "Game View Options");
    btnGame->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnGame->SetIsDropdown(true);

    toolbar->AddSeparator();

    // Display dropdown
    auto btnDisplay = toolbar->AddTool(Icons::ConsoleName, "Display 1", [](){}, "Select Display");
    btnDisplay->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnDisplay->SetIsDropdown(true);

    toolbar->AddSeparator();

    // Resolution dropdown
    auto btnResolution = toolbar->AddTool(Icons::MaximizeName, "Auto Resolution", [](){}, "Select Resolution");
    btnResolution->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnResolution->SetIsDropdown(true);

    toolbar->AddSeparator();

    // Aspect Ratio dropdown
    auto btnAspect = toolbar->AddTool(Icons::CameraName, "Free Aspect", [](){}, "Aspect Ratio");
    btnAspect->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnAspect->SetIsDropdown(true);

    toolbar->AddSeparator();

    // Scale dropdown
    auto btnScale = toolbar->AddTool(Icons::ScaleName, "1x", [](){}, "View Scale");
    btnScale->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnScale->SetIsDropdown(true);

    toolbar->AddSeparator();

    // Play Focus
    auto btnPlayFocus = toolbar->AddTool(Icons::PlayName, "Play Focus", [](){}, "Play Focus Mode");
    btnPlayFocus->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnPlayFocus->SetIsDropdown(true);

    toolbar->AddSeparator();

    // Stats
    auto btnStats = toolbar->AddTool(Icons::ProfilerName, "Stats", [](){}, "Toggle Stats");
    btnStats->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    
    toolbar->AddSeparator();

    // Gizmos
    auto btnGizmos = toolbar->AddTool(Icons::GridName, "Gizmos", [](){}, "Toggle Gizmos");
    btnGizmos->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    btnGizmos->SetIsDropdown(true);

    panel->SetToolbar(toolbar);

    // Placeholder for actual game view rendering
    auto placeholder = std::make_shared<we::UI::Label>("");
    panel->SetContent(placeholder);

    return panel;
}

REGISTER_EDITOR_PANEL(Game, CreateGamePanel)

} // namespace we::programs::editor
