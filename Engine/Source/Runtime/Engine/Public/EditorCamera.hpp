#pragma once

#include "Engine/Export.hpp"

#if WE_HAS_GLM
#include <glm/glm.hpp>
#else
// Fallback simple math types when GLM is not available
struct glm_vec3 { float x, y, z; };
struct glm_mat4 { float data[16]; };
namespace glm {
    using vec3 = glm_vec3;
    using mat4 = glm_mat4;
}
#endif

#if WE_HAS_SDL3
#include <SDL3/SDL.h>
#endif

namespace we::runtime::engine {

inline constexpr float kEditorCameraMinSpeed = 1.0f;
inline constexpr float kEditorCameraMaxSpeed = 50.0f;
inline constexpr float kEditorCameraDefaultSpeed = 4.0f;

struct EditorCameraNavigationSettings {
    float mouseSensitivity = 0.15f;
    float cameraAcceleration = 1.0f;
    float cameraSmoothing = 12.0f;
    bool invertX = false;
    bool invertY = false;
    float maxBoostMultiplier = 4.0f;
    float slowMultiplier = 0.25f;
    float scrollWheelSpeedMultiplier = 1.0f;
};

class ENGINE_API EditorCamera {
public:
    EditorCamera();
    ~EditorCamera() = default;

    void Update(float dt);

    void SetNavigationSettings(const EditorCameraNavigationSettings& settings);

    // Fly mode (RMB held)
    void EnterFlyMode();
    void ExitFlyMode();
    bool IsFlyMode() const { return m_FlyMode; }

    void ProcessFlyLook(float dx, float dy);

    // Orbit navigation (Alt + mouse)
    void ProcessMouseOrbit(float dx, float dy);
    void ProcessMousePan(float dx, float dy);
    void ProcessMouseDolly(float delta);
    void ProcessMouseScroll(float yoffset);

    void ProcessFlyMovement(const bool* keys, float dt);
    void AdjustFlySpeed(float wheelDeltaY);

    void SetViewportSize(float width, float height);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::vec3 GetPosition() const { return m_Position; }
    glm::vec3 GetLookAt() const { return m_LookAt; }
    glm::vec3 GetForward() const;
    glm::vec3 GetRight() const;
    glm::vec3 GetUp() const;

    float GetDistance() const { return m_Distance; }
    float GetGridLodDistance() const;
    float GetFov() const { return m_Fov; }
    float GetPitch() const { return m_Pitch; }
    float GetYaw() const { return m_Yaw; }

    glm::vec3 GetPreviousPosition() const { return m_PrevPosition; }
    float GetPreviousPitch() const { return m_PrevPitch; }
    float GetPreviousYaw() const { return m_PrevYaw; }
    float GetLastDeltaTime() const { return m_LastDeltaTime; }

    float GetCameraSpeed() const { return m_MoveSpeed; }
    void SetCameraSpeed(float speed);

    glm::vec3 GetOrbitPivot() const { return m_TargetLookAt; }
    void SetOrbitPivot(const glm::vec3& pivot);

    void Focus(const glm::vec3& target);
    void Reset();
    void ResumeOrbitNavigation();

private:
    void SyncOrientationFromView();
    void SyncOrbitStateFromCameraPosition();
    void UpdateOrbitPositionFromAngles();
    void UpdateLookAtFromFlyOrientation();
    glm::vec3 ComputeForwardFromAngles() const;

    glm::vec3 m_Position{ 0.0f, 6.0f, 15.0f };
    glm::vec3 m_LookAt{ 0.0f, 0.0f, 0.0f };
    float m_Pitch = -15.0f;
    float m_Yaw = -90.0f;
    float m_Distance = 15.0f;

    glm::vec3 m_TargetPosition{ 0.0f, 6.0f, 15.0f };
    glm::vec3 m_TargetLookAt{ 0.0f, 0.0f, 0.0f };
    float m_TargetPitch = -15.0f;
    float m_TargetYaw = -90.0f;
    float m_TargetDistance = 15.0f;

    float m_Fov = 45.0f;
    float m_AspectRatio = 1.777f;
    float m_Near = 0.1f;
    float m_Far = 100000.0f;

    bool m_FlyMode = false;
    bool m_FreeLook = false;
    glm::vec3 m_SavedOrbitPivot{ 0.0f };

    float m_MoveSpeed = kEditorCameraDefaultSpeed;
    float m_Sensitivity = 0.15f;
    float m_LerpSpeed = 12.0f;
    float m_Acceleration = 1.0f;
    float m_BoostMultiplier = 4.0f;
    float m_SlowMultiplier = 0.25f;
    float m_ScrollWheelSpeedMultiplier = 1.0f;
    bool m_InvertX = false;
    bool m_InvertY = false;

    glm::vec3 m_PrevPosition{ 0.0f, 6.0f, 15.0f };
    float m_PrevPitch = -15.0f;
    float m_PrevYaw = -90.0f;
    float m_LastDeltaTime = 0.0f;
};

} // namespace we::runtime::engine
