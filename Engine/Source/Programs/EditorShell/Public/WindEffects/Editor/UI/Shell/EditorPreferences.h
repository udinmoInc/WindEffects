// ==============================================================================
// WindEffects — EditorShell — EditorPreferences
// Public API surface for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <memory>

namespace we::runtime::renderer {
class SceneRenderer;
}

namespace we::editor::services {

// Editor-only viewport rendering preferences.
class EDITORSHELL_API EditorPreferences {
public:
    static EditorPreferences& Get();

    void ApplyEditorViewportIfDirty(
        const std::shared_ptr<we::runtime::renderer::SceneRenderer>& sceneRenderer);

private:
    EditorPreferences() = default;
};

} // namespace we::editor::services
