#pragma once

#include "ProjectManager/Export.h"
#include "ProjectManager/IProjectManagerHost.h"

#include "KindUI/Core/Widget.h"

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
