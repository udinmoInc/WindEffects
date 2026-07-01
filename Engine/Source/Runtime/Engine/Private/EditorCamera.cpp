#include "EditorCamera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <algorithm>
#include <cmath>

namespace we::runtime::engine {

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
    m_MoveSpeed = std::clamp(speed, kMinCameraSpeed, kMaxCameraSpeed);
}

void EditorCamera::Reset() {
    m_FlyMode = false;
    m_FreeLook = false;
    m_TargetLookAt = glm::vec3(0.0f);
    m_TargetDistance = 15.0f;
    m_TargetPitch = -20.0f;
    m_TargetYaw = -135.0f;

    m_LookAt = m_TargetLookAt;
    m_Distance = m_TargetDistance;
    m_Pitch = m_TargetPitch;
    m_Yaw = m_TargetYaw;

    UpdateOrbitPositionFromAngles();
    m_Position = m_TargetPosition;
}

void EditorCamera::SetOrbitPivot(const glm::vec3& pivot) {
    m_SavedOrbitPivot = pivot;
    m_TargetLookAt = pivot;
    if (!m_FlyMode && !m_FreeLook) {
        UpdateOrbitPositionFromAngles();
    }
}

void EditorCamera::Focus(const glm::vec3& target) {
    m_TargetLookAt = target;
    m_SavedOrbitPivot = target;
    m_TargetDistance = 8.0f;
    m_FreeLook = false;
    if (m_FlyMode) {
        EnterFlyMode();
        m_TargetPosition = target - ComputeForwardFromAngles() * m_TargetDistance;
    } else {
        UpdateOrbitPositionFromAngles();
    }
}

void EditorCamera::SetViewportSize(float width, float height) {
    if (height > 0.0f) {
        m_AspectRatio = width / height;
    }
}

glm::vec3 EditorCamera::ComputeForwardFromAngles() const {
    const float theta = glm::radians(m_TargetYaw);
    const float phi = glm::radians(m_TargetPitch);

    glm::vec3 forward;
    forward.x = cos(phi) * cos(theta);
    forward.y = sin(phi);
    forward.z = cos(phi) * sin(theta);
    return glm::normalize(forward);
}

void EditorCamera::SyncOrientationFromView() {
    const glm::vec3 forward = glm::normalize(m_LookAt - m_Position);
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

    m_TargetPosition = m_TargetLookAt + offset;
}

void EditorCamera::SyncOrbitStateFromCameraPosition() {
    glm::vec3 offset = m_Position - m_TargetLookAt;
    float distance = glm::length(offset);
    if (distance < 0.5f) {
        distance = 0.5f;
        offset = glm::normalize(ComputeForwardFromAngles()) * -distance;
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
    m_TargetLookAt = m_TargetPosition + ComputeForwardFromAngles() * std::max(1.0f, m_TargetDistance);
}

void EditorCamera::EnterFlyMode() {
    m_SavedOrbitPivot = m_TargetLookAt;
    m_FlyMode = true;
    m_FreeLook = false;
    m_TargetPosition = m_Position;
    m_TargetLookAt = m_LookAt;
    m_TargetDistance = std::max(1.0f, glm::distance(m_Position, m_LookAt));
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
    m_TargetDistance = std::max(1.0f, glm::distance(m_Position, m_LookAt));
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
    m_TargetPitch = std::clamp(m_TargetPitch, -89.0f, 89.0f);

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

    const float t = std::clamp(m_LerpSpeed * dt, 0.0f, 1.0f);
    m_Pitch = glm::mix(m_Pitch, m_TargetPitch, t);
    m_Yaw = glm::mix(m_Yaw, m_TargetYaw, t);
    m_Distance = glm::mix(m_Distance, m_TargetDistance, t);
    m_LookAt = glm::mix(m_LookAt, m_TargetLookAt, t);
    m_Position = glm::mix(m_Position, m_TargetPosition, t);
}

void EditorCamera::ProcessFlyLook(float dx, float dy) {
    if (!m_FlyMode) {
        return;
    }

    const float xSign = m_InvertX ? -1.0f : 1.0f;
    const float ySign = m_InvertY ? 1.0f : -1.0f;
    m_TargetYaw += dx * m_Sensitivity * xSign;
    m_TargetPitch += dy * m_Sensitivity * ySign;
    m_TargetPitch = std::clamp(m_TargetPitch, -89.0f, 89.0f);
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
    m_TargetPitch = std::clamp(m_TargetPitch, -89.0f, 89.0f);
}

void EditorCamera::ProcessMousePan(float dx, float dy) {
    if (m_FlyMode) {
        return;
    }

    ResumeOrbitNavigation();

    const float speedScale = m_MoveSpeed / kDefaultCameraSpeed;
    const float panSpeed = std::max(0.01f, m_TargetDistance * 0.0015f) * speedScale;

    const glm::vec3 right = GetRight();
    const glm::vec3 up = GetUp();

    m_TargetLookAt -= right * dx * panSpeed;
    m_TargetLookAt += up * dy * panSpeed;
}

void EditorCamera::ProcessMouseDolly(float delta) {
    if (m_FlyMode) {
        return;
    }

    ResumeOrbitNavigation();

    const float speedScale = m_MoveSpeed / kDefaultCameraSpeed;
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

void EditorCamera::ProcessFlyMovement(const bool* keys, float dt) {
    if (!m_FlyMode || keys == nullptr) {
        return;
    }

    float speed = m_MoveSpeed * m_Acceleration;
    if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
        speed *= m_BoostMultiplier;
    } else if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) {
        speed *= m_SlowMultiplier;
    }

    const float distance = speed * dt;
    const glm::vec3 forward = ComputeForwardFromAngles();
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
        m_TargetPosition += forward * distance;
    }
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
        m_TargetPosition -= forward * distance;
    }
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
        m_TargetPosition -= right * distance;
    }
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
        m_TargetPosition += right * distance;
    }
    if (keys[SDL_SCANCODE_E]) {
        m_TargetPosition += up * distance;
    }
    if (keys[SDL_SCANCODE_Q]) {
        m_TargetPosition -= up * distance;
    }
}

void EditorCamera::AdjustFlySpeed(float wheelDeltaY) {
    if (std::abs(wheelDeltaY) < 1e-4f) {
        return;
    }
    const float direction = wheelDeltaY > 0.0f ? 1.0f : -1.0f;
    SetCameraSpeed(m_MoveSpeed + direction * m_ScrollWheelSpeedMultiplier);
}

glm::mat4 EditorCamera::GetViewMatrix() const {
    return glm::lookAt(m_Position, m_LookAt, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 EditorCamera::GetProjectionMatrix() const {
    const float farPlane = std::max(m_Far, m_TargetDistance * 120.0f);
    glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(m_Fov), m_AspectRatio, farPlane, m_Near);
    proj[1][1] *= -1.0f;
    return proj;
}

glm::vec3 EditorCamera::GetForward() const {
    return glm::normalize(m_LookAt - m_Position);
}

glm::vec3 EditorCamera::GetRight() const {
    return glm::normalize(glm::cross(GetForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 EditorCamera::GetUp() const {
    return glm::normalize(glm::cross(GetRight(), GetForward()));
}

float EditorCamera::GetGridLodDistance() const {
    if (m_FlyMode || m_FreeLook) {
        return std::max(m_Position.y, 0.5f);
    }
    return std::max(m_Distance, 0.5f);
}

} // namespace we::runtime::engine
