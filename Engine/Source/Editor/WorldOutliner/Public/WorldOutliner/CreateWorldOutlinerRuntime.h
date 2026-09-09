// ==============================================================================
// WindEffects — WorldOutliner — CreateWorldOutlinerRuntime
// Public API surface for the WorldOutliner module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "WorldOutliner/Export.h"
#include "WorldOutliner/OutlinerTypes.h"
#include "WorldOutliner/IWorldOutliner.h"

#include <functional>
#include <memory>
#include <string_view>

namespace we::editor::undo {
class IUndoRuntime;
}

namespace we::editor::property {
class IPropertyEditorRuntime;
class IDetailsView;
}

namespace we::editor::viewportedit {
class IViewportEditor;
}

namespace we::runtime::scene {
class Scene;
}

namespace we::runtime::world {
class IWorldRuntime;
}

namespace we::editor::outliner {

struct WORLDOUTLINER_API WorldOutlinerDependencies {
    undo::IUndoRuntime* undo = nullptr;
    property::IPropertyEditorRuntime* propertyEditor = nullptr;
    property::IDetailsView* detailsView = nullptr;
    viewportedit::IViewportEditor* viewportEdit = nullptr;
    scene::Scene* scene = nullptr;
    world::IWorldRuntime* world = nullptr;
    OutlinerConfig config{};
    std::function<void(std::string_view)> onLog;
};

[[nodiscard]] WORLDOUTLINER_API std::unique_ptr<IWorldOutlinerRuntime> CreateWorldOutlinerRuntime(
    const WorldOutlinerDependencies& deps);

} // namespace we::editor::outliner
