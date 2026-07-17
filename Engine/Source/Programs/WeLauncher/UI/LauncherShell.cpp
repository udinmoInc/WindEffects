#include "UI/LauncherShell.h"

#include "UI/LauncherHelpers.h"
#include "UI/Pages/LauncherPages.h"
#include "UI/Pages/Projects/ProjectsPage.h"

#include "KindUI/App/ViewHost.h"
#include "KindUI/Declarative/UI.h"
#include "UI/Widgets/CreateProjectViews.h"
#include "UI/Widgets/ManagerViews.h"
#include "UI/Widgets/SettingsViews.h"
#include "UI/Widgets/SkeletonViews.h"
#include "Util/LauncherMaintenance.h"
#include "Util/PathUtils.h"

#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Layout/Spacer.h"
#include "Platform/PlatformSDK.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Theming/ThemeToken.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string_view>

using namespace we::runtime::kindui;

namespace we::programs::welauncher {
namespace {
namespace UI = we::runtime::kindui::UI;

std::shared_ptr<ScrollLayout> FindScrollById(const std::shared_ptr<Widget>& root, std::string_view id) {
    if (!root) {
        return nullptr;
    }
    if (root->GetId() == id) {
        return std::dynamic_pointer_cast<ScrollLayout>(root);
    }
    for (const auto& child : root->GetChildren()) {
        if (auto found = FindScrollById(child, id)) {
            return found;
        }
    }
    return nullptr;
}

std::shared_ptr<Label> MakeLabel(const std::string& text, float size, Color color) {
    auto label = std::make_shared<Label>(text, color, size);
    label->SetHorizontalAlignment(HorizontalAlignment::Left);
    return label;
}

std::shared_ptr<Column> MakePageBodyPadding() {
    auto content = std::make_shared<Column>();
    content->Padding(Margin{
        kLauncherContentPadX * LScale(),
        LMetric(ThemeToken::Space4) * LScale(),
        kLauncherContentPadX * LScale(),
        LMetric(ThemeToken::Space4) * LScale()
    });
    content->Gap(LMetric(ThemeToken::Space4) * LScale());
    content->SetHorizontalAlignment(HorizontalAlignment::Fill);
    return content;
}

std::shared_ptr<Widget> MakeSectionHeader(const std::string& title, const std::string& subtitle = {}) {
    auto heading = std::make_shared<SectionHeader>(title, subtitle);
    heading->SetHorizontalAlignment(HorizontalAlignment::Fill);
    return heading;
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
    m_ProjectsPage = std::make_unique<ProjectsPage>(*m_Context, *this);
    m_ProjectsPage->SetSearchText(m_SearchQuery);

    BeginPageContentLoad(0.32f);
    RebuildChrome();
    EnsurePageBuilt(m_Page);
    ShowPage(m_Page);
    UpdateFooter();
    SyncHeaderFromState();
    SetStatus(m_Context->StatusMessage().empty() ? "Ready" : m_Context->StatusMessage());
    InvalidateUI();
}

void LauncherShell::SetHostContext(std::shared_ptr<IWidgetContext> context) {
    m_HostContext = std::move(context);
    InitializeApplicationServices();
}

void LauncherShell::InitializeApplicationServices() {
    if (!m_HostContext) {
        return;
    }

    if (auto widgetContext = std::dynamic_pointer_cast<WidgetContext>(m_HostContext)) {
        if (!m_PopupHost) {
            m_PopupHost = std::make_shared<OverlayHost>();
        }
        widgetContext->SetPopupHost(m_PopupHost.get());
    }

    if (!m_Services.dialogs) {
        m_Services.Initialize(m_HostContext, m_PopupHost ? m_PopupHost.get() : nullptr);
        m_DialogOverlay = m_Services.dialogs ? m_Services.dialogs->Overlay() : nullptr;
        if (m_DialogOverlay) {
            m_DialogOverlay->SetVisible(false);
            m_DialogOverlay->SetOnScrimClicked([this] {
                if (m_Services.dialogs) {
                    m_Services.dialogs->Dismiss();
                }
            });
        }
    }

    if (!m_RenameDialog) {
        m_RenameDialog = std::make_unique<RenameProjectDialog>(
            *this,
            [this] { return SelectedProjectSummary(); });
    }
}

void LauncherShell::EnsureProjectsPage() {
    if (!m_ProjectsPage) {
        m_ProjectsPage = std::make_unique<ProjectsPage>(*m_Context, *this);
    }
}

const ProjectSummary* LauncherShell::SelectedProjectSummary() const {
    if (!m_ProjectsPage) {
        return nullptr;
    }
    return m_ProjectsPage->ViewModel().SelectedProject();
}

void LauncherShell::BeginLoading(float durationSeconds) {
    BeginPageContentLoad(durationSeconds);
    EnsureProjectsPage();
    m_ProjectsPage->SetLoading(true);
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
    (void)codepoint;
}

void LauncherShell::RebuildChrome() {
    ClearChildren();

    auto root = std::make_shared<Column>();
    root->Gap(0.0f);
    root->Padding(Margin{});

    m_TitleBar = std::make_shared<LauncherTitleBar>(m_Window, "WindEffects Launcher");
    m_TitleBar->SetLogoTexture(m_LogoSet);
    m_TitleBar->SetOnHelp([this] { ShowHelp(); });
    m_TitleBar->SetOnSettings([this] { GoToPage(LauncherPage::Settings); });
    root->AddChild(m_TitleBar);

    auto body = std::make_shared<Row>();
    body->Gap(0.0f);
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

    auto mainColumn = std::make_shared<Column>();
    mainColumn->Gap(0.0f);
    mainColumn->SetHorizontalAlignment(HorizontalAlignment::Fill);
    mainColumn->SetVerticalAlignment(VerticalAlignment::Fill);

    auto contentHost = std::make_shared<Column>();
    contentHost->SetHorizontalAlignment(HorizontalAlignment::Fill);
    contentHost->SetVerticalAlignment(VerticalAlignment::Fill);
    contentHost->Background(LColor(ThemeToken::PanelContentBackground));
    m_ContentHost = contentHost;
    mainColumn->AddChild(m_ContentHost);

    body->AddChild(mainColumn);
    root->AddChild(body);

    m_Footer = std::make_shared<StatusFooter>();
    root->AddChild(m_Footer);

    m_ModalHost = we::runtime::kindui::MakeModalHost();
    m_ModalHost->SetVisible(false);
    m_ModalHost->SetOnScrimClicked([this] { CloseModal(); });

    m_Root = root;
    AddChild(m_Root);
    AddChild(m_ModalHost);
}

void LauncherShell::SyncHeaderFromState() {
    if (m_Page == LauncherPage::Projects && m_ProjectsPage) {
        m_SearchQuery = m_ProjectsPage->SearchText();
    }
}

void LauncherShell::CapturePageScroll(LauncherPage page) {
    const auto idx = static_cast<std::size_t>(page);
    if (idx >= m_Pages.size()) {
        return;
    }
    auto& state = m_Pages[idx];
    if (page == LauncherPage::Projects && m_ProjectsPage) {
        m_ProjectsPage->CaptureScroll(state);
        return;
    }
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
    if (page == LauncherPage::Projects && m_ProjectsPage) {
        m_ProjectsPage->RestoreScroll(state);
        return;
    }
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
    if (page == LauncherPage::Projects && m_ProjectsPage) {
        m_ProjectsPage->SetSearchText(m_SearchQuery);
    }
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
    auto root = std::make_shared<Column>();
    root->Gap(LMetric(ThemeToken::Space4) * LScale());
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
        EnsureProjectsPage();
        m_ProjectsPage->SetSearchText(text);
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

void LauncherShell::CycleSortMode() {
    EnsureProjectsPage();
    switch (m_ProjectsPage->SortMode()) {
    case ProjectSortMode::Recent: SetSortMode(ProjectSortMode::Name); break;
    case ProjectSortMode::Name: SetSortMode(ProjectSortMode::Engine); break;
    case ProjectSortMode::Engine:
    default: SetSortMode(ProjectSortMode::Recent); break;
    }
}

void LauncherShell::SetSortMode(ProjectSortMode mode) {
    EnsureProjectsPage();
    m_ProjectsPage->SetSortMode(mode);
}

void LauncherShell::ToggleCompatibleFilter() {
    EnsureProjectsPage();
    m_ProjectsPage->ToggleCompatibleFilter();
}

void LauncherShell::ShowHelp() {
    SetStatus(
        "Help â€” Ctrl+N new project, Ctrl+O browse, Enter open selected, "
        "Arrow keys navigate sidebar, Esc closes dialogs.");
}

bool LauncherShell::IsFavorite(const std::string& weprojPath) const {
    return m_ProjectsPage && m_ProjectsPage->Model().IsFavorite(weprojPath);
}

void LauncherShell::ToggleFavorite(const std::string& weprojPath) {
    if (weprojPath.empty() || !m_ProjectsPage) {
        return;
    }
    m_ProjectsPage->Model().ToggleFavorite(weprojPath);
    m_ProjectsPage->ViewModel().RecomputeVisibleProjects();
    SetStatus(m_ProjectsPage->Model().IsFavorite(weprojPath)
        ? "Pinned project"
        : "Removed from pinned projects");
}

void LauncherShell::ShowProjectMoreMenu(std::size_t index) {
    SelectProject(index);
    m_Modal = ModalKind::Actions;
    RebuildProjectActionsDialog();
}

void LauncherShell::RebuildProjectsPage() {
    EnsureProjectsPage();
    auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Projects)];
    m_ProjectsPage->SetLoading(IsPageContentLoading());
    m_ProjectsPage->SetSearchText(m_SearchQuery);
    m_ProjectsPage->Attach(state, m_HostContext);
}


void LauncherShell::ApplyDeclarativePage(PageState& state, const Element& view, const char* scrollId) {
    if (!m_HostContext) {
        return;
    }
    if (!state.viewHost) {
        state.viewHost = std::make_unique<ViewHost>(m_HostContext);
        state.viewHost->SetView(view);
    } else {
        state.viewHost->Reconcile(view);
    }
    state.root = state.viewHost->GetRoot();
    if (state.root) {
        state.root->SetHorizontalAlignment(HorizontalAlignment::Fill);
        state.root->SetVerticalAlignment(VerticalAlignment::Fill);
        if (auto box = std::dynamic_pointer_cast<Column>(state.root)) {
            box->Background(LColor(ThemeToken::PanelContentBackground));
        }
    }
    state.scroll = scrollId ? FindScrollById(state.root, scrollId) : nullptr;
}

LearnPageModel LauncherShell::BuildLearnPageModel() {
    LearnPageModel model;
    model.chrome.title = "Learn";
    model.chrome.subtitle = "Samples, guides, and getting started";
    model.chrome.searchPlaceholder = "Search Learn...";
    model.chrome.searchQuery = m_SearchQuery;
    model.chrome.loading = IsPageContentLoading();
    model.chrome.onSearchChanged = [this](const std::string& text) { OnSearchChanged(text); };

    model.chrome.toolbarActions.push_back(UI::Host([this]() {
        auto btn = MakeSecondaryAction("Open Docs", Icons::DocumentName);
        btn->SetOnClicked([this] { SetStatus("Documentation — coming soon"); });
        return btn;
    }));
    model.chrome.toolbarActions.push_back(UI::Host([this]() {
        auto btn = MakeSecondaryAction("Refresh", Icons::RefreshName);
        btn->SetOnClicked([this] {
            BeginPageContentLoad(0.2f);
            MarkPageDirty(LauncherPage::Learn);
            EnsurePageBuilt(LauncherPage::Learn);
            ShowPage(LauncherPage::Learn);
            SetStatus("Learn content refreshed");
        });
        return btn;
    }));

    if (model.chrome.loading) {
        return model;
    }

    const std::vector<LearnEntryModel> allEntries = {
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
    model.entries.reserve(allEntries.size());
    for (const auto& entry : allEntries) {
        if (!query.empty()) {
            const std::string hay = ToLowerCopy(entry.kind + " " + entry.name + " " + entry.detail);
            if (hay.find(query) == std::string::npos) {
                continue;
            }
        }
        model.entries.push_back(entry);
    }

    if (model.entries.empty()) {
        model.showEmpty = true;
        model.emptyTitle = query.empty() ? "Nothing here yet" : "No matching items";
        model.emptySubtitle = query.empty()
            ? "Samples and guides will appear here as they become available."
            : "Try a different search or clear the current query.";
        if (!query.empty()) {
            model.onClearSearch = [this] { OnSearchChanged({}); };
        }
    }
    return model;
}

EnginePageModel LauncherShell::BuildEnginePageModel() {
    EnginePageModel model;
    model.chrome.title = "Engine";
    model.chrome.subtitle = "Installed engines and SDK health";
    model.chrome.searchPlaceholder = "Search Engine installs...";
    model.chrome.searchQuery = m_SearchQuery;
    model.chrome.loading = IsPageContentLoading();
    model.chrome.onSearchChanged = [this](const std::string& text) { OnSearchChanged(text); };

    model.chrome.toolbarActions.push_back(UI::Host([this]() {
        auto btn = MakeSecondaryAction("Refresh", Icons::RefreshName);
        btn->SetOnClicked([this] {
            BeginPageContentLoad(0.2f);
            UpdateFooter();
            MarkPageDirty(LauncherPage::Engine);
            EnsurePageBuilt(LauncherPage::Engine);
            ShowPage(LauncherPage::Engine);
            SetStatus("Engine status refreshed");
        });
        return btn;
    }));

    if (model.chrome.loading) {
        return model;
    }

    model.hasEngine = m_Context->Engines().HasEngine()
        && !m_Context->Engines().InstalledEngines().empty();

    if (!model.hasEngine) {
        model.showEmpty = true;
        model.emptyTitle = "No Engine Found";
        model.emptySubtitle = "Place the launcher next to an Engine install, or build the engine from source.";
        model.onOpenProjects = [this] { GoToPage(LauncherPage::Projects); };
        model.onRefresh = [this] {
            BeginPageContentLoad(0.2f);
            MarkPageDirty(LauncherPage::Engine);
            EnsurePageBuilt(LauncherPage::Engine);
            ShowPage(LauncherPage::Engine);
        };
        return model;
    }

    std::string sdkSummary;
    {
        int pass = 0;
        int warn = 0;
        int fail = 0;
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
        model.showEmpty = true;
        model.emptyTitle = "No matching installs";
        model.emptySubtitle = "Try a different search or clear the current query.";
        model.onClearSearch = [this] { OnSearchChanged({}); };
        return model;
    }

    const float s = LScale();
    model.rows.reserve(visible.size() + m_Context->Sdk().RunChecks().size() + 1);
    for (std::size_t i = 0; i < visible.size(); ++i) {
        const EngineInstallInfo rowInfo = visible[i];
        const std::string root = rowInfo.engineRoot;
        model.rows.push_back(UI::Id(UI::Host([this, rowInfo, root]() {
            auto row = std::make_shared<EngineInstallRow>(rowInfo, rowInfo.isCurrent);
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
            return row;
        }), "engine-row-" + std::to_string(i)));
    }

    model.rows.push_back(UI::Host([s]() {
        auto label = std::make_shared<Label>(
            "SDK checks",
            LColor(ThemeToken::TextPrimary),
            LMetric(ThemeToken::TextSizeBody) * s);
        label->SetHorizontalAlignment(HorizontalAlignment::Left);
        return label;
    }));

    for (const auto& check : m_Context->Sdk().RunChecks()) {
        Color color = LColor(ThemeToken::TextMuted);
        if (check.status == SdkCheckStatus::Pass) {
            color = LColor(ThemeToken::Success);
        } else if (check.status == SdkCheckStatus::Warn) {
            color = LColor(ThemeToken::Warning);
        } else if (check.status == SdkCheckStatus::Fail) {
            color = LColor(ThemeToken::ErrorForeground);
        }
        const std::string line = check.name + " — " + check.detail;
        model.rows.push_back(UI::Host([s, line, color]() {
            auto label = std::make_shared<Label>(line, color, LMetric(ThemeToken::TextSizeCaption) * s);
            label->SetHorizontalAlignment(HorizontalAlignment::Left);
            return label;
        }));
    }

    return model;
}

void LauncherShell::RebuildLearnPage() {
    auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Learn)];
    const LearnPageModel model = BuildLearnPageModel();
    ApplyDeclarativePage(state, BuildLearnPage(model), model.showEmpty ? nullptr : "learn-scroll");
}

void LauncherShell::RebuildEnginePage() {
    auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Engine)];
    const EnginePageModel model = BuildEnginePageModel();
    ApplyDeclarativePage(state, BuildEnginePage(model), model.showEmpty ? nullptr : "engine-scroll");
}

void LauncherShell::PersistLauncherSettings(const std::string& statusMessage) {
    m_Context->Save();
    SetStatus(statusMessage.empty() ? "Settings saved" : statusMessage);
}

namespace {

std::shared_ptr<Column> MakeSettingsStack() {
    auto stack = std::make_shared<Column>();
    stack->Gap(8.0f * LScale());
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
    Column& body,
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
    const std::shared_ptr<Column>& body,
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

    auto scan = MakeSecondaryAction("Scan for Installed Engines", Icons::RefreshName);
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

    auto verify = MakeSecondaryAction("Verify Engine Installation", Icons::CheckName);
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

    auto updates = MakeSecondaryAction("Check for Launcher Updates", Icons::RefreshName);
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

    auto clearCache = MakeSecondaryAction("Clear Cache", Icons::DeleteName);
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

    auto clearThumbs = MakeSecondaryAction("Clear Cache", Icons::DeleteName);
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

    auto weproj = MakeSecondaryAction("Associate .weproj", Icons::DocumentName);
    weproj->SetOnClicked([this] {
        const auto result = AssociateProjectExtension(".weproj");
        SetStatus(result.message);
    });
    AppendSettingsRow(*body, queryLower, "Associate .weproj files", "Open WindEffects projects with WeLauncher", weproj);

    auto weproject = MakeSecondaryAction("Associate .weproject", Icons::DocumentName);
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

    auto logs = MakeSecondaryAction("Open Logs Folder", Icons::OpenFolderName);
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

    auto install = MakeSecondaryAction("Open Installation Folder", Icons::OpenFolderName);
    install->SetOnClicked([this] {
        const auto path = PathUtils::ToUtf8(PathUtils::GetExecutableDirectory());
        if (OpenPathInExplorer(path) || RevealInExplorer(path)) {
            SetStatus("Opened installation folder");
        } else {
            SetStatus("Failed to open installation folder");
        }
    });
    AppendSettingsRow(*body, queryLower, "Open Installation Folder", "Folder containing WeLauncher.exe", install);

    auto docs = MakeSecondaryAction("Documentation", Icons::DocumentName);
    docs->SetOnClicked([this] {
        if (OpenUrl("https://windeffects.dev/docs")) {
            SetStatus("Opened documentation");
        } else {
            SetStatus("Documentation — coming soon");
        }
    });
    AppendSettingsRow(*body, queryLower, "Documentation", "Online guides and API reference", docs);

    auto reset = MakeSecondaryAction("Reset Launcher Settings", Icons::RefreshName);
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

    auto page = std::make_shared<Column>();
    page->Gap(0.0f);
    page->SetHorizontalAlignment(HorizontalAlignment::Fill);
    page->SetVerticalAlignment(VerticalAlignment::Fill);
    page->Background(LColor(ThemeToken::PanelContentBackground));

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
    auto content = std::make_shared<Column>();
    content->Gap(20.0f * s);
    content->Padding(Margin{
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

void LauncherShell::CommitCreateProject() {
    m_Context->Settings().Settings().defaultTemplateId = m_WizardTemplateId;
    m_Context->Settings().Settings().qualityPreset = m_WizardQuality;
    PersistLauncherSettings({});
    const auto result = m_Context->Projects().CreateProject(
        m_WizardName,
        m_WizardTemplateId,
        PathUtils::FromUtf8(m_WizardLocation));
    SetStatus(result.message);
    if (result.success) {
        CloseModal();
        RefreshProjectList();
    }
}

void LauncherShell::SelectWizardTemplateByDelta(int delta) {
    const auto& templates = m_Context->Templates().Templates();
    const std::string query = ToLowerCopy(m_WizardTemplateQuery);
    std::vector<std::string> ids;
    for (const auto& tmpl : templates) {
        if (!TemplateMatchesWizardFilter(tmpl, m_WizardCategory)) {
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
        ids.push_back(tmpl.id);
    }
    if (ids.empty()) {
        return;
    }
    int index = 0;
    for (int i = 0; i < static_cast<int>(ids.size()); ++i) {
        if (ids[static_cast<std::size_t>(i)] == m_WizardTemplateId) {
            index = i;
            break;
        }
    }
    index = std::clamp(index + delta, 0, static_cast<int>(ids.size()) - 1);
    m_WizardTemplateId = ids[static_cast<std::size_t>(index)];
    RebuildCreateWizard();
}

void LauncherShell::RebuildCreateWizard() {
    if (!m_ModalHost) {
        return;
    }
    const float s = LScale();
    m_ModalHost->SetVisible(true);
    m_ModalHost->SetDialogWidth(WizardDialogShell::kWidth);
    m_ModalHost->SetDialogHeight(WizardDialogShell::kHeight);

    const auto& templates = m_Context->Templates().Templates();
    const std::string query = ToLowerCopy(m_WizardTemplateQuery);

    std::vector<const ProjectTemplateInfo*> visible;
    visible.reserve(templates.size());
    for (const auto& tmpl : templates) {
        if (!TemplateMatchesWizardFilter(tmpl, m_WizardCategory)) {
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
    }

    const ProjectTemplateInfo* selected = FindTemplateById(templates, m_WizardTemplateId);
    const std::string diskEstimate = selected
        ? EstimateTemplateDiskUsage(*selected)
        : "—";
    const std::string projectPathPreview = m_WizardLocation.empty()
        ? "—"
        : (m_WizardLocation + (m_WizardLocation.back() == '/' || m_WizardLocation.back() == '\\' ? "" : "/")
            + (m_WizardName.empty() ? "MyProject" : m_WizardName));

    auto shell = std::make_shared<WizardDialogShell>();

    auto root = std::make_shared<Column>();
    root->Gap(0.0f);
    root->SetHorizontalAlignment(HorizontalAlignment::Fill);
    root->SetVerticalAlignment(VerticalAlignment::Fill);
    root->Padding(Margin{ 24.0f * s, 24.0f * s, 24.0f * s, 24.0f * s });

    // Title
    auto title = MakeLabel("New project", 30.0f * s, LColor(ThemeToken::TextPrimary));
    title->SetHorizontalAlignment(HorizontalAlignment::Left);
    root->AddChild(title);
    root->AddChild(std::make_shared<FixedGap>(1.0f, 12.0f * s));

    // Horizontal filter chips
    auto chips = std::make_shared<Row>();
    chips->Gap(8.0f * s);
    chips->SetVerticalAlignment(VerticalAlignment::Center);
    static const char* kFilters[] = { "All", "Games", "3D", "VR", "XR", "Samples" };
    for (const char* filter : kFilters) {
        const bool active = m_WizardCategory == filter;
        auto chip = std::make_shared<FilterChip>(filter, active);
        chip->SetOnClicked([this, filter = std::string(filter)] {
            m_WizardCategory = filter;
            RebuildCreateWizard();
        });
        chips->AddChild(chip);
    }
    root->AddChild(chips);
    root->AddChild(std::make_shared<FixedGap>(1.0f, 16.0f * s));
    root->AddChild(std::make_shared<WizardSeparator>());
    root->AddChild(std::make_shared<FixedGap>(1.0f, 16.0f * s));

    // Main two columns
    auto body = std::make_shared<Row>();
    body->Gap(0.0f);
    body->SetHorizontalAlignment(HorizontalAlignment::Fill);
    body->SetVerticalAlignment(VerticalAlignment::Fill);

    // LEFT — template list (~420)
    auto left = std::make_shared<Column>();
    left->Gap(12.0f * s);
    left->SetHorizontalAlignment(HorizontalAlignment::Fill);
    left->SetVerticalAlignment(VerticalAlignment::Fill);
    left->AddChild(std::make_shared<FixedGap>(420.0f * s, 1.0f));

    auto search = std::make_shared<CompactSearchField>("Search Templates");
    search->SetText(m_WizardTemplateQuery);
    search->SetOnChanged([this](const std::string& text) {
        if (m_WizardTemplateQuery == text) {
            return;
        }
        m_WizardTemplateQuery = text;
        RebuildCreateWizard();
    });
    left->AddChild(search);

    auto listScroll = std::make_shared<ScrollLayout>();
    auto list = std::make_shared<Column>();
    list->Gap(4.0f * s);
    list->SetHorizontalAlignment(HorizontalAlignment::Fill);

    if (visible.empty()) {
        list->AddChild(MakeLabel(
            "No matching templates",
            13.0f * s,
            LColor(ThemeToken::TextMuted)));
    } else {
        for (const auto* tmpl : visible) {
            const bool isSelected = selected && tmpl->id == selected->id;
            auto row = std::make_shared<CreateTemplateRow>(*tmpl, isSelected);
            row->SetOnSelect([this, id = tmpl->id] {
                m_WizardTemplateId = id;
                m_Context->Settings().Settings().defaultTemplateId = id;
                RebuildCreateWizard();
            });
            row->SetOnActivate([this, id = tmpl->id] {
                m_WizardTemplateId = id;
                CommitCreateProject();
            });
            list->AddChild(row);
        }
    }
    listScroll->SetContent(list);
    listScroll->SetHorizontalAlignment(HorizontalAlignment::Fill);
    listScroll->SetVerticalAlignment(VerticalAlignment::Fill);
    left->AddChild(listScroll);
    body->AddChild(left);

    // Vertical separator
    auto vlineHost = std::make_shared<Column>();
    vlineHost->Padding(Margin{ 16.0f * s, 0.0f, 16.0f * s, 0.0f });
    vlineHost->SetVerticalAlignment(VerticalAlignment::Fill);
    auto vline = std::make_shared<Column>();
    vline->Background(LColor(ThemeToken::Separator));
    vline->SetVerticalAlignment(VerticalAlignment::Fill);
    vline->AddChild(std::make_shared<FixedGap>(1.0f * s, 1.0f));
    vlineHost->AddChild(vline);
    body->AddChild(vlineHost);

    // RIGHT — details + settings (fixed, no nested scroll)
    auto right = std::make_shared<Column>();
    right->Gap(12.0f * s);
    right->SetHorizontalAlignment(HorizontalAlignment::Fill);
    right->SetVerticalAlignment(VerticalAlignment::Fill);

    right->AddChild(MakeLabel(
        "Selected Template",
        16.0f * s,
        LColor(ThemeToken::TextPrimary)));

    if (selected) {
        right->AddChild(MakeLabel(
            selected->displayName,
            18.0f * s,
            LColor(ThemeToken::TextPrimary)));
        right->AddChild(MakeLabel(
            selected->description.empty() ? "No description." : selected->description,
            12.0f * s,
            LColor(ThemeToken::TextMuted)));

        auto addMeta = [&](const char* label, const std::string& value) {
            auto row = std::make_shared<Row>();
            row->Gap(8.0f * s);
            row->SetVerticalAlignment(VerticalAlignment::Center);
            auto l = MakeLabel(label, 13.0f * s, LColor(ThemeToken::TextSecondary));
            l->SetHorizontalAlignment(HorizontalAlignment::Left);
            row->AddChild(l);
            row->AddChild(std::make_shared<Spacer>());
            auto v = MakeLabel(value.empty() ? "—" : value, 13.0f * s, LColor(ThemeToken::TextPrimary));
            v->SetHorizontalAlignment(HorizontalAlignment::Right);
            row->AddChild(v);
            right->AddChild(row);
        };

        std::string engineCompat = "Any WindEffects install";
        if (m_Context->Engines().HasEngine()) {
            const auto& eng = m_Context->Engines().Current();
            engineCompat = eng.engineVersion.empty() ? "Local engine" : eng.engineVersion;
        }
        addMeta("Engine compatibility", engineCompat);
        addMeta("Required plugins", selected->plugins.empty() ? "None" : JoinList(selected->plugins));
        addMeta("Estimated disk usage", diskEstimate);
        addMeta("Recommended hardware", TemplateHardwareHint(*selected));
        addMeta("Template version", "1.0.0");
        addMeta("Template size", diskEstimate);
    } else {
        right->AddChild(MakeLabel(
            "Select a template from the list.",
            13.0f * s,
            LColor(ThemeToken::TextMuted)));
    }

    right->AddChild(std::make_shared<FixedGap>(1.0f, 8.0f * s));
    right->AddChild(std::make_shared<WizardSeparator>());
    right->AddChild(MakeLabel(
        "Project Settings",
        16.0f * s,
        LColor(ThemeToken::TextPrimary)));

    right->AddChild(MakeLabel("Project Name", 13.0f * s, LColor(ThemeToken::TextSecondary)));
    auto nameBox = std::make_shared<TextBox>(m_WizardName, [this](const std::string& text) {
        m_WizardName = text;
    });
    right->AddChild(nameBox);

    right->AddChild(MakeLabel("Project Location", 13.0f * s, LColor(ThemeToken::TextSecondary)));
    auto location = std::make_shared<PathPickerField>(m_WizardLocation, true);
    location->SetDialogTitle("Select Project Location");
    location->SetOnChanged([this](const std::string& path) {
        m_WizardLocation = path;
        RebuildCreateWizard();
    });
    right->AddChild(location);

    right->AddChild(MakeLabel("Engine Version", 13.0f * s, LColor(ThemeToken::TextSecondary)));
    std::vector<std::string> engineLabels{ "Auto-detect" };
    std::vector<std::string> engineRoots{ {} };
    int engineIndex = 0;
    const auto& settings = m_Context->Settings().Settings();
    for (const auto& install : m_Context->Engines().InstalledEngines()) {
        const std::string version = install.engineVersion.empty() ? "Unknown" : install.engineVersion;
        engineLabels.push_back(version);
        engineRoots.push_back(install.engineRoot);
        if (install.engineRoot == settings.selectedEngineRoot) {
            engineIndex = static_cast<int>(engineRoots.size()) - 1;
        }
    }
    auto engineDrop = std::make_shared<SettingsDropdown>(engineLabels, engineIndex);
    engineDrop->SetOnChanged([this, engineRoots](int index) {
        if (index < 0 || index >= static_cast<int>(engineRoots.size())) {
            return;
        }
        m_Context->Settings().Settings().selectedEngineRoot = engineRoots[static_cast<std::size_t>(index)];
        PersistLauncherSettings("Engine version updated");
    });
    right->AddChild(engineDrop);

    right->AddChild(MakeLabel("Quality Preset", 13.0f * s, LColor(ThemeToken::TextSecondary)));
    static const char* kQuality[] = { "Performance", "Balanced", "High Quality", "Ultra" };
    int qualityIndex = 1;
    for (int i = 0; i < 4; ++i) {
        if (m_WizardQuality == kQuality[i]) {
            qualityIndex = i;
            break;
        }
    }
    auto qualityDrop = std::make_shared<SettingsDropdown>(
        std::vector<std::string>{ kQuality[0], kQuality[1], kQuality[2], kQuality[3] },
        qualityIndex);
    qualityDrop->SetOnChanged([this](int index) {
        static const char* kQ[] = { "Performance", "Balanced", "High Quality", "Ultra" };
        if (index >= 0 && index < 4) {
            m_WizardQuality = kQ[index];
            m_Context->Settings().Settings().qualityPreset = m_WizardQuality;
        }
    });
    right->AddChild(qualityDrop);

    auto rightFiller = std::make_shared<FixedGap>(1.0f, 1.0f);
    rightFiller->SetVerticalAlignment(VerticalAlignment::Fill);
    right->AddChild(rightFiller);
    body->AddChild(right);

    root->AddChild(body);
    root->AddChild(std::make_shared<FixedGap>(1.0f, 16.0f * s));
    root->AddChild(std::make_shared<WizardSeparator>());
    root->AddChild(std::make_shared<FixedGap>(1.0f, 16.0f * s));

    // Footer
    auto footer = std::make_shared<Row>();
    footer->Gap(12.0f * s);
    footer->SetVerticalAlignment(VerticalAlignment::Center);

    auto footerLeft = std::make_shared<Column>();
    footerLeft->Gap(2.0f * s);
    footerLeft->AddChild(MakeLabel(
        "Estimated disk usage  ·  " + diskEstimate,
        12.0f * s,
        LColor(ThemeToken::TextMuted)));
    footerLeft->AddChild(MakeLabel(
        EllipsizePath(projectPathPreview, 64),
        12.0f * s,
        LColor(ThemeToken::TextSecondary)));
    footer->AddChild(footerLeft);
    footer->AddChild(std::make_shared<Spacer>());

    auto cancel = MakeSecondaryAction("Cancel", "");
    cancel->SetOnClicked([this] { CloseModal(); });
    footer->AddChild(cancel);
    auto create = MakePrimaryAction("Create Project", Icons::PlusName);
    create->SetOnClicked([this] { CommitCreateProject(); });
    footer->AddChild(create);
    root->AddChild(footer);

    shell->SetContent(root);
    m_ModalHost->SetContent(shell);
    InvalidateUI();
}

void LauncherShell::RebuildProjectActionsDialog() {
    if (!m_ModalHost) {
        return;
    }
    const ProjectSummary* project = SelectedProjectSummary();
    if (!project) {
        CloseModal();
        return;
    }

    m_ModalHost->SetVisible(true);
    m_ModalHost->SetDialogWidth(420.0f);

    auto panel = std::make_shared<Column>();
    panel->Padding(Margin{
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale(),
        LMetric(ThemeToken::Space6) * LScale()
    });
    panel->Gap(LMetric(ThemeToken::Space2) * LScale());
    panel->Background(LColor(ThemeToken::DialogBackground));
    panel->AddChild(MakeLabel(
        project->descriptor.displayName,
        LMetric(ThemeToken::TextSizeHeader) * LScale(),
        LColor(ThemeToken::TextPrimary)));
    panel->AddChild(MakeLabel(
        EllipsizePath(project->projectRoot, 52),
        LMetric(ThemeToken::TextSizeCaption) * LScale(),
        LColor(ThemeToken::TextMuted)));

    auto addAction = [this, &panel](const char* label, const char* icon, auto fn, bool closeAfter = true) {
        auto btn = MakeSecondaryAction(label, icon);
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
        IsFavorite(project->weprojPath) ? "Unpin" : "Pin",
        Icons::StarName,
        [this, path = project->weprojPath] { ToggleFavorite(path); });
    addAction("Show in Explorer", Icons::OpenFolderName, [this] { ShowSelectedInExplorer(); });
    addAction("Regenerate Project Files", Icons::BuildName, [this] { RegenerateSelectedProjectFiles(); });
    addAction("Delete", Icons::DeleteName, [this] { DeleteSelectedProject(); });

    auto closeRow = std::make_shared<Row>();
    closeRow->AddChild(std::make_shared<Spacer>());
    auto close = MakeSecondaryAction("Close", "");
    close->SetOnClicked([this] { CloseModal(); });
    closeRow->AddChild(close);
    panel->AddChild(closeRow);

    m_ModalHost->SetContent(panel);
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
    EnsureProjectsPage();
    m_ProjectsPage->Reload();
    UpdateFooter();
    BeginPageContentLoad(0.24f);
    MarkPageDirty(LauncherPage::Projects);
    if (m_Page == LauncherPage::Projects) {
        EnsurePageBuilt(LauncherPage::Projects);
        ShowPage(LauncherPage::Projects);
    }
    InvalidateUI();
}

void LauncherShell::SetStatus(const std::string& message) {
    m_Context->SetStatusMessage(message);
    if (m_Footer) {
        m_Footer->SetStatus(message);
    }
    InvalidateUI();
}

void LauncherShell::SelectProject(std::size_t index, bool additive) {
    EnsureProjectsPage();
    m_ProjectsPage->SelectProject(index, additive);
}

void LauncherShell::RegenerateSelectedProjectFiles() {
    const ProjectSummary* project = SelectedProjectSummary();
    if (!project) {
        SetStatus("No project selected.");
        return;
    }
    SetStatus("Regenerate project files queued for " + project->descriptor.displayName);
}

void LauncherShell::ShowCreateWizard() {
    m_Modal = ModalKind::Create;
    m_WizardCategory = "All";
    m_WizardTemplateQuery.clear();
    m_WizardQuality = m_Context->Settings().Settings().qualityPreset;
    if (m_WizardQuality.empty()) {
        m_WizardQuality = "Balanced";
    }
    if (m_WizardLocation.empty()) {
        m_WizardLocation = m_Context->Settings().Settings().defaultProjectsRoot;
    }
    m_WizardTemplateId = m_Context->Settings().Settings().defaultTemplateId;
    if (m_WizardTemplateId.empty() || !m_Context->Templates().Find(m_WizardTemplateId)) {
        m_WizardTemplateId = "Blank";
    }
    RebuildCreateWizard();
}

void LauncherShell::ShowRenameDialog() {
    if (!m_Services.dialogs || !m_RenameDialog) {
        SetStatus("Rename dialog unavailable.");
        return;
    }
    if (m_Modal != ModalKind::None) {
        CloseModal();
    }
    m_RenameDialog->Show(*m_Services.dialogs);
    InvalidateUI();
}

void LauncherShell::CommitRenameProject(const std::string& newName) {
    RenameSelectedProject(newName);
}

void LauncherShell::CloseModal() {
    if (m_Services.dialogs && m_Services.dialogs->IsOpen()) {
        m_Services.dialogs->Dismiss();
        if (m_DialogOverlay) {
            m_DialogOverlay->SetVisible(false);
        }
    }
    m_Modal = ModalKind::None;
    if (m_ModalHost) {
        m_ModalHost->SetContent(nullptr);
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
    const ProjectSummary* project = SelectedProjectSummary();
    if (!project) {
        SetStatus("No project selected.");
        return;
    }
    const auto launch = m_Context->EditorLaunch().Launch(
        PathUtils::FromUtf8(project->weprojPath));
    SetStatus(launch.message);
    RefreshProjectList();
}

void LauncherShell::CloneSelectedProject() {
    const ProjectSummary* selected = SelectedProjectSummary();
    if (!selected) {
        return;
    }
    const auto result = m_Context->Projects().CloneProject(
        PathUtils::FromUtf8(selected->weprojPath),
        selected->descriptor.displayName + "_Copy");
    SetStatus(result.message);
    RefreshProjectList();
}

void LauncherShell::RenameSelectedProject(const std::string& newName) {
    const ProjectSummary* selected = SelectedProjectSummary();
    if (!selected) {
        return;
    }
    if (newName.empty()) {
        SetStatus("Name cannot be empty.");
        return;
    }
    const auto result = m_Context->Projects().RenameProject(
        PathUtils::FromUtf8(selected->weprojPath),
        newName);
    SetStatus(result.message);
    RefreshProjectList();
}

void LauncherShell::DeleteSelectedProject() {
    const ProjectSummary* selected = SelectedProjectSummary();
    if (!selected) {
        return;
    }

    auto& platform = we::platform::Platform::Get();
    const auto confirm = platform.ShowMessageBox({
        .title = "Delete Project",
        .message = ("Delete project and all files?\n" + selected->weprojPath).c_str(),
        .type = we::platform::MessageBoxType::Warning,
        .yesNo = true,
    });
    if (confirm != we::platform::MessageBoxResult::Yes) {
        return;
    }

    const auto result = m_Context->Projects().DeleteProject(PathUtils::FromUtf8(selected->weprojPath));
    SetStatus(result.message);
    RefreshProjectList();
}

void LauncherShell::ShowSelectedInExplorer() {
    const ProjectSummary* selected = SelectedProjectSummary();
    if (!selected) {
        return;
    }
    if (RevealInExplorer(selected->projectRoot)) {
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
    if (m_DialogOverlay && m_Services.dialogs && m_Services.dialogs->IsOpen()) {
        m_DialogOverlay->Measure(availableSize);
    }
    if (m_ModalHost && m_ModalHost->IsVisible()) {
        m_ModalHost->Measure(availableSize);
    }
    if (m_PopupHost && m_PopupHost->HasOpenPopups()) {
        m_PopupHost->Measure(availableSize);
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
    if (m_DialogOverlay && m_Services.dialogs && m_Services.dialogs->IsOpen()) {
        m_DialogOverlay->Arrange(allottedRect);
    }
    if (m_ModalHost && m_ModalHost->IsVisible()) {
        m_ModalHost->Arrange(allottedRect);
    }
    if (m_PopupHost && m_PopupHost->HasOpenPopups()) {
        m_PopupHost->Arrange(allottedRect);
    }
}

void LauncherShell::Paint(PaintContext& context) {
    context.DrawRect(GetGeometry(), LColor(ThemeToken::WorkspaceBackground));
    if (m_Root) {
        m_Root->Paint(context);
    }
    if (m_DialogOverlay && m_Services.dialogs && m_Services.dialogs->IsOpen()) {
        m_DialogOverlay->Paint(context);
    }
    if (m_ModalHost && m_ModalHost->IsVisible()) {
        m_ModalHost->Paint(context);
    }
    if (m_PopupHost && m_PopupHost->HasOpenPopups()) {
        m_PopupHost->Paint(context);
    }
}

void LauncherShell::Tick(float deltaTime) {
    if (m_PageContentLoading) {
        m_PageLoadTimer += deltaTime;
        AdvanceSkeletonShimmer(deltaTime);
        if (m_PageLoadTimer >= m_PageLoadDuration) {
            m_PageContentLoading = false;
            m_PageLoadTimer = 0.0f;
            if (m_Page == LauncherPage::Projects && m_ProjectsPage) {
                m_ProjectsPage->SetLoading(false);
            }
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
    if (event.key == we::platform::KeyCode::Escape) {
        if (m_Services.dialogs && m_Services.dialogs->IsOpen()) {
            m_Services.dialogs->Dismiss();
            if (m_DialogOverlay) {
                m_DialogOverlay->SetVisible(false);
            }
            InvalidateUI();
            return;
        }
        if (m_Modal != ModalKind::None) {
            CloseModal();
            return;
        }
    }

    if (m_Modal == ModalKind::Create) {
        if (event.key == we::platform::KeyCode::Up) {
            SelectWizardTemplateByDelta(-1);
            return;
        }
        if (event.key == we::platform::KeyCode::Down) {
            SelectWizardTemplateByDelta(1);
            return;
        }
        if (event.key == we::platform::KeyCode::Enter) {
            CommitCreateProject();
            return;
        }
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
        && !event.ctrlDown && !event.altDown && m_ProjectsPage) {
        const auto& visible = m_ProjectsPage->ViewModel().VisibleProjects.Items();
        if (visible.empty()) {
            // fall through to sidebar navigation
        } else {
            auto currentVisible = [&]() -> int {
                const int selected = m_ProjectsPage->SelectedIndex();
                if (selected < 0) {
                    return -1;
                }
                for (std::size_t i = 0; i < visible.size(); ++i) {
                    if (static_cast<int>(visible[i].sourceIndex) == selected) {
                        return static_cast<int>(i);
                    }
                }
                return -1;
            };

            if (event.key == we::platform::KeyCode::Up) {
                int idx = currentVisible();
                idx = idx <= 0 ? 0 : idx - 1;
                SelectProject(visible[static_cast<std::size_t>(idx)].sourceIndex, false);
                return;
            }
            if (event.key == we::platform::KeyCode::Down) {
                int idx = currentVisible();
                idx = idx < 0 ? 0 : std::min(idx + 1, static_cast<int>(visible.size()) - 1);
                SelectProject(visible[static_cast<std::size_t>(idx)].sourceIndex, false);
                return;
            }
            if (event.key == we::platform::KeyCode::Home) {
                SelectProject(visible.front().sourceIndex, false);
                return;
            }
            if (event.key == we::platform::KeyCode::End) {
                SelectProject(visible.back().sourceIndex, false);
                return;
            }
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
