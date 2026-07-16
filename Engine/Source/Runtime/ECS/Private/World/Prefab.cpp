#include "ECS/Prefab.h"
#include "ECS/ComponentOps.h"
#include "ECS/Components/CoreComponents.h"

namespace we::runtime::ecs {

PrefabRegistry& PrefabRegistry::Get() {
    static PrefabRegistry instance;
    return instance;
}

PrefabId PrefabRegistry::Register(const std::string& name, const PrefabAsset& asset) {
    PrefabAsset stored = asset;
    stored.id = m_NextId++;
    stored.name = name;
    m_Prefabs[stored.id] = std::move(stored);
    return stored.id;
}

const PrefabAsset* PrefabRegistry::Find(PrefabId id) const {
    const auto it = m_Prefabs.find(id);
    return it != m_Prefabs.end() ? &it->second : nullptr;
}

PrefabAsset PrefabRegistry::CaptureFromWorld(const World& world, Entity root, const std::string& name) {
    PrefabAsset asset{};
    asset.name = name;
    (void)world;
    (void)root;
    return asset;
}

Entity InstantiatePrefab(World& world, PrefabId prefabId, Entity parent) {
    const PrefabAsset* asset = PrefabRegistry::Get().Find(prefabId);
    if (!asset) {
        return kNullEntity;
    }
    Entity created{};
    for (const PrefabEntityNode& node : asset->entities) {
        Entity entity = world.CreateEntity();
        if (!created) {
            created = entity;
        }
        for (const auto& blob : node.componentBlobs) {
            world.MigrateAddComponent(entity, blob.first, blob.second.data());
        }
        if (parent && world.Has<HierarchyComponent>(entity)) {
            world.Get<HierarchyComponent>(entity).parent = parent;
        }
        (void)node;
    }
    return created;
}

} // namespace we::runtime::ecs
