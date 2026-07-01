#include "EditorRegistry.hpp"
#include "Explorer/WorldOutlinerApi.h"
#include "Explorer/ExplorerPanelAssets.hpp"
#include "Core/DockTabIconRegistry.hpp"
#include "Widgets/Panel.hpp"
#include "Widgets/ExplorerPanelHeader.hpp"
#include "Widgets/TreeView.hpp"
#include "Widgets/SearchBox.hpp"
#include "Layout/Box.hpp"
#include "Core/Icon.hpp"
#include "Localization.hpp"

namespace we::programs::editor {

namespace {
struct ExplorerDockTabRegistration {
    ExplorerDockTabRegistration() {
        we::UI::DockTabIconRegistry::Get().RegisterIcon("Explorer", we::UI::Icons::HierarchyName);
    }
};
ExplorerDockTabRegistration g_ExplorerDockTabRegistration;
} // namespace

std::shared_ptr<we::UI::Panel> CreateWorldOutlinerPanel() {
    auto panel = std::make_shared<we::UI::Panel>("Explorer");
    panel->SetHeaderHeight(0.0f);
    panel->SetCollapsible(false);

    auto header = std::make_shared<we::UI::ExplorerPanelHeader>();
    header->SetTitle("Explorer");
    header->SetHeight(we::UI::ExplorerPanelHeader::kDefaultHeight);

    auto searchRow = std::make_shared<we::UI::HorizontalBox>();
    searchRow->SetPadding(we::UI::Margin{ 8.0f, 4.0f, 8.0f, 6.0f });

    auto searchBox = std::make_shared<we::UI::SearchBox>();
    searchBox->SetFillWidth(true);
    const auto searchPlaceholder = we::core::Localization::Get().GetString("UI_SearchPlaceholder", "Search actors...");
    searchBox->SetPlaceholder(std::string(searchPlaceholder));
    searchRow->AddChild(searchBox);

    auto treeView = std::make_shared<we::UI::TreeView>();
    RegisterExplorerTreeView(treeView);

    auto content = std::make_shared<we::UI::VerticalBox>();
    content->SetSpacing(0.0f);
    content->AddChild(header);
    content->AddChild(searchRow);
    content->AddChild(treeView);

    panel->SetContent(content);
    return panel;
}

REGISTER_EDITOR_PANEL(WorldOutliner, CreateWorldOutlinerPanel)

} // namespace we::programs::editor
