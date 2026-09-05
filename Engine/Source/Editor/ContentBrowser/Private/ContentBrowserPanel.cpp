#include "WindEffects/Editor/EditorSDK.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "ContentBrowser/ContentBrowserApi.h"
#include "ContentBrowser/ContentBrowserRuntime.h"
#include "ContentBrowser/ContentBrowserSession.h"
#include "KindUI/Rendering/FontImportService.h"
#include "ContentBrowser/Widgets/ContentBrowser.h"
#include "ContentBrowser/Widgets/ContentBrowserToolbar.h"
#include "ContentBrowser/Widgets/SearchBox.h"
#include "ContentBrowser/Widgets/TreeView.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/Splitter.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "Core/Localization.h"
#include "Core/Paths.h"
#include "Services/ContentBrowserService.h"
#include "Registry/ContentAssetRegistry.h"
#include "Controllers/FilterController.h"
#include "ContentBrowser/Models/ContentBrowserModel.h"
#include <filesystem>
#include <memory>
#include <sstream>

namespace we::programs::editor {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

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
    we::runtime::kindui::WindIconRef icon, bool expanded = false)
{
    auto node = std::make_shared<::we::editor::contentbrowser::TreeNode>();
    node->id = id;
    node->label = label;
    node->icon = icon;
    node->expanded = expanded;
    return node;
}

std::shared_ptr<::we::editor::contentbrowser::TreeNode> BuildFolderNode(const AssetRecord* folder) {
    auto node = std::make_shared<::we::editor::contentbrowser::TreeNode>();
    node->id = folder->id;
    node->label = folder->name;
    node->expanded = folder->virtualPath == "/Game";
    node->icon = node->expanded ? WindIcons::FolderOpen16 : WindIcons::Folder16;

    for (const auto* child : ContentAssetRegistry::Get().GetChildren(folder->virtualPath)) {
        if (child->isFolder) node->children.push_back(BuildFolderNode(child));
    }
    return node;
}

void RefreshFolderTree(const std::shared_ptr<::we::editor::contentbrowser::TreeView>& tree) {
    auto root = std::make_shared<::we::editor::contentbrowser::TreeNode>();
    root->id = "root";
    root->label = "Content";
    root->expanded = true;

    root->children.push_back(MakeSection("__favorites__", "Favorites", WindIcons::Star16));
    root->children.push_back(MakeSection("__collections__", "Collections", WindIcons::Layers16));
    root->children.push_back(MakeSection("__plugins__", "Plugins", WindIcons::Plugin16));
    root->children.push_back(MakeSection("__engine__", "Engine Content", WindIcons::Globe16, false));

    auto project = MakeSection("__project__", "Project Content", WindIcons::Folder16, true);
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
    const std::shared_ptr<::we::editor::contentbrowser::Breadcrumb>& breadcrumb)
{
    ContentBrowserService::Get().SetCurrentFolder(virtualPath);
    if (breadcrumb) {
        UpdateBreadcrumb(breadcrumb, virtualPath);
    }
    
    // Clear selection when navigating to a new folder
    if (browser) {
        browser->ClearSelection();
    }
}

void WireContentBrowser(
    const std::shared_ptr<::we::editor::contentbrowser::ContentBrowser>& browser,
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
    browser->SetOnItemDoubleClicked([&service, browser, breadcrumb](const ::we::editor::contentbrowser::ContentItem& item) {
        if (item.isFolder) NavigateToFolder(item.path, browser, breadcrumb);
    });
    service.SetOnThumbnailReady([browser](const std::string& id, we::rhi::RHIDescriptorSetHandle texture) {
        if (browser->GetController()) browser->GetController()->UpdateItemIcon(id, texture);
    });
}

} // namespace

void InitializeContentBrowserService(
    we::runtime::kindui::IconRenderer* iconRenderer,
    const std::string& contentRoot) {
    // Compatibility entry — prefer CreateContentBrowserRuntime + ContentBrowserSession.
    if (::we::editor::contentbrowser::ContentBrowserSession::IsInstalled()) {
        return;
    }
    ::we::editor::contentbrowser::ContentBrowserDependencies deps;
    deps.iconRenderer = iconRenderer;
    deps.contentRoot = contentRoot;
    auto runtime = ::we::editor::contentbrowser::CreateContentBrowserRuntime(deps);
    ::we::editor::contentbrowser::ContentBrowserSession::Install(
        std::shared_ptr<::we::editor::contentbrowser::IContentBrowserRuntime>(std::move(runtime)));
}

void ShutdownContentBrowserService() {
    if (auto* runtime = ::we::editor::contentbrowser::ContentBrowserSession::Runtime()) {
        runtime->Shutdown();
    }
    ::we::editor::contentbrowser::ContentBrowserSession::Clear();
    ContentBrowserService::Get().Shutdown();
}

std::shared_ptr<::we::editor::panels::Panel> CreateContentBrowserPanel() {
    auto title = we::core::Localization::Get().GetString("Panel_ContentBrowser", "Content Browser");

    auto folderTree = std::make_shared<::we::editor::contentbrowser::TreeView>();
    folderTree->SetExplorerStyle(false);
    folderTree->SetPaintNavigationBackground(false);
    folderTree->SetShowColumnHeader(false);
    folderTree->SetItemHeight(we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::ListRowHeight));
    folderTree->SetIndentWidth(we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::TreeIndentWidth));
    folderTree->SetShowRowControls(false);

    // Right pane: vertical column with toolbar on top and asset grid below
    auto rightPane = std::make_shared<we::runtime::kindui::Column>();
    rightPane->SetFlexGrow(1.0f);
    rightPane->SetFlexShrink(1.0f);

    auto assetToolbar = ::we::editor::contentbrowser::ContentBrowserToolbarControls::Create(::we::editor::contentbrowser::ContentBrowserToolbarControls::ToolbarMode::AssetPane);
    auto contentBrowser = std::make_shared<::we::editor::contentbrowser::ContentBrowser>();

    assetToolbar->SetFlexShrink(0.0f);
    contentBrowser->SetFlexGrow(1.0f);
    contentBrowser->SetFlexShrink(1.0f);

    rightPane->AddChild(assetToolbar);
    rightPane->AddChild(contentBrowser);

    // Split content area into left (folder tree sidebar) and right (content area with toolbar).
    const float treePaneWidth = std::max(200.0f * we::runtime::kindui::DPIContext::GetScale(),
        we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::PropertyLabelColumnWidth) * 2.0f);
    auto contentSplitter = std::make_shared<we::runtime::kindui::Splitter>(we::runtime::kindui::Orientation::Horizontal, treePaneWidth);
    contentSplitter->SetFirstChild(folderTree);
    contentSplitter->SetSecondChild(rightPane);
    contentSplitter->SetResizeMode(we::runtime::kindui::Splitter::ResizeMode::FixedFirst);
    contentSplitter->SetFixedFirstWidth(treePaneWidth);
    contentSplitter->SetFlexGrow(1.0f);
    contentSplitter->SetFlexShrink(1.0f);

    auto panel = PanelBuilder(title)
        .TabIcon(WindIcons::FolderSearch16)
        .WithCloseButton([]() {
            if (EditorWorkspaceController::Get().IsContentBrowserExpanded()) {
                EditorWorkspaceController::Get().ToggleContentBrowserExpanded();
            }
        })
        .Content(contentSplitter);

    RefreshFolderTree(folderTree);
    WireContentBrowser(contentBrowser, nullptr);
    NavigateToFolder(ContentBrowserService::Get().GetCurrentFolder(), contentBrowser, nullptr);

    // Wire up asset toolbar - create, import, search, save, filter
    assetToolbar->SetOnCreateClicked([]() {
        // Add asset menu placeholder – layout hook for future creation workflow.
    });

    assetToolbar->SetOnImportClicked([]() {
        auto& paths = we::core::PathService::Get();
        const auto inputCandidates = paths.FontCandidates("Roboto-Regular.ttf");
        const auto inputFont = we::core::PathService::FindExisting(inputCandidates);
        if (!inputFont) {
            return;
        }
        const auto outputDir = inputFont->parent_path();
        (void)we::runtime::kindui::FontImportService::ImportFontFile(*inputFont, outputDir, 18.0f);
    });

    assetToolbar->GetSearchBox()->SetOnTextChanged([contentBrowser](const std::string& text) {
        ContentBrowserService::Get().GetSearchController().SetQuery(text);
        if (contentBrowser->GetModel()) contentBrowser->GetModel()->NotifyChanged();
    });

    assetToolbar->SetOnSaveClicked([]() {
        // Save all placeholder – layout hook for future save workflow.
    });

    assetToolbar->SetOnFabClicked([]() {
        // Fab 3D marketplace placeholder – layout hook for future library workflow.
    });

    assetToolbar->SetOnFilterClicked([contentBrowser]() {
        ContentBrowserService::Get().GetFilterController().ToggleFilter(ContentFilter::Textures);
        if (contentBrowser->GetModel()) contentBrowser->GetModel()->NotifyChanged();
    });

    folderTree->SetOnSelectionChanged([contentBrowser](const std::vector<std::string>& ids) {
        if (ids.empty()) return;
        const std::string& id = ids.front();
        if (id == "__project__") {
            NavigateToFolder("/Game", contentBrowser, nullptr);
            return;
        }
        const auto* asset = ContentAssetRegistry::Get().FindById(id);
        if (!asset || !asset->isFolder || asset->id.rfind("__", 0) == 0) return;
        NavigateToFolder(asset->virtualPath, contentBrowser, nullptr);
    });

    folderTree->SetOnItemDoubleClicked([contentBrowser](const std::string& id) {
        if (id == "__project__") {
            NavigateToFolder("/Game", contentBrowser, nullptr);
            return;
        }
        const auto* asset = ContentAssetRegistry::Get().FindById(id);
        if (!asset || !asset->isFolder || asset->id.rfind("__", 0) == 0) return;
        NavigateToFolder(asset->virtualPath, contentBrowser, nullptr);
    });

    ContentAssetRegistry::Get().SetOnRegistryRefreshed([folderTree, contentBrowser]() {
        RefreshFolderTree(folderTree);
        NavigateToFolder(ContentBrowserService::Get().GetCurrentFolder(), contentBrowser, nullptr);
    });

    return panel;
}

REGISTER_UI_PANEL(ContentBrowser,
    WE_PANEL(ContentBrowser).Title("Content Browser").Icon("content-browser").Zone(DockZone::Bottom).WindowMenu("Content Browser").SortOrder(4),
    CreateContentBrowserPanel)

} // namespace we::programs::editor
