#pragma once

#include "ECS/Export.h"

#include <cstddef>
#include <cstdint>

namespace we::runtime::ecs {

// Chunk size target for SoA storage (16–64 KB). Default 64 KB for AAA scale.
inline constexpr std::size_t kDefaultChunkSizeBytes = 64u * 1024u;
inline constexpr std::size_t kMinChunkSizeBytes = 16u * 1024u;
inline constexpr std::size_t kMaxChunkSizeBytes = 64u * 1024u;

// Maximum component types addressable by the archetype mask.
inline constexpr std::uint32_t kMaxComponentTypes = 256u;
inline constexpr std::uint32_t kComponentMaskWords = kMaxComponentTypes / 64u;

inline constexpr std::uint32_t kMaxArchetypes = 65536u;
inline constexpr std::uint32_t kInvalidArchetypeIndex = 0xFFFFFFFFu;
inline constexpr std::uint32_t kInvalidChunkIndex = 0xFFFFFFFFu;
inline constexpr std::uint16_t kInvalidChunkSlot = 0xFFFFu;

inline constexpr std::uint32_t kMaxJobThreads = 64u;
inline constexpr std::uint32_t kDefaultJobThreads = 0u; // 0 = auto (hardware concurrency - 1)

enum class ComponentCategory : std::uint8_t {
    Data = 0,
    Tag,        // Zero-size marker; affects archetype only
    Singleton,  // One instance per world
    Shared,     // Shared instance across entity batches
    Buffer      // Dynamic growable buffer column
};

enum class EntityFlags : std::uint16_t {
    None = 0,
    Enabled = 1u << 0,
    Destroyed = 1u << 1
};

[[nodiscard]] inline EntityFlags operator|(EntityFlags a, EntityFlags b) {
    return static_cast<EntityFlags>(static_cast<std::uint16_t>(a) | static_cast<std::uint16_t>(b));
}
[[nodiscard]] inline EntityFlags operator&(EntityFlags a, EntityFlags b) {
    return static_cast<EntityFlags>(static_cast<std::uint16_t>(a) & static_cast<std::uint16_t>(b));
}
[[nodiscard]] inline bool HasFlag(EntityFlags mask, EntityFlags bit) {
    return (static_cast<std::uint16_t>(mask) & static_cast<std::uint16_t>(bit)) != 0;
}

enum class AccessMode : std::uint8_t {
    Read = 0,
    Write
};

enum class StructuralChangeType : std::uint8_t {
    CreateEntity = 0,
    DestroyEntity,
    AddComponent,
    RemoveComponent,
    SetEntityEnabled,
    SetComponentEnabled,
    SetParent,
    InstantiatePrefab
};

} // namespace we::runtime::ecs
