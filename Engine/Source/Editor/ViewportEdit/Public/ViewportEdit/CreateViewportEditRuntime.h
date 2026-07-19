#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "ViewportEdit/IViewportEditor.h"

#include <memory>

namespace we::editor::undo {
class IUndoRuntime;
}

namespace we::editor::property {
class IPropertyEditorRuntime;
}

namespace we::runtime::scene {
class Scene;
}

namespace we::runtime::world {
class IWorldRuntime;
}

namespace we::runtime::engine {
class EditorCamera;
}

namespace we::editor::viewportedit {

struct VIEWPORTEDIT_API ViewportEditDependencies {
    undo::IUndoRuntime* undo = nullptr;
    property::IPropertyEditorRuntime* propertyEditor = nullptr;
    scene::Scene* scene = nullptr;
    world::IWorldRuntime* world = nullptr;
    engine::EditorCamera* editorCamera = nullptr;
    ViewportEditConfig config{};
};

[[nodiscard]] VIEWPORTEDIT_API std::unique_ptr<IViewportEditor> CreateViewportEditRuntime(
    const ViewportEditDependencies& deps);

} // namespace we::editor::viewportedit
