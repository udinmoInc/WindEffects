#pragma once

#include "Application/Export.hpp"

#include <memory>

namespace we::runtime::renderer {
class SceneRenderer;
}

namespace we::programs::editor {

// Editor-only viewport rendering preferences.
class APPLICATION_API EditorPreferences {
public:
    static EditorPreferences& Get();

    void ApplyEditorViewportIfDirty(
        const std::shared_ptr<we::runtime::renderer::SceneRenderer>& sceneRenderer);

private:
    EditorPreferences() = default;
};

} // namespace we::programs::editor
