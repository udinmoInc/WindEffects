#pragma once

#include <string>
#include <glm/glm.hpp>
#include <volk.h>

namespace we::runtime::scene {

enum class EntityType {
    Cube,
    Plane,
    DirectionalLight,
    CameraIcon
};

struct Entity {
    std::string Name;
    EntityType Type;
    glm::vec3 Position{ 0.0f };
    glm::vec3 Rotation{ 0.0f }; // Euler angles in degrees
    glm::vec3 Scale{ 1.0f };
    glm::vec4 Color{ 1.0f };
    int Mode = 0; // 0 = Lit, 1 = Unlit, 2 = Wireframe

    // Vulkan resources (allocated when added to Scene)
    VkBuffer UniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory UniformMemory = VK_NULL_HANDLE;
    VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;

    glm::mat4 GetModelMatrix() const;
};

} // namespace we::runtime::scene
