#include "EditorCamera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <algorithm>
#include <iostream>

namespace we::runtime::engine {

EditorCamera::EditorCamera() {
    UpdateCameraVectors();
    Reset();
}

void EditorCamera::SetCameraSpeed(float speed) {
    m_MoveSpeed = std::clamp(speed, kMinCameraSpeed, kMaxCameraSpeed);
}

void EditorCamera::Reset() {
    m_TargetLookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    m_TargetDistance = 15.0f;
    m_TargetPitch = -20.0f;
    m_TargetYaw = -135.0f;

    // Immediately snap to targets on reset
    m_LookAt = m_TargetLookAt;
    m_Distance = m_TargetDistance;
    m_Pitch = m_TargetPitch;
    m_Yaw = m_TargetYaw;

    UpdateCameraVectors();
}

void EditorCamera::Focus(const glm::vec3& target) {
    m_TargetLookAt = target;
    m_TargetDistance = 8.0f; // Zoom in to focus
    UpdateCameraVectors();
}

void EditorCamera::SetViewportSize(float width, float height) {
    if (height > 0.0f) {
        m_AspectRatio = width / height;
    }
}

void EditorCamera::SetFPSMode(bool enabled) {
    m_FPSMode = enabled;
    if (m_FPSMode) {
        // Keep target distance from camera during FPS mode
        m_TargetDistance = glm::distance(m_Position, m_LookAt);
    }
}

void EditorCamera::UpdateCameraVectors() {
    // 1. In FPS mode, we calculate target LookAt based on position and direction
    if (m_FPSMode) {
        float theta = glm::radians(m_TargetYaw);
        float phi = glm::radians(m_TargetPitch);

        glm::vec3 forward;
        forward.x = cos(phi) * cos(theta);
        forward.y = sin(phi);
        forward.z = cos(phi) * sin(theta);
        forward = glm::normalize(forward);

        m_TargetLookAt = m_TargetPosition + forward * m_TargetDistance;
    } else {
        // 2. In Orbit mode, we calculate position relative to LookAt
        float theta = glm::radians(m_TargetYaw);
        float phi = glm::radians(m_TargetPitch);

        glm::vec3 offset;
        offset.x = m_TargetDistance * cos(phi) * cos(theta);
        offset.y = m_TargetDistance * sin(phi);
        offset.z = m_TargetDistance * cos(phi) * sin(theta);

        m_TargetPosition = m_TargetLookAt + offset;
    }
}

void EditorCamera::Update(float dt) {
    // Clamp target pitch to prevent gimbal lock
    m_TargetPitch = std::clamp(m_TargetPitch, -89.0f, 89.0f);

    // Dynamic camera vectors update
    UpdateCameraVectors();

    // Smoothly interpolate current states to target states
    float t = std::clamp(m_LerpSpeed * dt, 0.0f, 1.0f);
    m_Pitch = glm::mix(m_Pitch, m_TargetPitch, t);
    m_Yaw = glm::mix(m_Yaw, m_TargetYaw, t);
    m_Distance = glm::mix(m_Distance, m_TargetDistance, t);
    m_LookAt = glm::mix(m_LookAt, m_TargetLookAt, t);
    m_Position = glm::mix(m_Position, m_TargetPosition, t);
}

void EditorCamera::ProcessMouseScroll(float yoffset) {
    if (m_FPSMode) return; // Zoom is disabled in FPS mode

    // Zoom scales with orbit distance and camera speed (UE5-style).
    const float speedScale = m_MoveSpeed / kDefaultCameraSpeed;
    float zoomStep = std::max(0.5f, m_TargetDistance * 0.1f) * speedScale;
    m_TargetDistance -= yoffset * zoomStep;
    m_TargetDistance = std::max(1.0f, m_TargetDistance);
}

void EditorCamera::ProcessMousePan(float dx, float dy) {
    if (m_FPSMode) return;

    const float speedScale = m_MoveSpeed / kDefaultCameraSpeed;
    float panSpeed = std::max(0.01f, m_TargetDistance * 0.0015f) * speedScale;

    glm::vec3 right = GetRight();
    glm::vec3 up = GetUp();

    m_TargetLookAt -= right * dx * panSpeed;
    m_TargetLookAt += up * dy * panSpeed;
}

void EditorCamera::ProcessMouseOrbit(float dx, float dy) {
    if (m_FPSMode) return;

    m_TargetYaw += dx * m_Sensitivity;
    m_TargetPitch -= dy * m_Sensitivity;
}

void EditorCamera::ProcessMouseFPS(float dx, float dy) {
    if (!m_FPSMode) return;

    m_TargetYaw += dx * m_Sensitivity;
    m_TargetPitch -= dy * m_Sensitivity;
}

void EditorCamera::ProcessKeyboard(const bool* keys, float dt) {
    if (!m_FPSMode) return;

    float speed = m_MoveSpeed;
    if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
        speed *= 2.5f; // Fast mode
    }

    float distance = speed * dt;

    glm::vec3 forward = GetForward();
    glm::vec3 right = GetRight();
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f); // World Up for vertical movement

    // Forward/Backward
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
        m_TargetPosition += forward * distance;
    }
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
        m_TargetPosition -= forward * distance;
    }

    // Strafe Left/Right
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
        m_TargetPosition -= right * distance;
    }
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
        m_TargetPosition += right * distance;
    }

    // Vertical Up/Down
    if (keys[SDL_SCANCODE_E]) {
        m_TargetPosition += up * distance;
    }
    if (keys[SDL_SCANCODE_Q]) {
        m_TargetPosition -= up * distance;
    }
}

glm::mat4 EditorCamera::GetViewMatrix() const {
    return glm::lookAt(m_Position, m_LookAt, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 EditorCamera::GetProjectionMatrix() const {
    const float farPlane = std::max(m_Far, m_TargetDistance * 120.0f);
    // Reverse-Z projection improves depth precision at distance.
    glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(m_Fov), m_AspectRatio, farPlane, m_Near);
    proj[1][1] *= -1.0f; // Flip Y for Vulkan
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
    if (m_FPSMode) {
        return std::max(m_Position.y, 0.5f);
    }
    return std::max(m_Distance, 0.5f);
}

} // namespace we::runtime::engine
