#pragma once

#include "Renderer/SceneRenderer.hpp"
#include <memory>

namespace we::programs::editor {

// Editor-only viewport rendering preferences.
class EditorPreferences {
public:
    static EditorPreferences& Get();

    const we::runtime::renderer::SceneRenderer::EditorBackgroundSettings& GetEditorBackgroundSettings() const {
        return m_EditorBackground;
    }

    void SetEditorBackgroundSettings(const we::runtime::renderer::SceneRenderer::EditorBackgroundSettings& settings) {
        m_EditorBackground = settings;
        m_BackgroundDirty = true;
    }

    void ApplyEditorViewportIfDirty(
        const std::shared_ptr<we::runtime::renderer::SceneRenderer>& sceneRenderer);

private:
    EditorPreferences();

    we::runtime::renderer::SceneRenderer::EditorBackgroundSettings m_EditorBackground{};
    bool m_BackgroundDirty = true;
};

} // namespace we::programs::editor
