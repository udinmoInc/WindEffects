#include "UI/Shell/LauncherShell.h"

#include "UI/Shell/LauncherHelpers.h"
#include "UI/Pages/LauncherPages.h"
#include "UI/Pages/Projects/ProjectsPage.h"

#include "KindUI/App/ViewHost.h"
#include "KindUI/Declarative/UI.h"
#include "UI/Pages/CreateProject/CreateProjectViews.h"
#include "UI/Pages/Library/ManagerViews.h"
#include "UI/Pages/Settings/SettingsViews.h"
#include "Util/LauncherMaintenance.h"
#include "Util/PathUtils.h"

#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Layout/Spacer.h"
#include "Platform/PlatformSDK.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string_view>

using namespace we::runtime::kindui;

namespace we::programs::welauncher {

using we::runtime::kindui::ColorToken;
void LauncherShell::RebuildProjectsPage() {
    EnsureProjectsPage();
    auto& state = m_Pages[static_cast<std::size_t>(LauncherPage::Projects)];
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
            box->Background(LColor(ColorToken::PanelContentBackground));
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
    model.chrome.onSearchChanged = [this](const std::string& text) { OnSearchChanged(text); };

    model.chrome.toolbarActions.push_back(UI::Host([this]() {
        auto btn = MakeSecondaryAction("Open Docs", Icons::DocumentName);
        btn->SetOnClicked([this] { SetStatus("Documentation ÃƒÂ¢Ã¢â€šÂ¬Ã¢â‚¬Â coming soon"); });
        return btn;
    }));
    model.chrome.toolbarActions.push_back(UI::Host([this]() {
        auto btn = MakeSecondaryAction("Refresh", Icons::RefreshName);
        btn->SetOnClicked([this] {
            MarkPageDirty(LauncherPage::Learn);
            EnsurePageBuilt(LauncherPage::Learn);
            ShowPage(LauncherPage::Learn);
            SetStatus("Learn content refreshed");
        });
        return btn;
    }));

    const std::vector<LearnEntryModel> allEntries = {
        { "Guide", "Create your first project", "Walk through New Project and open it in the editor",
          Icons::PlusName, [this] { ShowCreateWizard(); } },
        { "Guide", "Open documentation", "API reference and engine guides",
          Icons::DocumentName, [this] { SetStatus("Documentation ÃƒÂ¢Ã¢â€šÂ¬Ã¢â‚¬Â coming soon"); } },
        { "Guide", "Explore the engine install", "Verify SDK health and local builds",
          Icons::BuildName, [this] { GoToPage(LauncherPage::Engine); } },
        { "Sample", "First Person Demo", "Movement and camera baseline",
          Icons::PlayName, [this] { SetStatus("Sample projects ÃƒÂ¢Ã¢â€šÂ¬Ã¢â‚¬Â coming soon"); } },
        { "Sample", "Atmosphere Showcase", "Sky, clouds, and fog overview",
          Icons::SunName, [this] { SetStatus("Sample projects ÃƒÂ¢Ã¢â€šÂ¬Ã¢â‚¬Â coming soon"); } },
        { "Sample", "ECS Sandbox", "Entity component patterns",
          Icons::ComponentName, [this] { SetStatus("Sample projects ÃƒÂ¢Ã¢â€šÂ¬Ã¢â‚¬Â coming soon"); } },
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
    model.chrome.onSearchChanged = [this](const std::string& text) { OnSearchChanged(text); };

    model.chrome.toolbarActions.push_back(UI::Host([this]() {
        auto btn = MakeSecondaryAction("Refresh", Icons::RefreshName);
        btn->SetOnClicked([this] {
            UpdateFooter();
            MarkPageDirty(LauncherPage::Engine);
            EnsurePageBuilt(LauncherPage::Engine);
            ShowPage(LauncherPage::Engine);
            SetStatus("Engine status refreshed");
        });
        return btn;
    }));

    model.hasEngine = m_Context->Engines().HasEngine()
        && !m_Context->Engines().InstalledEngines().empty();

    if (!model.hasEngine) {
        model.showEmpty = true;
        model.emptyTitle = "No Engine Found";
        model.emptySubtitle = "Place the launcher next to an Engine install, or build the engine from source.";
        model.onOpenProjects = [this] { GoToPage(LauncherPage::Projects); };
        model.onRefresh = [this] {
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
            LColor(ColorToken::TextPrimary),
            LMetric(MetricToken::TextSizeBody) * s);
        label->SetHorizontalAlignment(HorizontalAlignment::Left);
        return label;
    }));

    for (const auto& check : m_Context->Sdk().RunChecks()) {
        Color color = LColor(ColorToken::TextMuted);
        if (check.status == SdkCheckStatus::Pass) {
            color = LColor(ColorToken::Success);
        } else if (check.status == SdkCheckStatus::Warn) {
            color = LColor(ColorToken::Warning);
        } else if (check.status == SdkCheckStatus::Fail) {
            color = LColor(ColorToken::ErrorForeground);
        }
        const std::string line = check.name + " ÃƒÂ¢Ã¢â€šÂ¬Ã¢â‚¬Â " + check.detail;
        model.rows.push_back(UI::Host([s, line, color]() {
            auto label = std::make_shared<Label>(line, color, LMetric(MetricToken::TextSizeCaption) * s);
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
    body->SetFlexGrow(1.0f);
    body->SetFlexBasis(0.0f);

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
    mainColumn->SetFlexGrow(1.0f);
    mainColumn->SetFlexBasis(0.0f);
    mainColumn->SetFlexShrink(1.0f);

    auto contentHost = std::make_shared<Column>();
    contentHost->SetHorizontalAlignment(HorizontalAlignment::Fill);
    contentHost->SetVerticalAlignment(VerticalAlignment::Fill);
    contentHost->SetFlexGrow(1.0f);
    contentHost->SetFlexBasis(0.0f);
    contentHost->Background(LColor(ColorToken::PanelContentBackground));
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
    if (page == LauncherPage::Projects && m_ProjectsPage) {
        m_ProjectsPage->SetSearchText(m_SearchQuery);
    }
    MarkPageDirty(page);
    EnsurePageBuilt(page);
    ShowPage(page);
    SyncHeaderFromState();
    InvalidateUI();
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

} // namespace we::programs::welauncher
