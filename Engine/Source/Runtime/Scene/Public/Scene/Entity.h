#pragma once

#include "Scene/Export.h"

#include <cstdint>
#include <string>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#else
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
    Landscape,
    GroundPlane,
    CameraIcon,
    AudioSource,
    Volume,
};

struct Entity {
    std::uint64_t Id = 0;
    std::string Name;
    EntityType Type = EntityType::EmptyActor;
    glm::vec3 Position{0.0f, 0.0f, 0.0f};
    glm::vec3 Rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 Scale{1.0f, 1.0f, 1.0f};
    glm::vec4 Color{1.0f, 1.0f, 1.0f, 1.0f};
    int Mode = 0;
    bool EditorOnly = false;
    std::uint64_t ParentId = 0;

    // Opaque GPU bindings owned by the active RHI backend (not Vulkan/DX types).
    std::uint64_t GpuUniformBuffer = 0;
    std::uint64_t GpuUniformMemory = 0;
    std::uint64_t GpuDescriptorSet = 0;

    SCENE_API glm::mat4 GetModelMatrix() const;
};

} // namespace we::runtime::scene
