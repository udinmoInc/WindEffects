#include "EditorRegistry.hpp"
#include "EditorLayoutController.hpp"
#include "Widgets/Panel.hpp"
#include "Widgets/ContentBrowser.hpp"
#include "Widgets/SearchBox.hpp"
#include "Layout/Box.hpp"
#include "Core/Icon.hpp"
#include "Localization.hpp"

namespace we::programs::editor {

namespace {

void PopulateSampleContent(const std::shared_ptr<we::UI::ContentBrowser>& browser) {
    auto model = browser->GetModel();
    if (!model || !model->items.empty()) return;

    auto addItem = [&](const std::string& id, const std::string& name, const std::string& type,
                       const std::string& icon, bool folder = false) {
        we::UI::ContentItem item;
        item.id = id;
        item.name = name;
        item.type = type;
        item.path = "/Game/" + id;
        item.iconName = icon;
        item.isFolder = folder;
        model->items.push_back(std::move(item));
    };

    addItem("Maps", "Maps", "folder", "folder", true);
    addItem("Materials", "Materials", "folder", "material", true);
    addItem("Meshes", "Meshes", "folder", "cube", true);
    addItem("Textures", "Textures", "folder", "layers", true);
    addItem("PlayerCharacter", "PlayerChar...", "blueprint", "hierarchy");
    addItem("M_Concrete", "M_Concrete", "material", "material");
    addItem("SM_Crate", "SM_Crate", "mesh", "cube");
    addItem("T_Brick_D", "T_Brick_D", "texture", "layers");
    addItem("Level_01", "Level_01", "level", "grid");
    model->NotifyChanged();
}

} // namespace

std::shared_ptr<we::UI::Panel> CreateContentBrowserPanel() {
    auto title = we::core::Localization::Get().GetString("Panel_ContentBrowser", "Content Browser");
    auto panel = std::make_shared<we::UI::Panel>(std::string(title));
    panel->SetHeaderHeight(30.0f);

    panel->AddHeaderAction(we::UI::Icons::XName, []() {
        if (EditorLayoutController::Get().IsContentBrowserExpanded()) {
            EditorLayoutController::Get().ToggleContentBrowserExpanded();
        }
    });

    auto toolbarBox = std::make_shared<we::UI::HorizontalBox>();
    toolbarBox->SetPadding(we::UI::Margin{8.0f, 4.0f, 8.0f, 4.0f});
    auto searchBox = std::make_shared<we::UI::SearchBox>();
    searchBox->SetFillWidth(true);
    auto searchPlaceholder = we::core::Localization::Get().GetString("UI_SearchPlaceholder", "Search...");
    searchBox->SetPlaceholder(std::string(searchPlaceholder));
    toolbarBox->AddChild(searchBox);
    panel->SetToolbar(toolbarBox);

    auto contentBrowser = std::make_shared<we::UI::ContentBrowser>();
    PopulateSampleContent(contentBrowser);
    panel->SetContent(contentBrowser);

    return panel;
}

REGISTER_EDITOR_PANEL(ContentBrowser, CreateContentBrowserPanel)

} // namespace we::programs::editor
