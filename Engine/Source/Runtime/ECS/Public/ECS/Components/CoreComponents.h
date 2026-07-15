#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Export.h"
#include "ECS/Entity.h"
#include "ECS/ComponentType.h"

#include <array>
#include <cstdint>
#include <string>

#include <glm/glm.hpp>

namespace we::runtime::ecs {

struct Uuid {
    std::array<std::uint8_t, 16> bytes{};

    static Uuid Generate();
    static Uuid FromString(const std::string& text);
    std::string ToString() const;
    bool IsNil() const;
    bool operator==(const Uuid& o) const { return bytes == o.bytes; }
};

// ---- Core components (POD, no virtuals). Always GLM for stable cross-DLL ABI. ----

struct NameComponent {
    std::string value;
};

struct TagComponent {
    std::uint64_t mask = 0;
    std::string label;
};

struct UuidComponent {
    Uuid id{};
};

struct TransformComponent {
    glm::vec3 localPosition{ 0.0f };
    glm::vec3 localRotation{ 0.0f }; // euler degrees
    glm::vec3 localScale{ 1.0f };
    glm::mat4 worldMatrix{ 1.0f };
    bool dirty = true;
};

struct HierarchyComponent {
    Entity parent{};
    Entity firstChild{};
    Entity nextSibling{};
    Entity prevSibling{};
    std::uint16_t depth = 0;
};

struct VisibilityComponent {
    bool visible = true;
    bool castShadows = true;
    bool receiveShadows = true;
};

struct CameraComponent {
    float fieldOfViewDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 10000.0f;
    bool primary = false;
};

struct DirectionalLightComponent {
    glm::vec3 color{ 1.0f };
    glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
    float intensity = 1.0f;
    bool castShadows = true;
};

struct PointLightComponent {
    glm::vec3 color{ 1.0f };
    float intensity = 1.0f;
    float range = 10.0f;
};

struct SpotLightComponent {
    glm::vec3 color{ 1.0f };
    glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
    float intensity = 1.0f;
    float range = 10.0f;
    float innerConeDegrees = 20.0f;
    float outerConeDegrees = 30.0f;
};

struct StaticMeshComponent {
    std::uint64_t meshAssetId = 0;
    std::string meshPath;
};

struct MaterialComponent {
    std::uint64_t materialAssetId = 0;
    std::string materialPath;
    glm::vec4 color{ 1.0f };
};

struct SkyAtmosphereComponent {
    bool enabled = true;
    float mieScale = 1.0f;
    float rayleighScale = 1.0f;
};

struct VolumetricCloudComponent {
    bool enabled = true;
    float coverage = 0.55f;
    float density = 1.15f;
    float bottomAltitude = 900.0f;
    float topAltitude = 1600.0f;
};

struct TerrainComponent {
    bool enabled = true;
    std::uint64_t landscapeId = 0;
    int resolutionX = 1009;
    int resolutionY = 1009;
};

struct WaterComponent {
    bool enabled = true;
    float level = 0.0f;
};

struct ColliderComponent {
    enum class Shape : std::uint8_t { Box, Sphere, Capsule, Mesh };
    Shape shape = Shape::Box;
    glm::vec3 size{ 1.0f };
    bool isTrigger = false;
};

struct RigidBodyComponent {
    float mass = 1.0f;
    bool kinematic = false;
    bool useGravity = true;
};

struct AudioSourceComponent {
    std::string clipPath;
    float volume = 1.0f;
    bool looping = false;
    bool spatial = true;
};

struct ScriptComponent {
    std::string scriptPath;
    std::uint64_t instanceId = 0;
};

struct AnimationComponent {
    std::string clipPath;
    float time = 0.0f;
    float speed = 1.0f;
    bool playing = true;
    bool looping = true;
};

struct LODComponent {
    float screenSizeBias = 1.0f;
    int forcedLod = -1;
};

struct LegacyActorComponent {
    int entityType = 0;
    bool editorOnly = false;
    int mode = 0;
};

WE_ECS_DECLARE_COMPONENT(NameComponent, Name);
WE_ECS_DECLARE_COMPONENT(TagComponent, Tag);
WE_ECS_DECLARE_COMPONENT(UuidComponent, Uuid);
WE_ECS_DECLARE_COMPONENT(TransformComponent, Transform);
WE_ECS_DECLARE_COMPONENT(HierarchyComponent, Hierarchy);
WE_ECS_DECLARE_COMPONENT(VisibilityComponent, Visibility);
WE_ECS_DECLARE_COMPONENT(CameraComponent, Camera);
WE_ECS_DECLARE_COMPONENT(DirectionalLightComponent, DirectionalLight);
WE_ECS_DECLARE_COMPONENT(PointLightComponent, PointLight);
WE_ECS_DECLARE_COMPONENT(SpotLightComponent, SpotLight);
WE_ECS_DECLARE_COMPONENT(StaticMeshComponent, StaticMesh);
WE_ECS_DECLARE_COMPONENT(MaterialComponent, Material);
WE_ECS_DECLARE_COMPONENT(SkyAtmosphereComponent, SkyAtmosphere);
WE_ECS_DECLARE_COMPONENT(VolumetricCloudComponent, VolumetricCloud);
WE_ECS_DECLARE_COMPONENT(TerrainComponent, Terrain);
WE_ECS_DECLARE_COMPONENT(WaterComponent, Water);
WE_ECS_DECLARE_COMPONENT(ColliderComponent, Collider);
WE_ECS_DECLARE_COMPONENT(RigidBodyComponent, RigidBody);
WE_ECS_DECLARE_COMPONENT(AudioSourceComponent, AudioSource);
WE_ECS_DECLARE_COMPONENT(ScriptComponent, Script);
WE_ECS_DECLARE_COMPONENT(AnimationComponent, Animation);
WE_ECS_DECLARE_COMPONENT(LODComponent, LOD);
WE_ECS_DECLARE_COMPONENT(LegacyActorComponent, LegacyActor);

} // namespace we::runtime::ecs

#pragma warning(pop)
