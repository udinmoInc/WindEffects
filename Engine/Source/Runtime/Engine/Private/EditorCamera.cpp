#include "EditorCamera.h"

// EditorCamera methods must use real glm types (see EditorCamera.h) so DLL exports match editor modules.

#if WE_HAS_GLM
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#endif
#include <algorithm>
#include <cmath>

#include "Core/Math/GlmInterop.h"
namespace we::runtime::engine {

#if WE_HAS_GLM

EditorCamera::EditorCamera() {
    Reset();
}

void EditorCamera::SetNavigationSettings(const EditorCameraNavigationSettings& settings) {
    m_Sensitivity = settings.mouseSensitivity;
    m_LerpSpeed = settings.cameraSmoothing;
    m_Acceleration = settings.cameraAcceleration;
    m_InvertX = settings.invertX;
    m_InvertY = settings.invertY;
    m_BoostMultiplier = settings.maxBoostMultiplier;
    m_SlowMultiplier = settings.slowMultiplier;
    m_ScrollWheelSpeedMultiplier = settings.scrollWheelSpeedMultiplier;
}

void EditorCamera::SetCameraSpeed(float speed) {
    m_MoveSpeed = std::clamp(speed, kEditorCameraMinSpeed, kEditorCameraMaxSpeed);
}

void EditorCamera::Reset() {
    m_FlyMode = false;
    m_FreeLook = false;
    m_SavedOrbitPivot = we::math::Vec3(0.0f);
    m_MoveSpeed = kEditorCameraDefaultSpeed;
    m_Sensitivity = 0.15f;
    m_LerpSpeed = 12.0f;
    m_Acceleration = 1.0f;
    m_BoostMultiplier = 4.0f;
    m_SlowMultiplier = 0.25f;
    m_ScrollWheelSpeedMultiplier = 1.0f;
    m_InvertX = false;
    m_InvertY = false;
    m_Position = we::math::Vec3(0.0f, 6.0f, 15.0f);
    m_LookAt = we::math::Vec3(0.0f, 0.0f, 0.0f);
    m_Pitch = -15.0f;
    m_Yaw = -90.0f;
    m_Distance = 15.0f;
    m_TargetPosition = we::math::Vec3(0.0f, 6.0f, 15.0f);
    m_TargetLookAt = we::math::Vec3(0.0f, 0.0f, 0.0f);
    m_TargetPitch = -15.0f;
    m_TargetYaw = -90.0f;
    m_TargetDistance = 15.0f;
    m_Fov = 45.0f;
    m_AspectRatio = 1.777f;
    m_Near = 0.1f;
    m_Far = 100000.0f;
    m_PrevPosition = m_Position;
    m_PrevPitch = m_Pitch;
    m_PrevYaw = m_Yaw;
    m_LastDeltaTime = 0.0f;
}

void EditorCamera::SetOrbitPivot(const we::math::Vec3& pivot) {
    m_TargetLookAt = pivot;
    if (!m_FlyMode && !m_FreeLook) {
        UpdateOrbitPositionFromAngles();
    }
}

void EditorCamera::Focus(const we::math::Vec3& target) {
    Focus(target, 8.0f);
}

void EditorCamera::Focus(const we::math::Vec3& target, float distance) {
    m_TargetLookAt = target;
    m_SavedOrbitPivot = target;
    m_TargetDistance = std::max(1.0f, distance);
    m_FreeLook = false;
    if (m_FlyMode) {
        EnterFlyMode();
        m_TargetPosition = we::math::FromGlm(we::math::ToGlm(target) - we::math::ToGlm(ComputeForwardFromAngles()) * m_TargetDistance);
    } else {
        UpdateOrbitPositionFromAngles();
    }
}

void EditorCamera::SetViewportSize(float width, float height) {
    if (height > 0.0f) {
        m_AspectRatio = width / height;
    }
}

we::math::Vec3 EditorCamera::ComputeForwardFromAngles() const {
    const float theta = glm::radians(m_TargetYaw);
    const float phi = glm::radians(m_TargetPitch);

    glm::vec3 forward;
    forward.x = cos(phi) * cos(theta);
    forward.y = sin(phi);
    forward.z = cos(phi) * sin(theta);
    const float lenSq = glm::dot(forward, forward);
    if (lenSq < 1e-12f) {
        return we::math::Vec3(0.0f, 0.0f, -1.0f);
    }
    return we::math::FromGlm(forward * (1.0f / std::sqrt(lenSq)));
}

we::math::Vec3 EditorCamera::ComputeRightFromYaw(float yawDegrees) const {
    // Yaw-derived lateral axis stays valid at extreme pitch (avoids normalize(0)).
    const float yaw = glm::radians(yawDegrees);
    return we::math::Vec3(-std::sin(yaw), 0.0f, std::cos(yaw));
}

void EditorCamera::SyncOrientationFromView() {
    const glm::vec3 forward = glm::normalize(we::math::ToGlm(m_LookAt) - we::math::ToGlm(m_Position));
    m_TargetPitch = glm::degrees(std::asin(std::clamp(forward.y, -1.0f, 1.0f)));
    m_TargetYaw = glm::degrees(std::atan2(forward.z, forward.x));
    m_Pitch = m_TargetPitch;
    m_Yaw = m_TargetYaw;
}

void EditorCamera::UpdateOrbitPositionFromAngles() {
    const float theta = glm::radians(m_TargetYaw);
    const float phi = glm::radians(m_TargetPitch);

    glm::vec3 offset;
    offset.x = m_TargetDistance * cos(phi) * cos(theta);
    offset.y = m_TargetDistance * sin(phi);
    offset.z = m_TargetDistance * cos(phi) * sin(theta);

    m_TargetPosition = we::math::FromGlm(we::math::ToGlm(m_TargetLookAt) + offset);
}

void EditorCamera::SyncOrbitStateFromCameraPosition() {
    glm::vec3 offset = we::math::ToGlm(m_Position) - we::math::ToGlm(m_TargetLookAt);
    float distance = glm::length(offset);
    if (distance < 0.5f) {
        distance = 0.5f;
        offset = glm::normalize(we::math::ToGlm(ComputeForwardFromAngles())) * -distance;
    }

    m_TargetDistance = distance;
    m_Distance = distance;

    const glm::vec3 direction = offset / distance;
    m_TargetPitch = glm::degrees(std::asin(std::clamp(direction.y, -1.0f, 1.0f)));
    m_TargetYaw = glm::degrees(std::atan2(direction.z, direction.x));
    m_Pitch = m_TargetPitch;
    m_Yaw = m_TargetYaw;

    m_TargetPosition = m_Position;
    m_LookAt = m_TargetLookAt;
}

void EditorCamera::UpdateLookAtFromFlyOrientation() {
    m_TargetLookAt = we::math::FromGlm(we::math::ToGlm(m_TargetPosition) + we::math::ToGlm(ComputeForwardFromAngles()) * std::max(1.0f, m_TargetDistance));
}

void EditorCamera::EnterFlyMode() {
    m_SavedOrbitPivot = m_TargetLookAt;
    m_FlyMode = true;
    m_FreeLook = false;
    m_TargetPosition = m_Position;
    m_TargetLookAt = m_LookAt;
    m_TargetDistance = std::max(1.0f, glm::distance(we::math::ToGlm(m_Position), we::math::ToGlm(m_LookAt)));
    SyncOrientationFromView();
    UpdateLookAtFromFlyOrientation();
}

void EditorCamera::ExitFlyMode() {
    if (!m_FlyMode) {
        return;
    }

    m_FlyMode = false;
    m_FreeLook = true;
    m_TargetPosition = m_Position;
    m_TargetLookAt = m_LookAt;
    m_TargetPitch = m_Pitch;
    m_TargetYaw = m_Yaw;
    m_TargetDistance = std::max(1.0f, glm::distance(we::math::ToGlm(m_Position), we::math::ToGlm(m_LookAt)));
}

void EditorCamera::ResumeOrbitNavigation() {
    if (!m_FreeLook) {
        return;
    }

    m_FreeLook = false;
    m_TargetLookAt = m_SavedOrbitPivot;
    SyncOrbitStateFromCameraPosition();
}

void EditorCamera::Update(float dt) {
    m_LastDeltaTime = dt;
    m_PrevPosition = m_Position;
    m_PrevPitch = m_Pitch;
    m_PrevYaw = m_Yaw;

    m_TargetPitch = std::clamp(m_TargetPitch, -88.5f, 88.5f);

    if (m_FlyMode) {
        UpdateLookAtFromFlyOrientation();
        m_Pitch = m_TargetPitch;
        m_Yaw = m_TargetYaw;
        m_Position = m_TargetPosition;
        m_LookAt = m_TargetLookAt;
        return;
    }

    if (m_FreeLook) {
        m_Pitch = m_TargetPitch;
        m_Yaw = m_TargetYaw;
        m_Position = m_TargetPosition;
        m_LookAt = m_TargetLookAt;
        return;
    }

    UpdateOrbitPositionFromAngles();

    const float t = 1.0f - std::exp(-m_LerpSpeed * dt);
    m_Pitch = glm::mix(m_Pitch, m_TargetPitch, t);
    m_Yaw = glm::mix(m_Yaw, m_TargetYaw, t);
    m_Distance = glm::mix(m_Distance, m_TargetDistance, t);
    m_LookAt = we::math::FromGlm(glm::mix(we::math::ToGlm(m_LookAt), we::math::ToGlm(m_TargetLookAt), t));
    m_Position = we::math::FromGlm(glm::mix(we::math::ToGlm(m_Position), we::math::ToGlm(m_TargetPosition), t));
}

void EditorCamera::ProcessFlyLook(float dx, float dy) {
    if (!m_FlyMode) {
        return;
    }

    const float xSign = m_InvertX ? -1.0f : 1.0f;
    const float ySign = m_InvertY ? 1.0f : -1.0f;
    m_TargetYaw += dx * m_Sensitivity * xSign;
    m_TargetPitch += dy * m_Sensitivity * ySign;
    m_TargetPitch = std::clamp(m_TargetPitch, -88.5f, 88.5f);
}

void EditorCamera::ProcessMouseOrbit(float dx, float dy) {
    if (m_FlyMode) {
        return;
    }

    ResumeOrbitNavigation();

    const float xSign = m_InvertX ? -1.0f : 1.0f;
    const float ySign = m_InvertY ? 1.0f : -1.0f;
    m_TargetYaw += dx * m_Sensitivity * xSign;
    m_TargetPitch += dy * m_Sensitivity * ySign;
    m_TargetPitch = std::clamp(m_TargetPitch, -88.5f, 88.5f);
}

void EditorCamera::ProcessMousePan(float dx, float dy) {
    if (m_FlyMode) {
        return;
    }

    ResumeOrbitNavigation();

    const float speedScale = m_MoveSpeed / kEditorCameraDefaultSpeed;
    const float panSpeed = std::max(0.01f, m_TargetDistance * 0.0015f) * speedScale;

    const glm::vec3 forward = we::math::ToGlm(ComputeForwardFromAngles());
    const glm::vec3 right = we::math::ToGlm(ComputeRightFromYaw(m_TargetYaw));
    const glm::vec3 up = glm::normalize(glm::cross(right, forward));

    m_TargetLookAt = we::math::FromGlm(we::math::ToGlm(m_TargetLookAt) - right * dx * panSpeed);
    m_TargetLookAt = we::math::FromGlm(we::math::ToGlm(m_TargetLookAt) + up * dy * panSpeed);
}

void EditorCamera::ProcessMouseDolly(float delta) {
    if (m_FlyMode) {
        return;
    }

    ResumeOrbitNavigation();

    const float speedScale = m_MoveSpeed / kEditorCameraDefaultSpeed;
    const float zoomStep = std::max(0.5f, m_TargetDistance * 0.1f) * speedScale;
    m_TargetDistance -= delta * zoomStep;
    m_TargetDistance = std::max(0.5f, m_TargetDistance);
}

void EditorCamera::ProcessMouseScroll(float yoffset) {
    if (m_FlyMode) {
        return;
    }
    ProcessMouseDolly(yoffset);
}

void EditorCamera::ProcessFlyMovement(const EditorCameraFlyKeys& keys, float dt) {
    if (!m_FlyMode) {
        return;
    }

    float speed = m_MoveSpeed * m_Acceleration;
    if (keys.boost) {
        speed *= m_BoostMultiplier;
    } else if (keys.slow) {
        speed *= m_SlowMultiplier;
    }

    const float distance = speed * dt;
    const glm::vec3 forward = we::math::ToGlm(ComputeForwardFromAngles());
    const glm::vec3 right = we::math::ToGlm(ComputeRightFromYaw(m_TargetYaw));
    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    if (keys.forward) {
        m_TargetPosition = we::math::FromGlm(we::math::ToGlm(m_TargetPosition) + forward * distance);
    }
    if (keys.back) {
        m_TargetPosition = we::math::FromGlm(we::math::ToGlm(m_TargetPosition) - forward * distance);
    }
    if (keys.left) {
        m_TargetPosition = we::math::FromGlm(we::math::ToGlm(m_TargetPosition) - right * distance);
    }
    if (keys.right) {
        m_TargetPosition = we::math::FromGlm(we::math::ToGlm(m_TargetPosition) + right * distance);
    }
    if (keys.up) {
        m_TargetPosition = we::math::FromGlm(we::math::ToGlm(m_TargetPosition) + up * distance);
    }
    if (keys.down) {
        m_TargetPosition = we::math::FromGlm(we::math::ToGlm(m_TargetPosition) - up * distance);
    }
}

void EditorCamera::AdjustFlySpeed(float wheelDeltaY) {
    if (std::abs(wheelDeltaY) < 1e-4f) {
        return;
    }
    const float direction = wheelDeltaY > 0.0f ? 1.0f : -1.0f;
    SetCameraSpeed(m_MoveSpeed + direction * m_ScrollWheelSpeedMultiplier);
}

we::math::Mat4 EditorCamera::GetViewMatrix() const {
    // Build an orthonormal basis from yaw/pitch so look-at never collapses when
    // pitched near ±90° (world-up lookAt produces NaNs and hard GPU/device faults).
    const float yaw = glm::radians(m_Yaw);
    const float pitch = glm::radians(m_Pitch);

    glm::vec3 forward;
    forward.x = std::cos(pitch) * std::cos(yaw);
    forward.y = std::sin(pitch);
    forward.z = std::cos(pitch) * std::sin(yaw);
    const float fLenSq = glm::dot(forward, forward);
    if (fLenSq < 1e-12f) {
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
    } else {
        forward *= 1.0f / std::sqrt(fLenSq);
    }

    const glm::vec3 right = we::math::ToGlm(ComputeRightFromYaw(m_Yaw));
    glm::vec3 up = glm::cross(right, forward);
    const float uLenSq = glm::dot(up, up);
    if (uLenSq < 1e-12f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        up *= 1.0f / std::sqrt(uLenSq);
    }

    return we::math::FromGlm(glm::lookAt(we::math::ToGlm(m_Position), we::math::ToGlm(m_Position) + forward, up));
}

we::math::Mat4 EditorCamera::GetProjectionMatrix() const {
    const float aspect = std::max(m_AspectRatio, 0.01f);
    glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(m_Fov), aspect, m_Near, m_Far);
    proj[1][1] *= -1.0f;
    return we::math::FromGlm(proj);
}

we::math::Vec3 EditorCamera::GetForward() const {
    const float yaw = glm::radians(m_Yaw);
    const float pitch = glm::radians(m_Pitch);
    glm::vec3 forward;
    forward.x = std::cos(pitch) * std::cos(yaw);
    forward.y = std::sin(pitch);
    forward.z = std::cos(pitch) * std::sin(yaw);
    const float lenSq = glm::dot(forward, forward);
    if (lenSq < 1e-12f) {
        return we::math::Vec3(0.0f, 0.0f, -1.0f);
    }
    return we::math::FromGlm(forward * (1.0f / std::sqrt(lenSq)));
}

we::math::Vec3 EditorCamera::GetRight() const {
    return ComputeRightFromYaw(m_Yaw);
}

we::math::Vec3 EditorCamera::GetUp() const {
    const glm::vec3 forward = we::math::ToGlm(GetForward());
    const glm::vec3 right = we::math::ToGlm(GetRight());
    glm::vec3 up = glm::cross(right, forward);
    const float lenSq = glm::dot(up, up);
    if (lenSq < 1e-12f) {
        return we::math::Vec3(0.0f, 1.0f, 0.0f);
    }
    return we::math::FromGlm(up * (1.0f / std::sqrt(lenSq)));
}

float EditorCamera::GetGridLodDistance() const {
    if (m_FlyMode || m_FreeLook) {
        return std::max(m_Position.y, 0.5f);
    }
    return std::max(m_Distance, 0.5f);
}

bool EditorCamera::IsFlyMode() const {
    return m_FlyMode;
}

we::math::Vec3 EditorCamera::GetPosition() const {
    return m_Position;
}

we::math::Vec3 EditorCamera::GetLookAt() const {
    return m_LookAt;
}

float EditorCamera::GetDistance() const {
    return m_Distance;
}

float EditorCamera::GetFov() const {
    return m_Fov;
}

float EditorCamera::GetPitch() const {
    return m_Pitch;
}

float EditorCamera::GetYaw() const {
    return m_Yaw;
}

we::math::Vec3 EditorCamera::GetPreviousPosition() const {
    return m_PrevPosition;
}

float EditorCamera::GetPreviousPitch() const {
    return m_PrevPitch;
}

float EditorCamera::GetPreviousYaw() const {
    return m_PrevYaw;
}

float EditorCamera::GetLastDeltaTime() const {
    return m_LastDeltaTime;
}

float EditorCamera::GetCameraSpeed() const {
    return m_MoveSpeed;
}

we::math::Vec3 EditorCamera::GetOrbitPivot() const {
    return m_TargetLookAt;
}

#else // !WE_HAS_GLM

// Stub implementations when GLM is not available
EditorCamera::EditorCamera() {
    // Initialize with default values
    m_Position = we::math::Vec3{0.0f, 6.0f, 15.0f};
    m_LookAt = we::math::Vec3{0.0f, 0.0f, 0.0f};
    m_Pitch = -15.0f;
    m_Yaw = -90.0f;
    m_Distance = 15.0f;
    m_Fov = 45.0f;
    m_AspectRatio = 1.777f;
    m_Near = 0.1f;
    m_Far = 100000.0f;
    m_MoveSpeed = kEditorCameraDefaultSpeed;
}

void EditorCamera::SetNavigationSettings(const EditorCameraNavigationSettings& settings) {
    (void)settings; // Suppress unused parameter warning
}

void EditorCamera::SetCameraSpeed(float speed) {
    m_MoveSpeed = std::clamp(speed, kEditorCameraMinSpeed, kEditorCameraMaxSpeed);
}

void EditorCamera::Reset() {
    // Stub
}

void EditorCamera::SetOrbitPivot(const we::math::Vec3& pivot) {
    (void)pivot; // Suppress unused parameter warning
}

void EditorCamera::Focus(const we::math::Vec3& target) {
    Focus(target, 8.0f);
}

void EditorCamera::Focus(const we::math::Vec3& target, float distance) {
    (void)target;
    (void)distance;
}

void EditorCamera::SetViewportSize(float width, float height) {
    if (height > 0.0f) {
        m_AspectRatio = width / height;
    }
}

void EditorCamera::EnterFlyMode() {
    m_FlyMode = true;
}

void EditorCamera::ExitFlyMode() {
    m_FlyMode = false;
}

void EditorCamera::ResumeOrbitNavigation() {
    // Stub
}

void EditorCamera::Update(float dt) {
    (void)dt; // Suppress unused parameter warning
}

void EditorCamera::ProcessFlyLook(float dx, float dy) {
    (void)dx; // Suppress unused parameter warning
    (void)dy; // Suppress unused parameter warning
}

void EditorCamera::ProcessMouseOrbit(float dx, float dy) {
    (void)dx; // Suppress unused parameter warning
    (void)dy; // Suppress unused parameter warning
}

void EditorCamera::ProcessMousePan(float dx, float dy) {
    (void)dx; // Suppress unused parameter warning
    (void)dy; // Suppress unused parameter warning
}

void EditorCamera::ProcessMouseDolly(float delta) {
    (void)delta; // Suppress unused parameter warning
}

void EditorCamera::ProcessMouseScroll(float yoffset) {
    (void)yoffset; // Suppress unused parameter warning
}

void EditorCamera::ProcessFlyMovement(const EditorCameraFlyKeys& keys, float dt) {
    (void)keys;
    (void)dt;
}

void EditorCamera::AdjustFlySpeed(float wheelDeltaY) {
    (void)wheelDeltaY; // Suppress unused parameter warning
}

we::math::Mat4 EditorCamera::GetViewMatrix() const {
    glm::mat4 result{};
    // Identity matrix stub
    result.data[0] = 1.0f;
    result.data[5] = 1.0f;
    result.data[10] = 1.0f;
    result.data[15] = 1.0f;
    return result;
}

we::math::Mat4 EditorCamera::GetProjectionMatrix() const {
    glm::mat4 result{};
    // Identity matrix stub
    result.data[0] = 1.0f;
    result.data[5] = 1.0f;
    result.data[10] = 1.0f;
    result.data[15] = 1.0f;
    return result;
}

we::math::Vec3 EditorCamera::GetForward() const {
    glm::vec3 result{0.0f, 0.0f, -1.0f};
    return result;
}

we::math::Vec3 EditorCamera::GetRight() const {
    glm::vec3 result{1.0f, 0.0f, 0.0f};
    return result;
}

we::math::Vec3 EditorCamera::GetUp() const {
    glm::vec3 result{0.0f, 1.0f, 0.0f};
    return result;
}

float EditorCamera::GetGridLodDistance() const {
    return 10.0f;
}

bool EditorCamera::IsFlyMode() const {
    return m_FlyMode;
}

we::math::Vec3 EditorCamera::GetPosition() const {
    return m_Position;
}

we::math::Vec3 EditorCamera::GetLookAt() const {
    return m_LookAt;
}

float EditorCamera::GetDistance() const {
    return m_Distance;
}

float EditorCamera::GetFov() const {
    return m_Fov;
}

float EditorCamera::GetPitch() const {
    return m_Pitch;
}

float EditorCamera::GetYaw() const {
    return m_Yaw;
}

we::math::Vec3 EditorCamera::GetPreviousPosition() const {
    return m_PrevPosition;
}

float EditorCamera::GetPreviousPitch() const {
    return m_PrevPitch;
}

float EditorCamera::GetPreviousYaw() const {
    return m_PrevYaw;
}

float EditorCamera::GetLastDeltaTime() const {
    return m_LastDeltaTime;
}

float EditorCamera::GetCameraSpeed() const {
    return m_MoveSpeed;
}

we::math::Vec3 EditorCamera::GetOrbitPivot() const {
    return m_TargetLookAt;
}

#endif // WE_HAS_GLM

} // namespace we::runtime::engine
