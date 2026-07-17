#include "UI/LauncherShell.h"

#include "UI/LauncherHelpers.h"
#include "UI/Widgets/ManagerViews.h"
#include "UI/Widgets/SettingsViews.h"
#include "UI/Widgets/SkeletonViews.h"
#include "Util/LauncherMaintenance.h"
#include "Util/PathUtils.h"

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
#include "Widgets/Label.h"
#include "Widgets/TextBox.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>

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
        kLauncherContentPadX * LScale(),
        LMetric(ThemeToken::Space4) * LScale(),
        kLauncherContentPadX * LScale(),
        LMetric(ThemeToken::Space2) * LScale()
    });
    row->SetVerticalAlignment(VerticalAlignment::Center);
    return row;
}

std::shared_ptr<VerticalBox> MakePageBodyPadding() {
    auto content = std::make_shared<VerticalBox>();
    content->SetPadding(Margin{
        kLauncherContentPadX * LScale(),
        LMetric(ThemeToken::Space4) * LScale(),
        kLauncherContentPadX * LScale(),
        LMetric(ThemeToken::Space4) * LScale()
    });
    content->SetSpacing(LMetric(ThemeToken::Space4) * LScale());
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
    case LauncherPage::Learn: {
        root->AddChild(MakeSectionHeader("Learn", "Loading samples…"));
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
        root->AddChild(MakeSectionHeader("Settings", "Loading preferences…"));
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
    case LauncherPage::Learn: RebuildLearnPage(); break;
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
    } else if (m_Page == LauncherPage::Learn) {
        MarkPageDirty(LauncherPage::Learn);
        EnsurePageBuilt(LauncherPage::Learn);
        ShowPage(LauncherPage::Learn);
    } else if (m_Page == LauncherPage::Engine) {
        MarkPageDirty(LauncherPage::Engine);
        EnsurePageBuilt(LauncherPage::Engine);
        ShowPage(LauncherPage::Engine);
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

std::shared_ptr<Widget> LauncherShell::BuildProjectsPageHeader(bool showActions) {
    const float s = LScale();
    auto row = std::make_shared<HorizontalBox>();
    row->SetSpacing(kLauncherButtonGap * s);
    row->SetVerticalAlignment(VerticalAlignment::Center);
    row->SetHorizontalAlignment(HorizontalAlignment::Fill);
    row->SetPadding(Margin{});
    row->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

    auto titleCol = std::make_shared<VerticalBox>();
    titleCol->SetSpacing(2.0f * s);
    titleCol->SetVerticalAlignment(VerticalAlignment::Center);
    titleCol->SetHorizontalAlignment(HorizontalAlignment::Left);
    titleCol->SetPadding(Margin{});
    titleCol->SetBackgroundColor(Color::Transparent());

    auto title = MakeLabel(
        "Projects",
        LMetric(ThemeToken::TextSizeTitle) * s,
        LColor(ThemeToken::TextPrimary));
    title->SetVerticalAlignment(VerticalAlignment::Center);
    title->SetHorizontalAlignment(HorizontalAlignment::Left);
    titleCol->AddChild(title);

    auto subtitle = MakeLabel(
        "Open, create, and manage WindEffects projects",
        LMetric(ThemeToken::TextSizeBody) * s,
        LColor(ThemeToken::TextMuted));
    subtitle->SetVerticalAlignment(VerticalAlignment::Center);
    subtitle->SetHorizontalAlignment(HorizontalAlignment::Left);
    titleCol->AddChild(subtitle);

    row->AddChild(titleCol);
    row->AddChild(std::make_shared<Spacer>());

    if (showActions) {
        auto openBtn = std::make_shared<SecondaryToolbarButton>("Open Project", Icons::OpenFolderName);
        openBtn->SetOnClicked([this] { BrowseForProject(); });
        openBtn->SetHorizontalAlignment(HorizontalAlignment::Left);
        openBtn->SetVerticalAlignment(VerticalAlignment::Center);
        row->AddChild(openBtn);

        auto newBtn = std::make_shared<PrimaryToolbarButton>("New Project", Icons::PlusName);
        newBtn->SetOnClicked([this] { ShowCreateWizard(); });
        newBtn->SetHorizontalAlignment(HorizontalAlignment::Left);
        newBtn->SetVerticalAlignment(VerticalAlignment::Center);
        row->AddChild(newBtn);
    }

    return row;
}

std::shared_ptr<Widget> LauncherShell::BuildProjectSearchRow() {
    const float s = LScale();
    auto bar = std::make_shared<HorizontalBox>();
    bar->SetSpacing(0.0f);
    bar->SetVerticalAlignment(VerticalAlignment::Center);
    bar->SetHorizontalAlignment(HorizontalAlignment::Fill);
    bar->SetPadding(Margin{});
    bar->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

    bar->AddChild(std::make_shared<Spacer>());

    auto search = std::make_shared<CompactSearchField>("Search Projects...");
    search->SetText(m_SearchQuery);
    search->SetOnChanged([this](const std::string& text) { OnSearchChanged(text); });
    search->SetVerticalAlignment(VerticalAlignment::Center);
    search->SetHorizontalAlignment(HorizontalAlignment::Left);
    m_ProjectsSearch = search;
    bar->AddChild(search);

    return bar;
}

void LauncherShell::RebuildProjectsPage() {
    auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Projects)];

    auto page = std::make_shared<VerticalBox>();
    page->SetSpacing(0.0f);
    page->SetHorizontalAlignment(HorizontalAlignment::Fill);
    page->SetVerticalAlignment(VerticalAlignment::Fill);
    page->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

    const float s = LScale();

    // Padded chrome (title / actions / search) — table below is full-bleed.
    auto chrome = std::make_shared<VerticalBox>();
    chrome->SetSpacing(0.0f);
    chrome->SetHorizontalAlignment(HorizontalAlignment::Fill);
    chrome->SetVerticalAlignment(VerticalAlignment::Top);
    chrome->SetPadding(Margin{
        kLauncherContentPadX * s,
        kLauncherContentPadTop * s,
        kLauncherContentPadX * s,
        0.0f
    });
    chrome->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));
    chrome->AddChild(BuildProjectsPageHeader(true));

    if (IsPageContentLoading()) {
        chrome->AddChild(std::make_shared<FixedGap>(1.0f, kLauncherTitleToToolbar * s));
        chrome->AddChild(BuildProjectSearchRow());
        chrome->AddChild(std::make_shared<FixedGap>(1.0f, kLauncherToolbarToDivider * s));
        page->AddChild(chrome);

        auto tableHost = std::make_shared<VerticalBox>();
        tableHost->SetSpacing(0.0f);
        tableHost->SetHorizontalAlignment(HorizontalAlignment::Fill);
        tableHost->SetVerticalAlignment(VerticalAlignment::Fill);
        tableHost->SetPadding(Margin{});
        tableHost->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));
        for (int i = 0; i < 8; ++i) {
            auto row = std::make_shared<SkeletonCard>(SkeletonKind::ListRow);
            row->SetHorizontalAlignment(HorizontalAlignment::Fill);
            tableHost->AddChild(row);
        }
        auto filler = std::make_shared<FixedGap>(1.0f, 1.0f);
        filler->SetVerticalAlignment(VerticalAlignment::Fill);
        tableHost->AddChild(filler);
        page->AddChild(tableHost);

        state.root = page;
        state.scroll = nullptr;
        return;
    }

    chrome->AddChild(std::make_shared<FixedGap>(1.0f, kLauncherTitleToToolbar * s));
    chrome->AddChild(BuildProjectSearchRow());
    chrome->AddChild(std::make_shared<FixedGap>(1.0f, kLauncherToolbarToDivider * s));
    page->AddChild(chrome);

    auto tableHost = std::make_shared<VerticalBox>();
    tableHost->SetSpacing(0.0f);
    tableHost->SetHorizontalAlignment(HorizontalAlignment::Fill);
    tableHost->SetVerticalAlignment(VerticalAlignment::Fill);
    tableHost->SetPadding(Margin{});
    tableHost->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));
    state.scroll = nullptr;

    // Column header always visible (Unity Hub-style), then empty message or rows.
    auto header = std::make_shared<ProjectTableHeader>();
    header->SetSortMode(m_SortMode);
    header->SetOnSort([this](ProjectSortMode mode) { SetSortMode(mode); });
    tableHost->AddChild(header);

    if (m_Projects.empty() && m_SearchQuery.empty()) {
        auto empty = std::make_shared<EmptyStatePanel>(
            "No projects yet",
            "Create a new project or open an existing one.",
            Icons::Cube3DName);
        empty->SetHorizontalAlignment(HorizontalAlignment::Fill);
        empty->SetVerticalAlignment(VerticalAlignment::Fill);
        tableHost->AddChild(empty);
    } else if (m_FilteredProjects.empty()) {
        auto empty = std::make_shared<EmptyStatePanel>(
            "No matching projects",
            "Try a different search or clear the current query.",
            Icons::SearchName);
        empty->SetPrimaryAction("Clear Search", Icons::RefreshName, [this] {
            OnSearchChanged({});
        });
        empty->SetHorizontalAlignment(HorizontalAlignment::Fill);
        empty->SetVerticalAlignment(VerticalAlignment::Fill);
        tableHost->AddChild(empty);
    } else {
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

void LauncherShell::RebuildLearnPage() {
    auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Learn)];

    auto page = std::make_shared<VerticalBox>();
    page->SetSpacing(0.0f);
    page->SetHorizontalAlignment(HorizontalAlignment::Fill);
    page->SetVerticalAlignment(VerticalAlignment::Fill);
    page->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

    const float s = LScale();

    auto chrome = std::make_shared<VerticalBox>();
    chrome->SetSpacing(0.0f);
    chrome->SetHorizontalAlignment(HorizontalAlignment::Fill);
    chrome->SetVerticalAlignment(VerticalAlignment::Top);
    chrome->SetPadding(Margin{
        kLauncherContentPadX * s,
        kLauncherContentPadTop * s,
        kLauncherContentPadX * s,
        0.0f
    });
    chrome->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

    auto headerRow = std::make_shared<HorizontalBox>();
    headerRow->SetSpacing(kLauncherButtonGap * s);
    headerRow->SetVerticalAlignment(VerticalAlignment::Center);
    headerRow->SetHorizontalAlignment(HorizontalAlignment::Fill);

    auto titleCol = std::make_shared<VerticalBox>();
    titleCol->SetSpacing(2.0f * s);
    titleCol->SetHorizontalAlignment(HorizontalAlignment::Left);
    auto title = MakeLabel(
        "Learn",
        LMetric(ThemeToken::TextSizeTitle) * s,
        LColor(ThemeToken::TextPrimary));
    title->SetVerticalAlignment(VerticalAlignment::Center);
    titleCol->AddChild(title);
    auto subtitle = MakeLabel(
        "Samples, guides, and getting started",
        LMetric(ThemeToken::TextSizeBody) * s,
        LColor(ThemeToken::TextMuted));
    subtitle->SetVerticalAlignment(VerticalAlignment::Center);
    titleCol->AddChild(subtitle);
    headerRow->AddChild(titleCol);
    headerRow->AddChild(std::make_shared<Spacer>());

    auto docsBtn = std::make_shared<SecondaryToolbarButton>("Open Docs", Icons::DocumentName);
    docsBtn->SetOnClicked([this] { SetStatus("Documentation — coming soon"); });
    headerRow->AddChild(docsBtn);
    auto refreshBtn = std::make_shared<SecondaryToolbarButton>("Refresh", Icons::RefreshName);
    refreshBtn->SetOnClicked([this] {
        BeginPageContentLoad(0.2f);
        MarkPageDirty(LauncherPage::Learn);
        EnsurePageBuilt(LauncherPage::Learn);
        ShowPage(LauncherPage::Learn);
        SetStatus("Learn content refreshed");
    });
    headerRow->AddChild(refreshBtn);
    chrome->AddChild(headerRow);

    auto searchBar = std::make_shared<HorizontalBox>();
    searchBar->SetSpacing(0.0f);
    searchBar->SetVerticalAlignment(VerticalAlignment::Center);
    searchBar->SetHorizontalAlignment(HorizontalAlignment::Fill);
    searchBar->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));
    searchBar->AddChild(std::make_shared<Spacer>());
    auto search = std::make_shared<CompactSearchField>("Search Learn...");
    search->SetText(m_SearchQuery);
    search->SetOnChanged([this](const std::string& text) { OnSearchChanged(text); });
    searchBar->AddChild(search);

    if (IsPageContentLoading()) {
        chrome->AddChild(std::make_shared<FixedGap>(1.0f, kLauncherTitleToToolbar * s));
        chrome->AddChild(searchBar);
        chrome->AddChild(std::make_shared<FixedGap>(1.0f, kLauncherToolbarToDivider * s));
        page->AddChild(chrome);

        auto tableHost = std::make_shared<VerticalBox>();
        tableHost->SetSpacing(0.0f);
        tableHost->SetHorizontalAlignment(HorizontalAlignment::Fill);
        tableHost->SetVerticalAlignment(VerticalAlignment::Fill);
        tableHost->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));
        for (int i = 0; i < 8; ++i) {
            tableHost->AddChild(std::make_shared<SkeletonCard>(SkeletonKind::ListRow));
        }
        auto filler = std::make_shared<FixedGap>(1.0f, 1.0f);
        filler->SetVerticalAlignment(VerticalAlignment::Fill);
        tableHost->AddChild(filler);
        page->AddChild(tableHost);
        state.root = page;
        state.scroll = nullptr;
        return;
    }

    chrome->AddChild(std::make_shared<FixedGap>(1.0f, kLauncherTitleToToolbar * s));
    chrome->AddChild(searchBar);
    chrome->AddChild(std::make_shared<FixedGap>(1.0f, kLauncherToolbarToDivider * s));
    page->AddChild(chrome);

    struct LearnEntry {
        const char* kind;
        const char* name;
        const char* detail;
        const char* icon;
        std::function<void()> onClick;
    };

    std::vector<LearnEntry> entries = {
        { "Guide", "Create your first project", "Walk through New Project and open it in the editor",
          Icons::PlusName, [this] { ShowCreateWizard(); } },
        { "Guide", "Open documentation", "API reference and engine guides",
          Icons::DocumentName, [this] { SetStatus("Documentation — coming soon"); } },
        { "Guide", "Explore the engine install", "Verify SDK health and local builds",
          Icons::BuildName, [this] { GoToPage(LauncherPage::Engine); } },
        { "Sample", "First Person Demo", "Movement and camera baseline",
          Icons::PlayName, [this] { SetStatus("Sample projects — coming soon"); } },
        { "Sample", "Atmosphere Showcase", "Sky, clouds, and fog overview",
          Icons::SunName, [this] { SetStatus("Sample projects — coming soon"); } },
        { "Sample", "ECS Sandbox", "Entity component patterns",
          Icons::ComponentName, [this] { SetStatus("Sample projects — coming soon"); } },
    };

    const std::string query = ToLowerCopy(m_SearchQuery);
    std::vector<LearnEntry> visible;
    visible.reserve(entries.size());
    for (auto& entry : entries) {
        if (!query.empty()) {
            const std::string hay = ToLowerCopy(
                std::string(entry.kind) + " " + entry.name + " " + entry.detail);
            if (hay.find(query) == std::string::npos) {
                continue;
            }
        }
        visible.push_back(std::move(entry));
    }

    auto tableHost = std::make_shared<VerticalBox>();
    tableHost->SetSpacing(0.0f);
    tableHost->SetHorizontalAlignment(HorizontalAlignment::Fill);
    tableHost->SetVerticalAlignment(VerticalAlignment::Fill);
    tableHost->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));
    tableHost->AddChild(std::make_shared<SimpleColumnHeader>(
        std::vector<std::string>{ "NAME", "TYPE", "DETAIL" }));

    if (visible.empty()) {
        auto empty = std::make_shared<EmptyStatePanel>(
            query.empty() ? "Nothing here yet" : "No matching items",
            query.empty()
                ? "Samples and guides will appear here as they become available."
                : "Try a different search or clear the current query.",
            query.empty() ? Icons::MediaPlayName : Icons::SearchName);
        if (!query.empty()) {
            empty->SetPrimaryAction("Clear Search", Icons::RefreshName, [this] {
                OnSearchChanged({});
            });
        }
        empty->SetHorizontalAlignment(HorizontalAlignment::Fill);
        empty->SetVerticalAlignment(VerticalAlignment::Fill);
        tableHost->AddChild(empty);
        state.scroll = nullptr;
    } else {
        auto scroll = std::make_shared<ScrollLayout>();
        auto list = std::make_shared<VerticalBox>();
        list->SetSpacing(0.0f);
        list->SetHorizontalAlignment(HorizontalAlignment::Fill);
        list->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

        for (const auto& entry : visible) {
            list->AddChild(std::make_shared<LibraryPackageRow>(
                entry.kind,
                entry.name,
                entry.detail,
                entry.icon,
                entry.onClick));
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

void LauncherShell::RebuildEnginePage() {
    auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Engine)];

    auto page = std::make_shared<VerticalBox>();
    page->SetSpacing(0.0f);
    page->SetHorizontalAlignment(HorizontalAlignment::Fill);
    page->SetVerticalAlignment(VerticalAlignment::Fill);
    page->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

    const float s = LScale();

    auto chrome = std::make_shared<VerticalBox>();
    chrome->SetSpacing(0.0f);
    chrome->SetHorizontalAlignment(HorizontalAlignment::Fill);
    chrome->SetVerticalAlignment(VerticalAlignment::Top);
    chrome->SetPadding(Margin{
        kLauncherContentPadX * s,
        kLauncherContentPadTop * s,
        kLauncherContentPadX * s,
        0.0f
    });
    chrome->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

    auto headerRow = std::make_shared<HorizontalBox>();
    headerRow->SetSpacing(kLauncherButtonGap * s);
    headerRow->SetVerticalAlignment(VerticalAlignment::Center);
    headerRow->SetHorizontalAlignment(HorizontalAlignment::Fill);

    auto titleCol = std::make_shared<VerticalBox>();
    titleCol->SetSpacing(2.0f * s);
    titleCol->SetHorizontalAlignment(HorizontalAlignment::Left);
    auto title = MakeLabel(
        "Engine",
        LMetric(ThemeToken::TextSizeTitle) * s,
        LColor(ThemeToken::TextPrimary));
    title->SetVerticalAlignment(VerticalAlignment::Center);
    titleCol->AddChild(title);
    auto subtitle = MakeLabel(
        "Installed engines and SDK health",
        LMetric(ThemeToken::TextSizeBody) * s,
        LColor(ThemeToken::TextMuted));
    subtitle->SetVerticalAlignment(VerticalAlignment::Center);
    titleCol->AddChild(subtitle);
    headerRow->AddChild(titleCol);
    headerRow->AddChild(std::make_shared<Spacer>());

    auto refresh = std::make_shared<SecondaryToolbarButton>("Refresh", Icons::RefreshName);
    refresh->SetOnClicked([this] {
        BeginPageContentLoad(0.2f);
        UpdateFooter();
        MarkPageDirty(LauncherPage::Engine);
        EnsurePageBuilt(LauncherPage::Engine);
        ShowPage(LauncherPage::Engine);
        SetStatus("Engine status refreshed");
    });
    headerRow->AddChild(refresh);
    chrome->AddChild(headerRow);

    auto searchBar = std::make_shared<HorizontalBox>();
    searchBar->SetSpacing(0.0f);
    searchBar->SetVerticalAlignment(VerticalAlignment::Center);
    searchBar->SetHorizontalAlignment(HorizontalAlignment::Fill);
    searchBar->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));
    searchBar->AddChild(std::make_shared<Spacer>());
    auto search = std::make_shared<CompactSearchField>("Search Engine installs...");
    search->SetText(m_SearchQuery);
    search->SetOnChanged([this](const std::string& text) { OnSearchChanged(text); });
    searchBar->AddChild(search);

    if (IsPageContentLoading()) {
        chrome->AddChild(std::make_shared<FixedGap>(1.0f, kLauncherTitleToToolbar * s));
        chrome->AddChild(searchBar);
        chrome->AddChild(std::make_shared<FixedGap>(1.0f, kLauncherToolbarToDivider * s));
        page->AddChild(chrome);

        auto tableHost = std::make_shared<VerticalBox>();
        tableHost->SetSpacing(0.0f);
        tableHost->SetHorizontalAlignment(HorizontalAlignment::Fill);
        tableHost->SetVerticalAlignment(VerticalAlignment::Fill);
        tableHost->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));
        for (int i = 0; i < 4; ++i) {
            tableHost->AddChild(std::make_shared<SkeletonCard>(SkeletonKind::ListRow));
        }
        auto filler = std::make_shared<FixedGap>(1.0f, 1.0f);
        filler->SetVerticalAlignment(VerticalAlignment::Fill);
        tableHost->AddChild(filler);
        page->AddChild(tableHost);
        state.root = page;
        state.scroll = nullptr;
        return;
    }

    chrome->AddChild(std::make_shared<FixedGap>(1.0f, kLauncherTitleToToolbar * s));
    chrome->AddChild(searchBar);
    chrome->AddChild(std::make_shared<FixedGap>(1.0f, kLauncherToolbarToDivider * s));
    page->AddChild(chrome);

    auto tableHost = std::make_shared<VerticalBox>();
    tableHost->SetSpacing(0.0f);
    tableHost->SetHorizontalAlignment(HorizontalAlignment::Fill);
    tableHost->SetVerticalAlignment(VerticalAlignment::Fill);
    tableHost->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));
    tableHost->AddChild(std::make_shared<SimpleColumnHeader>(
        std::vector<std::string>{ "VERSION", "BUILD", "PATH", "SDK" }));

    const bool hasEngine = m_Context->Engines().HasEngine()
        && !m_Context->Engines().InstalledEngines().empty();

    if (!hasEngine) {
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
        empty->SetHorizontalAlignment(HorizontalAlignment::Fill);
        empty->SetVerticalAlignment(VerticalAlignment::Fill);
        tableHost->AddChild(empty);
        state.scroll = nullptr;
        page->AddChild(tableHost);
        state.root = page;
        return;
    }

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

    const std::string query = ToLowerCopy(m_SearchQuery);
    std::vector<EngineInstallInfo> visible;
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
        if (!query.empty()) {
            const std::string hay = ToLowerCopy(
                rowInfo.engineVersion + " " + rowInfo.buildId + " "
                + rowInfo.engineRoot + " " + rowInfo.sdkStatus + " "
                + rowInfo.updateStatus);
            if (hay.find(query) == std::string::npos) {
                continue;
            }
        }
        visible.push_back(std::move(rowInfo));
    }

    if (visible.empty()) {
        auto empty = std::make_shared<EmptyStatePanel>(
            "No matching installs",
            "Try a different search or clear the current query.",
            Icons::SearchName);
        empty->SetPrimaryAction("Clear Search", Icons::RefreshName, [this] {
            OnSearchChanged({});
        });
        empty->SetHorizontalAlignment(HorizontalAlignment::Fill);
        empty->SetVerticalAlignment(VerticalAlignment::Fill);
        tableHost->AddChild(empty);
        state.scroll = nullptr;
    } else {
        auto scroll = std::make_shared<ScrollLayout>();
        auto list = std::make_shared<VerticalBox>();
        list->SetSpacing(0.0f);
        list->SetHorizontalAlignment(HorizontalAlignment::Fill);
        list->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

        for (const auto& rowInfo : visible) {
            auto row = std::make_shared<EngineInstallRow>(rowInfo, rowInfo.isCurrent);
            const std::string root = rowInfo.engineRoot;
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
            list->AddChild(row);
        }

        list->AddChild(MakeLabel(
            "SDK checks",
            LMetric(ThemeToken::TextSizeBody) * s,
            LColor(ThemeToken::TextPrimary)));
        for (const auto& check : m_Context->Sdk().RunChecks()) {
            Color color = LColor(ThemeToken::TextMuted);
            if (check.status == SdkCheckStatus::Pass) color = LColor(ThemeToken::Success);
            else if (check.status == SdkCheckStatus::Warn) color = LColor(ThemeToken::Warning);
            else if (check.status == SdkCheckStatus::Fail) color = LColor(ThemeToken::ErrorForeground);
            list->AddChild(MakeLabel(
                check.name + " — " + check.detail,
                LMetric(ThemeToken::TextSizeCaption) * s,
                color));
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
    AppendSettingsRow(*body, queryLower, "Default Projects Folder", "Where New Project creates projects", folder, "path browse");

    std::vector<std::string> engineLabels;
    std::vector<std::string> engineRoots;
    engineLabels.push_back("Auto-detect");
    engineRoots.push_back({});
    for (const auto& install : m_Context->Engines().InstalledEngines()) {
        const std::string version = install.engineVersion.empty() ? "Unknown" : install.engineVersion;
        engineLabels.push_back(version + "  —  " + EllipsizePath(install.engineRoot, 36));
        engineRoots.push_back(install.engineRoot);
    }
    int engineIndex = 0;
    for (int i = 0; i < static_cast<int>(engineRoots.size()); ++i) {
        if (engineRoots[static_cast<std::size_t>(i)] == settings.selectedEngineRoot) {
            engineIndex = i;
            break;
        }
    }
    auto engineDrop = std::make_shared<SettingsDropdown>(engineLabels, engineIndex);
    engineDrop->SetOnChanged([this, engineRoots](int index) {
        if (index < 0 || index >= static_cast<int>(engineRoots.size())) {
            return;
        }
        m_Context->Settings().Settings().selectedEngineRoot = engineRoots[static_cast<std::size_t>(index)];
        PersistLauncherSettings(
            engineRoots[static_cast<std::size_t>(index)].empty()
                ? "Default engine: Auto-detect"
                : "Default engine updated");
    });
    AppendSettingsRow(*body, queryLower, "Default Engine Version", "Preferred install for new and recent projects", engineDrop);

    static const int kLimits[] = { 10, 20, 50, 0 };
    static const char* kLimitLabels[] = { "10", "20", "50", "Unlimited" };
    int limitIndex = 1;
    for (int i = 0; i < 4; ++i) {
        if (settings.recentProjectsLimit == kLimits[i]) {
            limitIndex = i;
            break;
        }
    }
    auto limitDrop = std::make_shared<SettingsDropdown>(
        std::vector<std::string>{ kLimitLabels[0], kLimitLabels[1], kLimitLabels[2], kLimitLabels[3] },
        limitIndex);
    limitDrop->SetOnChanged([this](int index) {
        if (index < 0 || index >= 4) {
            return;
        }
        m_Context->Settings().Settings().recentProjectsLimit = kLimits[index];
        PersistLauncherSettings("Recent projects limit updated");
    });
    AppendSettingsRow(*body, queryLower, "Recent Projects Limit", "How many projects stay in the recent list", limitDrop);

    auto openLast = std::make_shared<ToggleSwitch>(settings.openLastProjectOnStart);
    openLast->SetOnChanged([this](bool on) {
        m_Context->Settings().Settings().openLastProjectOnStart = on;
        PersistLauncherSettings(on ? "Open last project on start enabled" : "Open last project on start disabled");
    });
    AppendSettingsRow(*body, queryLower, "Open Last Project on Launcher Start", "Launch the most recent project when the launcher opens", openLast);

    if (auto group = WrapSettingsGroup("General", "Projects folder, default engine, and startup", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsEngine(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto& settings = m_Context->Settings().Settings();
    auto body = MakeSettingsStack();

    std::string installDir = settings.engineInstallDirectory;
    if (installDir.empty() && m_Context->Engines().HasEngine()) {
        installDir = PathUtils::ToUtf8(m_Context->Engines().Current().engineRoot);
    }
    if (installDir.empty()) {
        installDir = PathUtils::ToUtf8(PathUtils::GetExecutableDirectory());
    }

    auto folder = std::make_shared<PathPickerField>(installDir, true);
    folder->SetDialogTitle("Select Engine Installation Directory");
    folder->SetOnChanged([this](const std::string& path) {
        m_Context->Settings().Settings().engineInstallDirectory = path;
        PersistLauncherSettings("Engine installation directory updated");
        MarkPageDirty(LauncherPage::Settings);
        EnsurePageBuilt(LauncherPage::Settings);
        ShowPage(LauncherPage::Settings);
    });
    AppendSettingsRow(*body, queryLower, "Engine Installation Directory", "Root used when scanning for WindEffects installs", folder, "path browse");

    auto scan = std::make_shared<SecondaryToolbarButton>("Scan for Installed Engines", Icons::RefreshName);
    scan->SetOnClicked([this] {
        auto& settings = m_Context->Settings().Settings();
        std::filesystem::path start = PathUtils::GetExecutableDirectory();
        if (!settings.engineInstallDirectory.empty()) {
            start = PathUtils::FromUtf8(settings.engineInstallDirectory);
        }
        const bool ok = m_Context->Engines().Discover(start);
        UpdateFooter();
        MarkPageDirty(LauncherPage::Engine);
        MarkPageDirty(LauncherPage::Settings);
        EnsurePageBuilt(LauncherPage::Settings);
        ShowPage(LauncherPage::Settings);
        SetStatus(ok ? "Engine scan complete" : "No engine found at that location");
    });
    AppendSettingsRow(*body, queryLower, "Scan for Installed Engines", "Rediscover local engine installs", scan);

    auto verify = std::make_shared<SecondaryToolbarButton>("Verify Engine Installation", Icons::CheckName);
    verify->SetOnClicked([this] {
        int pass = 0, warn = 0, fail = 0;
        for (const auto& check : m_Context->Sdk().RunChecks()) {
            switch (check.status) {
            case SdkCheckStatus::Pass: ++pass; break;
            case SdkCheckStatus::Warn: ++warn; break;
            case SdkCheckStatus::Fail: ++fail; break;
            }
        }
        UpdateFooter();
        MarkPageDirty(LauncherPage::Engine);
        SetStatus(
            "SDK verify: " + std::to_string(pass) + " ok / "
            + std::to_string(warn) + " warn / " + std::to_string(fail) + " fail");
    });
    AppendSettingsRow(*body, queryLower, "Verify Engine Installation", "Run SDK and toolchain health checks", verify);

    auto updates = std::make_shared<SecondaryToolbarButton>("Check for Launcher Updates", Icons::RefreshName);
    updates->SetOnClicked([this] {
        SetStatus(std::string("Launcher ") + kLauncherVersion + " — update check is not available yet");
    });
    AppendSettingsRow(*body, queryLower, "Check for Launcher Updates", "Look for a newer WeLauncher build", updates);

    if (auto group = WrapSettingsGroup("Engine", "Installs, verification, and launcher updates", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsStorage(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto body = MakeSettingsStack();

    const auto cacheRoot = PathUtils::GetLauncherCacheRoot();
    const auto thumbRoot = PathUtils::GetThumbnailCacheRoot();
    const std::string cacheSize = PathUtils::FormatByteSize(PathUtils::EstimateDirectoryBytes(cacheRoot));
    const std::string thumbSize = PathUtils::FormatByteSize(PathUtils::EstimateDirectoryBytes(thumbRoot));

    auto clearCache = std::make_shared<SecondaryToolbarButton>("Clear Cache", Icons::DeleteName);
    clearCache->SetOnClicked([this] {
        const bool ok = PathUtils::ClearDirectoryContents(PathUtils::GetLauncherCacheRoot());
        MarkPageDirty(LauncherPage::Settings);
        EnsurePageBuilt(LauncherPage::Settings);
        ShowPage(LauncherPage::Settings);
        SetStatus(ok ? "Launcher cache cleared" : "Failed to clear launcher cache");
    });
    AppendSettingsRow(
        *body,
        queryLower,
        "Launcher Cache",
        cacheSize + " on disk",
        clearCache,
        "storage clear");

    auto clearThumbs = std::make_shared<SecondaryToolbarButton>("Clear Cache", Icons::DeleteName);
    clearThumbs->SetOnClicked([this] {
        const bool ok = PathUtils::ClearDirectoryContents(PathUtils::GetThumbnailCacheRoot());
        MarkPageDirty(LauncherPage::Settings);
        EnsurePageBuilt(LauncherPage::Settings);
        ShowPage(LauncherPage::Settings);
        SetStatus(ok ? "Thumbnail cache cleared" : "Failed to clear thumbnail cache");
    });
    AppendSettingsRow(
        *body,
        queryLower,
        "Thumbnail Cache",
        thumbSize + " on disk",
        clearThumbs,
        "storage clear");

    if (auto group = WrapSettingsGroup("Storage", "Local cache used by the launcher", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsFileAssociations(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto body = MakeSettingsStack();

    auto weproj = std::make_shared<SecondaryToolbarButton>("Associate .weproj", Icons::DocumentName);
    weproj->SetOnClicked([this] {
        const auto result = AssociateProjectExtension(".weproj");
        SetStatus(result.message);
    });
    AppendSettingsRow(*body, queryLower, "Associate .weproj files", "Open WindEffects projects with WeLauncher", weproj);

    auto weproject = std::make_shared<SecondaryToolbarButton>("Associate .weproject", Icons::DocumentName);
    weproject->SetOnClicked([this] {
        const auto result = AssociateProjectExtension(".weproject");
        SetStatus(result.message);
    });
    AppendSettingsRow(*body, queryLower, "Associate .weproject files", "Alternate project extension", weproject);

    if (auto group = WrapSettingsGroup("File Associations", "Register project file types with Windows", body, queryLower)) {
        root->AddChild(group);
    }
    return root;
}

std::shared_ptr<Widget> LauncherShell::BuildSettingsAbout(const std::string& queryLower) {
    auto root = MakeSettingsStack();
    auto body = MakeSettingsStack();

    AppendSettingsRow(
        *body,
        queryLower,
        "Launcher Version",
        "WindEffects Launcher",
        MakeLabel(kLauncherVersion, LMetric(ThemeToken::TextSizeBody) * LScale(), LColor(ThemeToken::TextPrimary)));

    auto logs = std::make_shared<SecondaryToolbarButton>("Open Logs Folder", Icons::OpenFolderName);
    logs->SetOnClicked([this] {
        const auto path = PathUtils::GetLauncherLogsRoot();
        std::error_code ec;
        std::filesystem::create_directories(path, ec);
        if (OpenPathInExplorer(PathUtils::ToUtf8(path))) {
            SetStatus("Opened logs folder");
        } else {
            SetStatus("Failed to open logs folder");
        }
    });
    AppendSettingsRow(*body, queryLower, "Open Logs Folder", "Launcher diagnostic logs", logs);

    auto install = std::make_shared<SecondaryToolbarButton>("Open Installation Folder", Icons::OpenFolderName);
    install->SetOnClicked([this] {
        const auto path = PathUtils::ToUtf8(PathUtils::GetExecutableDirectory());
        if (OpenPathInExplorer(path) || RevealInExplorer(path)) {
            SetStatus("Opened installation folder");
        } else {
            SetStatus("Failed to open installation folder");
        }
    });
    AppendSettingsRow(*body, queryLower, "Open Installation Folder", "Folder containing WeLauncher.exe", install);

    auto docs = std::make_shared<SecondaryToolbarButton>("Documentation", Icons::DocumentName);
    docs->SetOnClicked([this] {
        if (OpenUrl("https://windeffects.dev/docs")) {
            SetStatus("Opened documentation");
        } else {
            SetStatus("Documentation — coming soon");
        }
    });
    AppendSettingsRow(*body, queryLower, "Documentation", "Online guides and API reference", docs);

    auto reset = std::make_shared<SecondaryToolbarButton>("Reset Launcher Settings", Icons::RefreshName);
    reset->SetOnClicked([this] {
        m_Context->Settings().ResetToDefaults();
        PersistLauncherSettings("Launcher settings reset to defaults");
        MarkPageDirty(LauncherPage::Settings);
        EnsurePageBuilt(LauncherPage::Settings);
        ShowPage(LauncherPage::Settings);
    });
    AppendSettingsRow(*body, queryLower, "Reset Launcher Settings", "Restore defaults without deleting projects", reset);

    if (auto group = WrapSettingsGroup("About", "Version, folders, and maintenance", body, queryLower)) {
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
    page->SetBackgroundColor(LColor(ThemeToken::PanelContentBackground));

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
    content->SetSpacing(20.0f * s);
    content->SetPadding(Margin{
        kLauncherContentPadX * s,
        kLauncherContentPadTop * s,
        kLauncherContentPadX * s,
        kLauncherContentPadBottom * s
    });
    content->SetHorizontalAlignment(HorizontalAlignment::Fill);

    content->AddChild(MakeLabel(
        "Settings",
        LMetric(ThemeToken::TextSizeTitle) * s,
        LColor(ThemeToken::TextPrimary)));
    content->AddChild(MakeLabel(
        queryLower.empty()
            ? "Manage projects, engines, cache, and launcher maintenance"
            : ("Showing matches for \"" + m_SearchQuery + "\""),
        LMetric(ThemeToken::TextSizeBody) * s,
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
    addSection(SettingsCategory::Engine, [this](const std::string& q) { return BuildSettingsEngine(q); });
    addSection(SettingsCategory::Storage, [this](const std::string& q) { return BuildSettingsStorage(q); });
    addSection(SettingsCategory::FileAssociations, [this](const std::string& q) { return BuildSettingsFileAssociations(q); });
    addSection(SettingsCategory::About, [this](const std::string& q) { return BuildSettingsAbout(q); });

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
    const float s = LScale();
    m_ModalHost->SetVisible(true);
    m_ModalHost->SetDialogWidth(960.0f);
    m_ModalHost->SetDialogHeight(580.0f);

    const auto& templates = m_Context->Templates().Templates();
    const std::string query = ToLowerCopy(m_WizardTemplateQuery);

    std::vector<const ProjectTemplateInfo*> visible;
    visible.reserve(templates.size());
    for (const auto& tmpl : templates) {
        if (m_WizardCategory != "All" && tmpl.category != m_WizardCategory) {
            continue;
        }
        if (!query.empty()) {
            const std::string hay = ToLowerCopy(
                tmpl.displayName + " " + tmpl.id + " " + tmpl.category + " "
                + tmpl.description + " " + JoinList(tmpl.tags));
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
    } else if (!templates.empty() && m_WizardCategory == "All" && query.empty()) {
        m_WizardTemplateId = templates.front().id;
    }

    const ProjectTemplateInfo* selected = FindTemplateById(templates, m_WizardTemplateId);

    auto panel = std::make_shared<VerticalBox>();
    panel->SetSpacing(0.0f);
    panel->SetHorizontalAlignment(HorizontalAlignment::Fill);
    panel->SetVerticalAlignment(VerticalAlignment::Fill);
    panel->SetBackgroundColor(LColor(ThemeToken::DialogBackground));
    panel->SetPadding(Margin{});

    // Header
    auto header = std::make_shared<VerticalBox>();
    header->SetSpacing(4.0f * s);
    header->SetPadding(Margin{ 20.0f * s, 16.0f * s, 20.0f * s, 12.0f * s });
    header->SetHorizontalAlignment(HorizontalAlignment::Fill);
    auto title = MakeLabel(
        "New project",
        LMetric(ThemeToken::TextSizeTitle) * s,
        LColor(ThemeToken::TextPrimary));
    title->SetHorizontalAlignment(HorizontalAlignment::Center);
    header->AddChild(title);
    auto subtitle = MakeLabel(
        "Choose a 3D template and project destination",
        LMetric(ThemeToken::TextSizeCaption) * s,
        LColor(ThemeToken::TextMuted));
    subtitle->SetHorizontalAlignment(HorizontalAlignment::Center);
    header->AddChild(subtitle);
    panel->AddChild(header);

    // Body: categories | templates | details
    auto body = std::make_shared<HorizontalBox>();
    body->SetSpacing(0.0f);
    body->SetHorizontalAlignment(HorizontalAlignment::Fill);
    body->SetVerticalAlignment(VerticalAlignment::Fill);
    body->SetPadding(Margin{ 12.0f * s, 0.0f, 12.0f * s, 0.0f });

    // Left: categories
    auto catCol = std::make_shared<VerticalBox>();
    catCol->SetSpacing(4.0f * s);
    catCol->SetPadding(Margin{ 8.0f * s, 4.0f * s, 8.0f * s, 8.0f * s });
    catCol->SetHorizontalAlignment(HorizontalAlignment::Fill);
    catCol->SetVerticalAlignment(VerticalAlignment::Fill);
    catCol->AddChild(std::make_shared<FixedGap>(148.0f * s, 1.0f));
    catCol->AddChild(MakeLabel(
        "TEMPLATES",
        LMetric(ThemeToken::TextSizeCaption) * s,
        LColor(ThemeToken::TextMuted)));

    static const char* kCategories[] = { "All", "Core", "Games", "XR" };
    static const char* kCategoryIcons[] = {
        Icons::LayersName, Icons::Cube3DName, Icons::PlayName, Icons::Cube3DName
    };
    for (int i = 0; i < 4; ++i) {
        const std::string cat = kCategories[i];
        const bool active = m_WizardCategory == cat;
        auto btn = std::make_shared<SecondaryToolbarButton>(
            cat == "All" ? "All templates" : cat,
            kCategoryIcons[i]);
        if (active) {
            // Rebuild keeps selection; click still works.
        }
        btn->SetOnClicked([this, cat] {
            m_WizardCategory = cat;
            RebuildCreateWizard();
        });
        catCol->AddChild(btn);
    }
    auto catFiller = std::make_shared<FixedGap>(1.0f, 1.0f);
    catFiller->SetVerticalAlignment(VerticalAlignment::Fill);
    catCol->AddChild(catFiller);
    body->AddChild(catCol);

    // Middle: search + list
    auto midCol = std::make_shared<VerticalBox>();
    midCol->SetSpacing(8.0f * s);
    midCol->SetPadding(Margin{ 8.0f * s, 4.0f * s, 8.0f * s, 8.0f * s });
    midCol->SetHorizontalAlignment(HorizontalAlignment::Fill);
    midCol->SetVerticalAlignment(VerticalAlignment::Fill);

    auto search = std::make_shared<CompactSearchField>("Search 3D templates...");
    search->SetText(m_WizardTemplateQuery);
    search->SetOnChanged([this](const std::string& text) {
        if (m_WizardTemplateQuery == text) {
            return;
        }
        m_WizardTemplateQuery = text;
        RebuildCreateWizard();
    });
    midCol->AddChild(search);

    auto listScroll = std::make_shared<ScrollLayout>();
    auto list = std::make_shared<VerticalBox>();
    list->SetSpacing(4.0f * s);
    list->SetHorizontalAlignment(HorizontalAlignment::Fill);

    if (visible.empty()) {
        list->AddChild(MakeLabel(
            "No matching 3D templates",
            LMetric(ThemeToken::TextSizeBody) * s,
            LColor(ThemeToken::TextMuted)));
    } else {
        for (const auto* tmpl : visible) {
            const bool isSelected = selected && tmpl->id == selected->id;
            auto row = std::make_shared<TemplateListRow>(*tmpl, isSelected);
            row->SetOnSelect([this, id = tmpl->id] {
                m_WizardTemplateId = id;
                RebuildCreateWizard();
                SetStatus("Template: " + id);
            });
            list->AddChild(row);
        }
    }
    listScroll->SetContent(list);
    listScroll->SetHorizontalAlignment(HorizontalAlignment::Fill);
    listScroll->SetVerticalAlignment(VerticalAlignment::Fill);
    midCol->AddChild(listScroll);
    body->AddChild(midCol);

    // Right: details + project settings
    auto rightCol = std::make_shared<VerticalBox>();
    rightCol->SetSpacing(8.0f * s);
    rightCol->SetPadding(Margin{ 12.0f * s, 4.0f * s, 12.0f * s, 8.0f * s });
    rightCol->SetHorizontalAlignment(HorizontalAlignment::Fill);
    rightCol->SetVerticalAlignment(VerticalAlignment::Fill);
    rightCol->AddChild(std::make_shared<FixedGap>(280.0f * s, 1.0f));

    if (selected) {
        // Preview band
        auto preview = std::make_shared<VerticalBox>();
        preview->SetSpacing(6.0f * s);
        preview->SetPadding(Margin{ 12.0f * s, 12.0f * s, 12.0f * s, 12.0f * s });
        preview->SetBackgroundColor(LColor(ThemeToken::PanelBackground));
        preview->SetHorizontalAlignment(HorizontalAlignment::Fill);
        preview->AddChild(MakeLabel(
            selected->displayName,
            LMetric(ThemeToken::TextSizeHeader) * s,
            LColor(ThemeToken::TextPrimary)));
        preview->AddChild(MakeLabel(
            selected->category.empty() ? "3D Template" : (selected->category + " · 3D"),
            LMetric(ThemeToken::TextSizeCaption) * s,
            LColor(ThemeToken::AccentPrimary)));
        preview->AddChild(MakeLabel(
            selected->description.empty() ? "No description." : selected->description,
            LMetric(ThemeToken::TextSizeBody) * s,
            LColor(ThemeToken::TextMuted)));
        if (!selected->features.empty()) {
            preview->AddChild(MakeLabel(
                "Includes: " + JoinList(selected->features),
                LMetric(ThemeToken::TextSizeCaption) * s,
                LColor(ThemeToken::TextMuted)));
        }
        rightCol->AddChild(preview);
    } else {
        rightCol->AddChild(MakeLabel(
            "Select a template",
            LMetric(ThemeToken::TextSizeBody) * s,
            LColor(ThemeToken::TextMuted)));
    }

    rightCol->AddChild(MakeLabel(
        "PROJECT SETTINGS",
        LMetric(ThemeToken::TextSizeCaption) * s,
        LColor(ThemeToken::TextMuted)));

    rightCol->AddChild(MakeLabel(
        "Project name",
        LMetric(ThemeToken::TextSizeCaption) * s,
        LColor(ThemeToken::TextMuted)));
    auto nameBox = std::make_shared<TextBox>(m_WizardName, [this](const std::string& text) {
        m_WizardName = text;
    });
    rightCol->AddChild(nameBox);

    rightCol->AddChild(MakeLabel(
        "Location",
        LMetric(ThemeToken::TextSizeCaption) * s,
        LColor(ThemeToken::TextMuted)));
    auto location = std::make_shared<PathPickerField>(m_WizardLocation, true);
    location->SetDialogTitle("Select Project Location");
    location->SetOnChanged([this](const std::string& path) {
        m_WizardLocation = path;
        RebuildCreateWizard();
    });
    rightCol->AddChild(location);

    auto rightFiller = std::make_shared<FixedGap>(1.0f, 1.0f);
    rightFiller->SetVerticalAlignment(VerticalAlignment::Fill);
    rightCol->AddChild(rightFiller);
    body->AddChild(rightCol);

    panel->AddChild(body);

    // Footer
    auto footer = std::make_shared<HorizontalBox>();
    footer->SetSpacing(8.0f * s);
    footer->SetPadding(Margin{ 20.0f * s, 12.0f * s, 20.0f * s, 16.0f * s });
    footer->SetVerticalAlignment(VerticalAlignment::Center);
    footer->AddChild(std::make_shared<Spacer>());
    auto cancel = std::make_shared<SecondaryToolbarButton>("Cancel", "");
    cancel->SetOnClicked([this] { CloseModal(); });
    footer->AddChild(cancel);
    auto create = std::make_shared<PrimaryToolbarButton>("Create project", Icons::PlusName);
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
    footer->AddChild(create);
    panel->AddChild(footer);

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
    m_WizardCategory = "All";
    m_WizardTemplateQuery.clear();
    if (m_WizardLocation.empty()) {
        m_WizardLocation = m_Context->Settings().Settings().defaultProjectsRoot;
    }
    if (m_WizardTemplateId.empty() || !m_Context->Templates().Find(m_WizardTemplateId)) {
        m_WizardTemplateId = m_Context->Settings().Settings().defaultTemplateId;
        if (!m_Context->Templates().Find(m_WizardTemplateId)) {
            m_WizardTemplateId = "Blank";
        }
    }
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
        m_ModalHost->SetDialogHeight(0.0f);
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
