// ==============================================================================
// WindEffects — WeLauncher — RenameProjectDialog
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/App/DialogService.h"
#include "KindUI/Core/Observable.h"
#include "Model/WeProjectDescriptor.h"

#include <functional>
#include <memory>
#include <string>

namespace we::programs::welauncher {

class IProjectsHostActions;

/// Declarative rename-project dialog (View → ViewModel → Commands → ViewHost).
class RenameProjectDialog {
public:
    RenameProjectDialog(IProjectsHostActions& host, std::function<const ProjectSummary*()> selectedProject);

    void Show(we::runtime::kindui::DialogService& dialogs);
    void Dismiss(we::runtime::kindui::DialogService& dialogs);

private:
    void Bind(we::runtime::kindui::DialogService& dialogs);
    [[nodiscard]] we::runtime::kindui::Element BuildView();

    IProjectsHostActions& m_Host;
    std::function<const ProjectSummary*()> m_SelectedProject;
    we::runtime::kindui::DialogService* m_Dialogs = nullptr;
    we::runtime::kindui::Observable<std::string> m_Name;
    bool m_Bound = false;
};

} // namespace we::programs::welauncher
