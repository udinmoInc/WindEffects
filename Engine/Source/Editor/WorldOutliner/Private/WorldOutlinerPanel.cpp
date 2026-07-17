#include "Explorer/WorldOutlinerApi.h"
#include "Explorer/ExplorerPanelAssets.h"
#include "WindEffects/Editor/EditorSDK.h"
#include "Widgets/Panel.h"
#include "Widgets/TreeView.h"
#include "Widgets/ExplorerPanelHeader.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "Widgets/ExplorerFilterMenu.h"
#include "KindUI/Core/Icon.h"
#include "EditorWorkspaceController.h"

namespace we::programs::editor {
using namespace we::runtime::kindui;

namespace {
std::shared_ptr<ExplorerPanelHeader> g_ExplorerHeader;
}

std::shared_ptr<Panel> CreateWorldOutlinerPanel() {
    auto panel = std::make_shared<Panel>("Explorer");
    panel->SetHeaderHeight(ResolveMetric(MetricToken::PanelHeaderHeight));
    panel->SetCollapsible(false);
    panel->SetTabIcon(Icons::HierarchyName);

    auto treeView = std::make_shared<TreeView>();
    treeView->SetExplorerStyle(true);
    treeView->SetItemHeight(26.0f);
    treeView->SetIndentWidth(16.0f);
    RegisterExplorerTreeView(treeView);

    g_ExplorerHeader = std::make_shared<ExplorerPanelHeader>();

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

        if (auto* overlay = GetEditorPopupHost()) {
            overlay->CloseAllPopups();
            overlay->ShowPopup(menu, menuPos);
        }
    });

    g_ExplorerHeader->SetOnRefresh([treeView]() {
        treeView->SetSearchQuery(treeView->GetSearchQuery());
    });

    panel->SetToolbar(g_ExplorerHeader);
    panel->SetContent(treeView);
    return panel;
}

REGISTER_UI_PANEL(WorldOutliner,
    WE_PANEL(WorldOutliner).Title("Explorer").Icon("outliner").Zone(DockZone::Right).WindowMenu("Explorer").SortOrder(2),
    CreateWorldOutlinerPanel)

} // namespace we::programs::editor
