// ==============================================================================
// WindEffects — EditorShell — EditorPreferences
// Internal implementation for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/UI/Shell/EditorPreferences.h"

namespace we::editor::services {

EditorPreferences& EditorPreferences::Get() {
    static EditorPreferences instance;
    return instance;
}

void EditorPreferences::ApplyEditorViewportIfDirty(
    const std::shared_ptr<we::runtime::renderer::SceneRenderer>& /*sceneRenderer*/) {
}

} // namespace we::editor::services
