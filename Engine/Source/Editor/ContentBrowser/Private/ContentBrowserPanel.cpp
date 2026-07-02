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
#include "Models/ContentBrowserModel.hpp"
#include <memory>
#include <sstream>

namespace we::programs::editor {

namespace {

using we::editor::contentbrowser::AssetRecord;
using we::editor::contentbrowser::ContentAssetRegistry;
using we::editor::contentbrowser::ContentBrowserService;
using we::editor::contentbrowser::ContentFilter;
using we::UI::ContentViewMode;

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
    
    // Clear selection when navigating to a new folder
    if (browser) {
        browser->ClearSelection();
    }
    
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

    // Panel toolbar with breadcrumb and global actions
    auto panelToolbar = we::UI::ContentBrowserToolbarControls::Create(we::UI::ContentBrowserToolbarControls::ToolbarMode::Full);
    auto breadcrumb = panelToolbar->GetBreadcrumb();
    panel->SetToolbar(panelToolbar);

    // Left sidebar with fixed width (280px) for folder tree
    auto folderTree = std::make_shared<we::UI::TreeView>();
    folderTree->SetExplorerStyle(true);
    folderTree->SetItemHeight(26.0f);
    folderTree->SetIndentWidth(16.0f);
    folderTree->SetShowRowControls(false);

    // Right pane: asset browser with its own toolbar (AssetPane mode)
    auto assetToolbar = we::UI::ContentBrowserToolbarControls::Create(we::UI::ContentBrowserToolbarControls::ToolbarMode::AssetPane);
    auto contentBrowser = std::make_shared<we::UI::ContentBrowser>();
    auto statusBar = std::make_shared<we::UI::ContentBrowserStatusBar>();

    // Build right pane: asset toolbar + content browser + status bar
    auto rightPane = std::make_shared<we::UI::VerticalBox>();
    rightPane->SetSpacing(0.0f);
    rightPane->AddChild(assetToolbar);
    rightPane->AddChild(contentBrowser);
    rightPane->AddChild(statusBar);

    // Split content area into left (folder tree) and right (asset browser)
    auto contentSplitter = std::make_shared<we::UI::Splitter>(we::UI::Orientation::Horizontal, 280.0f);
    contentSplitter->SetFirstChild(folderTree);
    contentSplitter->SetSecondChild(rightPane);
    contentSplitter->SetResizeMode(we::UI::Splitter::ResizeMode::FixedFirst);
    panel->SetContent(contentSplitter);

    RefreshFolderTree(folderTree);
    WireContentBrowser(contentBrowser, statusBar, breadcrumb);
    NavigateToFolder(ContentBrowserService::Get().GetCurrentFolder(), contentBrowser, breadcrumb, statusBar);

    // Wire up asset toolbar (right pane) - search, save, filter
    assetToolbar->GetSearchBox()->SetOnTextChanged([contentBrowser](const std::string& text) {
        ContentBrowserService::Get().GetSearchController().SetQuery(text);
        if (contentBrowser->GetModel()) contentBrowser->GetModel()->NotifyChanged();
    });

    assetToolbar->SetOnSaveClicked([]() {
        // Save all placeholder – layout hook for future save workflow.
    });

    assetToolbar->SetOnFilterClicked([contentBrowser]() {
        ContentBrowserService::Get().GetFilterController().ToggleFilter(ContentFilter::Textures);
        if (contentBrowser->GetModel()) contentBrowser->GetModel()->NotifyChanged();
    });

    // Wire up panel toolbar (top) - prev/next, create, import, sort, settings
    panelToolbar->SetOnPreviousClicked([breadcrumb, contentBrowser, statusBar]() {
        // Navigate to previous folder in history
        // Placeholder for navigation history implementation
    });

    panelToolbar->SetOnNextClicked([breadcrumb, contentBrowser, statusBar]() {
        // Navigate to next folder in history
        // Placeholder for navigation history implementation
    });

    panelToolbar->SetOnCreateClicked([]() {
        // Create asset menu placeholder – layout hook for future creation workflow.
    });

    panelToolbar->SetOnImportClicked([]() {
        // Import dialog placeholder – layout hook for future import workflow.
    });

    panelToolbar->SetOnSortClicked([]() {
        // Sort menu placeholder – layout hook for future asset sorting.
    });

    panelToolbar->SetOnSettingsClicked([]() {
        // Settings menu placeholder – layout hook for future settings workflow.
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
