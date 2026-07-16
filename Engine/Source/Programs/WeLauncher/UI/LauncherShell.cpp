#include "UI/LauncherShell.h"

#include "UI/LauncherHelpers.h"
#include "UI/Widgets/ManagerViews.h"
#include "UI/Widgets/SettingsViews.h"
#include "UI/Widgets/SkeletonViews.h"

#include "Core/EventSystem.h"
#include "Core/Icon.h"
#include "Core/PaintContext.h"
#include "Core/Widgets/DesignSystemControls.h"
#include "Core/Widgets/PrimaryToolbarButton.h"
#include "Core/Widgets/SecondaryToolbarButton.h"
#include "Layout/Box.h"
#include "Layout/ScrollLayout.h"
#include "Layout/Spacer.h"
#include "Platform/PlatformSDK.h"
#include "Util/PathUtils.h"
#include "Widgets/Label.h"
#include "Widgets/TextBox.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>

using namespace WindEffects::Editor::UI;

namespace we::programs::welauncher {
namespace {

std::shared_ptr<Label> MakeLabel(const std::string& text, float size, Color color) {
    auto label = std::make_shared<Label>(text, color, size);
    label->SetHorizontalAlignment(HorizontalAlignment::Left);
    return label;
}

std::shared_ptr<HorizontalBox> MakePageHeaderRow() {
    auto row = std::make_shared<HorizontalBox>();
    row->SetSpacing(LMetric(ThemeToken::Space2) * LScale());
    row->SetPadding(Margin{
        LMetric(ThemeToken::Space4) * LScale(),
        LMetric(ThemeToken::Space3) * LScale(),
        LMetric(ThemeToken::Space4) * LScale(),
        LMetric(ThemeToken::Space2) * LScale()
    });
    row->SetVerticalAlignment(VerticalAlignment::Center);
    return row;
}

std::shared_ptr<VerticalBox> MakePageBodyPadding() {
    auto content = std::make_shared<VerticalBox>();
    content->SetPadding(Margin{
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space4) * LScale(),
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale()
    });
    content->SetSpacing(LMetric(ThemeToken::Space5) * LScale());
    content->SetHorizontalAlignment(HorizontalAlignment::Fill);
    return content;
}

std::shared_ptr<Widget> MakeSectionHeader(const std::string& title, const std::string& subtitle = {}) {
    auto heading = std::make_shared<SectionHeader>(title, subtitle);
    heading->SetHorizontalAlignment(HorizontalAlignment::Fill);
    return heading;
}

std::shared_ptr<Widget> MakePlaceholderCard(const std::string& title, const std::string& body, const char* icon) {
    auto tile = std::make_shared<DashboardTile>(title, body, icon);
    return tile;
}

std::string JoinList(const std::vector<std::string>& items, const char* sep = ", ") {
    if (items.empty()) {
        return "â€”";
    }
    std::string out;
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            out += sep;
        }
        out += items[i];
    }
    return out;
}

std::string SortModeLabel(ProjectSortMode mode) {
    switch (mode) {
    case ProjectSortMode::Name: return "Sort: Name";
    case ProjectSortMode::Engine: return "Sort: Engine";
    case ProjectSortMode::Recent:
    default: return "Sort: Recent";
    }
}

std::string ToLowerCopy(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

std::shared_ptr<VerticalBox> MakeSettingsRow(const std::string& caption, const std::string& value) {
    auto row = std::make_shared<VerticalBox>();
    row->SetSpacing(2.0f * LScale());
    row->SetHorizontalAlignment(HorizontalAlignment::Fill);
    row->AddChild(MakeLabel(
        caption,
        LMetric(ThemeToken::TextSizeCaption) * LScale(),
        LColor(ThemeToken::TextMuted)));
    row->AddChild(MakeLabel(
        value.empty() ? "â€”" : value,
        LMetric(ThemeToken::TextSizeBody) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    return row;
}

std::shared_ptr<VerticalBox> MakeFeatureTile(
    const std::string& title,
    const std::string& body,
    const char* icon) {
    (void)icon;
    auto tile = std::make_shared<VerticalBox>();
    tile->SetSpacing(LMetric(ThemeToken::Space1) * LScale());
    tile->SetPadding(Margin{
        LMetric(ThemeToken::Space3) * LScale(),
        LMetric(ThemeToken::Space3) * LScale(),
        LMetric(ThemeToken::Space3) * LScale(),
        LMetric(ThemeToken::Space3) * LScale()
    });
    tile->SetBackgroundColor(LColor(ThemeToken::PanelBackground));
    tile->SetHorizontalAlignment(HorizontalAlignment::Fill);
    tile->AddChild(MakeLabel(
        title,
        LMetric(ThemeToken::TextSizeBody) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    tile->AddChild(MakeLabel(
        body,
        LMetric(ThemeToken::TextSizeCaption) * LScale(),
        LColor(ThemeToken::TextMuted)));
    return tile;
}

const ProjectTemplateInfo* FindTemplateById(
    const std::vector<ProjectTemplateInfo>& templates,
    const std::string& id) {
    for (const auto& tmpl : templates) {
        if (tmpl.id == id) {
            return &tmpl;
        }
    }
    return templates.empty() ? nullptr : &templates.front();
}

} // namespace

LauncherShell::LauncherShell(std::shared_ptr<LauncherContext> context, we::platform::WindowId window)
    : m_Context(std::move(context))
    , m_Window(window) {
}

void LauncherShell::Construct(float dpiScale) {
    (void)dpiScale;
    m_WizardLocation = m_Context->Settings().Settings().defaultProjectsRoot;
    m_Projects = m_Context->Projects().LoadRecentSummaries();
    ApplySearchFilter();

    BeginPageContentLoad(0.32f);
    RebuildChrome();
    EnsurePageBuilt(m_Page);
    ShowPage(m_Page);
    UpdateFooter();
    SyncHeaderFromState();
    SetStatus(m_Context->StatusMessage().empty() ? "Ready" : m_Context->StatusMessage());
    InvalidateUI();
}

void LauncherShell::SetLogoTexture(we::rhi::RHIDescriptorSetHandle logoSet) {
    m_LogoSet = logoSet;
    if (m_TitleBar) {
        m_TitleBar->SetLogoTexture(logoSet);
    }
    InvalidateUI();
}

void LauncherShell::OnWindowStateChanged() {
    if (m_TitleBar) {
        m_TitleBar->UpdateMaximizeIcon();
    }
    InvalidateUI();
}

void LauncherShell::OnTextInput(char32_t codepoint) {
    if (m_ProjectsSearch && m_ProjectsSearch->IsFocused()) {
        m_ProjectsSearch->AppendCodepoint(codepoint);
    }
}

void LauncherShell::RebuildChrome() {
    ClearChildren();

    auto root = std::make_shared<VerticalBox>();
    root->SetSpacing(0.0f);
    root->SetPadding(Margin{});

    m_TitleBar = std::make_shared<LauncherTitleBar>(m_Window, "WindEffects Launcher");
    m_TitleBar->SetLogoTexture(m_LogoSet);
    m_TitleBar->SetOnHelp([this] { ShowHelp(); });
    m_TitleBar->SetOnSettings([this] { GoToPage(LauncherPage::Settings); });
    root->AddChild(m_TitleBar);

    auto body = std::make_shared<HorizontalBox>();
    body->SetSpacing(0.0f);
    body->SetVerticalAlignment(VerticalAlignment::Fill);

    m_Sidebar = std::make_shared<NavSidebar>();
    m_Sidebar->SetActivePage(m_Page);
    m_Sidebar->SetCollapsed(m_SidebarCollapsed);
    m_Sidebar->SetOnPageChanged([this](LauncherPage page) { GoToPage(page); });
    m_Sidebar->SetVerticalAlignment(VerticalAlignment::Fill);
    body->AddChild(m_Sidebar);

    auto sidebarRule = std::make_shared<ThinVerticalDivider>();
    sidebarRule->SetVerticalAlignment(VerticalAlignment::Fill);
    body->AddChild(sidebarRule);

    auto mainColumn = std::make_shared<VerticalBox>();
    mainColumn->SetSpacing(0.0f);
    mainColumn->SetHorizontalAlignment(HorizontalAlignment::Fill);
    mainColumn->SetVerticalAlignment(VerticalAlignment::Fill);

    auto contentHost = std::make_shared<VerticalBox>();
    contentHost->SetHorizontalAlignment(HorizontalAlignment::Fill);
    contentHost->SetVerticalAlignment(VerticalAlignment::Fill);
    contentHost->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));
    m_ContentHost = contentHost;
    mainColumn->AddChild(m_ContentHost);

    body->AddChild(mainColumn);
    root->AddChild(body);

    m_Footer = std::make_shared<StatusFooter>();
    root->AddChild(m_Footer);

    m_ModalHost = std::make_shared<ModalOverlay>();
    m_ModalHost->SetVisible(false);
    m_ModalHost->SetOnScrimClicked([this] { CloseModal(); });

    m_Root = root;
    AddChild(m_Root);
    AddChild(m_ModalHost);
}

void LauncherShell::SyncHeaderFromState() {
    if (m_ProjectsSearch) {
        m_ProjectsSearch->SetText(m_SearchQuery);
    }
}

void LauncherShell::CapturePageScroll(LauncherPage page) {
    const auto idx = static_cast<std::size_t>(page);
    if (idx >= m_Pages.size()) {
        return;
    }
    auto& state = m_Pages[idx];
    if (state.scroll) {
        state.scrollOffset = state.scroll->GetScrollOffset();
    }
}

void LauncherShell::RestorePageScroll(LauncherPage page) {
    const auto idx = static_cast<std::size_t>(page);
    if (idx >= m_Pages.size()) {
        return;
    }
    auto& state = m_Pages[idx];
    if (state.scroll) {
        state.scroll->SetScrollOffset(state.scrollOffset);
    }
}

void LauncherShell::GoToPage(LauncherPage page) {
    if (page == m_Page) {
        return;
    }
    CloseModal();
    CapturePageScroll(m_Page);
    m_Page = page;
    if (m_Sidebar) {
        m_Sidebar->SetActivePage(page);
    }
    BeginPageContentLoad(0.22f);
    MarkPageDirty(page);
    EnsurePageBuilt(page);
    ShowPage(page);
    SyncHeaderFromState();
    InvalidateUI();
}

void LauncherShell::BeginPageContentLoad(float durationSeconds) {
    m_PageContentLoading = true;
    m_PageLoadTimer = 0.0f;
    m_PageLoadDuration = std::max(0.12f, durationSeconds);
}

std::shared_ptr<Widget> LauncherShell::BuildPageSkeleton(LauncherPage page) {
    auto root = std::make_shared<VerticalBox>();
    root->SetSpacing(LMetric(ThemeToken::Space4) * LScale());
    root->SetHorizontalAlignment(HorizontalAlignment::Fill);

    switch (page) {
    case LauncherPage::Projects: {
        root->AddChild(MakeSectionHeader("Projects", "Loading workspace…"));
        for (int i = 0; i < 8; ++i) {
            root->AddChild(std::make_shared<SkeletonCard>(SkeletonKind::ListRow));
        }
        break;
    }
    case LauncherPage::Templates: {
        root->AddChild(MakeSectionHeader("Templates", "Loading starters…"));
        for (int i = 0; i < 6; ++i) {
            root->AddChild(std::make_shared<SkeletonCard>(SkeletonKind::ListRow));
        }
        break;
    }
    case LauncherPage::Library: {
        root->AddChild(MakeSectionHeader("Library", "Loading packages…"));
        for (int i = 0; i < 8; ++i) {
            root->AddChild(std::make_shared<SkeletonCard>(SkeletonKind::ListRow));
        }
        break;
    }
    case LauncherPage::Engine: {
        root->AddChild(MakeSectionHeader("Engine", "Reading installs…"));
        for (int i = 0; i < 4; ++i) {
            root->AddChild(std::make_shared<SkeletonCard>(SkeletonKind::ListRow));
        }
        break;
    }
    case LauncherPage::Settings:
    default: {
        root->AddChild(MakeSectionHeader("Settings", "Loading preferencesâ€¦"));
        for (int i = 0; i < 4; ++i) {
            root->AddChild(std::make_shared<SkeletonCard>(SkeletonKind::ListRow));
        }
        break;
    }
    }
    return root;
}

void LauncherShell::MarkPageDirty(LauncherPage page) {
    const auto idx = static_cast<std::size_t>(page);
    if (idx < m_Pages.size()) {
        m_Pages[idx].dirty = true;
    }
}

void LauncherShell::EnsurePageBuilt(LauncherPage page) {
    const auto idx = static_cast<std::size_t>(page);
    if (idx >= m_Pages.size()) {
        return;
    }
    auto& state = m_Pages[idx];
    if (state.built && !state.dirty) {
        return;
    }

    CapturePageScroll(page);

    switch (page) {
    case LauncherPage::Projects: RebuildProjectsPage(); break;
    case LauncherPage::Templates: RebuildTemplatesPage(); break;
    case LauncherPage::Library: RebuildLibraryPage(); break;
    case LauncherPage::Engine: RebuildEnginePage(); break;
    case LauncherPage::Settings: RebuildSettingsPage(); break;
    default: break;
    }

    state.built = true;
    state.dirty = false;
    RestorePageScroll(page);
}

void LauncherShell::ShowPage(LauncherPage page) {
    if (!m_ContentHost) {
        return;
    }
    m_ContentHost->ClearChildren();

    const auto idx = static_cast<std::size_t>(page);
    if (idx < m_Pages.size() && m_Pages[idx].root) {
        m_ContentHost->AddChild(m_Pages[idx].root);
        RestorePageScroll(page);
    }
    InvalidateUI();
}

void LauncherShell::OnSearchChanged(const std::string& text) {
    if (m_SearchQuery == text) {
        return;
    }
    m_SearchQuery = text;
    if (m_Page == LauncherPage::Projects) {
        ApplySearchFilter();
        MarkPageDirty(LauncherPage::Projects);
        EnsurePageBuilt(LauncherPage::Projects);
        ShowPage(LauncherPage::Projects);
    } else if (m_Page == LauncherPage::Templates) {
        MarkPageDirty(LauncherPage::Templates);
        EnsurePageBuilt(LauncherPage::Templates);
        ShowPage(LauncherPage::Templates);
    } else if (m_Page == LauncherPage::Settings) {
        MarkPageDirty(LauncherPage::Settings);
        EnsurePageBuilt(LauncherPage::Settings);
        ShowPage(LauncherPage::Settings);
    }
    InvalidateUI();
}

void LauncherShell::OnViewModeChanged(ProjectViewMode mode) {
    if (m_ViewMode == mode) {
        return;
    }
    m_ViewMode = mode;
    MarkPageDirty(LauncherPage::Projects);
    if (m_Page == LauncherPage::Projects) {
        EnsurePageBuilt(LauncherPage::Projects);
        ShowPage(LauncherPage::Projects);
    }
    InvalidateUI();
}

void LauncherShell::CycleSortMode() {
    switch (m_SortMode) {
    case ProjectSortMode::Recent: SetSortMode(ProjectSortMode::Name); break;
    case ProjectSortMode::Name: SetSortMode(ProjectSortMode::Engine); break;
    case ProjectSortMode::Engine:
    default: SetSortMode(ProjectSortMode::Recent); break;
    }
}

void LauncherShell::SetSortMode(ProjectSortMode mode) {
    if (m_SortMode == mode) {
        ApplyProjectSort();
        MarkPageDirty(LauncherPage::Projects);
        if (m_Page == LauncherPage::Projects) {
            EnsurePageBuilt(LauncherPage::Projects);
            ShowPage(LauncherPage::Projects);
        }
        InvalidateUI();
        return;
    }
    m_SortMode = mode;
    ApplyProjectSort();
    MarkPageDirty(LauncherPage::Projects);
    if (m_Page == LauncherPage::Projects) {
        EnsurePageBuilt(LauncherPage::Projects);
        ShowPage(LauncherPage::Projects);
    }
    InvalidateUI();
}

void LauncherShell::ToggleCompatibleFilter() {
    m_CompatibleOnly = !m_CompatibleOnly;
    ApplySearchFilter();
    MarkPageDirty(LauncherPage::Projects);
    if (m_Page == LauncherPage::Projects) {
        EnsurePageBuilt(LauncherPage::Projects);
        ShowPage(LauncherPage::Projects);
    }
    InvalidateUI();
}

void LauncherShell::ShowHelp() {
    SetStatus(
        "Help â€” Ctrl+N new project, Ctrl+O browse, Enter open selected, "
        "Arrow keys navigate sidebar, Esc closes dialogs.");
}

bool LauncherShell::IsFavorite(const std::string& weprojPath) const {
    return std::find(m_FavoritePaths.begin(), m_FavoritePaths.end(), weprojPath) != m_FavoritePaths.end();
}

void LauncherShell::ToggleFavorite(const std::string& weprojPath) {
    if (weprojPath.empty()) {
        return;
    }
    const auto it = std::find(m_FavoritePaths.begin(), m_FavoritePaths.end(), weprojPath);
    if (it != m_FavoritePaths.end()) {
        m_FavoritePaths.erase(it);
        SetStatus("Removed from pinned projects");
    } else {
        m_FavoritePaths.push_back(weprojPath);
        SetStatus("Pinned project");
    }
    MarkPageDirty(LauncherPage::Projects);
    if (m_Page == LauncherPage::Projects) {
        EnsurePageBuilt(LauncherPage::Projects);
        ShowPage(LauncherPage::Projects);
    }
}

void LauncherShell::ShowProjectMoreMenu(std::size_t index) {
    SelectProject(index);
    m_Modal = ModalKind::Actions;
    RebuildProjectActionsDialog();
}

std::shared_ptr<Widget> LauncherShell::BuildProjectsEmptyState() {
    auto empty = std::make_shared<EmptyStatePanel>(
        "No Projects Found",
        "Create a new WindEffects project or open an existing .weproj from disk.",
        Icons::PackageName);
    empty->SetPrimaryAction("Create Project", Icons::PlusName, [this] { ShowCreateWizard(); });
    empty->SetSecondaryAction("Open Existing Project", Icons::OpenFolderName, [this] { BrowseForProject(); });
    empty->SetHorizontalAlignment(HorizontalAlignment::Fill);
    empty->SetVerticalAlignment(VerticalAlignment::Fill);
    return empty;
}

std::shared_ptr<Widget> LauncherShell::BuildProjectsPageHeader() {
    const float s = LScale();
    auto row = std::make_shared<HorizontalBox>();
    row->SetSpacing(8.0f * s);
    row->SetVerticalAlignment(VerticalAlignment::Center);
    row->SetHorizontalAlignment(HorizontalAlignment::Fill);
    row->SetPadding(Margin{ 12.0f * s, 10.0f * s, 12.0f * s, 10.0f * s });
    row->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

    auto title = MakeLabel(
        "Projects",
        LMetric(ThemeToken::TextSizeTitle) * s,
        LColor(ThemeToken::TextPrimary));
    title->SetVerticalAlignment(VerticalAlignment::Center);
    row->AddChild(title);
    row->AddChild(std::make_shared<Spacer>());

    auto openBtn = std::make_shared<SecondaryToolbarButton>("Open ▼", Icons::OpenFolderName);
    openBtn->SetOnClicked([this] { BrowseForProject(); });
    row->AddChild(openBtn);

    auto newBtn = std::make_shared<PrimaryToolbarButton>("New Project", Icons::PlusName);
    newBtn->SetOnClicked([this] { ShowCreateWizard(); });
    row->AddChild(newBtn);

    return row;
}

std::shared_ptr<Widget> LauncherShell::BuildProjectToolbar() {
    const float s = LScale();
    auto bar = std::make_shared<HorizontalBox>();
    bar->SetSpacing(8.0f * s);
    bar->SetVerticalAlignment(VerticalAlignment::Center);
    bar->SetHorizontalAlignment(HorizontalAlignment::Fill);
    bar->SetPadding(Margin{ 12.0f * s, 8.0f * s, 12.0f * s, 8.0f * s });
    bar->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

    auto search = std::make_shared<CompactSearchField>("Search Projects");
    search->SetText(m_SearchQuery);
    search->SetOnChanged([this](const std::string& text) { OnSearchChanged(text); });
    m_ProjectsSearch = search;
    bar->AddChild(search);

    auto sortBtn = std::make_shared<SecondaryToolbarButton>(SortModeLabel(m_SortMode), Icons::ChevronDownName);
    sortBtn->SetOnClicked([this] { CycleSortMode(); });
    bar->AddChild(sortBtn);

    auto filterBtn = std::make_shared<SecondaryToolbarButton>(
        m_CompatibleOnly ? "Compatible" : "Filter",
        Icons::FilterName);
    filterBtn->SetOnClicked([this] { ToggleCompatibleFilter(); });
    bar->AddChild(filterBtn);

    bar->AddChild(std::make_shared<Spacer>());

    return bar;
}

void LauncherShell::RebuildProjectsPage() {
    auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Projects)];

    auto page = std::make_shared<VerticalBox>();
    page->SetSpacing(0.0f);
    page->SetHorizontalAlignment(HorizontalAlignment::Fill);
    page->SetVerticalAlignment(VerticalAlignment::Fill);
    page->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

    if (IsPageContentLoading()) {
        auto scroll = std::make_shared<ScrollLayout>();
        auto content = MakePageBodyPadding();
        content->AddChild(BuildPageSkeleton(LauncherPage::Projects));
        scroll->SetContent(content);
        page->AddChild(scroll);
        state.root = page;
        state.scroll = scroll;
        return;
    }

    const float s = LScale();
    page->AddChild(BuildProjectsPageHeader());
    page->AddChild(std::make_shared<ThinDivider>());
    page->AddChild(BuildProjectToolbar());
    page->AddChild(std::make_shared<ThinDivider>());

    auto tableHost = std::make_shared<VerticalBox>();
    tableHost->SetSpacing(0.0f);
    tableHost->SetHorizontalAlignment(HorizontalAlignment::Fill);
    tableHost->SetVerticalAlignment(VerticalAlignment::Fill);
    tableHost->SetPadding(Margin{ 12.0f * s, 0.0f, 12.0f * s, 8.0f * s });
    tableHost->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

    state.scroll = nullptr;

    if (m_FilteredProjects.empty()) {
        if (m_Projects.empty() && m_SearchQuery.empty() && !m_CompatibleOnly) {
            tableHost->AddChild(BuildProjectsEmptyState());
        } else {
            auto empty = std::make_shared<EmptyStatePanel>(
                "No matching projects",
                m_CompatibleOnly
                    ? "No compatible projects match the current filter."
                    : "Try a different search or clear the filter.",
                Icons::SearchName);
            empty->SetPrimaryAction("Clear Search", Icons::RefreshName, [this] {
                OnSearchChanged({});
            });
            if (m_CompatibleOnly) {
                empty->SetSecondaryAction("Show All", Icons::FilterName, [this] {
                    ToggleCompatibleFilter();
                });
            }
            empty->SetHorizontalAlignment(HorizontalAlignment::Fill);
            empty->SetVerticalAlignment(VerticalAlignment::Fill);
            tableHost->AddChild(empty);
        }
    } else {
        auto header = std::make_shared<ProjectTableHeader>();
        header->SetSortMode(m_SortMode);
        header->SetOnSort([this](ProjectSortMode mode) { SetSortMode(mode); });
        tableHost->AddChild(header);

        auto scroll = std::make_shared<ScrollLayout>();
        auto list = std::make_shared<VerticalBox>();
        list->SetSpacing(0.0f);
        list->SetHorizontalAlignment(HorizontalAlignment::Fill);
        list->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

        auto mapIndex = [this](const ProjectSummary& project, std::size_t fallback) -> std::size_t {
            for (std::size_t j = 0; j < m_Projects.size(); ++j) {
                if (m_Projects[j].weprojPath == project.weprojPath) {
                    return j;
                }
            }
            return fallback;
        };

        for (std::size_t i = 0; i < m_FilteredProjects.size(); ++i) {
            const auto& project = m_FilteredProjects[i];
            const std::size_t selectIndex = mapIndex(project, i);
            const bool selected = static_cast<int>(selectIndex) == m_SelectedIndex
                || std::find(m_MultiSelected.begin(), m_MultiSelected.end(), selectIndex)
                    != m_MultiSelected.end();
            auto row = std::make_shared<ProjectTableRow>(
                project, selected, IsFavorite(project.weprojPath));
            row->SetOnSelect([this, selectIndex](bool additive) {
                SelectProject(selectIndex, additive);
            });
            row->SetOnAction([this, selectIndex](ProjectCardAction action) {
                HandleProjectAction(selectIndex, action);
            });
            list->AddChild(row);
        }

        auto filler = std::make_shared<FixedGap>(1.0f, 1.0f);
        filler->SetVerticalAlignment(VerticalAlignment::Fill);
        list->AddChild(filler);

        scroll->SetContent(list);
        scroll->SetHorizontalAlignment(HorizontalAlignment::Fill);
        scroll->SetVerticalAlignment(VerticalAlignment::Fill);
        tableHost->AddChild(scroll);
        state.scroll = scroll;
    }

    page->AddChild(tableHost);
    state.root = page;
}

void LauncherShell::RebuildTemplatesPage() {
    auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Templates)];

    auto page = std::make_shared<VerticalBox>();
    page->SetSpacing(0.0f);
    page->SetHorizontalAlignment(HorizontalAlignment::Fill);
    page->SetVerticalAlignment(VerticalAlignment::Fill);
    page->SetBackgroundColor(LColor(ThemeToken::WindowBackground));

    if (IsPageContentLoading()) {
        auto scroll = std::make_shared<ScrollLayout>();
        auto content = MakePageBodyPadding();
        content->AddChild(BuildPageSkeleton(LauncherPage::Templates));
        scroll->SetContent(content);
        page->AddChild(scroll);
        state.root = page;
        state.scroll = scroll;
        return;
    }

    const auto& templates = m_Context->Templates().Templates();
    const std::string query = ToLowerCopy(m_SearchQuery);

    std::vector<const ProjectTemplateInfo*> visible;
    visible.reserve(templates.size());
    for (const auto& tmpl : templates) {
        if (!query.empty()) {
            const std::string hay = ToLowerCopy(
                tmpl.displayName + " " + tmpl.id + " " + tmpl.category + " " + tmpl.description);
            if (hay.find(query) == std::string::npos) {
                continue;
            }
        }
        visible.push_back(&tmpl);
    }

    if (!visible.empty()) {
        bool stillVisible = false;
        for (const auto* tmpl : visible) {
            if (tmpl->id == m_WizardTemplateId) {
                stillVisible = true;
                break;
            }
        }
        if (!stillVisible) {
            m_WizardTemplateId = visible.front()->id;
        }
    }

    const ProjectTemplateInfo* selected = FindTemplateById(templates, m_WizardTemplateId);

    auto toolbar = std::make_shared<HorizontalBox>();
    toolbar->SetSpacing(LMetric(ThemeToken::Space2) * LScale());
    toolbar->SetPadding(Margin{
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space3) * LScale(),
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space2) * LScale()
    });
    toolbar->SetVerticalAlignment(VerticalAlignment::Center);
    auto titleCol = std::make_shared<VerticalBox>();
    titleCol->SetSpacing(2.0f * LScale());
    titleCol->AddChild(MakeLabel(
        "Templates",
        LMetric(ThemeToken::TextSizeHeader) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    titleCol->AddChild(MakeLabel(
        std::to_string(visible.size()) + " starter"
            + (visible.size() == 1 ? "" : "s") + " available",
        LMetric(ThemeToken::TextSizeCaption) * LScale(),
        LColor(ThemeToken::TextMuted)));
    toolbar->AddChild(titleCol);
    toolbar->AddChild(std::make_shared<Spacer>());
    auto create = std::make_shared<PrimaryToolbarButton>("Create Project", Icons::PlusName);
    create->SetOnClicked([this] { ShowCreateWizard(); });
    toolbar->AddChild(create);
    page->AddChild(toolbar);

    auto body = std::make_shared<HorizontalBox>();
    body->SetSpacing(LMetric(ThemeToken::Space3) * LScale());
    body->SetPadding(Margin{
        LMetric(ThemeToken::Space6) * LScale(),
        0.0f,
        LMetric(ThemeToken::Space4) * LScale(),
        LMetric(ThemeToken::Space4) * LScale()
    });
    body->SetHorizontalAlignment(HorizontalAlignment::Fill);
    body->SetVerticalAlignment(VerticalAlignment::Fill);

    auto scroll = std::make_shared<ScrollLayout>();
    auto list = std::make_shared<VerticalBox>();
    list->SetSpacing(2.0f * LScale());
    list->SetHorizontalAlignment(HorizontalAlignment::Fill);

    if (visible.empty()) {
        auto empty = std::make_shared<EmptyStatePanel>(
            query.empty() ? "No templates yet" : "No matching templates",
            query.empty()
                ? "Template packs will appear here when the engine templates folder is available."
                : "Try a different search from the header search field.",
            Icons::LayersName);
        empty->SetPrimaryAction("Create Blank Project", Icons::PlusName, [this] {
            m_WizardTemplateId = "Blank";
            ShowCreateWizard();
        });
        list->AddChild(empty);
    } else {
        for (const auto* tmpl : visible) {
            const bool isSelected = selected && tmpl->id == selected->id;
            auto row = std::make_shared<TemplateListRow>(*tmpl, isSelected);
            row->SetOnSelect([this, id = tmpl->id] {
                m_WizardTemplateId = id;
                SetStatus("Selected template: " + id);
                MarkPageDirty(LauncherPage::Templates);
                EnsurePageBuilt(LauncherPage::Templates);
                ShowPage(LauncherPage::Templates);
            });
            list->AddChild(row);
        }
    }

    scroll->SetContent(list);
    scroll->SetHorizontalAlignment(HorizontalAlignment::Fill);
    scroll->SetVerticalAlignment(VerticalAlignment::Fill);
    body->AddChild(scroll);

    auto detailsWrap = std::make_shared<VerticalBox>();
    detailsWrap->SetSpacing(0.0f);
    detailsWrap->SetVerticalAlignment(VerticalAlignment::Fill);
    detailsWrap->AddChild(std::make_shared<FixedGap>(280.0f * LScale(), 1.0f));

    auto details = std::make_shared<VerticalBox>();
    details->SetSpacing(LMetric(ThemeToken::Space3) * LScale());
    details->SetPadding(Margin{
        LMetric(ThemeToken::Space4) * LScale(),
        LMetric(ThemeToken::Space4) * LScale(),
        LMetric(ThemeToken::Space4) * LScale(),
        LMetric(ThemeToken::Space4) * LScale()
    });
    details->SetBackgroundColor(LColor(ThemeToken::PanelBackground));
    details->SetHorizontalAlignment(HorizontalAlignment::Fill);
    details->SetVerticalAlignment(VerticalAlignment::Fill);

    if (selected) {
        details->AddChild(MakeLabel(
            selected->displayName,
            LMetric(ThemeToken::TextSizeHeader) * LScale(),
            LColor(ThemeToken::TextPrimary)));
        details->AddChild(MakeLabel(
            selected->category.empty() ? "Template" : selected->category,
            LMetric(ThemeToken::TextSizeCaption) * LScale(),
            LColor(ThemeToken::AccentPrimary)));
        details->AddChild(MakeLabel(
            selected->description.empty() ? "No description." : selected->description,
            LMetric(ThemeToken::TextSizeBody) * LScale(),
            LColor(ThemeToken::TextMuted)));
        details->AddChild(MakeSettingsRow("Platforms", JoinList(selected->platforms)));
        details->AddChild(MakeSettingsRow("Features", JoinList(selected->features)));
        details->AddChild(MakeSettingsRow("Plugins", JoinList(selected->plugins)));
        auto use = std::make_shared<PrimaryToolbarButton>("Use Template", Icons::PlusName);
        use->SetOnClicked([this] { ShowCreateWizard(); });
        details->AddChild(use);
    } else {
        details->AddChild(MakeLabel(
            "Select a template",
            LMetric(ThemeToken::TextSizeBody) * LScale(),
            LColor(ThemeToken::TextMuted)));
    }

    detailsWrap->AddChild(details);
    body->AddChild(detailsWrap);
    page->AddChild(body);

    state.root = page;
    state.scroll = scroll;
}

void LauncherShell::RebuildLibraryPage() {
    auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Library)];

    auto page = std::make_shared<VerticalBox>();
    page->SetSpacing(0.0f);
    page->SetHorizontalAlignment(HorizontalAlignment::Fill);
    page->SetVerticalAlignment(VerticalAlignment::Fill);
    page->SetBackgroundColor(LColor(ThemeToken::WindowBackground));

    auto scroll = std::make_shared<ScrollLayout>();
    auto content = std::make_shared<VerticalBox>();
    content->SetSpacing(LMetric(ThemeToken::Space3) * LScale());
    content->SetPadding(Margin{
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space3) * LScale(),
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale()
    });
    content->SetHorizontalAlignment(HorizontalAlignment::Fill);

    if (IsPageContentLoading()) {
        content->AddChild(BuildPageSkeleton(LauncherPage::Library));
        scroll->SetContent(content);
        page->AddChild(scroll);
        state.root = page;
        state.scroll = scroll;
        return;
    }

    content->AddChild(MakeLabel(
        "Library",
        LMetric(ThemeToken::TextSizeHeader) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    content->AddChild(MakeLabel(
        "Templates, plugins, samples, and managed packages",
        LMetric(ThemeToken::TextSizeCaption) * LScale(),
        LColor(ThemeToken::TextMuted)));

    content->AddChild(MakeLabel(
        "Templates",
        LMetric(ThemeToken::TextSizeBody) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    for (const auto& tmpl : m_Context->Templates().Templates()) {
        content->AddChild(std::make_shared<LibraryPackageRow>(
            "Template",
            tmpl.displayName,
            tmpl.category + " · " + JoinList(tmpl.platforms),
            TemplateTypeIcon(tmpl.id),
            [this, id = tmpl.id] {
                m_WizardTemplateId = id;
                GoToPage(LauncherPage::Templates);
            }));
    }

    content->AddChild(MakeLabel(
        "Plugins",
        LMetric(ThemeToken::TextSizeBody) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    content->AddChild(std::make_shared<LibraryPackageRow>(
        "Plugin", "Render Debug", "Visualization overlays — not installed", Icons::EyeName,
        [this] { SetStatus("Plugin manager — coming soon"); }));
    content->AddChild(std::make_shared<LibraryPackageRow>(
        "Plugin", "Asset Importer", "Mesh and texture import — not installed", Icons::OpenFolderName,
        [this] { SetStatus("Plugin manager — coming soon"); }));
    content->AddChild(std::make_shared<LibraryPackageRow>(
        "Plugin", "Input Remapper", "Binding tools — not installed", Icons::SettingsName,
        [this] { SetStatus("Plugin manager — coming soon"); }));

    content->AddChild(MakeLabel(
        "Samples",
        LMetric(ThemeToken::TextSizeBody) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    content->AddChild(std::make_shared<LibraryPackageRow>(
        "Sample", "First Person Demo", "Movement and camera baseline", Icons::PlayName,
        [this] { SetStatus("Sample projects — coming soon"); }));
    content->AddChild(std::make_shared<LibraryPackageRow>(
        "Sample", "Atmosphere Showcase", "Sky, clouds, and fog overview", Icons::SunName,
        [this] { SetStatus("Sample projects — coming soon"); }));
    content->AddChild(std::make_shared<LibraryPackageRow>(
        "Sample", "ECS Sandbox", "Entity component patterns", Icons::ComponentName,
        [this] { SetStatus("Sample projects — coming soon"); }));

    content->AddChild(MakeLabel(
        "Downloads",
        LMetric(ThemeToken::TextSizeBody) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    content->AddChild(std::make_shared<LibraryPackageRow>(
        "Download", "No managed packages", "Installed marketplace items will appear here", Icons::OpenFolderName));

    scroll->SetContent(content);
    page->AddChild(scroll);
    state.root = page;
    state.scroll = scroll;
}

void LauncherShell::RebuildEnginePage() {
    auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Engine)];

    auto page = std::make_shared<VerticalBox>();
    page->SetSpacing(0.0f);
    page->SetHorizontalAlignment(HorizontalAlignment::Fill);
    page->SetVerticalAlignment(VerticalAlignment::Fill);
    page->SetBackgroundColor(LColor(ThemeToken::WindowBackground));

    auto toolbar = std::make_shared<HorizontalBox>();
    toolbar->SetSpacing(LMetric(ThemeToken::Space2) * LScale());
    toolbar->SetPadding(Margin{
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space3) * LScale(),
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space2) * LScale()
    });
    toolbar->SetVerticalAlignment(VerticalAlignment::Center);
    auto titleCol = std::make_shared<VerticalBox>();
    titleCol->SetSpacing(2.0f * LScale());
    titleCol->AddChild(MakeLabel(
        "Engine",
        LMetric(ThemeToken::TextSizeHeader) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    titleCol->AddChild(MakeLabel(
        "Installed engines and SDK health",
        LMetric(ThemeToken::TextSizeCaption) * LScale(),
        LColor(ThemeToken::TextMuted)));
    toolbar->AddChild(titleCol);
    toolbar->AddChild(std::make_shared<Spacer>());
    auto refresh = std::make_shared<SecondaryToolbarButton>("Refresh", Icons::RefreshName);
    refresh->SetOnClicked([this] {
        BeginPageContentLoad(0.2f);
        UpdateFooter();
        MarkPageDirty(LauncherPage::Engine);
        EnsurePageBuilt(LauncherPage::Engine);
        ShowPage(LauncherPage::Engine);
        SetStatus("Engine status refreshed");
    });
    toolbar->AddChild(refresh);
    page->AddChild(toolbar);

    auto scroll = std::make_shared<ScrollLayout>();
    auto content = std::make_shared<VerticalBox>();
    content->SetSpacing(2.0f * LScale());
    content->SetPadding(Margin{
        LMetric(ThemeToken::Space6) * LScale(),
        0.0f,
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale()
    });
    content->SetHorizontalAlignment(HorizontalAlignment::Fill);

    if (IsPageContentLoading()) {
        content->AddChild(BuildPageSkeleton(LauncherPage::Engine));
        scroll->SetContent(content);
        page->AddChild(scroll);
        state.root = page;
        state.scroll = scroll;
        return;
    }

    if (!m_Context->Engines().HasEngine() || m_Context->Engines().InstalledEngines().empty()) {
        auto empty = std::make_shared<EmptyStatePanel>(
            "No Engine Found",
            "Place the launcher next to an Engine install, or build the engine from source.",
            Icons::BuildName);
        empty->SetPrimaryAction("Open Projects", Icons::OpenFolderName, [this] {
            GoToPage(LauncherPage::Projects);
        });
        empty->SetSecondaryAction("Refresh", Icons::RefreshName, [this] {
            BeginPageContentLoad(0.2f);
            MarkPageDirty(LauncherPage::Engine);
            EnsurePageBuilt(LauncherPage::Engine);
            ShowPage(LauncherPage::Engine);
        });
        content->AddChild(empty);
    } else {
        std::string sdkSummary;
        {
            int pass = 0, warn = 0, fail = 0;
            for (const auto& check : m_Context->Sdk().RunChecks()) {
                switch (check.status) {
                case SdkCheckStatus::Pass: ++pass; break;
                case SdkCheckStatus::Warn: ++warn; break;
                case SdkCheckStatus::Fail: ++fail; break;
                }
            }
            sdkSummary = std::to_string(pass) + " ok / "
                + std::to_string(warn) + " warn / "
                + std::to_string(fail) + " fail";
        }

        // Compact column header
        content->AddChild(MakeLabel(
            "VERSION · BUILD · PATH · SDK · PLUGINS · UPDATES",
            LMetric(ThemeToken::TextSizeCaption) * LScale(),
            LColor(ThemeToken::TextMuted)));

        for (const auto& install : m_Context->Engines().InstalledEngines()) {
            EngineInstallInfo rowInfo = install;
            rowInfo.sdkStatus = sdkSummary;
            rowInfo.pluginCount = 0;
            if (rowInfo.buildId.empty()) {
                rowInfo.buildId = "Development";
            }
            if (rowInfo.updateStatus.empty()) {
                rowInfo.updateStatus = install.isCurrent
                    ? "Local development build"
                    : "Alternate install";
            }

            auto row = std::make_shared<EngineInstallRow>(rowInfo, install.isCurrent);
            const std::string root = install.engineRoot;
            row->SetOnAction([this, root](const std::string& action) {
                if (action == "launch") {
                    GoToPage(LauncherPage::Projects);
                    SetStatus("Open a project to launch the editor");
                } else if (action == "verify") {
                    UpdateFooter();
                    MarkPageDirty(LauncherPage::Engine);
                    EnsurePageBuilt(LauncherPage::Engine);
                    ShowPage(LauncherPage::Engine);
                    SetStatus("SDK verification complete");
                } else if (action == "repair") {
                    SetStatus("Engine repair queued");
                } else if (action == "folder") {
                    if (RevealInExplorer(root)) {
                        SetStatus("Opened engine folder");
                    } else {
                        SetStatus("Failed to open engine folder.");
                    }
                } else if (action == "uninstall") {
                    SetStatus("Uninstall is not available for local development installs");
                }
            });
            content->AddChild(row);
        }

        content->AddChild(MakeLabel(
            "SDK checks",
            LMetric(ThemeToken::TextSizeBody) * LScale(),
            LColor(ThemeToken::TextPrimary)));
        for (const auto& check : m_Context->Sdk().RunChecks()) {
            Color color = LColor(ThemeToken::TextMuted);
            if (check.status == SdkCheckStatus::Pass) color = LColor(ThemeToken::Success);
            else if (check.status == SdkCheckStatus::Warn) color = LColor(ThemeToken::Warning);
            else if (check.status == SdkCheckStatus::Fail) color = LColor(ThemeToken::ErrorForeground);
            content->AddChild(MakeLabel(
                check.name + " — " + check.detail,
                LMetric(ThemeToken::TextSizeCaption) * LScale(),
                color));
        }
    }

    scroll->SetContent(content);
    page->AddChild(scroll);
    state.root = page;
    state.scroll = scroll;
}

void LauncherShell::PersistLauncherSettings(const std::string& statusMessage) {
    m_Context->Save();
    SetStatus(statusMessage.empty() ? "Settings saved" : statusMessage);
}

namespace {

std::shared_ptr<VerticalBox> MakeSettingsStack() {
    auto stack = std::make_shared<VerticalBox>();
    stack->SetSpacing(8.0f * LScale());
    stack->SetHorizontalAlignment(HorizontalAlignment::Fill);
    return stack;
}

bool SettingsCategoryMatchesQuery(SettingsCategory category, const std::string& queryLower) {
    if (queryLower.empty()) {
        return true;
    }
    const std::string hay = ToLowerCopy(
        std::string(SettingsCategoryTitle(category)) + " " + SettingsCategoryKeywords(category));
    return hay.find(queryLower) != std::string::npos;
}

void AppendSettingsRow(
    VerticalBox& body,
    const std::string& queryLower,
    const std::string& label,
    const std::string& hint,
    std::shared_ptr<Widget> control,
    const std::string& extraSearch = {}) {
    auto row = std::make_shared<SettingsRow>(
        label,
        hint,
        std::move(control),
        label + " " + hint + " " + extraSearch);
    row->AttachControl();
    if (!queryLower.empty() && !row->MatchesQuery(queryLower)) {
        return;
    }
    if (!queryLower.empty()) {
        row->SetHighlighted(true);
    }
    body.AddChild(row);
}

std::shared_ptr<SettingsGroup> WrapSettingsGroup(
    const std::string& title,
    const std::string& description,
    const std::shared_ptr<VerticalBox>& body,
    const std::string& queryLower) {
    if (body->GetChildren().empty()) {
        return nullptr;
    }
    auto group = std::make_shared<SettingsGroup>(title, description);
    if (!queryLower.empty()) {
        group->SetHighlighted(true);
    }
    group->SetContent(body);
    return group;
}

void HighlightSettingsSubtree(const std::shared_ptr<Widget>& root) {
    if (!root) {
        return;
    }
    if (auto group = std::dynamic_pointer_cast<SettingsGroup>(root)) {
        group->SetHighlighted(true);
    }
    if (auto row = std::dynamic_pointer_cast<SettingsRow>(root)) {
        row->SetHighlighted(true);
    }
    for (const auto& child : root->GetChildren()) {
        HighlightSettingsSubtree(child);
    }
}

} // namespace

std::shared_ptr<Widget> LauncherShell::BuildSettingsGeneral(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto& settings = m_Context->Settings().Settings();

    {
        auto body = MakeSettingsStack();

        auto status = MakeLabel(
            m_Context->StatusMessage().empty() ? "Ready" : m_Context->StatusMessage(),
            LMetric(ThemeToken::TextSizeBody) * LScale(),
            LColor(ThemeToken::TextMuted));
        AppendSettingsRow(*body, queryLower, "Status", "Current launcher status", status);

        auto lang = std::make_shared<SettingsDropdown>(
            std::vector<std::string>{ "English", "Japanese", "Korean", "German", "French" },
            IndexOfOption({ "English", "Japanese", "Korean", "German", "French" }, settings.language));
        lang->SetOnChanged([this](int index) {
            static const char* kLang[] = { "English", "Japanese", "Korean", "German", "French" };
            if (index >= 0 && index < 5) {
                m_Context->Settings().Settings().language = kLang[index];
                PersistLauncherSettings("Language: " + m_Context->Settings().Settings().language);
            }
        });
        AppendSettingsRow(*body, queryLower, "Language", "Launcher UI language", lang, "locale");

        auto autoSave = std::make_shared<ToggleSwitch>(settings.autoSave);
        autoSave->SetOnChanged([this](bool on) {
            m_Context->Settings().Settings().autoSave = on;
            PersistLauncherSettings(on ? "Auto-save enabled" : "Auto-save disabled");
        });
        AppendSettingsRow(*body, queryLower, "Automatically Save", "Persist launcher preferences immediately", autoSave);

        auto updates = std::make_shared<ToggleSwitch>(settings.checkUpdatesOnStartup);
        updates->SetOnChanged([this](bool on) {
            m_Context->Settings().Settings().checkUpdatesOnStartup = on;
            PersistLauncherSettings("Startup update check updated");
            MarkPageDirty(LauncherPage::Settings);
            EnsurePageBuilt(LauncherPage::Settings);
            ShowPage(LauncherPage::Settings);
        });
        AppendSettingsRow(*body, queryLower, "Check for Updates on Startup", "Look for launcher and engine updates", updates);

        auto telemetry = std::make_shared<SettingsCheckBox>(settings.telemetry);
        telemetry->SetOnChanged([this](bool on) {
            m_Context->Settings().Settings().telemetry = on;
            PersistLauncherSettings("Telemetry preference saved");
        });
        AppendSettingsRow(*body, queryLower, "Anonymous Telemetry", "Help improve WindEffects (optional)", telemetry);

        if (auto group = WrapSettingsGroup("General", "Language, saving, and startup behavior", body, queryLower)) {
            root->AddChild(group);
        }
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsProjects(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto& settings = m_Context->Settings().Settings();
    auto body = MakeSettingsStack();

    auto folder = std::make_shared<PathPickerField>(settings.defaultProjectsRoot, true);
    folder->SetDialogTitle("Select Default Projects Folder");
    folder->SetOnChanged([this](const std::string& path) {
        m_Context->Settings().Settings().defaultProjectsRoot = path;
        PersistLauncherSettings("Default projects folder updated");
        MarkPageDirty(LauncherPage::Settings);
        EnsurePageBuilt(LauncherPage::Settings);
        ShowPage(LauncherPage::Settings);
    });
    AppendSettingsRow(*body, queryLower, "Project Folder", "Default location for new projects", folder, "path browse");

    const std::vector<std::string> configs{ "Debug", "Development", "Shipping" };
    auto config = std::make_shared<SettingsSegmented>(configs);
    config->SetSelected(IndexOfOption(configs, settings.lastBuildConfig));
    config->SetOnChanged([this, configs](int index) {
        if (index >= 0 && index < static_cast<int>(configs.size())) {
            m_Context->Settings().Settings().lastBuildConfig = configs[static_cast<std::size_t>(index)];
            PersistLauncherSettings("Build configuration: " + configs[static_cast<std::size_t>(index)]);
        }
    });
    AppendSettingsRow(*body, queryLower, "Build Configuration", "Preferred editor launch configuration", config);

    auto enginePath = std::make_shared<SettingsTextField>(
        settings.selectedEngineRoot.empty() ? "Auto-detect" : settings.selectedEngineRoot,
        280.0f);
    enginePath->SetOnChanged([this](const std::string& text) {
        if (text == "Auto-detect") {
            m_Context->Settings().Settings().selectedEngineRoot.clear();
        } else {
            m_Context->Settings().Settings().selectedEngineRoot = text;
        }
        PersistLauncherSettings("Selected engine path updated");
    });
    AppendSettingsRow(*body, queryLower, "Selected Engine", "Override auto-detected engine install", enginePath);

    if (auto group = WrapSettingsGroup("Projects", "Folders and build defaults", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsTemplates(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto& settings = m_Context->Settings().Settings();
    auto body = MakeSettingsStack();

    std::vector<std::string> ids{ "Blank" };
    const auto& templates = m_Context->Templates().Templates();
    for (const auto& tmpl : templates) {
        if (std::find(ids.begin(), ids.end(), tmpl.id) == ids.end()) {
            ids.push_back(tmpl.id);
        }
    }
    auto dropdown = std::make_shared<SettingsDropdown>(ids, IndexOfOption(ids, settings.defaultTemplateId));
    dropdown->SetOnChanged([this, ids](int index) {
        if (index >= 0 && index < static_cast<int>(ids.size())) {
            m_Context->Settings().Settings().defaultTemplateId = ids[static_cast<std::size_t>(index)];
            m_WizardTemplateId = ids[static_cast<std::size_t>(index)];
            PersistLauncherSettings("Default template: " + ids[static_cast<std::size_t>(index)]);
        }
    });
    AppendSettingsRow(*body, queryLower, "Default Template", "Pre-selected template in Create Project", dropdown);

    auto open = std::make_shared<SecondaryToolbarButton>("Browse Templates", Icons::LayersName);
    open->SetOnClicked([this] { GoToPage(LauncherPage::Templates); });
    AppendSettingsRow(*body, queryLower, "Template Library", "Open the full template browser", open);

    if (auto group = WrapSettingsGroup("Templates", "Starter packs and defaults", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsEngine(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto body = MakeSettingsStack();

    std::string version = "Not detected";
    std::string install = "â€”";
    std::string sdk = "â€”";
    std::string plugins = "0";
    std::string disk = "â€”";
    std::string buildConfig = m_Context->Settings().Settings().lastBuildConfig;

    if (m_Context->Engines().HasEngine()) {
        const auto& engine = m_Context->Engines().Current();
        version = engine.engineVersion.empty() ? "Unknown" : engine.engineVersion;
        install = PathUtils::ToUtf8(engine.engineRoot);
        int pass = 0, warn = 0, fail = 0;
        for (const auto& check : m_Context->Sdk().RunChecks()) {
            switch (check.status) {
            case SdkCheckStatus::Pass: ++pass; break;
            case SdkCheckStatus::Warn: ++warn; break;
            case SdkCheckStatus::Fail: ++fail; break;
            }
        }
        sdk = std::to_string(pass) + " ok Â· " + std::to_string(warn) + " warn Â· " + std::to_string(fail) + " fail";
        plugins = std::to_string(m_Context->Engines().InstalledEngines().size());
        disk = "Local install";
    }

    auto addInfo = [&](const std::string& label, const std::string& value, const std::string& keys) {
        auto valueLabel = MakeLabel(value, LMetric(ThemeToken::TextSizeBody) * LScale(), LColor(ThemeToken::TextPrimary));
        AppendSettingsRow(*body, queryLower, label, {}, valueLabel, keys);
    };
    addInfo("Engine Version", version, "engine version");
    addInfo("Install Location", EllipsizePath(install, 48), "install path folder");
    addInfo("SDK Status", sdk, "sdk");
    addInfo("Plugin Count", plugins, "plugins");
    addInfo("Disk Usage", disk, "disk");
    addInfo("Build Configuration", buildConfig, "build");

    auto actions = std::make_shared<SettingsActionBar>();
    actions->AddAction("Launch Editor", Icons::PlayName, [this] {
        SetStatus("Launch Editor from Settings â€” open a project or use Engine page");
        GoToPage(LauncherPage::Engine);
    }, true);
    actions->AddAction("Verify Installation", Icons::CheckName, [this] {
        UpdateFooter();
        SetStatus("Installation verified");
    });
    actions->AddAction("Repair", Icons::BuildName, [this] {
        SetStatus("Repair queued â€” run from a full engine install");
    });
    actions->AddAction("Open Installation Folder", Icons::OpenFolderName, [this] {
        if (m_Context->Engines().HasEngine()) {
            const std::string root = PathUtils::ToUtf8(m_Context->Engines().Current().engineRoot);
            if (RevealInExplorer(root)) {
                SetStatus("Opened engine install folder");
            }
        } else {
            SetStatus("No engine install detected");
        }
    });
    if (queryLower.empty()
        || queryLower.find("launch") != std::string::npos
        || queryLower.find("verify") != std::string::npos
        || queryLower.find("repair") != std::string::npos
        || queryLower.find("folder") != std::string::npos) {
        body->AddChild(actions);
    }

    if (auto group = WrapSettingsGroup("Engine", "Install details and maintenance", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsAppearance(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto& settings = m_Context->Settings().Settings();

    auto body = MakeSettingsStack();

    auto preview = std::make_shared<AppearancePreviewPanel>();
    preview->SetTheme(settings.theme);
    preview->SetAccentHex(settings.accentColor);
    preview->SetIconStyle(settings.iconStyle);
    preview->SetUiScale(settings.uiScale);
    preview->SetFontSize(settings.fontSize);
    if (queryLower.empty()
        || queryLower.find("theme") != std::string::npos
        || queryLower.find("accent") != std::string::npos
        || queryLower.find("preview") != std::string::npos
        || queryLower.find("appearance") != std::string::npos) {
        body->AddChild(preview);
    }

    auto theme = std::make_shared<SettingsDropdown>(
        std::vector<std::string>{ "Graphite Dark", "Graphite Light", "High Contrast" },
        IndexOfOption({ "Graphite Dark", "Graphite Light", "High Contrast" }, settings.theme));
    theme->SetOnChanged([this](int index) {
        static const char* kThemes[] = { "Graphite Dark", "Graphite Light", "High Contrast" };
        if (index >= 0 && index < 3) {
            m_Context->Settings().Settings().theme = kThemes[index];
            PersistLauncherSettings("Theme: " + m_Context->Settings().Settings().theme);
            MarkPageDirty(LauncherPage::Settings);
            EnsurePageBuilt(LauncherPage::Settings);
            ShowPage(LauncherPage::Settings);
        }
    });
    AppendSettingsRow(*body, queryLower, "Theme", "Overall color scheme", theme);

    auto accent = std::make_shared<ColorSwatchPicker>(settings.accentColor);
    accent->SetOnChanged([this](const std::string& hex) {
        m_Context->Settings().Settings().accentColor = hex;
        PersistLauncherSettings("Accent color updated");
        MarkPageDirty(LauncherPage::Settings);
        EnsurePageBuilt(LauncherPage::Settings);
        ShowPage(LauncherPage::Settings);
    });
    AppendSettingsRow(*body, queryLower, "Accent Color", "Highlights and primary actions", accent);

    auto icons = std::make_shared<SettingsDropdown>(
        std::vector<std::string>{ "Outline", "Filled", "Duotone" },
        IndexOfOption({ "Outline", "Filled", "Duotone" }, settings.iconStyle));
    icons->SetOnChanged([this](int index) {
        static const char* kStyles[] = { "Outline", "Filled", "Duotone" };
        if (index >= 0 && index < 3) {
            m_Context->Settings().Settings().iconStyle = kStyles[index];
            PersistLauncherSettings("Icon style: " + m_Context->Settings().Settings().iconStyle);
            MarkPageDirty(LauncherPage::Settings);
            EnsurePageBuilt(LauncherPage::Settings);
            ShowPage(LauncherPage::Settings);
        }
    });
    AppendSettingsRow(*body, queryLower, "Icon Style", "Atlas icon rendering preference", icons);

    auto scale = std::make_shared<SettingsDropdown>(
        std::vector<std::string>{ "100%", "110%", "125%", "150%" },
        IndexOfOption(
            { "100%", "110%", "125%", "150%" },
            std::to_string(static_cast<int>(settings.uiScale * 100.0f + 0.5f)) + "%"));
    scale->SetOnChanged([this](int index) {
        static const float kScales[] = { 1.0f, 1.1f, 1.25f, 1.5f };
        if (index >= 0 && index < 4) {
            m_Context->Settings().Settings().uiScale = kScales[index];
            PersistLauncherSettings("UI scale updated");
            MarkPageDirty(LauncherPage::Settings);
            EnsurePageBuilt(LauncherPage::Settings);
            ShowPage(LauncherPage::Settings);
        }
    });
    AppendSettingsRow(*body, queryLower, "UI Scale", "Interface density and control size", scale);

    auto font = std::make_shared<NumberStepper>(settings.fontSize, 11.0f, 18.0f, 1.0f, "px");
    font->SetOnChanged([this](float value) {
        m_Context->Settings().Settings().fontSize = value;
        PersistLauncherSettings("Font size updated");
        MarkPageDirty(LauncherPage::Settings);
        EnsurePageBuilt(LauncherPage::Settings);
        ShowPage(LauncherPage::Settings);
    });
    AppendSettingsRow(*body, queryLower, "Font Size", "Base text size for launcher UI", font);

    if (auto group = WrapSettingsGroup("Appearance", "Theme, accent, scale, and typography", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsUpdates(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto body = MakeSettingsStack();

    std::string engineVer = "â€”";
    if (m_Context->Engines().HasEngine()) {
        engineVer = m_Context->Engines().Current().engineVersion;
        if (engineVer.empty()) {
            engineVer = "Unknown";
        }
    }
    auto addInfo = [&](const std::string& label, const std::string& value) {
        AppendSettingsRow(
            *body,
            queryLower,
            label,
            {},
            MakeLabel(value, LMetric(ThemeToken::TextSizeBody) * LScale(), LColor(ThemeToken::TextPrimary)));
    };
    addInfo("Launcher Version", "1.0.0");
    addInfo("Engine Version", engineVer);
    addInfo("SDK Version", "Bundled");
    addInfo("Plugin Updates", "None pending");

    auto actions = std::make_shared<SettingsActionBar>();
    actions->AddAction("Check for Updates", Icons::RefreshName, [this] {
        SetStatus("Checked for updates â€” you are up to date");
    }, true);
    actions->AddAction("Update", Icons::RecentName, [this] {
        SetStatus("No updates available");
    });
    actions->AddAction("Repair", Icons::BuildName, [this] {
        SetStatus("Repair channel ready");
    });
    actions->AddAction("Verify", Icons::CheckName, [this] {
        SetStatus("Update channel verified");
    });
    if (queryLower.empty()
        || queryLower.find("update") != std::string::npos
        || queryLower.find("check") != std::string::npos
        || queryLower.find("verify") != std::string::npos
        || queryLower.find("repair") != std::string::npos) {
        body->AddChild(actions);
    }

    auto toggle = std::make_shared<ToggleSwitch>(m_Context->Settings().Settings().checkUpdatesOnStartup);
    toggle->SetOnChanged([this](bool on) {
        m_Context->Settings().Settings().checkUpdatesOnStartup = on;
        PersistLauncherSettings("Update preferences saved");
    });
    AppendSettingsRow(*body, queryLower, "Automatic Checks", "Check when the launcher starts", toggle);

    if (auto group = WrapSettingsGroup("Updates", "Versions and update actions", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsCache(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto body = MakeSettingsStack();

    auto addBar = [&](const std::string& label, float used, float cap, const std::string& keys) {
        if (!queryLower.empty() && ToLowerCopy(label + " " + keys).find(queryLower) == std::string::npos) {
            return;
        }
        body->AddChild(std::make_shared<CacheUsageBar>(label, used, cap));
    };
    addBar("Thumbnail Cache", 48.0f, 256.0f, "thumbnail");
    addBar("Shader Cache", 312.0f, 1024.0f, "shader");
    addBar("Derived Data Cache", 128.0f, 2048.0f, "derived ddc");
    addBar("Temporary Files", 22.0f, 512.0f, "temp temporary");

    auto actions = std::make_shared<SettingsActionBar>();
    actions->AddAction("Clear Cache", Icons::XName, [this] {
        SetStatus("Cache clear requested");
    }, true);
    actions->AddAction("Rebuild Cache", Icons::RefreshName, [this] {
        SetStatus("Cache rebuild queued");
    });
    actions->AddAction("Open Cache Folder", Icons::OpenFolderName, [this] {
        const auto path = PathUtils::ToUtf8(PathUtils::GetLauncherSettingsPath().parent_path());
        if (RevealInExplorer(path)) {
            SetStatus("Opened cache / settings folder");
        }
    });
    if (queryLower.empty()
        || queryLower.find("clear") != std::string::npos
        || queryLower.find("rebuild") != std::string::npos
        || queryLower.find("folder") != std::string::npos
        || queryLower.find("cache") != std::string::npos) {
        body->AddChild(actions);
    }

    if (body->GetChildren().empty()) {
        return root;
    }
    auto group = std::make_shared<SettingsGroup>("Cache", "Disk usage for generated data");
    if (!queryLower.empty()) {
        group->SetHighlighted(true);
    }
    group->SetContent(body);
    root->AddChild(group);
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsDownloads(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto body = MakeSettingsStack();

    auto metered = std::make_shared<ToggleSwitch>(m_Context->Settings().Settings().downloadOverMetered);
    metered->SetOnChanged([this](bool on) {
        m_Context->Settings().Settings().downloadOverMetered = on;
        PersistLauncherSettings("Download preference saved");
    });
    AppendSettingsRow(*body, queryLower, "Allow Metered Networks", "Download packages on cellular / metered links", metered);

    auto openLib = std::make_shared<SecondaryToolbarButton>("Open Library", Icons::Cube3DName);
    openLib->SetOnClicked([this] { GoToPage(LauncherPage::Library); });
    AppendSettingsRow(*body, queryLower, "Download History", "Managed packages appear on the Library page", openLib);

    if (auto group = WrapSettingsGroup("Downloads", "Network and package preferences", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsPlugins(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto body = MakeSettingsStack();

    AppendSettingsRow(
        *body,
        queryLower,
        "Installed Plugins",
        "Managed from the Engine page",
        MakeLabel(
            m_Context->Engines().HasEngine() ? "See Engine â†’ Plugins" : "No engine detected",
            LMetric(ThemeToken::TextSizeBody) * LScale(),
            LColor(ThemeToken::TextPrimary)),
        "plugins extensions");

    auto open = std::make_shared<SecondaryToolbarButton>("Open Engine Plugins", Icons::ComponentName);
    open->SetOnClicked([this] { GoToPage(LauncherPage::Engine); });
    AppendSettingsRow(*body, queryLower, "Plugin Manager", "Browse and enable editor plugins", open);

    if (auto group = WrapSettingsGroup("Plugins", "Editor and runtime extensions", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsDeveloper(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto& settings = m_Context->Settings().Settings();
    auto body = MakeSettingsStack();

    auto bindToggle = [this](bool& field, const std::string& msg) {
        auto toggle = std::make_shared<ToggleSwitch>(field);
        toggle->SetOnChanged([this, &field, msg](bool on) {
            field = on;
            PersistLauncherSettings(msg);
        });
        return toggle;
    };

    // Careful: capturing references to settings fields that live in m_Context is OK
    AppendSettingsRow(*body, queryLower, "Developer Mode", "Unlock advanced diagnostics",
        bindToggle(settings.developerMode, "Developer mode updated"), "dev");
    AppendSettingsRow(*body, queryLower, "GPU Validation", "Enable GPU API validation layers",
        bindToggle(settings.gpuValidation, "GPU validation updated"), "vulkan validation");
    AppendSettingsRow(*body, queryLower, "Render Graph Debug", "Visualize render graph passes",
        bindToggle(settings.renderGraphDebug, "Render graph debug updated"), "rg");
    AppendSettingsRow(*body, queryLower, "ECS Debug", "Entity component system overlays",
        bindToggle(settings.ecsDebug, "ECS debug updated"), "entity");
    AppendSettingsRow(*body, queryLower, "Crash Dumps", "Write crash dumps on failure",
        bindToggle(settings.crashDumps, "Crash dump preference saved"), "minidump");

    auto rhi = std::make_shared<SettingsDropdown>(
        std::vector<std::string>{ "Vulkan", "DirectX 12", "Null" },
        IndexOfOption({ "Vulkan", "DirectX 12", "Null" }, settings.rhiBackend));
    rhi->SetOnChanged([this](int index) {
        static const char* kBackends[] = { "Vulkan", "DirectX 12", "Null" };
        if (index >= 0 && index < 3) {
            m_Context->Settings().Settings().rhiBackend = kBackends[index];
            PersistLauncherSettings("RHI backend: " + m_Context->Settings().Settings().rhiBackend);
        }
    });
    AppendSettingsRow(*body, queryLower, "RHI Backend", "Preferred rendering backend", rhi);

    auto log = std::make_shared<SettingsDropdown>(
        std::vector<std::string>{ "Error", "Warn", "Info", "Verbose", "Debug" },
        IndexOfOption({ "Error", "Warn", "Info", "Verbose", "Debug" }, settings.loggingLevel));
    log->SetOnChanged([this](int index) {
        static const char* kLevels[] = { "Error", "Warn", "Info", "Verbose", "Debug" };
        if (index >= 0 && index < 5) {
            m_Context->Settings().Settings().loggingLevel = kLevels[index];
            PersistLauncherSettings("Logging level: " + m_Context->Settings().Settings().loggingLevel);
        }
    });
    AppendSettingsRow(*body, queryLower, "Logging Level", "Console and file verbosity", log);

    if (auto group = WrapSettingsGroup("Developer", "Diagnostics and low-level options", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsExperimental(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto body = MakeSettingsStack();

    auto show = std::make_shared<ToggleSwitch>(m_Context->Settings().Settings().showExperimental);
    show->SetOnChanged([this](bool on) {
        m_Context->Settings().Settings().showExperimental = on;
        PersistLauncherSettings(on ? "Experimental features visible" : "Experimental features hidden");
    });
    AppendSettingsRow(*body, queryLower, "Show Experimental Features", "Surface unfinished tools in the launcher", show);

    body->AddChild(MakeLabel(
        "Experimental options may change or be removed without notice.",
        LMetric(ThemeToken::TextSizeCaption) * LScale(),
        LColor(ThemeToken::Warning)));

    if (auto group = WrapSettingsGroup("Experimental", "Preview features under active development", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

void LauncherShell::RebuildSettingsPage() {
    auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Settings)];

    auto page = std::make_shared<VerticalBox>();
    page->SetSpacing(0.0f);
    page->SetHorizontalAlignment(HorizontalAlignment::Fill);
    page->SetVerticalAlignment(VerticalAlignment::Fill);
    page->SetBackgroundColor(LColor(ThemeToken::WindowBackground));

    if (IsPageContentLoading()) {
        auto scroll = std::make_shared<ScrollLayout>();
        auto content = MakePageBodyPadding();
        content->AddChild(BuildPageSkeleton(LauncherPage::Settings));
        scroll->SetContent(content);
        page->AddChild(scroll);
        state.root = page;
        state.scroll = scroll;
        m_SettingsScrollTarget.reset();
        m_SettingsPendingScroll = false;
        return;
    }

    const float s = LScale();
    const std::string queryLower = ToLowerCopy(m_SearchQuery);

    auto contentScroll = std::make_shared<ScrollLayout>();
    auto content = std::make_shared<VerticalBox>();
    content->SetSpacing(16.0f * s);
    content->SetPadding(Margin{ 24.0f * s, 16.0f * s, 24.0f * s, 24.0f * s });
    content->SetHorizontalAlignment(HorizontalAlignment::Fill);

    content->AddChild(MakeLabel(
        "Settings",
        LMetric(ThemeToken::TextSizeHeader) * s,
        LColor(ThemeToken::TextPrimary)));
    content->AddChild(MakeLabel(
        queryLower.empty()
            ? "Preferences for WindEffects Launcher"
            : ("Showing matches for \"" + m_SearchQuery + "\""),
        LMetric(ThemeToken::TextSizeCaption) * s,
        LColor(ThemeToken::TextMuted)));

    m_SettingsScrollTarget.reset();
    m_SettingsPendingScroll = false;

    auto addSection = [&](SettingsCategory category, auto builder) {
        auto section = builder(queryLower);
        if ((!section || section->GetChildren().empty()) && !queryLower.empty()
            && SettingsCategoryMatchesQuery(category, queryLower)) {
            section = builder(std::string{});
            HighlightSettingsSubtree(section);
        }
        if (!section || section->GetChildren().empty()) {
            return;
        }
        if (!queryLower.empty() && !m_SettingsScrollTarget) {
            m_SettingsScrollTarget = section->GetChildren().front();
            m_SettingsPendingScroll = true;
        }
        content->AddChild(section);
    };

    addSection(SettingsCategory::General, [this](const std::string& q) { return BuildSettingsGeneral(q); });
    addSection(SettingsCategory::Projects, [this](const std::string& q) { return BuildSettingsProjects(q); });
    addSection(SettingsCategory::Templates, [this](const std::string& q) { return BuildSettingsTemplates(q); });
    addSection(SettingsCategory::Engine, [this](const std::string& q) { return BuildSettingsEngine(q); });
    addSection(SettingsCategory::Appearance, [this](const std::string& q) { return BuildSettingsAppearance(q); });
    addSection(SettingsCategory::Downloads, [this](const std::string& q) { return BuildSettingsDownloads(q); });
    addSection(SettingsCategory::Cache, [this](const std::string& q) { return BuildSettingsCache(q); });
    addSection(SettingsCategory::Updates, [this](const std::string& q) { return BuildSettingsUpdates(q); });
    addSection(SettingsCategory::Plugins, [this](const std::string& q) { return BuildSettingsPlugins(q); });
    addSection(SettingsCategory::Developer, [this](const std::string& q) { return BuildSettingsDeveloper(q); });
    addSection(SettingsCategory::Experimental, [this](const std::string& q) { return BuildSettingsExperimental(q); });

    if (!queryLower.empty() && content->GetChildren().size() <= 2) {
        content->AddChild(MakeLabel(
            "No settings match your search.",
            LMetric(ThemeToken::TextSizeBody) * s,
            LColor(ThemeToken::TextMuted)));
    }

    contentScroll->SetContent(content);
    contentScroll->SetHorizontalAlignment(HorizontalAlignment::Fill);
    contentScroll->SetVerticalAlignment(VerticalAlignment::Fill);
    page->AddChild(contentScroll);

    state.root = page;
    state.scroll = contentScroll;
}

void LauncherShell::RebuildCreateWizard() {
    if (!m_ModalHost) {
        return;
    }
    m_ModalHost->SetVisible(true);
    m_ModalHost->SetDialogWidth(520.0f);

    auto panel = std::make_shared<VerticalBox>();
    panel->SetPadding(Margin{
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale()
    });
    panel->SetSpacing(LMetric(ThemeToken::Space3) * LScale());
    panel->SetBackgroundColor(LColor(ThemeToken::DialogBackground));

    panel->AddChild(MakeLabel(
        "Create New Project",
        LMetric(ThemeToken::TextSizeHeader) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    panel->AddChild(MakeLabel(
        "Choose a template and destination.",
        LMetric(ThemeToken::TextSizeBody) * LScale(),
        LColor(ThemeToken::TextMuted)));

    auto templateRow = std::make_shared<HorizontalBox>();
    templateRow->SetSpacing(LMetric(ThemeToken::Space2) * LScale());
    for (const auto& tmpl : m_Context->Templates().Templates()) {
        auto btn = std::make_shared<SecondaryToolbarButton>(tmpl.displayName, Icons::LayersName);
        btn->SetOnClicked([this, id = tmpl.id] {
            m_WizardTemplateId = id;
            RebuildCreateWizard();
            SetStatus("Template: " + id);
        });
        templateRow->AddChild(btn);
    }
    panel->AddChild(templateRow);
    panel->AddChild(MakeLabel(
        "Selected: " + m_WizardTemplateId,
        LMetric(ThemeToken::TextSizeCaption) * LScale(),
        LColor(ThemeToken::AccentPrimary)));

    panel->AddChild(MakeLabel(
        "Project Name",
        LMetric(ThemeToken::TextSizeCaption) * LScale(),
        LColor(ThemeToken::TextMuted)));
    auto nameBox = std::make_shared<TextBox>(m_WizardName, [this](const std::string& text) {
        m_WizardName = text;
    });
    panel->AddChild(nameBox);

    panel->AddChild(MakeLabel(
        "Location",
        LMetric(ThemeToken::TextSizeCaption) * LScale(),
        LColor(ThemeToken::TextMuted)));
    auto locationBox = std::make_shared<TextBox>(m_WizardLocation, [this](const std::string& text) {
        m_WizardLocation = text;
    });
    panel->AddChild(locationBox);

    auto buttons = std::make_shared<HorizontalBox>();
    buttons->SetSpacing(LMetric(ThemeToken::Space2) * LScale());
    auto cancel = std::make_shared<SecondaryToolbarButton>("Cancel", "");
    cancel->SetOnClicked([this] { CloseModal(); });
    buttons->AddChild(cancel);
    buttons->AddChild(std::make_shared<Spacer>());
    auto create = std::make_shared<PrimaryToolbarButton>("Create", Icons::PlusName);
    create->SetOnClicked([this] {
        const auto result = m_Context->Projects().CreateProject(
            m_WizardName,
            m_WizardTemplateId,
            PathUtils::FromUtf8(m_WizardLocation));
        SetStatus(result.message);
        if (result.success) {
            CloseModal();
            RefreshProjectList();
        }
    });
    buttons->AddChild(create);
    panel->AddChild(buttons);

    m_ModalHost->SetDialog(panel);
    InvalidateUI();
}

void LauncherShell::RebuildRenameDialog() {
    if (!m_ModalHost) {
        return;
    }
    m_ModalHost->SetVisible(true);
    m_ModalHost->SetDialogWidth(420.0f);

    auto panel = std::make_shared<VerticalBox>();
    panel->SetPadding(Margin{
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale()
    });
    panel->SetSpacing(LMetric(ThemeToken::Space3) * LScale());
    panel->SetBackgroundColor(LColor(ThemeToken::DialogBackground));
    panel->AddChild(MakeLabel(
        "Rename Project",
        LMetric(ThemeToken::TextSizeHeader) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    panel->AddChild(MakeLabel(
        "Enter a new display name.",
        LMetric(ThemeToken::TextSizeBody) * LScale(),
        LColor(ThemeToken::TextMuted)));
    auto nameBox = std::make_shared<TextBox>(m_RenameName, [this](const std::string& text) {
        m_RenameName = text;
    });
    panel->AddChild(nameBox);

    auto buttons = std::make_shared<HorizontalBox>();
    buttons->SetSpacing(LMetric(ThemeToken::Space2) * LScale());
    auto cancel = std::make_shared<SecondaryToolbarButton>("Cancel", "");
    cancel->SetOnClicked([this] { CloseModal(); });
    buttons->AddChild(cancel);
    buttons->AddChild(std::make_shared<Spacer>());
    auto rename = std::make_shared<PrimaryToolbarButton>("Rename", Icons::CheckName);
    rename->SetOnClicked([this] {
        RenameSelectedProject(m_RenameName);
        CloseModal();
    });
    buttons->AddChild(rename);
    panel->AddChild(buttons);

    m_ModalHost->SetDialog(panel);
    InvalidateUI();
}

void LauncherShell::RebuildProjectActionsDialog() {
    if (!m_ModalHost) {
        return;
    }
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        CloseModal();
        return;
    }

    const auto& project = m_Projects[static_cast<std::size_t>(m_SelectedIndex)];
    m_ModalHost->SetVisible(true);
    m_ModalHost->SetDialogWidth(420.0f);

    auto panel = std::make_shared<VerticalBox>();
    panel->SetPadding(Margin{
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale()
    });
    panel->SetSpacing(LMetric(ThemeToken::Space2) * LScale());
    panel->SetBackgroundColor(LColor(ThemeToken::DialogBackground));
    panel->AddChild(MakeLabel(
        project.descriptor.displayName,
        LMetric(ThemeToken::TextSizeHeader) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    panel->AddChild(MakeLabel(
        EllipsizePath(project.projectRoot, 52),
        LMetric(ThemeToken::TextSizeCaption) * LScale(),
        LColor(ThemeToken::TextMuted)));

    auto addAction = [this, &panel](const char* label, const char* icon, auto fn, bool closeAfter = true) {
        auto btn = std::make_shared<SecondaryToolbarButton>(label, icon);
        btn->SetOnClicked([this, fn = std::move(fn), closeAfter] {
            fn();
            if (closeAfter) {
                CloseModal();
            }
        });
        panel->AddChild(btn);
    };

    addAction("Open", Icons::PlayName, [this] { OpenSelectedProject(); });
    addAction("Clone", Icons::CopyName, [this] { CloneSelectedProject(); });
    addAction("Rename", Icons::DocumentName, [this] { ShowRenameDialog(); }, false);
    addAction(
        IsFavorite(project.weprojPath) ? "Unpin" : "Pin",
        Icons::StarName,
        [this, path = project.weprojPath] { ToggleFavorite(path); });
    addAction("Show in Explorer", Icons::OpenFolderName, [this] { ShowSelectedInExplorer(); });
    addAction("Regenerate Project Files", Icons::BuildName, [this] { RegenerateSelectedProjectFiles(); });
    addAction("Delete", Icons::DeleteName, [this] { DeleteSelectedProject(); });

    auto closeRow = std::make_shared<HorizontalBox>();
    closeRow->AddChild(std::make_shared<Spacer>());
    auto close = std::make_shared<SecondaryToolbarButton>("Close", "");
    close->SetOnClicked([this] { CloseModal(); });
    closeRow->AddChild(close);
    panel->AddChild(closeRow);

    m_ModalHost->SetDialog(panel);
    InvalidateUI();
}

void LauncherShell::UpdateFooter() {
    if (!m_Footer) {
        return;
    }
    if (m_Context->Engines().HasEngine()) {
        m_Footer->SetEngineLabel("Engine " + m_Context->Engines().Current().engineVersion);
    } else {
        m_Footer->SetEngineLabel("No engine");
    }

    int pass = 0;
    int fail = 0;
    int warn = 0;
    for (const auto& check : m_Context->Sdk().RunChecks()) {
        switch (check.status) {
        case SdkCheckStatus::Pass: ++pass; break;
        case SdkCheckStatus::Warn: ++warn; break;
        case SdkCheckStatus::Fail: ++fail; break;
        }
    }
    m_Footer->SetSdkSummary(
        "SDK " + std::to_string(pass) + " ok"
        + (warn ? (", " + std::to_string(warn) + " warn") : "")
        + (fail ? (", " + std::to_string(fail) + " fail") : ""));
}

void LauncherShell::UpdateAdaptiveLayout(float width) {
    const bool collapse = width < 1100.0f * LScale();
    if (collapse != m_SidebarCollapsed && m_Sidebar) {
        m_SidebarCollapsed = collapse;
        m_Sidebar->SetCollapsed(collapse);
        InvalidateUI();
    }
}

void LauncherShell::RefreshProjectList() {
    m_Projects = m_Context->Projects().LoadRecentSummaries();
    ApplySearchFilter();
    if (m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        m_SelectedIndex = m_Projects.empty() ? -1 : 0;
    }
    UpdateFooter();
    BeginPageContentLoad(0.24f);
    MarkPageDirty(LauncherPage::Projects);
    if (m_Page == LauncherPage::Projects) {
        EnsurePageBuilt(LauncherPage::Projects);
        ShowPage(LauncherPage::Projects);
    }
    InvalidateUI();
}

void LauncherShell::ApplySearchFilter() {
    m_FilteredProjects.clear();
    const std::string query = ToLowerCopy(m_SearchQuery);
    for (const auto& project : m_Projects) {
        if (m_CompatibleOnly && !project.compatible) {
            continue;
        }
        if (!query.empty()) {
            const std::string hay = ToLowerCopy(
                project.descriptor.displayName + " " + project.projectRoot + " "
                + project.descriptor.templateId + " " + project.descriptor.engineVersion);
            if (hay.find(query) == std::string::npos) {
                continue;
            }
        }
        m_FilteredProjects.push_back(project);
    }
    ApplyProjectSort();
}

void LauncherShell::ApplyProjectSort() {
    switch (m_SortMode) {
    case ProjectSortMode::Name:
        std::stable_sort(
            m_FilteredProjects.begin(),
            m_FilteredProjects.end(),
            [](const ProjectSummary& a, const ProjectSummary& b) {
                return ToLowerCopy(a.descriptor.displayName) < ToLowerCopy(b.descriptor.displayName);
            });
        break;
    case ProjectSortMode::Engine:
        std::stable_sort(
            m_FilteredProjects.begin(),
            m_FilteredProjects.end(),
            [](const ProjectSummary& a, const ProjectSummary& b) {
                if (a.descriptor.engineVersion != b.descriptor.engineVersion) {
                    return a.descriptor.engineVersion < b.descriptor.engineVersion;
                }
                return ToLowerCopy(a.descriptor.displayName) < ToLowerCopy(b.descriptor.displayName);
            });
        break;
    case ProjectSortMode::Recent:
    default:
        std::stable_sort(
            m_FilteredProjects.begin(),
            m_FilteredProjects.end(),
            [](const ProjectSummary& a, const ProjectSummary& b) {
                return a.descriptor.lastOpenedUtc > b.descriptor.lastOpenedUtc;
            });
        break;
    }
}

void LauncherShell::SetStatus(const std::string& message) {
    m_Context->SetStatusMessage(message);
    if (m_Footer) {
        m_Footer->SetStatus(message);
    }
    InvalidateUI();
}

void LauncherShell::SelectProject(std::size_t index, bool additive) {
    if (m_Projects.empty() || index >= m_Projects.size()) {
        m_SelectedIndex = -1;
        m_MultiSelected.clear();
        return;
    }

    if (additive) {
        const auto it = std::find(m_MultiSelected.begin(), m_MultiSelected.end(), index);
        if (it != m_MultiSelected.end()) {
            m_MultiSelected.erase(it);
            if (m_SelectedIndex == static_cast<int>(index)) {
                m_SelectedIndex = m_MultiSelected.empty()
                    ? -1
                    : static_cast<int>(m_MultiSelected.back());
            }
        } else {
            m_MultiSelected.push_back(index);
            m_SelectedIndex = static_cast<int>(index);
        }
    } else {
        if (m_SelectedIndex == static_cast<int>(index) && m_MultiSelected.size() <= 1) {
            // Keep selection; still refresh details if needed.
        }
        m_SelectedIndex = static_cast<int>(index);
        m_MultiSelected.clear();
        m_MultiSelected.push_back(index);
    }

    if (m_SelectedIndex >= 0) {
        SetStatus("Selected " + m_Projects[static_cast<std::size_t>(m_SelectedIndex)].descriptor.displayName);
    }
    MarkPageDirty(LauncherPage::Projects);
    if (m_Page == LauncherPage::Projects) {
        EnsurePageBuilt(LauncherPage::Projects);
        ShowPage(LauncherPage::Projects);
    }
}

void LauncherShell::HandleProjectAction(std::size_t index, ProjectCardAction action) {
    SelectProject(index, false);
    switch (action) {
    case ProjectCardAction::Open:
        OpenSelectedProject();
        break;
    case ProjectCardAction::Clone:
        CloneSelectedProject();
        break;
    case ProjectCardAction::Rename:
        ShowRenameDialog();
        break;
    case ProjectCardAction::Delete:
        DeleteSelectedProject();
        break;
    case ProjectCardAction::ShowInExplorer:
        ShowSelectedInExplorer();
        break;
    case ProjectCardAction::Favorite:
        if (index < m_Projects.size()) {
            ToggleFavorite(m_Projects[index].weprojPath);
        }
        break;
    case ProjectCardAction::More:
        ShowProjectMoreMenu(index);
        break;
    case ProjectCardAction::Regenerate:
        RegenerateSelectedProjectFiles();
        break;
    }
}

void LauncherShell::RegenerateSelectedProjectFiles() {
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        SetStatus("No project selected.");
        return;
    }
    const auto& project = m_Projects[static_cast<std::size_t>(m_SelectedIndex)];
    SetStatus("Regenerate project files queued for " + project.descriptor.displayName);
}

void LauncherShell::ShowCreateWizard() {
    m_Modal = ModalKind::Create;
    RebuildCreateWizard();
}

void LauncherShell::ShowRenameDialog() {
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        SetStatus("No project selected.");
        return;
    }
    m_RenameName = m_Projects[static_cast<std::size_t>(m_SelectedIndex)].descriptor.displayName;
    m_Modal = ModalKind::Rename;
    RebuildRenameDialog();
}

void LauncherShell::CloseModal() {
    m_Modal = ModalKind::None;
    if (m_ModalHost) {
        m_ModalHost->SetDialog(nullptr);
        m_ModalHost->SetVisible(false);
    }
    InvalidateUI();
}

void LauncherShell::BrowseForProject() {
    auto& platform = we::platform::Platform::Get();
    we::platform::FileDialogDesc desc{};
    desc.mode = we::platform::FileDialogMode::OpenFile;
    desc.title = "Open WindEffects Project";
    desc.filters = { { "WindEffects Project", "*.weproj" }, { "All Files", "*.*" } };

    const auto paths = platform.ShowFileDialog(desc);
    if (paths.empty()) {
        return;
    }

    const auto launch = m_Context->EditorLaunch().Launch(PathUtils::FromUtf8(paths[0]));
    SetStatus(launch.message);
    RefreshProjectList();
}

void LauncherShell::OpenSelectedProject() {
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        SetStatus("No project selected.");
        return;
    }
    const auto launch = m_Context->EditorLaunch().Launch(
        PathUtils::FromUtf8(m_Projects[static_cast<std::size_t>(m_SelectedIndex)].weprojPath));
    SetStatus(launch.message);
    RefreshProjectList();
}

void LauncherShell::CloneSelectedProject() {
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        return;
    }
    const auto& selected = m_Projects[static_cast<std::size_t>(m_SelectedIndex)];
    const auto result = m_Context->Projects().CloneProject(
        PathUtils::FromUtf8(selected.weprojPath),
        selected.descriptor.displayName + "_Copy");
    SetStatus(result.message);
    RefreshProjectList();
}

void LauncherShell::RenameSelectedProject(const std::string& newName) {
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        return;
    }
    if (newName.empty()) {
        SetStatus("Name cannot be empty.");
        return;
    }
    const auto& selected = m_Projects[static_cast<std::size_t>(m_SelectedIndex)];
    const auto result = m_Context->Projects().RenameProject(
        PathUtils::FromUtf8(selected.weprojPath),
        newName);
    SetStatus(result.message);
    RefreshProjectList();
}

void LauncherShell::DeleteSelectedProject() {
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        return;
    }
    const auto& selected = m_Projects[static_cast<std::size_t>(m_SelectedIndex)];

    auto& platform = we::platform::Platform::Get();
    const auto confirm = platform.ShowMessageBox({
        .title = "Delete Project",
        .message = ("Delete project and all files?\n" + selected.weprojPath).c_str(),
        .type = we::platform::MessageBoxType::Warning,
        .yesNo = true,
    });
    if (confirm != we::platform::MessageBoxResult::Yes) {
        return;
    }

    const auto result = m_Context->Projects().DeleteProject(PathUtils::FromUtf8(selected.weprojPath));
    SetStatus(result.message);
    m_SelectedIndex = -1;
    RefreshProjectList();
}

void LauncherShell::ShowSelectedInExplorer() {
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        return;
    }
    const auto& selected = m_Projects[static_cast<std::size_t>(m_SelectedIndex)];
    if (RevealInExplorer(selected.weprojPath)) {
        SetStatus("Revealed in Explorer");
    } else {
        SetStatus("Failed to reveal project in Explorer.");
    }
}

we::platform::WindowHitTestResult LauncherShell::WindowHitTest(we::platform::Int2 point) const {
    if (m_TitleBar) {
        return m_TitleBar->WindowHitTest(point);
    }
    return we::platform::WindowHitTestResult::Client;
}

Size LauncherShell::Measure(const Size& availableSize) {
    UpdateAdaptiveLayout(availableSize.width);
    if (m_Root) {
        m_Root->Measure(availableSize);
    }
    if (m_ModalHost && m_ModalHost->IsVisible()) {
        m_ModalHost->Measure(availableSize);
    }
    m_DesiredSize = availableSize;
    return availableSize;
}

void LauncherShell::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    UpdateAdaptiveLayout(allottedRect.width);
    if (m_Root) {
        m_Root->Arrange(allottedRect);
    }
    if (m_ModalHost && m_ModalHost->IsVisible()) {
        m_ModalHost->Arrange(allottedRect);
    }
}

void LauncherShell::Paint(PaintContext& context) {
    context.DrawRect(GetGeometry(), LColor(ThemeToken::WorkspaceBackground));
    if (m_Root) {
        m_Root->Paint(context);
    }
    if (m_ModalHost && m_ModalHost->IsVisible()) {
        Color scrim = LColor(ThemeToken::ModalScrim);
        context.DrawRect(GetGeometry(), scrim);
        m_ModalHost->Paint(context);
    }
}

void LauncherShell::Tick(float deltaTime) {
    if (m_PageContentLoading) {
        m_PageLoadTimer += deltaTime;
        AdvanceSkeletonShimmer(deltaTime);
        if (m_PageLoadTimer >= m_PageLoadDuration) {
            m_PageContentLoading = false;
            m_PageLoadTimer = 0.0f;
            MarkPageDirty(m_Page);
            EnsurePageBuilt(m_Page);
            ShowPage(m_Page);
            InvalidateUI();
        } else {
            InvalidateUI();
        }
    }

    if (m_SettingsPendingScroll && m_SettingsScrollTarget && m_Page == LauncherPage::Settings) {
        auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Settings)];
        if (state.scroll && state.scroll->GetContent()) {
            const auto& targetGeom = m_SettingsScrollTarget->GetGeometry();
            const auto& contentGeom = state.scroll->GetContent()->GetGeometry();
            if (targetGeom.height > 0.0f && contentGeom.height > 0.0f) {
                const float localY = targetGeom.y - contentGeom.y;
                state.scroll->ScrollToMakeVisible(Rect{
                    0.0f,
                    localY,
                    targetGeom.width,
                    targetGeom.height
                });
                state.scrollOffset = state.scroll->GetScrollOffset();
                m_SettingsPendingScroll = false;
                InvalidateUI();
            }
        }
    }

    Widget::Tick(deltaTime);
}

void LauncherShell::OnKeyDown(const KeyEvent& event) {
    if (m_ProjectsSearch && m_ProjectsSearch->IsFocused()) {
        m_ProjectsSearch->OnKeyDown(event);
        return;
    }

    if (event.key == we::platform::KeyCode::Escape && m_Modal != ModalKind::None) {
        CloseModal();
        return;
    }
    if (event.ctrlDown && event.key == we::platform::KeyCode::N) {
        ShowCreateWizard();
        return;
    }
    if (event.ctrlDown && event.key == we::platform::KeyCode::O) {
        BrowseForProject();
        return;
    }

    if (m_Page == LauncherPage::Projects && m_Modal == ModalKind::None
        && !event.ctrlDown && !event.altDown && !m_FilteredProjects.empty()) {
        auto mapFilteredToProject = [this](std::size_t filteredIndex) -> std::size_t {
            if (filteredIndex >= m_FilteredProjects.size()) {
                return 0;
            }
            const auto& path = m_FilteredProjects[filteredIndex].weprojPath;
            for (std::size_t j = 0; j < m_Projects.size(); ++j) {
                if (m_Projects[j].weprojPath == path) {
                    return j;
                }
            }
            return 0;
        };
        auto currentFiltered = [&]() -> int {
            if (m_SelectedIndex < 0) {
                return -1;
            }
            const auto& path = m_Projects[static_cast<std::size_t>(m_SelectedIndex)].weprojPath;
            for (std::size_t i = 0; i < m_FilteredProjects.size(); ++i) {
                if (m_FilteredProjects[i].weprojPath == path) {
                    return static_cast<int>(i);
                }
            }
            return -1;
        };

        if (event.key == we::platform::KeyCode::Up) {
            int idx = currentFiltered();
            idx = idx <= 0 ? 0 : idx - 1;
            SelectProject(mapFilteredToProject(static_cast<std::size_t>(idx)), false);
            return;
        }
        if (event.key == we::platform::KeyCode::Down) {
            int idx = currentFiltered();
            idx = idx < 0 ? 0 : std::min(idx + 1, static_cast<int>(m_FilteredProjects.size()) - 1);
            SelectProject(mapFilteredToProject(static_cast<std::size_t>(idx)), false);
            return;
        }
        if (event.key == we::platform::KeyCode::Home) {
            SelectProject(mapFilteredToProject(0), false);
            return;
        }
        if (event.key == we::platform::KeyCode::End) {
            SelectProject(mapFilteredToProject(m_FilteredProjects.size() - 1), false);
            return;
        }
    }

    if (!event.ctrlDown && !event.altDown && m_Sidebar) {
        if (event.key == we::platform::KeyCode::Up) {
            m_Sidebar->NavigateByDelta(-1);
            return;
        }
        if (event.key == we::platform::KeyCode::Down) {
            m_Sidebar->NavigateByDelta(1);
            return;
        }
        if (event.key == we::platform::KeyCode::Home) {
            m_Sidebar->NavigateToIndex(0);
            return;
        }
        if (event.key == we::platform::KeyCode::End) {
            m_Sidebar->NavigateToIndex(static_cast<int>(LauncherPage::Count) - 1);
            return;
        }
    }

    if (event.key == we::platform::KeyCode::Enter && m_Modal == ModalKind::None) {
        OpenSelectedProject();
    }
}

} // namespace we::programs::welauncher
