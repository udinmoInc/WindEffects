// ==============================================================================
// WindEffects — ContentBrowser — ContentBrowserPanel
// Internal implementation for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/EditorSDK.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "ContentBrowser/ContentBrowserApi.h"
#include "ContentBrowser/ContentBrowserRuntime.h"
#include "ContentBrowser/ContentBrowserSession.h"
#include "KindUI/Rendering/FontImportService.h"
#include "ContentBrowser/Widgets/ContentBrowser.h"
#include "ContentBrowser/Widgets/ContentBrowserToolbar.h"
#include "ContentBrowser/Widgets/TreeView.h"
#include <KindUI/EditorUI.h>
#include "Core/Localization.h"
#include "Core/Paths.h"
#include "Services/ContentBrowserService.h"
#include "Registry/ContentAssetRegistry.h"
#include "Controllers/FilterController.h"
#include "ContentBrowser/Models/ContentBrowserModel.h"
#include <filesystem>
#include <memory>
#include <sstream>

#include <unordered_set>

namespace we::programs::editor {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

using namespace ::we::runtime::kindui;
using ::we::runtime::kindui::panels::Panel;
using ::we::runtime::kindui::panels::PanelBuilder;
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

void CollectExpandedNodes(const std::shared_ptr<::we::editor::contentbrowser::TreeNode>& node,
    std::unordered_set<std::string>& outExpanded) {
    if (!node) return;
    if (node->expanded) {
        outExpanded.insert(node->id);
    }
    for (const auto& child : node->children) {
        CollectExpandedNodes(child, outExpanded);
    }
}

std::shared_ptr<::we::editor::contentbrowser::TreeNode> BuildFolderNode(
    const AssetRecord* folder,
    const std::unordered_set<std::string>& expandedPaths,
    const std::string& currentFolder)
{
    auto node = std::make_shared<::we::editor::contentbrowser::TreeNode>();
    node->id = folder->id;
    node->label = folder->name;
    const bool isCurrentFolderOrParent = (currentFolder == folder->virtualPath) ||
        (currentFolder.rfind(folder->virtualPath + "/", 0) == 0);
    const bool wasExpanded = expandedPaths.find(folder->virtualPath) != expandedPaths.end();
    node->expanded = (folder->virtualPath == "/Game") || isCurrentFolderOrParent || wasExpanded;
    node->icon = node->expanded ? WindIcons::FolderOpenMask16 : WindIcons::FolderMask16;

    for (const auto* child : ContentAssetRegistry::Get().GetChildren(folder->virtualPath)) {
        if (child->isFolder) node->children.push_back(BuildFolderNode(child, expandedPaths, currentFolder));
    }
    return node;
}

void SyncFolderTreeSelection(const std::shared_ptr<::we::editor::contentbrowser::TreeView>& tree, const std::string&
    virtualPath) {
    if (!tree) return;

    std::function<bool(const std::shared_ptr<::we::editor::contentbrowser::TreeNode>&)> expandPath =
        [&](const std::shared_ptr<::we::editor::contentbrowser::TreeNode>& node) -> bool {
            if (!node) return false;
            if (node->id == virtualPath) return true;
            for (const auto& child : node->children) {
                if (expandPath(child)) {
                    node->expanded = true;
                    node->icon = WindIcons::FolderOpenMask16;
                    return true;
                }
            }
            return false;
        };

    if (auto root = tree->GetRoot()) {
        expandPath(root);
    }
    tree->SetSelectedId(virtualPath);
    tree->RefreshLayout();
}

void RefreshFolderTree(const std::shared_ptr<::we::editor::contentbrowser::TreeView>& tree) {
    if (!tree) return;
    std::unordered_set<std::string> expanded;
    if (tree->GetRoot()) {
        CollectExpandedNodes(tree->GetRoot(), expanded);
    }
    const std::string currentFolder = ContentBrowserService::Get().GetCurrentFolder();
    if (const auto* game = ContentAssetRegistry::Get().FindByVirtualPath("/Game")) {
        auto root = BuildFolderNode(game, expanded, currentFolder);
        root->label = "Content";
        root->expanded = true;
        tree->SetRoot(root);
    } else {
        auto root = std::make_shared<::we::editor::contentbrowser::TreeNode>();
        root->id = "/Game";
        root->label = "Content";
        root->expanded = true;
        root->icon = WindIcons::FolderOpenMask16;
        tree->SetRoot(root);
    }
    const std::string selectedId = (ContentAssetRegistry::Get().FindByVirtualPath(currentFolder) != nullptr)
        ? currentFolder : "/Game";
    SyncFolderTreeSelection(tree, selectedId);
}

void UpdateBreadcrumb(const std::shared_ptr<::we::runtime::kindui::Breadcrumb>& breadcrumb, const std::string&
    virtualPath) {
    if (!breadcrumb) return;
    std::vector<std::string> crumbs;
    crumbs.push_back("All");
    if (virtualPath.rfind("/Game", 0) == 0) {
        crumbs.push_back("Content");
        std::string remainder = virtualPath.substr(5);
        std::stringstream ss(remainder);
        std::string segment;
        while (std::getline(ss, segment, '/')) {
            if (!segment.empty()) crumbs.push_back(segment);
        }
    } else if (virtualPath.rfind("/Engine", 0) == 0) {
        crumbs.push_back("Engine");
        std::string remainder = virtualPath.substr(7);
        std::stringstream ss(remainder);
        std::string segment;
        while (std::getline(ss, segment, '/')) {
            if (!segment.empty()) crumbs.push_back(segment);
        }
    } else if (virtualPath.rfind("/Plugins", 0) == 0) {
        crumbs.push_back("Plugins");
        std::string remainder = virtualPath.substr(8);
        std::stringstream ss(remainder);
        std::string segment;
        while (std::getline(ss, segment, '/')) {
            if (!segment.empty()) crumbs.push_back(segment);
        }
    } else {
        std::stringstream ss(virtualPath);
        std::string segment;
        while (std::getline(ss, segment, '/')) {
            if (!segment.empty()) crumbs.push_back(segment);
        }
    }
    breadcrumb->SetPath(crumbs);
}

std::string GetPathFromCrumbIndex(const std::vector<std::string>& crumbs, size_t index) {
    if (index == 0 || index == 1) {
        return "/Game";
    }
    std::string path = "/Game";
    for (size_t i = 2; i <= index && i < crumbs.size(); ++i) {
        path += "/" + crumbs[i];
    }
    return path;
}

void NavigateToFolder(const std::string& virtualPath,
    const std::shared_ptr<::we::editor::contentbrowser::ContentBrowser>& browser,
    const std::shared_ptr<::we::runtime::kindui::Breadcrumb>& breadcrumb,
    const std::shared_ptr<::we::runtime::kindui::SearchBoxControl>& searchBox = nullptr)
{
    ContentBrowserService::Get().SetCurrentFolder(virtualPath);
    if (breadcrumb) {
        UpdateBreadcrumb(breadcrumb, virtualPath);
    }
    if (searchBox) {
        std::string folderName = "Assets";
        size_t lastSlash = virtualPath.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash + 1 < virtualPath.size()) {
            folderName = virtualPath.substr(lastSlash + 1);
        } else if (virtualPath == "/Game") {
            folderName = "Content";
        }
        searchBox->SetPlaceholder("Search " + folderName + "...");
    }
    if (browser) {
        browser->ClearSelection();
    }
}

void WireContentBrowser(
    const std::shared_ptr<::we::editor::contentbrowser::ContentBrowser>& browser,
    const std::shared_ptr<::we::runtime::kindui::Breadcrumb>& breadcrumb,
    const std::shared_ptr<::we::runtime::kindui::SearchBoxControl>& searchBox,
    std::function<void(const std::string&, bool)> onNavigateFolder)
{
    auto& service = ContentBrowserService::Get();
    service.RefreshBrowserModel(browser->GetModel());

    browser->SetOnItemNeedsThumbnail([&service](const std::string& id) {
        service.RequestThumbnailForItem(id);
    });
    browser->SetOnVisibleItemsChanged([&service](const std::unordered_set<std::string>& ids) {
        service.SetVisibleItemIds(ids);
    });
    browser->SetOnItemDoubleClicked([onNavigateFolder](const ::we::editor::contentbrowser::ContentItem& item) {
        if (item.isFolder && onNavigateFolder) onNavigateFolder(item.path, true);
    });
    service.SetOnThumbnailReady([browser](const std::string& id, we::rhi::RHIDescriptorSetHandle texture) {
        if (browser) browser->UpdateItemIcon(id, texture);
    });
}

}

void InitializeContentBrowserService(
    we::runtime::kindui::IconRenderer* iconRenderer,
    const std::string& contentRoot) {
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

std::shared_ptr<::we::runtime::kindui::panels::Panel> CreateContentBrowserPanel() {
    auto title = we::core::Localization::Get().GetString("Panel_ContentBrowser", "Asset Explorer");

    auto folderTree = std::make_shared<::we::editor::contentbrowser::TreeView>();
    folderTree->SetExplorerStyle(false);
    folderTree->SetPaintNavigationBackground(false);
    folderTree->SetShowColumnHeader(false);
    folderTree->SetItemHeight(we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::ListRowHeight));
    folderTree->SetIndentWidth(we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::TreeIndentWidth));
    folderTree->SetShowRowControls(false);

    auto mainColumn = std::make_shared<we::runtime::kindui::Column>();
    mainColumn->SetFlexGrow(1.0f);
    mainColumn->SetFlexShrink(1.0f);

    auto assetToolbar = ::we::editor::contentbrowser::ContentBrowserToolbarControls::Create(::we::editor::contentbrowser::ContentBrowserToolbarControls::ToolbarMode::Full);
    auto contentBrowser = std::make_shared<::we::editor::contentbrowser::ContentBrowser>();

    assetToolbar->SetFlexShrink(0.0f);
    contentBrowser->SetFlexGrow(1.0f);
    contentBrowser->SetFlexShrink(1.0f);
    folderTree->SetFlexGrow(1.0f);
    folderTree->SetFlexShrink(1.0f);

    auto rightPane = std::make_shared<we::runtime::kindui::Column>();
    rightPane->SetFlexGrow(1.0f);
    rightPane->SetFlexShrink(1.0f);

    auto searchRow = std::make_shared<we::runtime::kindui::Row>();
    searchRow->Background(we::runtime::kindui::Hex("#151515"));
    searchRow->Padding(Margin{ 6.0f, 3.0f, 6.0f, 3.0f });
    searchRow->Gap(4.0f);
    searchRow->Align(AlignItems::Center);
    searchRow->SetFlexShrink(0.0f);

    auto filterBtn = std::make_shared<we::runtime::kindui::ToolbarIconButton>(WindIcons::ListFilter16, "Filter");
    filterBtn->SetFlexShrink(0.0f);

    auto searchBox = std::make_shared<::we::runtime::kindui::SearchBoxControl>();
    searchBox->SetPlaceholder("Search Assets...");
    searchBox->SetToolbarInset(true);
    searchBox->SetFillWidth(false);
    const float uiScale = (std::max)(1.0f, we::runtime::kindui::DPIContext::GetScale());
    searchBox->SetWidth(450.0f * uiScale);
    searchBox->SetFlexGrow(0.0f);
    searchBox->SetFlexShrink(0.0f);

    searchRow->AddChild(filterBtn);
    searchRow->AddChild(searchBox);

    rightPane->AddChild(searchRow);
    rightPane->AddChild(contentBrowser);

    const float treePaneWidth = std::max(200.0f * we::runtime::kindui::DPIContext::GetScale(),
        we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::PropertyLabelColumnWidth) * 2.0f);
    auto contentSplitter =
        std::make_shared<we::runtime::kindui::Splitter>(we::runtime::kindui::Orientation::Horizontal, treePaneWidth);
    contentSplitter->SetFirstChild(folderTree);
    contentSplitter->SetSecondChild(rightPane);
    contentSplitter->SetResizeMode(we::runtime::kindui::Splitter::ResizeMode::FixedFirst);
    contentSplitter->SetFixedFirstWidth(treePaneWidth);
    contentSplitter->SetFlexGrow(1.0f);
    contentSplitter->SetFlexShrink(1.0f);

    mainColumn->AddChild(assetToolbar);
    mainColumn->AddChild(contentSplitter);

    assetToolbar->SetOnExpandAllClicked([folderTree]() {
        folderTree->ExpandAll();
    });
    assetToolbar->SetOnCollapseAllClicked([folderTree]() {
        folderTree->CollapseAll();
    });

    auto panel = we::editor::dsl::Panel(std::string(title), [&](we::editor::dsl::PanelContext& p) {
        p.TabIcon(WindIcons::FolderSearch16)
         .WithCloseButton([]() {
             EditorWorkspaceController::Get().SetPanelVisible("ContentBrowser", false);
         })
         .Content(mainColumn);
    });

    auto breadcrumb = assetToolbar->GetBreadcrumb();

    auto history = std::make_shared<std::vector<std::string>>();
    auto historyIndex = std::make_shared<int>(-1);

    auto updateNavButtons = [assetToolbar, history, historyIndex]() {
        if (!assetToolbar) return;
        if (auto backBtn = assetToolbar->GetBackBtn()) {
            backBtn->SetEnabled(*historyIndex > 0);
        }
        if (auto forwardBtn = assetToolbar->GetForwardBtn()) {
            forwardBtn->SetEnabled(*historyIndex + 1 < static_cast<int>(history->size()));
        }
    };

    auto pushHistory = [history, historyIndex](const std::string& path) {
        if (*historyIndex >= 0 && *historyIndex < static_cast<int>(history->size()) && (*history)[*historyIndex] ==
            path) {
            return;
        }
        if (*historyIndex + 1 < static_cast<int>(history->size())) {
            history->erase(history->begin() + *historyIndex + 1, history->end());
        }
        history->push_back(path);
        *historyIndex = static_cast<int>(history->size()) - 1;
    };

    auto doNavigate = [contentBrowser, folderTree, breadcrumb, searchBox, pushHistory,
        updateNavButtons](const std::string& path,
        bool recordHistory = true) {
        NavigateToFolder(path, contentBrowser, breadcrumb, searchBox);
        SyncFolderTreeSelection(folderTree, path);
        if (recordHistory) {
            pushHistory(path);
        }
        updateNavButtons();
    };

    RefreshFolderTree(folderTree);
    WireContentBrowser(contentBrowser, breadcrumb, searchBox, doNavigate);
    doNavigate(ContentBrowserService::Get().GetCurrentFolder(), true);
    assetToolbar->SetOnPreviousClicked([history, historyIndex, doNavigate, updateNavButtons]() {
        if (*historyIndex > 0) {
            --(*historyIndex);
            doNavigate((*history)[*historyIndex], false);
            updateNavButtons();
        }
    });

    assetToolbar->SetOnNextClicked([history, historyIndex, doNavigate, updateNavButtons]() {
        if (*historyIndex + 1 < static_cast<int>(history->size())) {
            ++(*historyIndex);
            doNavigate((*history)[*historyIndex], false);
            updateNavButtons();
        }
    });

    if (breadcrumb) {
        breadcrumb->SetOnCrumbClicked([breadcrumb, doNavigate](size_t index) {
            const auto& crumbs = breadcrumb->GetPath();
            std::string targetPath = GetPathFromCrumbIndex(crumbs, index);
            doNavigate(targetPath, true);
        });
    }

    assetToolbar->SetOnViewModeChanged([contentBrowser](ContentViewMode mode) {
        if (contentBrowser) {
            contentBrowser->SetViewMode(mode);
        }
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

    searchBox->SetOnTextChanged([contentBrowser](const std::string& text) {
        ContentBrowserService::Get().GetSearchController().SetQuery(text);
        if (contentBrowser->GetModel()) contentBrowser->GetModel()->NotifyChanged();
    });

    assetToolbar->SetOnSaveClicked([]() {
    });

    assetToolbar->SetOnFabClicked([]() {
    });

    folderTree->SetOnSelectionChanged([doNavigate](const std::vector<std::string>& ids) {
        if (ids.empty()) return;
        const std::string& id = ids.front();
        if (id == "__project__" || id == "/Game") {
            doNavigate("/Game", true);
            return;
        }
        const auto* asset = ContentAssetRegistry::Get().FindById(id);
        if (asset && asset->isFolder && asset->id.rfind("__", 0) != 0) {
            doNavigate(asset->virtualPath, true);
        } else if (!id.empty() && id.rfind("__", 0) != 0) {
            doNavigate(id, true);
        }
    });

    folderTree->SetOnItemDoubleClicked([doNavigate](const std::string& id) {
        if (id.empty()) return;
        if (id == "__project__" || id == "/Game") {
            doNavigate("/Game", true);
            return;
        }
        const auto* asset = ContentAssetRegistry::Get().FindById(id);
        if (asset && asset->isFolder && asset->id.rfind("__", 0) != 0) {
            doNavigate(asset->virtualPath, true);
        } else if (!id.empty() && id.rfind("__", 0) != 0) {
            doNavigate(id, true);
        }
    });

    ContentAssetRegistry::Get().SetOnRegistryRefreshed([folderTree, doNavigate]() {
        RefreshFolderTree(folderTree);
        doNavigate(ContentBrowserService::Get().GetCurrentFolder(), false);
    });

    return panel;
}

REGISTER_UI_PANEL(ContentBrowser,
    WE_PANEL(ContentBrowser).Title("Asset Explorer").Icon("content-browser").Zone(DockZone::Bottom).WindowMenu("Asset Explorer").SortOrder(4),
    CreateContentBrowserPanel)

}
