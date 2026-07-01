#pragma once

#include <string>
#include <cstdint>
#include <glm/glm.hpp>
#include <volk.h>

namespace we::runtime::scene {

enum class EntityType {
    EmptyActor,
    Character,
    Blueprint,
    Cube,
    Sphere,
    Cylinder,
    Plane,
    DirectionalLight,
    PointLight,
    SpotLight,
    SkyLight,
    SkyAtmosphere,
    HeightFog,
    VolumetricClouds,
    GroundPlane,
    CameraIcon,
    AudioSource,
    Volume,
};

struct Entity {
    std::uint64_t Id = 0;
    std::string Name;
    EntityType Type;
    glm::vec3 Position{ 0.0f };
    glm::vec3 Rotation{ 0.0f }; // Euler angles in degrees
    glm::vec3 Scale{ 1.0f };
    glm::vec4 Color{ 1.0f };
    int Mode = 0; // 0 = Lit, 1 = Unlit, 2 = Wireframe
    bool EditorOnly = false;
    std::uint64_t ParentId = 0;

    // Vulkan resources (allocated when added to Scene)
    VkBuffer UniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory UniformMemory = VK_NULL_HANDLE;
    VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;

    glm::mat4 GetModelMatrix() const;
};

} // namespace we::runtime::scene
