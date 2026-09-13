// ==============================================================================
// WindEffects — Environment — EnvironmentEditorApi
// Public API surface for the Environment module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Environment/Export.h"

#include <KindUI/EditorUI.h>
#include <memory>

namespace we::runtime::scene {
class Scene;
}

namespace we::editor::contentbrowser { class TreeView; }
namespace we::editor::property { class IDetailsView; }

namespace we::editor::environment {

ENVIRONMENT_API void InitializeEditor(
    const std::shared_ptr<we::runtime::scene::Scene>& scene,
    const std::shared_ptr<::we::editor::contentbrowser::TreeView>& outliner,
    const std::shared_ptr<::we::editor::property::IDetailsView>& details);

ENVIRONMENT_API std::shared_ptr<we::runtime::kindui::Widget> CreateEnvironmentToolbarMenu();
ENVIRONMENT_API void TickEditor();

} // namespace we::editor::environment
