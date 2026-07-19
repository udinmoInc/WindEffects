#pragma once

#include "World/Export.h"
#include "World/WorldGuid.h"

#include <cstdint>
#include <string>

namespace we::runtime::world {

/// Opaque generational handles — indices into runtime registries (not ECS entity IDs).
struct WORLD_API WorldHandle {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return generation != 0; }
    [[nodiscard]] constexpr bool operator==(const WorldHandle& o) const noexcept {
        return index == o.index && generation == o.generation;
    }
    [[nodiscard]] constexpr bool operator!=(const WorldHandle& o) const noexcept { return !(*this == o); }
};

struct WORLD_API LevelHandle {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return generation != 0; }
    [[nodiscard]] constexpr bool operator==(const LevelHandle& o) const noexcept {
        return index == o.index && generation == o.generation;
    }
    [[nodiscard]] constexpr bool operator!=(const LevelHandle& o) const noexcept { return !(*this == o); }
};

struct WORLD_API ActorHandle {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return generation != 0; }
    [[nodiscard]] constexpr bool operator==(const ActorHandle& o) const noexcept {
        return index == o.index && generation == o.generation;
    }
    [[nodiscard]] constexpr bool operator!=(const ActorHandle& o) const noexcept { return !(*this == o); }
};

struct WORLD_API LayerId {
    std::uint32_t value = 0;
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const LayerId& o) const noexcept { return value == o.value; }
};

struct WORLD_API TagId {
    std::uint64_t value = 0;
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const TagId& o) const noexcept { return value == o.value; }
};

enum class WorldState : std::uint8_t {
    Uninitialized = 0,
    Loading,
    Ready,
    Streaming,
    Saving,
    Suspended,
    Destroying,
    Destroyed,
};

enum class LevelState : std::uint8_t {
    Unloaded = 0,
    Loading,
    Loaded,
    Visible,
    Unloading,
    Failed,
};

enum class ActorLifecycle : std::uint8_t {
    Constructed = 0,
    BeginPlayPending,
    Playing,
    EndPlayPending,
    Destroyed,
};

enum class TickGroup : std::uint8_t {
    PrePhysics = 0,
    DuringPhysics,
    PostPhysics,
    PreUpdate,
    DuringUpdate,
    PostUpdate,
    PreLateUpdate,
    DuringLateUpdate,
    PostLateUpdate,
    Count,
};

enum class EndPlayReason : std::uint8_t {
    Destroyed = 0,
    LevelUnload,
    WorldTeardown,
    Replaced,
};

enum class WorldNetMode : std::uint8_t {
    Standalone = 0,
    DedicatedServer,
    ListenServer,
    Client,
};

enum class SpatialQueryShape : std::uint8_t {
    Point = 0,
    Sphere,
    Box,
    Capsule,
};

/// Serializable world descriptor (Reflection/Serialization owned payload shape).
struct WORLD_API WorldDescriptor {
    WorldGuid guid{};
    char name[64]{};
    std::uint32_t schemaVersion = 1;
    bool persistent = false;
    bool allowStreaming = true;
    bool largeWorldCoordinates = false;
    WorldNetMode netMode = WorldNetMode::Standalone;
};

struct WORLD_API LevelDescriptor {
    WorldGuid guid{};
    char name[64]{};
    WorldGuid worldGuid{};
    std::uint32_t schemaVersion = 1;
    bool persistent = false;
    bool streamable = true;
    float boundsMin[3]{0.f, 0.f, 0.f};
    float boundsMax[3]{0.f, 0.f, 0.f};
};

struct WORLD_API ActorSpawnParams {
    WorldGuid guid{};
    char name[64]{};
    ActorHandle parent{};
    LayerId layer{};
    TagId tags{};
    float localPosition[3]{0.f, 0.f, 0.f};
    float localRotation[3]{0.f, 0.f, 0.f};
    float localScale[3]{1.f, 1.f, 1.f};
    bool beginPlayImmediately = true;
};

struct WORLD_API WorldTickParams {
    float deltaSeconds = 0.f;
    float fixedDeltaSeconds = 1.f / 60.f;
    double worldTimeSeconds = 0.0;
    std::uint64_t frameIndex = 0;
    bool runFixedUpdate = true;
    bool runBeginPlay = true;
};

struct WORLD_API Vec3f {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

struct WORLD_API Aabb3f {
    Vec3f min{};
    Vec3f max{};
};

struct WORLD_API Sphere3f {
    Vec3f center{};
    float radius = 0.f;
};

} // namespace we::runtime::world
