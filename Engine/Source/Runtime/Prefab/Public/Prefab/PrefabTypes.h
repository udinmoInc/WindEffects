#pragma once

#include "Prefab/Export.h"
#include "Core/Math/Types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::prefab {

/// Stable prefab asset identity (maps to AssetGuid / WorldGuid bytes).
struct PREFAB_API PrefabGuid {
    std::uint64_t hi = 0;
    std::uint64_t lo = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return hi != 0 || lo != 0; }
    [[nodiscard]] constexpr bool operator==(const PrefabGuid& o) const noexcept {
        return hi == o.hi && lo == o.lo;
    }
    [[nodiscard]] constexpr bool operator!=(const PrefabGuid& o) const noexcept {
        return !(*this == o);
    }
};

struct PREFAB_API PrefabGuidHash {
    [[nodiscard]] std::size_t operator()(const PrefabGuid& g) const noexcept {
        return static_cast<std::size_t>(g.hi ^ (g.lo * 0x9e3779b97f4a7c15ull));
    }
};

struct PREFAB_API PrefabInstanceId {
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const PrefabInstanceId& o) const noexcept {
        return value == o.value;
    }
};

struct PREFAB_API PrefabInstanceIdHash {
    [[nodiscard]] std::size_t operator()(const PrefabInstanceId& id) const noexcept {
        return static_cast<std::size_t>(id.value);
    }
};

enum class PrefabOverrideKind : std::uint8_t {
    Property = 0,
    ComponentAdded,
    ComponentRemoved,
    ChildAdded,
    ChildRemoved,
    Transform,
    Custom,
};

enum class PrefabLinkState : std::uint8_t {
    Linked = 0,
    Broken,
    MissingAsset,
    Updating,
};

enum class PrefabEventKind : std::uint8_t {
    AssetRegistered = 0,
    AssetUnregistered,
    InstanceSpawned,
    InstanceDestroyed,
    OverrideChanged,
    AppliedToAsset,
    Reverted,
    LinkBroken,
    LinkRestored,
    ValidationFailed,
};

struct PREFAB_API PrefabNodeTemplate {
    std::string name;
    std::string typeName; // Scene EntityType name or actor class
    we::math::Vec3 position{};
    we::math::Vec3 rotation{};
    we::math::Vec3 scale{1.f, 1.f, 1.f};
    std::vector<std::uint8_t> propertySnapshot; // Serialization snapshot bytes
    std::vector<PrefabNodeTemplate> children;
};

struct PREFAB_API PrefabSpawnParams {
    PrefabGuid prefab{};
    we::math::Vec3 position{};
    we::math::Vec3 rotation{};
    we::math::Vec3 scale{1.f, 1.f, 1.f};
    std::string instanceName;
    std::uint64_t parentEntityId = 0; // Scene ParentId near-term
};

struct PREFAB_API PrefabConfig {
    bool allowNestedPrefabs = true;
    bool detectCircularDependencies = true;
    bool autoUpdateInstancesOnApply = true;
    std::uint32_t maxNestingDepth = 64;
};

struct PREFAB_API PrefabValidationIssue {
    enum class Severity : std::uint8_t { Info, Warning, Error } severity = Severity::Error;
    std::string message;
    PrefabGuid asset{};
    PrefabInstanceId instance{};
};

} // namespace we::runtime::prefab
