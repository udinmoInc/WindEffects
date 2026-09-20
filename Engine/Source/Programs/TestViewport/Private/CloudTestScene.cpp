// ==============================================================================
// WindEffects — TestViewport — CloudTestScene
// Reusable minimal cloud test scene for fast cloud development.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// ==============================================================================
#include "CloudTestScene.h"

#include "EditorCamera.h"
#include "Renderer/Renderer.h"
#include "Lighting/DaylightConfig.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cmath>

namespace we::test {

CloudTestScene::CloudTestScene(we::runtime::renderer::Renderer* renderer)
    : m_Renderer(renderer)
{
}

CloudTestScene::~CloudTestScene() = default;

void CloudTestScene::Init(uint32_t width, uint32_t height)
{
    m_Width = std::max(width, 64u);
    m_Height = std::max(height, 64u);

    // Create and frame camera looking up into the open sky and cloud test region
    m_Camera = std::make_shared<we::runtime::engine::EditorCamera>();
    m_Camera->SetViewportSize(static_cast<float>(m_Width), static_cast<float>(m_Height));

    // Place camera at a reasonable altitude looking slightly upward (+20 deg)
    // into the bounded cloud test volume
    m_Camera->SetCameraSpeed(80.0f);
    m_Camera->Focus(we::math::Vec3{ 0.0f, 2200.0f, 0.0f }, 4500.0f);

    SetupDefaultLighting();
    SetupDefaultCloudVolume();

    // Disable editor floor grid — clean open sky only
    if (m_Renderer) {
        m_Renderer->SetGridVisible(false);
    }

    HE_INFO("[CloudTestScene] Minimal cloud test scene initialized. Viewport: "
        + std::to_string(m_Width) + "x" + std::to_string(m_Height));
}

void CloudTestScene::SetupDefaultLighting()
{
    // Sun direction: natural afternoon angle (~45 deg elevation, angled forward-left)
    const we::math::Vec3 rawSunDir{ 0.45f, -0.65f, 0.60f };
    const float len = std::sqrt(rawSunDir.x * rawSunDir.x + rawSunDir.y * rawSunDir.y + rawSunDir.z * rawSunDir.z);
    m_Environment.sunDirection = (len > 1e-4f)
        ? we::math::Vec3{ rawSunDir.x / len, rawSunDir.y / len, rawSunDir.z / len }
        : we::math::Vec3{ 0.0f, -1.0f, 0.0f };

    m_Environment.sunIntensity = 1.30f;
    m_Environment.sunColor = { 1.0f, 0.98f, 0.93f };
    m_Environment.skyLightIntensity = 1.0f;
    m_Environment.skyAmbientColor = { 0.32f, 0.40f, 0.52f };

    // Atmospheric scattering parameters (Earth defaults)
    m_Environment.atmosphereRayleigh = { 0.005802f, 0.013558f, 0.033100f };
    m_Environment.mieScattering = 0.003996f;
    m_Environment.mieAnisotropy = 0.80f;
    m_Environment.enableSunDisk = 1.0f;
    m_Environment.hdrSkyLuminance = 1.0f;
    m_Environment.atmosphereDebugMode = 0; // Normal lit rendering
}

void CloudTestScene::SetupDefaultCloudVolume()
{
    // Small bounded test world for fast cloud iteration:
    // Sized for approximately 5-10 small cloud formations.
    we::runtime::renderer::ApplyDaylightCloudDefaults(m_CloudConfig);

    m_CloudConfig.enabled = 1.0f;
    m_CloudConfig.densityScale = 0.65f;
    m_CloudConfig.coverage = 0.55f;

    // Small bounded height extent (1200m base to 4500m ceiling)
    m_CloudConfig.innerOffset = 1200.0f;
    m_CloudConfig.outerOffset = 4500.0f;

    // Small test world noise and formation scales
    m_CloudConfig.baseNoiseScale = 1.0f / 6000.0f;
    m_CloudConfig.detailNoiseScale = 0.0008f;
    m_CloudConfig.weatherMapScale = 0.000035f;

    // Formation settings (tuned for small test region: 5-10 formations)
    m_CloudConfig.formationFreqLarge  = 0.000045f;
    m_CloudConfig.formationFreqMedium = 0.000120f;
    m_CloudConfig.formationFreqSmall  = 0.000350f;
    m_CloudConfig.formationThreshold  = 0.40f;

    // Minimal raymarch step count for fast performance
    m_CloudConfig.maxSteps = 128;
    m_CloudConfig.lightMarchLength = 2200.0f;

    // No curl or erosion in initial test configuration
    m_CloudConfig.erosionStrength = 0.0f;
    m_CloudConfig.curlStrength = 0.0f;
    m_CloudConfig.bottomWispStrength = 0.0f;
}

void CloudTestScene::Resize(uint32_t width, uint32_t height)
{
    m_Width = std::max(width, 64u);
    m_Height = std::max(height, 64u);
    if (m_Camera) {
        m_Camera->SetViewportSize(static_cast<float>(m_Width), static_cast<float>(m_Height));
    }
}

void CloudTestScene::Update(float dt)
{
    m_TotalTime += dt;

    if (m_Camera) {
        // Fly movement keys
        we::runtime::engine::EditorCameraFlyKeys keys{};
        keys.forward = m_KeyW;
        keys.back    = m_KeyS;
        keys.left    = m_KeyA;
        keys.right   = m_KeyD;
        keys.up      = m_KeyE;
        keys.down    = m_KeyQ;
        keys.boost   = m_KeyShift;

        if (m_RightMouseDown) {
            m_Camera->ProcessFlyMovement(keys, dt);
        }
        m_Camera->Update(dt);
    }
}

void CloudTestScene::Render()
{
    if (!m_Renderer || !m_Camera) {
        return;
    }

    // Set viewport targets to match native window client area (100% of window)
    m_Renderer->SetViewportRenderTargetSize(m_Width, m_Height);

    // Prepare camera UBO
    m_CameraUniform.view = m_Camera->GetViewMatrix();
    m_CameraUniform.proj = m_Camera->GetProjectionMatrix();
    m_CameraUniform.position = m_Camera->GetPosition();

    // Upload uniforms
    m_Renderer->UploadCameraUniform(m_CameraUniform);
    m_Renderer->UploadEnvironmentUniform(m_Environment);

    // Apply cloud test volume configuration
    m_CloudConfig.timeSeconds = m_TotalTime;
    m_Renderer->SetCloudUniform(m_CloudConfig);
    m_Renderer->SetCloudUniformOverride(true);
    m_Renderer->SetGridVisible(false);

    // Render the 3D scene (Sky + Volumetric Clouds + Tonemap)
    m_Renderer->RenderScene();
}

void CloudTestScene::SetDebugMode(int mode)
{
    m_Environment.atmosphereDebugMode = mode;
    std::string modeName = "Lit (Normal)";
    switch (mode) {
    case 312: modeName = "Formation Coverage (312)"; break;
    case 313: modeName = "Formation Size (313)"; break;
    case 314: modeName = "Formation ID (314)"; break;
    case 315: modeName = "Formation Density (315)"; break;
    case 316: modeName = "Formation Height (316)"; break;
    case 317: modeName = "Base Cloud Shape (317)"; break;
    default: break;
    }
    HE_INFO("[CloudTestScene] AtmosphereDebugMode set to: " + modeName);
}

void CloudTestScene::OnMouseMove(float x, float y)
{
    const float dx = x - m_LastMouseX;
    const float dy = y - m_LastMouseY;
    m_LastMouseX = x;
    m_LastMouseY = y;

    if (!m_Camera) {
        return;
    }

    if (m_RightMouseDown) {
        m_Camera->ProcessFlyLook(dx, dy);
    }
}

void CloudTestScene::OnMouseButton(int button, bool pressed)
{
    if (button == 1) { // Right button
        m_RightMouseDown = pressed;
        if (m_Camera) {
            if (pressed) {
                m_Camera->EnterFlyMode();
            } else {
                m_Camera->ExitFlyMode();
            }
        }
    }
}

void CloudTestScene::OnMouseWheel(float delta)
{
    if (m_Camera) {
        if (m_RightMouseDown) {
            m_Camera->AdjustFlySpeed(delta);
        } else {
            m_Camera->ProcessMouseScroll(delta);
        }
    }
}

void CloudTestScene::OnKeyDown(int key, bool pressed)
{
    // WASD / QE navigation
    switch (key) {
    case 'W': case 'w': m_KeyW = pressed; break;
    case 'S': case 's': m_KeyS = pressed; break;
    case 'A': case 'a': m_KeyA = pressed; break;
    case 'D': case 'd': m_KeyD = pressed; break;
    case 'Q': case 'q': m_KeyQ = pressed; break;
    case 'E': case 'e': m_KeyE = pressed; break;
    case 0x10: /* VK_SHIFT */ m_KeyShift = pressed; break;

    // Fast debug mode hotkeys (0-6)
    case '0': if (pressed) SetDebugMode(0);   break; // Lit
    case '1': if (pressed) SetDebugMode(312); break; // Formation Coverage
    case '2': if (pressed) SetDebugMode(313); break; // Formation Size
    case '3': if (pressed) SetDebugMode(314); break; // Formation ID
    case '4': if (pressed) SetDebugMode(315); break; // Formation Density
    case '5': if (pressed) SetDebugMode(316); break; // Formation Height
    case '6': if (pressed) SetDebugMode(317); break; // Base Shape
    default: break;
    }
}

} // namespace we::test
