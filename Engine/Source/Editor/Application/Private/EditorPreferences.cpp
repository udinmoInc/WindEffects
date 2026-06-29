#include "EditorPreferences.hpp"
#include <algorithm>

namespace we::programs::editor {

namespace {
glm::vec3 HexToNeutralGray(uint8_t r, uint8_t g, uint8_t b) {
    const float gray = (static_cast<float>(r) + static_cast<float>(g) + static_cast<float>(b)) / (3.0f * 255.0f);
    return glm::vec3(gray);
}
} // namespace

EditorPreferences& EditorPreferences::Get() {
    static EditorPreferences instance;
    return instance;
}

EditorPreferences::EditorPreferences() {
    using Bg = we::runtime::renderer::SceneRenderer::EditorBackgroundSettings;

    Bg defaults{};
    // UE5-style almost pure black: zenith (#090909), subtle bottom (#0E0E0E).
    defaults.zenithColor = HexToNeutralGray(0x09, 0x09, 0x09);
    defaults.upperSkyColor = HexToNeutralGray(0x0A, 0x0A, 0x0A);
    defaults.midSkyColor = HexToNeutralGray(0x0B, 0x0B, 0x0B);
    defaults.horizonColor = HexToNeutralGray(0x0C, 0x0C, 0x0C);
    defaults.bottomColor = HexToNeutralGray(0x0E, 0x0E, 0x0E);

    defaults.backgroundBrightness = 1.0f;
    defaults.gradientStrength = 0.55f;
    defaults.horizonFade = 0.0f;
    defaults.backgroundContrast = 1.0f;

    m_EditorBackground = defaults;
    m_GridFadeDistance = 8000.0f;
    m_GridLodIntensity = 1.0f;
}

void EditorPreferences::ApplyEditorViewportIfDirty(
    const std::shared_ptr<we::runtime::renderer::SceneRenderer>& sceneRenderer,
    const std::shared_ptr<we::runtime::renderer::GridRenderer>& gridRenderer) {
    if (sceneRenderer && m_BackgroundDirty) {
        sceneRenderer->SetEditorBackgroundSettings(m_EditorBackground);
        m_BackgroundDirty = false;
    }
    if (gridRenderer && m_GridDirty) {
        gridRenderer->SetGridFadeDistance(m_GridFadeDistance);
        gridRenderer->SetGridLodIntensity(m_GridLodIntensity);
        m_GridDirty = false;
    }
}

} // namespace we::programs::editor
