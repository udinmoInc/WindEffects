// ==============================================================================
// WindEffects — ProjectManager — ProjectManagerView
// Public API surface for the ProjectManager module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "ProjectManager/Export.h"
#include "ProjectManager/IProjectManagerHost.h"

#include <KindUI/EditorUI.h>
#include <memory>

namespace we::editor::projectmanager {

struct PROJECTMANAGER_API ProjectManagerViewOptions {
    bool startInNewProjectMode = false;
};

class PROJECTMANAGER_API ProjectManagerView {
public:
    static std::shared_ptr<we::runtime::kindui::Widget> Build(
        IProjectManagerHost& host,
        const ProjectManagerViewOptions& options = {});
};

} // namespace we::editor::projectmanager
