#include "EditorRegistry.hpp"
#include "Explorer/WorldOutlinerApi.h"
#include "Explorer/ExplorerPanelAssets.hpp"
#include "Core/DockTabIconRegistry.hpp"
#include "Widgets/Panel.hpp"
#include "Widgets/TreeView.hpp"
#include "Core/Theme.hpp"

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

    auto treeView = std::make_shared<we::UI::TreeView>();
    treeView->SetExplorerStyle(true);
    treeView->SetItemHeight(22.0f);
    treeView->SetIndentWidth(18.0f);
    RegisterExplorerTreeView(treeView);

    panel->SetContent(treeView);
    return panel;
}

REGISTER_EDITOR_PANEL(WorldOutliner, CreateWorldOutlinerPanel)

} // namespace we::programs::editor
