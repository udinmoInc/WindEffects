#pragma once

#include <string>
#include <cstdint>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#else
// Fallback simple math types when GLM is not available
#ifndef GLM_FALLBACK_TYPES_DEFINED
#define GLM_FALLBACK_TYPES_DEFINED
struct glm_vec3 { float x, y, z; };
struct glm_vec4 { float x, y, z, w; };
struct glm_mat4 { float data[16]; };
namespace glm {
    using vec3 = glm_vec3;
    using vec4 = glm_vec4;
    using mat4 = glm_mat4;
}
#endif
#endif
#if WE_HAS_VULKAN
#include <volk.h>
#endif

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
    glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 Rotation{ 0.0f, 0.0f, 0.0f }; // Euler angles in degrees
    glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };
    glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
    int Mode = 0; // 0 = Lit, 1 = Unlit, 2 = Wireframe
    bool EditorOnly = false;
    std::uint64_t ParentId = 0;

#if WE_HAS_VULKAN
    // Vulkan resources (allocated when added to Scene)
    VkBuffer UniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory UniformMemory = VK_NULL_HANDLE;
    VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
#endif

    glm::mat4 GetModelMatrix() const;
};

} // namespace we::runtime::scene
