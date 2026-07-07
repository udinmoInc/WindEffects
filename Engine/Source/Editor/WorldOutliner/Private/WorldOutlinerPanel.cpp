#include "EditorRegistry.h"
#include "Explorer/WorldOutlinerApi.h"
#include "Explorer/ExplorerPanelAssets.h"
#include "Core/DockTabIconRegistry.h"
#include "Widgets/Panel.h"
#include "Widgets/TreeView.h"
#include "Widgets/ExplorerPanelHeader.h"
#include "Widgets/ExplorerFilterMenu.h"
#include "Core/Theme.h"
#include "Layout/OverlayManager.h"

namespace we::programs::editor {
using namespace we::UI;

namespace {
struct ExplorerDockTabRegistration {
    ExplorerDockTabRegistration() {
        DockTabIconRegistry::Get().RegisterIcon("Explorer", Icons::HierarchyName);
    }
};
ExplorerDockTabRegistration g_ExplorerDockTabRegistration;

// Global header instance for the Explorer panel
std::shared_ptr<ExplorerPanelHeader> g_ExplorerHeader;
} // namespace

std::shared_ptr<Panel> CreateWorldOutlinerPanel() {
    auto panel = std::make_shared<Panel>("Explorer");
    panel->SetHeaderHeight(ExplorerPanelHeader::kDefaultHeight);
    panel->SetCollapsible(false);

    auto treeView = std::make_shared<TreeView>();
    treeView->SetExplorerStyle(true);
    treeView->SetItemHeight(22.0f);
    treeView->SetIndentWidth(18.0f);
    RegisterExplorerTreeView(treeView);

    // Create enhanced header with search and filter
    g_ExplorerHeader = std::make_shared<ExplorerPanelHeader>();
    
    // Setup header callbacks
    g_ExplorerHeader->SetOnSearchChanged([treeView](const std::string& query) {
        treeView->SetSearchQuery(query);
    });
    
    g_ExplorerHeader->SetOnFilterClicked([treeView]() {
        const auto& headerGeom = g_ExplorerHeader->GetGeometry();
        const auto& filterBtnGeom = g_ExplorerHeader->GetFilterButtonGeometry();
        
        Point menuPos{ filterBtnGeom.x, headerGeom.y + headerGeom.height };
        
        auto filterOptions = g_ExplorerHeader->GetFilterOptions();
        auto menu = std::make_shared<ExplorerFilterMenu>(
            filterOptions,
            [treeView](const TreeView::FilterOptions& options) {
                g_ExplorerHeader->SetFilterOptions(options);
                treeView->SetFilterOptions(options);
            }
        );
        
        if (auto* overlay = OverlayManager::Get()) {
            overlay->CloseAllPopups();
            overlay->ShowPopup(menu, menuPos);
        }
    });
    
    g_ExplorerHeader->SetOnRefresh([treeView]() {
        treeView->SetSearchQuery(treeView->GetSearchQuery());
    });

    panel->SetContent(treeView);
    return panel;
}

REGISTER_EDITOR_PANEL(WorldOutliner, CreateWorldOutlinerPanel)

} // namespace we::programs::editor
