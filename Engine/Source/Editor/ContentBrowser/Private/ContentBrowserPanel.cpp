#include "WindEffects/Editor/EditorSDK.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "ContentBrowserApi.h"
#include "KindUI/Rendering/FontImportService.h"
#include "Widgets/ContentBrowser.h"
#include "Widgets/ContentBrowserToolbar.h"
#include "Widgets/SearchBox.h"
#include "Widgets/TreeView.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/Splitter.h"
#include "KindUI/Core/Icon.h"
#include "Core/Localization.h"
#include "Services/ContentBrowserService.h"
#include "Registry/ContentAssetRegistry.h"
#include "Controllers/FilterController.h"
#include "Models/ContentBrowserModel.h"
#include <filesystem>
#include <memory>
#include <sstream>

namespace we::programs::editor {
namespace Icons = ::we::runtime::kindui::Icons;

using namespace ::we::runtime::kindui;
using namespace ::we::runtime::kindui;
using ::we::editor::panels::Panel;
using ::we::editor::panels::PanelBuilder;
using ::we::editor::docking::DockZone;

namespace {

using ::we::editor::contentbrowser::AssetRecord;
using ::we::editor::contentbrowser::ContentAssetRegistry;
using ::we::editor::contentbrowser::ContentBrowserService;
using ::we::editor::contentbrowser::ContentFilter;
using ::we::editor::contentbrowser::ContentViewMode;

std::shared_ptr<::we::editor::contentbrowser::TreeNode> MakeSection(const std::string& id, const std::string& label,
    const std::string& icon, bool expanded = false)
{
    auto node = std::make_shared<::we::editor::contentbrowser::TreeNode>();
    node->id = id;
    node->label = label;
    node->iconName = icon;
    node->expanded = expanded;
    return node;
}

std::shared_ptr<::we::editor::contentbrowser::TreeNode> BuildFolderNode(const AssetRecord* folder) {
    auto node = std::make_shared<::we::editor::contentbrowser::TreeNode>();
    node->id = folder->id;
    node->label = folder->name;
    node->iconName = we::runtime::kindui::Icons::FolderName;
    node->expanded = folder->virtualPath == "/Game";

    for (const auto* child : ContentAssetRegistry::Get().GetChildren(folder->virtualPath)) {
        if (child->isFolder) node->children.push_back(BuildFolderNode(child));
    }
    return node;
}

void RefreshFolderTree(const std::shared_ptr<::we::editor::contentbrowser::TreeView>& tree) {
    auto root = std::make_shared<::we::editor::contentbrowser::TreeNode>();
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

void UpdateBreadcrumb(const std::shared_ptr<::we::editor::contentbrowser::Breadcrumb>& breadcrumb, const std::string& virtualPath) {
    std::vector<std::string> crumbs;
    if (virtualPath.size() <= 6) {
        crumbs.push_back("All");
        breadcrumb->SetPath(crumbs);
        return;
    }
    std::string remainder = virtualPath.substr(6);
    std::stringstream ss(remainder);
    std::string segment;
    while (std::getline(ss, segment, '/')) {
        if (!segment.empty()) crumbs.push_back(segment);
    }
    crumbs.push_back("All");
    breadcrumb->SetPath(crumbs);
}

void NavigateToFolder(const std::string& virtualPath,
    const std::shared_ptr<::we::editor::contentbrowser::ContentBrowser>& browser,
    const std::shared_ptr<::we::editor::contentbrowser::Breadcrumb>& breadcrumb,
    const std::shared_ptr<::we::editor::contentbrowser::ContentBrowserStatusBar>& statusBar)
{
    ContentBrowserService::Get().SetCurrentFolder(virtualPath);
    if (breadcrumb) {
        UpdateBreadcrumb(breadcrumb, virtualPath);
    }
    
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
    const std::shared_ptr<::we::editor::contentbrowser::ContentBrowser>& browser,
    const std::shared_ptr<::we::editor::contentbrowser::ContentBrowserStatusBar>& statusBar,
    const std::shared_ptr<::we::editor::contentbrowser::Breadcrumb>& breadcrumb)
{
    auto& service = ContentBrowserService::Get();
    service.RefreshBrowserModel(browser->GetModel());

    browser->SetOnItemNeedsThumbnail([&service](const std::string& id) {
        service.RequestThumbnailForItem(id);
    });
    browser->SetOnVisibleItemsChanged([&service](const std::unordered_set<std::string>& ids) {
        service.SetVisibleItemIds(ids);
    });
    browser->SetOnItemDoubleClicked([&service, browser, statusBar, breadcrumb](const ::we::editor::contentbrowser::ContentItem& item) {
        if (item.isFolder) NavigateToFolder(item.path, browser, breadcrumb, statusBar);
    });
    browser->SetOnItemSelected([browser, statusBar](const ::we::editor::contentbrowser::ContentItem&) {
        if (browser->GetModel() && statusBar) {
            statusBar->SetSelectedCount(browser->GetModel()->selectedIds.size());
        }
    });
    service.SetOnThumbnailReady([browser](const std::string& id, we::rhi::RHIDescriptorSetHandle texture) {
        if (browser->GetController()) browser->GetController()->UpdateItemIcon(id, texture);
    });
}

} // namespace

void InitializeContentBrowserService(we::runtime::kindui::IconRenderer* iconRenderer) {
    ContentBrowserService::Get().Initialize(iconRenderer, "Content");
}

void ShutdownContentBrowserService() {
    ContentBrowserService::Get().Shutdown();
}

std::shared_ptr<::we::editor::panels::Panel> CreateContentBrowserPanel() {
    auto title = we::core::Localization::Get().GetString("Panel_ContentBrowser", "Content Browser");

    auto panelToolbar = ::we::editor::contentbrowser::ContentBrowserToolbarControls::Create(
        ::we::editor::contentbrowser::ContentBrowserToolbarControls::ToolbarMode::Full);

    auto folderTree = std::make_shared<::we::editor::contentbrowser::TreeView>();
    folderTree->SetExplorerStyle(true);
    folderTree->SetItemHeight(26.0f);
    folderTree->SetIndentWidth(16.0f);
    folderTree->SetShowRowControls(false);

    // Right pane: asset browser with its own toolbar (AssetPane mode)
    auto assetToolbar = ::we::editor::contentbrowser::ContentBrowserToolbarControls::Create(::we::editor::contentbrowser::ContentBrowserToolbarControls::ToolbarMode::AssetPane);
    auto contentBrowser = std::make_shared<::we::editor::contentbrowser::ContentBrowser>();
    auto statusBar = std::make_shared<::we::editor::contentbrowser::ContentBrowserStatusBar>();

    // Build right pane: asset toolbar + content browser + status bar
    auto rightPane = std::make_shared<we::runtime::kindui::Column>();
    rightPane->Gap(0.0f);
    rightPane->AddChild(assetToolbar);
    rightPane->AddChild(contentBrowser);
    rightPane->AddChild(statusBar);

    // Split content area into left (folder tree) and right (asset browser)
    auto contentSplitter = std::make_shared<we::runtime::kindui::Splitter>(we::runtime::kindui::Orientation::Horizontal, 280.0f);
    contentSplitter->SetFirstChild(folderTree);
    contentSplitter->SetSecondChild(rightPane);
    contentSplitter->SetResizeMode(we::runtime::kindui::Splitter::ResizeMode::FixedFirst);

    auto panel = PanelBuilder(title)
        .TabIcon(Icons::ContentBrowserName)
        .WithCloseButton([]() {
            if (EditorWorkspaceController::Get().IsContentBrowserExpanded()) {
                EditorWorkspaceController::Get().ToggleContentBrowserExpanded();
            }
        })
        .Toolbar(panelToolbar)
        .Content(contentSplitter);

    RefreshFolderTree(folderTree);
    WireContentBrowser(contentBrowser, statusBar, nullptr);
    NavigateToFolder(ContentBrowserService::Get().GetCurrentFolder(), contentBrowser, nullptr, statusBar);

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

    // Wire up panel toolbar (top) - create, import, back, forward, folder
    panelToolbar->SetOnCreateClicked([]() {
        // Add asset menu placeholder – layout hook for future creation workflow.
    });

    panelToolbar->SetOnSaveClicked([]() {
        // Save all placeholder – layout hook for future save workflow.
    });

    panelToolbar->SetOnImportClicked([]() {
        const std::filesystem::path inputFont = "Assets/Fonts/Inter-Regular.ttf";
        const std::filesystem::path outputDir = "Assets/Fonts";
        if (std::filesystem::exists(inputFont)) {
            (void)we::runtime::kindui::FontImportService::ImportFontFile(inputFont, outputDir, 18.0f);
        }
    });

    folderTree->SetOnSelectionChanged([contentBrowser, statusBar](const std::vector<std::string>& ids) {
        if (ids.empty()) return;
        const auto* asset = ContentAssetRegistry::Get().FindById(ids.front());
        if (!asset || !asset->isFolder || asset->id.rfind("__", 0) == 0) return;
        NavigateToFolder(asset->virtualPath, contentBrowser, nullptr, statusBar);
    });

    folderTree->SetOnItemDoubleClicked([contentBrowser, statusBar](const std::string& id) {
        const auto* asset = ContentAssetRegistry::Get().FindById(id);
        if (!asset || !asset->isFolder || asset->id.rfind("__", 0) == 0) return;
        NavigateToFolder(asset->virtualPath, contentBrowser, nullptr, statusBar);
    });

    ContentAssetRegistry::Get().SetOnRegistryRefreshed([folderTree, contentBrowser, statusBar]() {
        RefreshFolderTree(folderTree);
        NavigateToFolder(ContentBrowserService::Get().GetCurrentFolder(), contentBrowser, nullptr, statusBar);
    });

    return panel;
}

REGISTER_UI_PANEL(ContentBrowser,
    WE_PANEL(ContentBrowser).Title("Content Browser").Icon("content-browser").Zone(DockZone::Bottom).WindowMenu("Content Browser").SortOrder(4),
    CreateContentBrowserPanel)

} // namespace we::programs::editor
