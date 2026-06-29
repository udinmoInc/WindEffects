#pragma once

#include "Renderer/SceneRenderer.hpp"
#include "Renderer/GridRenderer.hpp"
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

    float GetGridFadeDistance() const { return m_GridFadeDistance; }
    void SetGridFadeDistance(float distance) {
        m_GridFadeDistance = distance;
        m_GridDirty = true;
    }

    float GetGridLodIntensity() const { return m_GridLodIntensity; }
    void SetGridLodIntensity(float intensity) {
        m_GridLodIntensity = intensity;
        m_GridDirty = true;
    }

    void ApplyEditorViewportIfDirty(
        const std::shared_ptr<we::runtime::renderer::SceneRenderer>& sceneRenderer,
        const std::shared_ptr<we::runtime::renderer::GridRenderer>& gridRenderer);

private:
    EditorPreferences();

    we::runtime::renderer::SceneRenderer::EditorBackgroundSettings m_EditorBackground{};
    float m_GridFadeDistance = 8000.0f;
    float m_GridLodIntensity = 1.0f;
    bool m_BackgroundDirty = true;
    bool m_GridDirty = true;
};

} // namespace we::programs::editor
