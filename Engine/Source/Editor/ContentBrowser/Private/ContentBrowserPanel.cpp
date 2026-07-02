#include "EditorRegistry.hpp"
#include "EditorLayoutController.hpp"
#include "ContentBrowserApi.h"
#include "Widgets/Panel.hpp"
#include "Widgets/ContentBrowser.hpp"
#include "Widgets/ContentBrowserToolbar.hpp"
#include "Widgets/SearchBox.hpp"
#include "Widgets/TreeView.hpp"
#include "Layout/Box.hpp"
#include "Layout/Splitter.hpp"
#include "Core/Icon.hpp"
#include "Core/IconManager.hpp"
#include "Localization.hpp"
#include "Services/ContentBrowserService.hpp"
#include "Registry/ContentAssetRegistry.hpp"
#include "Controllers/FilterController.hpp"
#include <memory>
#include <sstream>

namespace we::programs::editor {

namespace {

using we::editor::contentbrowser::AssetRecord;
using we::editor::contentbrowser::ContentAssetRegistry;
using we::editor::contentbrowser::ContentBrowserService;
using we::editor::contentbrowser::ContentFilter;

std::shared_ptr<we::UI::TreeNode> MakeSection(const std::string& id, const std::string& label,
    const std::string& icon, bool expanded = false)
{
    auto node = std::make_shared<we::UI::TreeNode>();
    node->id = id;
    node->label = label;
    node->iconName = icon;
    node->expanded = expanded;
    return node;
}

std::shared_ptr<we::UI::TreeNode> BuildFolderNode(const AssetRecord* folder) {
    auto node = std::make_shared<we::UI::TreeNode>();
    node->id = folder->id;
    node->label = folder->name;
    node->iconName = we::UI::IconManager::FolderIcon;
    node->expanded = folder->virtualPath == "/Game";

    for (const auto* child : ContentAssetRegistry::Get().GetChildren(folder->virtualPath)) {
        if (child->isFolder) node->children.push_back(BuildFolderNode(child));
    }
    return node;
}

void RefreshFolderTree(const std::shared_ptr<we::UI::TreeView>& tree) {
    auto root = std::make_shared<we::UI::TreeNode>();
    root->id = "__content_root__";
    root->label = "Content";
    root->expanded = true;

    root->children.push_back(MakeSection("__favorites__", "Favorites", "check"));
    root->children.push_back(MakeSection("__collections__", "Collections", "layers"));
    root->children.push_back(MakeSection("__plugins__", "Plugins", "build"));
    root->children.push_back(MakeSection("__engine__", "Engine Content", "package", false));

    auto project = MakeSection("__project__", "Project Content", "open", true);
    if (const auto* game = ContentAssetRegistry::Get().FindByVirtualPath("/Game")) {
        project->children.push_back(BuildFolderNode(game));
    }
    root->children.push_back(project);

    tree->SetRoot(root);
}

void UpdateBreadcrumb(const std::shared_ptr<we::UI::Breadcrumb>& breadcrumb, const std::string& virtualPath) {
    std::vector<std::string> crumbs;
    crumbs.push_back("All");
    if (virtualPath.size() <= 6) {
        breadcrumb->SetPath(crumbs);
        return;
    }
    std::string remainder = virtualPath.substr(6);
    std::stringstream ss(remainder);
    std::string segment;
    while (std::getline(ss, segment, '/')) {
        if (!segment.empty()) crumbs.push_back(segment);
    }
    breadcrumb->SetPath(crumbs);
}

void NavigateToFolder(const std::string& virtualPath,
    const std::shared_ptr<we::UI::ContentBrowser>& browser,
    const std::shared_ptr<we::UI::Breadcrumb>& breadcrumb,
    const std::shared_ptr<we::UI::ContentBrowserStatusBar>& statusBar)
{
    ContentBrowserService::Get().SetCurrentFolder(virtualPath);
    UpdateBreadcrumb(breadcrumb, virtualPath);
    if (statusBar && browser->GetModel()) {
        statusBar->SetAssetCount(browser->GetModel()->assetCount);
        statusBar->SetFolderCount(browser->GetModel()->folderCount);
        statusBar->SetSelectedCount(browser->GetModel()->selectedIds.size());
    }
}

void WireContentBrowser(
    const std::shared_ptr<we::UI::ContentBrowser>& browser,
    const std::shared_ptr<we::UI::ContentBrowserStatusBar>& statusBar,
    const std::shared_ptr<we::UI::Breadcrumb>& breadcrumb)
{
    auto& service = ContentBrowserService::Get();
    service.RefreshBrowserModel(browser->GetModel());

    browser->SetOnItemNeedsThumbnail([&service](const std::string& id) {
        service.RequestThumbnailForItem(id);
    });
    browser->SetOnVisibleItemsChanged([&service](const std::unordered_set<std::string>& ids) {
        service.SetVisibleItemIds(ids);
    });
    browser->SetOnItemDoubleClicked([&service, browser, statusBar, breadcrumb](const we::UI::ContentItem& item) {
        if (item.isFolder) NavigateToFolder(item.path, browser, breadcrumb, statusBar);
    });
    browser->SetOnItemSelected([browser, statusBar](const we::UI::ContentItem&) {
        if (browser->GetModel() && statusBar) {
            statusBar->SetSelectedCount(browser->GetModel()->selectedIds.size());
        }
    });
    service.SetOnThumbnailReady([browser](const std::string& id, VkDescriptorSet texture) {
        if (browser->GetController()) browser->GetController()->UpdateItemIcon(id, texture);
    });
}

} // namespace

void InitializeContentBrowserService(we::UI::IconRenderer* iconRenderer) {
    if (iconRenderer) {
        we::UI::IconManager::Get().Initialize(iconRenderer);
    }
    ContentBrowserService::Get().Initialize(iconRenderer, "Content");
}

void ShutdownContentBrowserService() {
    ContentBrowserService::Get().Shutdown();
}

std::shared_ptr<we::UI::Panel> CreateContentBrowserPanel() {
    auto title = we::core::Localization::Get().GetString("Panel_ContentBrowser", "Content Browser");
    auto panel = std::make_shared<we::UI::Panel>(std::string(title));
    panel->SetHeaderHeight(28.0f);
    panel->SetBackgroundColor(we::UI::Color{ 0.10f, 0.10f, 0.11f, 1.0f });

    panel->AddHeaderAction(we::UI::Icons::XName, []() {
        if (EditorLayoutController::Get().IsContentBrowserExpanded()) {
            EditorLayoutController::Get().ToggleContentBrowserExpanded();
        }
    });

    auto folderTree = std::make_shared<we::UI::TreeView>();
    folderTree->SetExplorerStyle(true);
    folderTree->SetItemHeight(22.0f);

    auto breadcrumb = std::make_shared<we::UI::Breadcrumb>();
    auto toolbarControls = we::UI::ContentBrowserToolbarControls::Create();

    auto toolbarRow = std::make_shared<we::UI::HorizontalBox>();
    toolbarRow->SetPadding(we::UI::Margin{ 0.0f, 0.0f, 0.0f, 0.0f });
    toolbarRow->SetSpacing(0.0f);
    toolbarRow->AddChild(breadcrumb);

    auto toolbarColumn = std::make_shared<we::UI::VerticalBox>();
    toolbarColumn->SetSpacing(0.0f);
    toolbarColumn->AddChild(toolbarRow);
    toolbarColumn->AddChild(toolbarControls);
    panel->SetToolbar(toolbarColumn);

    auto contentBrowser = std::make_shared<we::UI::ContentBrowser>();
    auto statusBar = std::make_shared<we::UI::ContentBrowserStatusBar>();

    auto centerColumn = std::make_shared<we::UI::VerticalBox>();
    centerColumn->SetSpacing(0.0f);
    centerColumn->AddChild(contentBrowser);
    centerColumn->AddChild(statusBar);

    auto mainSplitter = std::make_shared<we::UI::Splitter>(we::UI::Orientation::Horizontal, 0.20f);
    mainSplitter->SetFirstChild(folderTree);
    mainSplitter->SetSecondChild(centerColumn);
    panel->SetContent(mainSplitter);

    RefreshFolderTree(folderTree);
    WireContentBrowser(contentBrowser, statusBar, breadcrumb);
    NavigateToFolder(ContentBrowserService::Get().GetCurrentFolder(), contentBrowser, breadcrumb, statusBar);

    toolbarControls->GetSearchBox()->SetOnTextChanged([contentBrowser](const std::string& text) {
        ContentBrowserService::Get().GetSearchController().SetQuery(text);
        if (contentBrowser->GetModel()) contentBrowser->GetModel()->NotifyChanged();
    });

    toolbarControls->SetOnViewModeChanged([contentBrowser, toolbarControls](we::UI::ContentViewMode mode) {
        contentBrowser->SetViewMode(mode);
        toolbarControls->SetViewMode(mode);
    });

    toolbarControls->SetOnFilterClicked([contentBrowser]() {
        ContentBrowserService::Get().GetFilterController().ToggleFilter(ContentFilter::Textures);
        if (contentBrowser->GetModel()) contentBrowser->GetModel()->NotifyChanged();
    });

    toolbarControls->SetOnSortClicked([]() {
        // Sort menu placeholder – layout hook for future asset sorting.
    });

    toolbarControls->SetOnImportClicked([]() {
        // Import dialog placeholder – layout hook for future import workflow.
    });

    toolbarControls->SetOnCreateClicked([]() {
        // Create asset menu placeholder – layout hook for future creation workflow.
    });

    folderTree->SetOnSelectionChanged([contentBrowser, breadcrumb, statusBar](const std::vector<std::string>& ids) {
        if (ids.empty()) return;
        const auto* asset = ContentAssetRegistry::Get().FindById(ids.front());
        if (!asset || !asset->isFolder || asset->id.rfind("__", 0) == 0) return;
        NavigateToFolder(asset->virtualPath, contentBrowser, breadcrumb, statusBar);
    });

    folderTree->SetOnItemDoubleClicked([contentBrowser, breadcrumb, statusBar](const std::string& id) {
        const auto* asset = ContentAssetRegistry::Get().FindById(id);
        if (!asset || !asset->isFolder || asset->id.rfind("__", 0) == 0) return;
        NavigateToFolder(asset->virtualPath, contentBrowser, breadcrumb, statusBar);
    });

    breadcrumb->SetOnCrumbClicked([contentBrowser, breadcrumb, statusBar](size_t index) {
        std::string path = "/Game";
        const auto& segments = breadcrumb->GetPath();
        for (size_t i = 1; i <= index && i < segments.size(); ++i) {
            path += "/" + segments[i];
        }
        NavigateToFolder(path, contentBrowser, breadcrumb, statusBar);
    });

    ContentAssetRegistry::Get().SetOnRegistryRefreshed([folderTree, breadcrumb, contentBrowser, statusBar]() {
        RefreshFolderTree(folderTree);
        NavigateToFolder(ContentBrowserService::Get().GetCurrentFolder(), contentBrowser, breadcrumb, statusBar);
    });

    return panel;
}

REGISTER_EDITOR_PANEL(ContentBrowser, CreateContentBrowserPanel)

} // namespace we::programs::editor
