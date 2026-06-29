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
    // Tight neutral charcoal anchor — shader enforces <=6% span and exposure clamps.
    defaults.zenithColor = HexToNeutralGray(0x1B, 0x1B, 0x1B);
    defaults.upperSkyColor = HexToNeutralGray(0x1A, 0x1A, 0x1A);
    defaults.midSkyColor = HexToNeutralGray(0x19, 0x19, 0x19);
    defaults.horizonColor = HexToNeutralGray(0x18, 0x18, 0x18);
    defaults.bottomColor = HexToNeutralGray(0x17, 0x17, 0x17);

    defaults.backgroundBrightness = 1.0f;
    defaults.gradientStrength = 1.0f;
    defaults.horizonFade = 0.0f;
    defaults.backgroundContrast = 1.0f;

    m_EditorBackground = defaults;
    m_GridFadeDistance = 200.0f;
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
        gridRenderer->SetGridOriginWeight(m_GridOriginWeight);
        m_GridDirty = false;
    }
}

} // namespace we::programs::editor
