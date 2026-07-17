#include "WindEffects/Editor/UI/Shell/EditorPreferences.h"

namespace we::programs::editor {

EditorPreferences& EditorPreferences::Get() {
    static EditorPreferences instance;
    return instance;
}

void EditorPreferences::ApplyEditorViewportIfDirty(
    const std::shared_ptr<we::runtime::renderer::SceneRenderer>& /*sceneRenderer*/) {
}

} // namespace we::programs::editor
