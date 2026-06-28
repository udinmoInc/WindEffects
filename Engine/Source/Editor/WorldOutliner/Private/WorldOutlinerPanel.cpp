#include "EditorRegistry.hpp"
#include "Widgets/Panel.hpp"
#include "Widgets/TreeView.hpp"
#include "Widgets/SearchBox.hpp"
#include "Layout/Box.hpp"
#include "Core/Icon.hpp"
#include "Runtime/Core/Localization.hpp"

namespace we::programs::editor {

std::shared_ptr<we::UI::Panel> CreateWorldOutlinerPanel() {
    auto title = we::core::Localization::Get().GetString("Panel_WorldOutliner", "World Outliner");
    auto panel = std::make_shared<we::UI::Panel>(std::string(title));
    panel->SetHeaderHeight(30.0f);
    panel->AddHeaderAction(we::UI::Icons::XName, []() {});
    
    auto toolbarBox = std::make_shared<we::UI::HorizontalBox>();
    toolbarBox->SetPadding(we::UI::Margin{8.0f, 4.0f, 8.0f, 4.0f});
    auto searchBox = std::make_shared<we::UI::SearchBox>();
    searchBox->SetFillWidth(true);
    auto searchPlaceholder = we::core::Localization::Get().GetString("UI_SearchPlaceholder", "Search...");
    searchBox->SetPlaceholder(std::string(searchPlaceholder));
    toolbarBox->AddChild(searchBox);
    
    panel->SetToolbar(toolbarBox);

    auto treeView = std::make_shared<we::UI::TreeView>();
    // TODO: Connect this to the ServiceLocator or Scene Manager in the future
    // For now, it will be populated elsewhere, or we move the logic here.
    
    panel->SetContent(treeView);
    return panel;
}

REGISTER_EDITOR_PANEL(WorldOutliner, CreateWorldOutlinerPanel)

} // namespace we::programs::editor
