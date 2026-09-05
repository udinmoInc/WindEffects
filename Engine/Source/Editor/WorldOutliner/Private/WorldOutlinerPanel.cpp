#include "Explorer/WorldOutlinerApi.h"
#include "Explorer/ExplorerPanelAssets.h"
#include "WorldOutliner/WorldOutlinerSession.h"
#include "WorldOutliner/OutlinerTypes.h"
#include "WindEffects/Editor/EditorSDK.h"
#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "ContentBrowser/Widgets/TreeView.h"
#include "ContentBrowser/Widgets/TreeColumnHeader.h"
#include "Widgets/ExplorerPanelHeader.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "Widgets/ExplorerFilterMenu.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Layout/Flex.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "KindUI/Tokens/DesignToken.h"

namespace we::programs::editor {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;


using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::TypographyToken;

using namespace ::we::runtime::kindui;
using namespace ::we::runtime::kindui;
using ::we::editor::panels::Panel;
using ::we::editor::panels::PanelBuilder;
using ::we::editor::docking::DockZone;
using ::we::editor::outliner::ExplorerPanelHeader;
using ::we::editor::outliner::ExplorerFilterMenu;
using ::we::editor::contentbrowser::TreeView;
using ::we::editor::contentbrowser::TreeColumnHeader;

namespace {
std::shared_ptr<ExplorerPanelHeader> g_ExplorerHeader;
}

std::shared_ptr<Panel> CreateWorldOutlinerPanel() {
    auto panel = std::make_shared<Panel>("Outliner");
    panel->AttachBodyLayout();
    panel->SetHeaderHeight(we::runtime::kindui::ResolveMetric(MetricToken::PanelHeaderHeight));
    panel->SetCollapsible(false);
    panel->SetTabIcon(WindIcons::Outliner16);

    auto treeView = std::make_shared<TreeView>();
    treeView->SetExplorerStyle(true);
    treeView->SetShowColumnHeader(false);
    treeView->SetItemHeight(we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::ListRowHeight));
    treeView->SetIndentWidth(we::runtime::kindui::ResolveMetric(MetricToken::TreeIndentWidth));
    treeView->SetFlexGrow(1.0f);
    RegisterExplorerTreeView(treeView);

    auto columnHeader = std::make_shared<TreeColumnHeader>();

    auto statusLabel = std::make_shared<Label>("15 actors (1 selected)", TypographyToken::Caption);
    auto statusRow = std::make_shared<Row>();
    statusRow->SetFlexShrink(0.0f);
    statusRow->Padding(Margin{
        we::runtime::kindui::ResolveMetric(MetricToken::Space2),
        we::runtime::kindui::ResolveMetric(MetricToken::Space1),
        we::runtime::kindui::ResolveMetric(MetricToken::Space2),
        we::runtime::kindui::ResolveMetric(MetricToken::Space1)
    });
    statusRow->AddChild(statusLabel);

    treeView->SetOnSelectionChanged([statusLabel, treeView](const std::vector<std::string>& selectedIds) {
        const size_t selCount = selectedIds.size();
        const size_t totalCount = treeView->GetRenderItemCount();
        if (selCount > 0) {
            statusLabel->SetText(std::to_string(totalCount) + " actors (" + std::to_string(selCount) + " selected)");
        } else {
            statusLabel->SetText(std::to_string(totalCount) + " actors");
        }
    });

    if (auto* outliner = ::we::editor::outliner::WorldOutlinerSession::Outliner()) {
        outliner->BindTreeView(treeView);
    }

    g_ExplorerHeader = std::make_shared<ExplorerPanelHeader>();
    g_ExplorerHeader->Initialize();

    g_ExplorerHeader->SetOnSearchChanged([treeView](const std::string& query) {
        treeView->SetSearchQuery(query);
        if (auto* outliner = ::we::editor::outliner::WorldOutlinerSession::Outliner()) {
            auto state = outliner->GetFilterState();
            state.searchQuery = query;
            outliner->SetFilterState(state);
        }
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
                if (auto* outliner = ::we::editor::outliner::WorldOutlinerSession::Outliner()) {
                    ::we::editor::outliner::OutlinerFilterState state = outliner->GetFilterState();
                    state.showFolders = options.showFolders;
                    state.showActors = options.showActors;
                    state.showComponents = options.showComponents;
                    state.showHidden = options.showHidden;
                    state.showLocked = options.showLocked;
                    state.showEmptyFolders = options.showEmptyFolders;
                    state.favoritesOnly = options.showFavorites;
                    state.sortMode = static_cast<::we::editor::outliner::OutlinerSortMode>(options.sortOrder);
                    outliner->SetFilterState(state);
                }
            }
        );

        if (auto* overlay = GetEditorPopupHost()) {
            overlay->CloseAllPopups();
            overlay->ShowPopup(menu, menuPos);
        }
    });

    g_ExplorerHeader->SetOnRefresh([treeView]() {
        treeView->SetSearchQuery(treeView->GetSearchQuery());
        if (auto* outliner = ::we::editor::outliner::WorldOutlinerSession::Outliner()) {
            outliner->RequestRebuild();
            outliner->Tick(0.f);
        }
    });

    g_ExplorerHeader->SetOnNewFolder([]() {
        if (auto* outliner = ::we::editor::outliner::WorldOutlinerSession::Outliner()) {
            (void)outliner->Commands().CreateFolder("New Folder");
            outliner->RequestRebuild();
            outliner->Tick(0.f);
        }
    });

    panel->SetSearch(g_ExplorerHeader);
    panel->SetColumnHeader(columnHeader);
    panel->SetContent(treeView);
    panel->SetFooter(statusRow);
    return panel;
}

REGISTER_UI_PANEL(WorldOutliner,
    WE_PANEL(WorldOutliner).Title("Outliner").Icon("outliner").Zone(DockZone::Right).WindowMenu("Outliner").SortOrder(2),
    CreateWorldOutlinerPanel)

} // namespace we::programs::editor
