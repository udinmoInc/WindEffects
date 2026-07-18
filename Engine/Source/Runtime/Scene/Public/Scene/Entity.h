#pragma once

#include "Scene/Export.h"

#include "Core/Math/Types.h"

#include <cstdint>
#include <string>

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
    we::math::Vec3 Position{0.0f, 0.0f, 0.0f};
    we::math::Vec3 Rotation{0.0f, 0.0f, 0.0f};
    we::math::Vec3 Scale{1.0f, 1.0f, 1.0f};
    we::math::Vec4 Color{1.0f, 1.0f, 1.0f, 1.0f};
    int Mode = 0;
    bool EditorOnly = false;
    std::uint64_t ParentId = 0;

    // Reserved for future RHI-owned entity GPU resources (opaque handle integers).
    std::uint64_t GpuUniformBuffer = 0;
    std::uint64_t GpuUniformMemory = 0;
    std::uint64_t GpuDescriptorSet = 0;

    SCENE_API we::math::Mat4 GetModelMatrix() const;
};

} // namespace we::runtime::scene
