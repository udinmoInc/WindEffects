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
namespace {

std::shared_ptr<Column> MakeSettingsStack() {
    auto stack = std::make_shared<Column>();
    stack->Gap(LMetric(MetricToken::FormRowGap) * LScale());
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
        engineLabels.push_back(version + "  â€”  " + EllipsizePath(install.engineRoot, 36));
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
        SetStatus(std::string("Launcher ") + kLauncherVersion + " â€” update check is not available yet");
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
        MakeLabel(kLauncherVersion, LMetric(MetricToken::TextSizeBody) * LScale(), LColor(ColorToken::TextPrimary)));

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
            SetStatus("Documentation â€” coming soon");
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
    page->Background(LColor(ColorToken::PanelContentBackground));

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

    auto pageTitle = std::make_shared<Label>("Settings", TypographyToken::PageTitle);
    pageTitle->SetHorizontalAlignment(HorizontalAlignment::Left);
    content->AddChild(pageTitle);
    auto pageSubtitle = std::make_shared<Label>(
        queryLower.empty()
            ? "Manage projects, engines, cache, and launcher maintenance"
            : ("Showing matches for \"" + m_SearchQuery + "\""),
        TypographyToken::Subtitle);
    pageSubtitle->SetHorizontalAlignment(HorizontalAlignment::Left);
    content->AddChild(pageSubtitle);

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
            LMetric(MetricToken::TextSizeBody) * s,
            LColor(ColorToken::TextMuted)));
    }

    contentScroll->SetContent(content);
    contentScroll->SetHorizontalAlignment(HorizontalAlignment::Fill);
    contentScroll->SetVerticalAlignment(VerticalAlignment::Fill);
    page->AddChild(contentScroll);

    state.root = page;
    state.scroll = contentScroll;
}
} // namespace we::programs::welauncher
