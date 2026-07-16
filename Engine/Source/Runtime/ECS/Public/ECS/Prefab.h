#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Export.h"
#include "ECS/World.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::ecs {

using PrefabId = std::uint32_t;
inline constexpr PrefabId kInvalidPrefabId = 0;

// Captured archetype + component blobs for instantiation.
struct PrefabEntityNode {
    Entity sourceEntity{};
    Entity parent{};
    ComponentMask mask{};
    std::vector<std::pair<ComponentTypeId, std::vector<std::uint8_t>>> componentBlobs;
};

struct PrefabAsset {
    PrefabId id = kInvalidPrefabId;
    std::string name;
    std::vector<PrefabEntityNode> entities;
};

class ECS_API PrefabRegistry {
public:
    static PrefabRegistry& Get();

    PrefabId Register(const std::string& name, const PrefabAsset& asset);
    [[nodiscard]] const PrefabAsset* Find(PrefabId id) const;
    PrefabAsset CaptureFromWorld(const World& world, Entity root, const std::string& name);

private:
    PrefabRegistry() = default;
    PrefabId m_NextId = 1;
    std::unordered_map<PrefabId, PrefabAsset> m_Prefabs;
};

ECS_API Entity InstantiatePrefab(World& world, PrefabId prefabId, Entity parent = {});

} // namespace we::runtime::ecs

#pragma warning(pop)
