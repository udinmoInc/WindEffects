#pragma once

#include "Engine/Export.h"

#include "Core/Math/Types.h"

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

struct EditorCameraFlyKeys {
    bool forward = false;
    bool back = false;
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool boost = false;
    bool slow = false;
};

class ENGINE_API EditorCamera {
public:
    EditorCamera();
    ~EditorCamera() = default;

    void Update(float dt);

    void SetNavigationSettings(const EditorCameraNavigationSettings& settings);

    void EnterFlyMode();
    void ExitFlyMode();
    bool IsFlyMode() const;

    void ProcessFlyLook(float dx, float dy);

    void ProcessMouseOrbit(float dx, float dy);
    void ProcessMousePan(float dx, float dy);
    void ProcessMouseDolly(float delta);
    void ProcessMouseScroll(float yoffset);

    void ProcessFlyMovement(const EditorCameraFlyKeys& keys, float dt);
    void AdjustFlySpeed(float wheelDeltaY);

    void SetViewportSize(float width, float height);

    we::math::Mat4 GetViewMatrix() const;
    we::math::Mat4 GetProjectionMatrix() const;
    we::math::Vec3 GetPosition() const;
    we::math::Vec3 GetLookAt() const;
    we::math::Vec3 GetForward() const;
    we::math::Vec3 GetRight() const;
    we::math::Vec3 GetUp() const;

    float GetDistance() const;
    float GetGridLodDistance() const;
    float GetFov() const;
    float GetPitch() const;
    float GetYaw() const;

    we::math::Vec3 GetPreviousPosition() const;
    float GetPreviousPitch() const;
    float GetPreviousYaw() const;
    float GetLastDeltaTime() const;

    float GetCameraSpeed() const;
    void SetCameraSpeed(float speed);

    we::math::Vec3 GetOrbitPivot() const;
    void SetOrbitPivot(const we::math::Vec3& pivot);

    void Focus(const we::math::Vec3& target);
    void Reset();
    void ResumeOrbitNavigation();

private:
    void SyncOrientationFromView();
    void SyncOrbitStateFromCameraPosition();
    void UpdateOrbitPositionFromAngles();
    void UpdateLookAtFromFlyOrientation();
    we::math::Vec3 ComputeForwardFromAngles() const;
    we::math::Vec3 ComputeRightFromYaw(float yawDegrees) const;

    we::math::Vec3 m_Position{ 0.0f, 6.0f, 15.0f };
    we::math::Vec3 m_LookAt{ 0.0f, 0.0f, 0.0f };
    float m_Pitch = -15.0f;
    float m_Yaw = -90.0f;
    float m_Distance = 15.0f;

    we::math::Vec3 m_TargetPosition{ 0.0f, 6.0f, 15.0f };
    we::math::Vec3 m_TargetLookAt{ 0.0f, 0.0f, 0.0f };
    float m_TargetPitch = -15.0f;
    float m_TargetYaw = -90.0f;
    float m_TargetDistance = 15.0f;

    float m_Fov = 45.0f;
    float m_AspectRatio = 1.777f;
    float m_Near = 0.1f;
    float m_Far = 100000.0f;

    bool m_FlyMode = false;
    bool m_FreeLook = false;
    we::math::Vec3 m_SavedOrbitPivot{ 0.0f };

    float m_MoveSpeed = kEditorCameraDefaultSpeed;
    float m_Sensitivity = 0.15f;
    float m_LerpSpeed = 12.0f;
    float m_Acceleration = 1.0f;
    float m_BoostMultiplier = 4.0f;
    float m_SlowMultiplier = 0.25f;
    float m_ScrollWheelSpeedMultiplier = 1.0f;
    bool m_InvertX = false;
    bool m_InvertY = false;

    we::math::Vec3 m_PrevPosition{ 0.0f, 6.0f, 15.0f };
    float m_PrevPitch = -15.0f;
    float m_PrevYaw = -90.0f;
    float m_LastDeltaTime = 0.0f;
};

} // namespace we::runtime::engine
