#include "UI/Dialogs/RenameProject/RenameProjectDialog.h"

#include "UI/Dialogs/RenameProject/RenameDialogView.h"
#include "UI/Pages/Projects/IProjectsHostActions.h"

namespace we::programs::welauncher {

RenameProjectDialog::RenameProjectDialog(
    IProjectsHostActions& host,
    std::function<const ProjectSummary*()> selectedProject)
    : m_Host(host)
    , m_SelectedProject(std::move(selectedProject))
    , m_Name("") {}

void RenameProjectDialog::Show(we::runtime::kindui::DialogService& dialogs) {
    const ProjectSummary* project = m_SelectedProject ? m_SelectedProject() : nullptr;
    if (!project) {
        m_Host.SetStatus("No project selected.");
        return;
    }

    m_Dialogs = &dialogs;
    m_Name = project->descriptor.displayName;
    Bind(dialogs);

    we::runtime::kindui::DialogSpec spec;
    spec.id = "rename-project";
    spec.kind = we::runtime::kindui::DialogKind::Modal;
    spec.width = 420.0f;
    spec.dismissOnScrim = true;

    dialogs.Show([this] { return BuildView(); }, spec);
}

void RenameProjectDialog::Dismiss(we::runtime::kindui::DialogService& dialogs) {
    dialogs.Dismiss("rename-project");
    m_Dialogs = nullptr;
}

void RenameProjectDialog::Bind(we::runtime::kindui::DialogService& dialogs) {
    if (!m_Bound) {
        dialogs.Observe(m_Name);
        m_Bound = true;
    }
}

we::runtime::kindui::Element RenameProjectDialog::BuildView() {
    RenameDialogState state;
    state.name = m_Name.Get();
    state.onNameChanged = [this](const std::string& text) { m_Name = text; };
    state.onCancel = [this] {
        if (m_Dialogs) {
            Dismiss(*m_Dialogs);
        }
    };
    state.onRename = [this] {
        m_Host.CommitRenameProject(m_Name.Get());
        if (m_Dialogs) {
            Dismiss(*m_Dialogs);
        }
    };
    return BuildRenameDialogView(state);
}

} // namespace we::programs::welauncher
