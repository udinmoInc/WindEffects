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
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::PaddingToken;

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
        LMetric(MetricToken::Space4) * LScale(),
        kLauncherContentPadX * LScale(),
        LMetric(MetricToken::Space4) * LScale()
    });
    content->Gap(LMetric(MetricToken::Space4) * LScale());
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
        return "ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â";
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
        "Help ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â Ctrl+N new project, Ctrl+O browse, Enter open selected, "
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


void LauncherShell::PersistLauncherSettings(const std::string& statusMessage) {
    m_Context->Save();
    SetStatus(statusMessage.empty() ? "Settings saved" : statusMessage);
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
        LMetric(MetricToken::Space6) * LScale(),
        LMetric(MetricToken::Space6) * LScale(),
        LMetric(MetricToken::Space6) * LScale(),
        LMetric(MetricToken::Space6) * LScale()
    });
    panel->Gap(LMetric(MetricToken::Space2) * LScale());
    panel->Background(LColor(ColorToken::DialogBackground));
    panel->AddChild(MakeLabel(
        project->descriptor.displayName,
        LMetric(MetricToken::TextSizeHeader) * LScale(),
        LColor(ColorToken::TextPrimary)));
    panel->AddChild(MakeLabel(
        EllipsizePath(project->projectRoot, 52),
        LMetric(MetricToken::TextSizeCaption) * LScale(),
        LColor(ColorToken::TextMuted)));

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
    ClearLayoutDirty();
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
    context.DrawRect(GetGeometry(), LColor(ColorToken::WorkspaceBackground));
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
    (void)deltaTime;

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