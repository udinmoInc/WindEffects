#include "EditorPreferences.hpp"
#include "Scene/SceneRenderer.h"

namespace we::programs::editor {

EditorPreferences& EditorPreferences::Get() {
    static EditorPreferences instance;
    return instance;
}

void EditorPreferences::ApplyEditorViewportIfDirty(
    const std::shared_ptr<we::runtime::renderer::SceneRenderer>& /*sceneRenderer*/) {
    // Editor background pass is not part of the atmosphere pipeline; nothing to apply.
}

} // namespace we::programs::editor
