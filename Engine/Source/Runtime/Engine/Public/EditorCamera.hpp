#pragma once

#include <glm/glm.hpp>
#include <SDL3/SDL.h>

namespace we::runtime::engine {

class EditorCamera {
public:
    EditorCamera();
    ~EditorCamera() = default;

    void Update(float dt);
    
    // Interaction methods
    void ProcessMouseScroll(float yoffset);
    void ProcessMousePan(float dx, float dy);
    void ProcessMouseOrbit(float dx, float dy);
    void ProcessMouseFPS(float dx, float dy);
    void ProcessKeyboard(const bool* keys, float dt);

    void SetViewportSize(float width, float height);
    void SetFPSMode(bool enabled);
    bool IsFPSMode() const { return m_FPSMode; }

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

    static constexpr float kMinCameraSpeed = 1.0f;
    static constexpr float kMaxCameraSpeed = 50.0f;
    static constexpr float kDefaultCameraSpeed = 4.0f;

    float GetCameraSpeed() const { return m_MoveSpeed; }
    void SetCameraSpeed(float speed);
    
    void Focus(const glm::vec3& target);
    void Reset();

private:
    void UpdateCameraVectors();

    // Interpolated states (what is rendered)
    glm::vec3 m_Position{ 0.0f, 6.0f, 15.0f };
    glm::vec3 m_LookAt{ 0.0f, 0.0f, 0.0f };
    float m_Pitch = -15.0f; // Degrees
    float m_Yaw = -90.0f;  // Degrees
    float m_Distance = 15.0f;

    // Desired states (user input targets)
    glm::vec3 m_TargetPosition{ 0.0f, 6.0f, 15.0f };
    glm::vec3 m_TargetLookAt{ 0.0f, 0.0f, 0.0f };
    float m_TargetPitch = -15.0f;
    float m_TargetYaw = -90.0f;
    float m_TargetDistance = 15.0f;

    // Projection params
    float m_Fov = 45.0f;
    float m_AspectRatio = 1.777f;
    float m_Near = 0.1f;
    float m_Far = 100000.0f;

    bool m_FPSMode = false;
    float m_MoveSpeed = kDefaultCameraSpeed;
    float m_Sensitivity = 0.15f;
    float m_LerpSpeed = 12.0f; // Higher is faster/snappier, lower is smoother
};

} // namespace we::runtime::engine
