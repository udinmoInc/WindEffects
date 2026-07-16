#pragma once

#include "App/LauncherContext.h"
#include "Core/Widget.h"

#include <functional>
#include <memory>
#include <string>

namespace we::programs::welauncher {

class LauncherShell : public WindEffects::Editor::UI::Widget {
public:
    explicit LauncherShell(std::shared_ptr<LauncherContext> context);
    ~LauncherShell() override = default;

    void Construct(float dpiScale);
    void RefreshProjectList();
    void SetStatus(const std::string& message);

    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Tick(float deltaTime) override;

private:
    void ShowCreateWizard();
    void BrowseForProject();
    void OpenSelectedProject();
    void CloneSelectedProject();
    void RenameSelectedProject();
    void DeleteSelectedProject();
    void SelectProject(std::size_t index);

    std::shared_ptr<LauncherContext> m_Context;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_Root;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_ProjectListPanel;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_DetailsPanel;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_EnginePanel;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_SdkPanel;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_FuturePanel;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_WizardOverlay;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_StatusLabel;

    std::vector<ProjectSummary> m_Projects;
    int m_SelectedIndex = -1;
    bool m_ShowWizard = false;

    std::string m_WizardTemplateId = "Blank";
    std::string m_WizardName = "MyProject";
    std::string m_WizardLocation;
};

} // namespace we::programs::welauncher
