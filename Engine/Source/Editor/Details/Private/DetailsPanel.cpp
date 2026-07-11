#include "WindEffects/Editor/UI/Extensions/ExtensionBootstrap.h"
#include "Widgets/Panel.h"
#include "Widgets/PropertyEditor.h"
#include "Widgets/SearchBox.h"
#include "Layout/Box.h"
#include "Core/Icon.h"
#include "Localization.h"

namespace we::programs::editor {

std::shared_ptr<WindEffects::Editor::UI::Panel> CreateDetailsPanel() {
    auto title = we::core::Localization::Get().GetString("Panel_Details", "Details");
    auto panel = std::make_shared<WindEffects::Editor::UI::Panel>(std::string(title));
    panel->SetHeaderHeight(30.0f);
    panel->AddHeaderAction(WindEffects::Editor::UI::Icons::XName, []() {});
    
    auto toolbarBox = std::make_shared<WindEffects::Editor::UI::HorizontalBox>();
    toolbarBox->SetPadding(WindEffects::Editor::UI::Margin{8.0f, 4.0f, 8.0f, 4.0f});
    auto searchBox = std::make_shared<WindEffects::Editor::UI::SearchBox>();
    searchBox->SetFillWidth(true);
    auto searchPlaceholder = we::core::Localization::Get().GetString("UI_SearchPlaceholder", "Search...");
    searchBox->SetPlaceholder(std::string(searchPlaceholder));
    toolbarBox->AddChild(searchBox);
    
    panel->SetToolbar(toolbarBox);

    auto propertyEditor = std::make_shared<WindEffects::Editor::UI::PropertyEditor>();
    panel->SetContent(propertyEditor);

    return panel;
}

REGISTER_UI_PANEL(Details,
    (WindEffects::Editor::UI::DockPanelDescriptor{.title = "Details", .defaultZone = WindEffects::Editor::UI::DockZone::Right, .defaultVisible = true}),
    CreateDetailsPanel)

} // namespace we::programs::editor
