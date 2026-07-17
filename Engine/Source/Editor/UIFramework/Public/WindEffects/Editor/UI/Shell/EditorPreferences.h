#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <memory>

namespace we::runtime::renderer {
class SceneRenderer;
}

namespace we::editor::services {

// Editor-only viewport rendering preferences.
class UIFRAMEWORK_API EditorPreferences {
public:
    static EditorPreferences& Get();

    void ApplyEditorViewportIfDirty(
        const std::shared_ptr<we::runtime::renderer::SceneRenderer>& sceneRenderer);

private:
    EditorPreferences() = default;
};

} // namespace we::editor::services
