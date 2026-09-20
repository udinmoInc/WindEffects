// ==============================================================================
// WindEffects — TestViewport — CloudTestScene
// Reusable minimal cloud test scene for fast cloud development.
//
// Contains ONLY:
//   - 1 Camera looking at the cloud volume / sky
//   - 1 Directional Sun / Sky lighting
//   - Bounded small cloud test region (10-15 km, 1200m-4500m)
//   - Minimal environment / rendering settings
//
// NO terrain, NO UI, NO gameplay, NO large worlds, NO weather simulation.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// ==============================================================================
#pragma once

#include "Camera/CameraUniform.h"
#include "Lighting/CloudUniform.h"
#include "Lighting/SceneEnvironmentUniform.h"
#include "Platform/Types.h"
#include "Core/Math/Types.h"

#include <memory>
#include <string>

namespace we::runtime::engine {
class EditorCamera;
}

namespace we::runtime::renderer {
class Renderer;
}

namespace we::test {

class CloudTestScene {
public:
    explicit CloudTestScene(we::runtime::renderer::Renderer* renderer);
    ~CloudTestScene();

    void Init(uint32_t width, uint32_t height);
    void Update(float dt);
    void Render();
    void Resize(uint32_t width, uint32_t height);

    // Input handlers for camera navigation
    void OnMouseMove(float x, float y);
    void OnMouseButton(int button, bool pressed);
    void OnMouseWheel(float delta);
    void OnKeyDown(int key, bool pressed);

    [[nodiscard]] we::runtime::engine::EditorCamera* GetCamera() { return m_Camera.get(); }
    [[nodiscard]] we::runtime::renderer::CloudUniform& GetCloudConfig() { return m_CloudConfig; }
    [[nodiscard]] we::runtime::renderer::SceneEnvironmentUniform& GetEnvironment() { return m_Environment; }
    [[nodiscard]] int GetDebugMode() const { return m_Environment.atmosphereDebugMode; }
    void SetDebugMode(int mode);

private:
    void SetupDefaultLighting();
    void SetupDefaultCloudVolume();

    we::runtime::renderer::Renderer* m_Renderer = nullptr;
    std::shared_ptr<we::runtime::engine::EditorCamera> m_Camera;
    we::runtime::renderer::CameraUniform m_CameraUniform{};
    we::runtime::renderer::SceneEnvironmentUniform m_Environment{};
    we::runtime::renderer::CloudUniform m_CloudConfig{};

    bool m_RightMouseDown = false;
    float m_LastMouseX = 0.0f;
    float m_LastMouseY = 0.0f;
    uint32_t m_Width = 1280;
    uint32_t m_Height = 720;
    float m_TotalTime = 0.0f;

    // Movement key states
    bool m_KeyW = false;
    bool m_KeyA = false;
    bool m_KeyS = false;
    bool m_KeyD = false;
    bool m_KeyQ = false;
    bool m_KeyE = false;
    bool m_KeyShift = false;
};

} // namespace we::test
