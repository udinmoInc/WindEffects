#pragma once

#include "App/LauncherContext.h"
#include "KindUI/App/ApplicationServices.h"
#include "KindUI/App/ViewHost.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WidgetContext.h"
#include "KindUI/Layout/OverlayManager.h"
#include "RHI/Types.h"
#include "UI/Dialogs/RenameProject/RenameProjectDialog.h"
#include "UI/Pages/LauncherPages.h"
#include "UI/Pages/Projects/IProjectsHostActions.h"
#include "UI/Pages/Projects/ProjectsPage.h"
#include "UI/Pages/PageState.h"
#include "KindUI/Widgets/ModalHost.h"
#include "UI/Controls/LauncherControls.h"
#include "UI/Pages/Settings/SettingsViews.h"

#include "Platform/Types.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace we::runtime::kindui {
class ScrollLayout;
}

namespace we::programs::welauncher {

class LauncherShell : public we::runtime::kindui::Widget, public IProjectsHostActions {
public:
    LauncherShell(std::shared_ptr<LauncherContext> context, we::platform::WindowId window);
    ~LauncherShell() override = default;

    void Construct(float dpiScale);
    void SetHostContext(std::shared_ptr<we::runtime::kindui::IWidgetContext> context);
    void SetLogoTexture(we::rhi::RHIDescriptorSetHandle logoSet);
    void RefreshProjectList();
    void OnWindowStateChanged();
    void OnTextInput(char32_t codepoint);

    [[nodiscard]] we::platform::WindowHitTestResult WindowHitTest(we::platform::Int2 point) const;

    void Paint(we::runtime::kindui::PaintContext& context) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Tick(float deltaTime) override;
    void OnKeyDown(const we::runtime::kindui::KeyEvent& event) override;

    // IProjectsHostActions
    void ShowCreateWizard() override;
    void BrowseForProject() override;
    void ShowRenameDialog() override;
    void CommitRenameProject(const std::string& newName) override;
    void ShowProjectMoreMenu(std::size_t index) override;
    void SetStatus(const std::string& message) override;
    void UpdateFooter() override;
    void BeginLoading(float durationSeconds = 0.24f) override;

private:
    enum class ModalKind { None, Create, Actions };

    void EnsureProjectsPage();
    [[nodiscard]] const ProjectSummary* SelectedProjectSummary() const;

    void RebuildChrome();
    void EnsurePageBuilt(LauncherPage page);
    void ShowPage(LauncherPage page);
    void MarkPageDirty(LauncherPage page);
    void RebuildProjectsPage();
    void RebuildLearnPage();
    void RebuildEnginePage();
    void RebuildSettingsPage();
    [[nodiscard]] LearnPageModel BuildLearnPageModel();
    [[nodiscard]] EnginePageModel BuildEnginePageModel();
    void ApplyDeclarativePage(PageState& state, const we::runtime::kindui::Element& view, const char* scrollId);
    void PersistLauncherSettings(const std::string& statusMessage = {});
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> BuildSettingsGeneral(const std::string& queryLower);
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> BuildSettingsEngine(const std::string& queryLower);
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> BuildSettingsStorage(const std::string& queryLower);
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> BuildSettingsFileAssociations(const std::string& queryLower);
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> BuildSettingsAbout(const std::string& queryLower);
    void RebuildCreateWizard();
    void CommitCreateProject();
    void SelectWizardTemplateByDelta(int delta);
    void RebuildProjectActionsDialog();
    void UpdateAdaptiveLayout(float width);
    void SyncHeaderFromState();
    void CapturePageScroll(LauncherPage page);
    void RestorePageScroll(LauncherPage page);
    void GoToPage(LauncherPage page);
    void BeginPageContentLoad(float durationSeconds = 0.28f);
    [[nodiscard]] bool IsPageContentLoading() const { return m_PageContentLoading; }
    std::shared_ptr<we::runtime::kindui::Widget> BuildPageSkeleton(LauncherPage page);

    void CloseModal();
    void OpenSelectedProject();
    void CloneSelectedProject();
    void RenameSelectedProject(const std::string& newName);
    void DeleteSelectedProject();
    void ShowSelectedInExplorer();
    void SelectProject(std::size_t index, bool additive = false);
    void OnSearchChanged(const std::string& text);
    void CycleSortMode();
    void SetSortMode(ProjectSortMode mode);
    void ToggleCompatibleFilter();
    void ShowHelp();
    void ToggleFavorite(const std::string& weprojPath);
    [[nodiscard]] bool IsFavorite(const std::string& weprojPath) const;
    void RegenerateSelectedProjectFiles();

    void InitializeApplicationServices();

    std::shared_ptr<LauncherContext> m_Context;
    std::shared_ptr<we::runtime::kindui::IWidgetContext> m_HostContext;
    we::runtime::kindui::ApplicationServices m_Services;
    std::shared_ptr<we::runtime::kindui::OverlayHost> m_PopupHost;
    std::shared_ptr<we::runtime::kindui::ModalHost> m_DialogOverlay;
    std::unique_ptr<RenameProjectDialog> m_RenameDialog;
    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;

    std::unique_ptr<ProjectsPage> m_ProjectsPage;

    std::shared_ptr<we::runtime::kindui::Widget> m_Root;
    std::shared_ptr<LauncherTitleBar> m_TitleBar;
    std::shared_ptr<NavSidebar> m_Sidebar;
    std::shared_ptr<we::runtime::kindui::Widget> m_ContentHost;
    std::shared_ptr<StatusFooter> m_Footer;
    std::shared_ptr<we::runtime::kindui::ModalHost> m_ModalHost;

    std::array<PageState, static_cast<std::size_t>(LauncherPage::Count)> m_Pages{};

    LauncherPage m_Page = LauncherPage::Projects;
    std::string m_SearchQuery;
    bool m_SidebarCollapsed = false;
    we::rhi::RHIDescriptorSetHandle m_LogoSet = we::rhi::RHIDescriptorSetHandle::Invalid;

    ModalKind m_Modal = ModalKind::None;
    std::string m_WizardTemplateId = "Blank";
    std::string m_WizardCategory = "All";
    std::string m_WizardTemplateQuery;
    std::string m_WizardName = "MyProject";
    std::string m_WizardLocation;
    std::string m_WizardQuality = "Balanced";

    bool m_PageContentLoading = false;
    float m_PageLoadTimer = 0.0f;
    float m_PageLoadDuration = 0.28f;
    std::shared_ptr<we::runtime::kindui::Widget> m_SettingsScrollTarget;
    bool m_SettingsPendingScroll = false;
};

} // namespace we::programs::welauncher
