#pragma once

#include "App/LauncherContext.h"
#include "Core/Widget.h"
#include "RHI/Types.h"
#include "UI/Widgets/LauncherControls.h"
#include "UI/Widgets/ProjectViews.h"
#include "UI/Widgets/SettingsViews.h"

#include "Platform/Types.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace WindEffects::Editor::UI {
class ScrollLayout;
}

namespace we::programs::welauncher {

class LauncherShell : public WindEffects::Editor::UI::Widget {
public:
    LauncherShell(std::shared_ptr<LauncherContext> context, we::platform::WindowId window);
    ~LauncherShell() override = default;

    void Construct(float dpiScale);
    void SetLogoTexture(we::rhi::RHIDescriptorSetHandle logoSet);
    void RefreshProjectList();
    void SetStatus(const std::string& message);
    void OnWindowStateChanged();
    void OnTextInput(char32_t codepoint);

    [[nodiscard]] we::platform::WindowHitTestResult WindowHitTest(we::platform::Int2 point) const;

    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Tick(float deltaTime) override;
    void OnKeyDown(const WindEffects::Editor::UI::KeyEvent& event) override;

private:
    enum class ModalKind { None, Create, Rename, Actions };

    struct PageState {
        std::shared_ptr<WindEffects::Editor::UI::Widget> root;
        std::shared_ptr<WindEffects::Editor::UI::ScrollLayout> scroll;
        float scrollOffset = 0.0f;
        bool built = false;
        bool dirty = true;
    };

    void RebuildChrome();
    void EnsurePageBuilt(LauncherPage page);
    void ShowPage(LauncherPage page);
    void MarkPageDirty(LauncherPage page);
    void RebuildProjectsPage();
    void RebuildLearnPage();
    void RebuildEnginePage();
    void RebuildSettingsPage();
    void PersistLauncherSettings(const std::string& statusMessage = {});
    [[nodiscard]] std::shared_ptr<WindEffects::Editor::UI::Widget> BuildSettingsGeneral(const std::string& queryLower);
    [[nodiscard]] std::shared_ptr<WindEffects::Editor::UI::Widget> BuildSettingsEngine(const std::string& queryLower);
    [[nodiscard]] std::shared_ptr<WindEffects::Editor::UI::Widget> BuildSettingsStorage(const std::string& queryLower);
    [[nodiscard]] std::shared_ptr<WindEffects::Editor::UI::Widget> BuildSettingsFileAssociations(const std::string& queryLower);
    [[nodiscard]] std::shared_ptr<WindEffects::Editor::UI::Widget> BuildSettingsAbout(const std::string& queryLower);
    void RebuildCreateWizard();
    void RebuildRenameDialog();
    void RebuildProjectActionsDialog();
    void UpdateFooter();
    void UpdateAdaptiveLayout(float width);
    void SyncHeaderFromState();
    void CapturePageScroll(LauncherPage page);
    void RestorePageScroll(LauncherPage page);
    void GoToPage(LauncherPage page);
    void BeginPageContentLoad(float durationSeconds = 0.28f);
    [[nodiscard]] bool IsPageContentLoading() const { return m_PageContentLoading; }
    std::shared_ptr<WindEffects::Editor::UI::Widget> BuildPageSkeleton(LauncherPage page);

    void ShowCreateWizard();
    void ShowRenameDialog();
    void CloseModal();
    void BrowseForProject();
    void OpenSelectedProject();
    void CloneSelectedProject();
    void RenameSelectedProject(const std::string& newName);
    void DeleteSelectedProject();
    void ShowSelectedInExplorer();
    void SelectProject(std::size_t index, bool additive = false);
    void HandleProjectAction(std::size_t index, ProjectCardAction action);
    void ApplySearchFilter();
    void ApplyProjectSort();
    void OnSearchChanged(const std::string& text);
    void OnViewModeChanged(ProjectViewMode mode);
    void CycleSortMode();
    void SetSortMode(ProjectSortMode mode);
    void ToggleCompatibleFilter();
    void ShowHelp();
    void ToggleFavorite(const std::string& weprojPath);
    [[nodiscard]] bool IsFavorite(const std::string& weprojPath) const;
    void ShowProjectMoreMenu(std::size_t index);
    void RegenerateSelectedProjectFiles();

    std::shared_ptr<WindEffects::Editor::UI::Widget> BuildProjectsPageHeader(bool showActions);
    std::shared_ptr<WindEffects::Editor::UI::Widget> BuildProjectSearchRow();

    std::shared_ptr<LauncherContext> m_Context;
    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;

    std::shared_ptr<WindEffects::Editor::UI::Widget> m_Root;
    std::shared_ptr<LauncherTitleBar> m_TitleBar;
    std::shared_ptr<NavSidebar> m_Sidebar;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_ContentHost;
    std::shared_ptr<StatusFooter> m_Footer;
    std::shared_ptr<ModalOverlay> m_ModalHost;
    std::shared_ptr<CompactSearchField> m_ProjectsSearch;

    std::array<PageState, static_cast<std::size_t>(LauncherPage::Count)> m_Pages{};

    std::vector<ProjectSummary> m_Projects;
    std::vector<ProjectSummary> m_FilteredProjects;
    std::vector<std::string> m_FavoritePaths;
    std::vector<std::size_t> m_MultiSelected;
    int m_SelectedIndex = -1;
    LauncherPage m_Page = LauncherPage::Projects;
    ProjectViewMode m_ViewMode = ProjectViewMode::List;
    ProjectSortMode m_SortMode = ProjectSortMode::Recent;
    bool m_CompatibleOnly = false;
    std::string m_SearchQuery;
    bool m_SidebarCollapsed = false;
    we::rhi::RHIDescriptorSetHandle m_LogoSet = we::rhi::RHIDescriptorSetHandle::Invalid;

    ModalKind m_Modal = ModalKind::None;
    std::string m_WizardTemplateId = "Blank";
    std::string m_WizardCategory = "All";
    std::string m_WizardTemplateQuery;
    std::string m_WizardName = "MyProject";
    std::string m_WizardLocation;
    std::string m_RenameName;

    bool m_PageContentLoading = false;
    float m_PageLoadTimer = 0.0f;
    float m_PageLoadDuration = 0.28f;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_SettingsScrollTarget;
    bool m_SettingsPendingScroll = false;
};

} // namespace we::programs::welauncher
