#include "ECS/Prefab.h"

#include "ECS/ComponentOps.h"
#include "ECS/Components/CoreComponents.h"
#include "ECS/Reflection.h"

#include <cstring>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace we::runtime::ecs {

namespace {

PrefabEntityNode CaptureEntityNode(const World& world, Entity entity) {
    PrefabEntityNode node{};
    node.sourceEntity = entity;

    if (const HierarchyComponent* hierarchy = world.TryGet<HierarchyComponent>(entity)) {
        node.parent = hierarchy->parent;
    }

    const ArchetypeLayout* archetype = world.Archetypes().Get(
        world.Entities().Location(entity).archetypeIndex);
    if (!archetype) {
        return node;
    }

    node.mask = archetype->mask;
    ChunkView view = world.GetChunkView(entity);
    if (!view.Valid()) {
        return node;
    }

    const EntityLocation& loc = world.Entities().Location(entity);
    const ReflectionRegistry& reflection = ReflectionRegistry::Get();

    for (ComponentTypeId typeId : archetype->typeIds) {
        const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(typeId);
        if (!info || info->size == 0) {
            continue;
        }
        const void* column = view.ColumnPtr(typeId);
        if (!column) {
            continue;
        }
        const void* slotPtr = static_cast<const std::uint8_t*>(column) + info->size * loc.slot;
        std::vector<std::uint8_t> blob = reflection.SerializeComponent(typeId, slotPtr);
        if (blob.empty()) {
            blob.resize(info->size);
            if (const ComponentOps* ops = FindComponentOps(typeId); ops && ops->copy) {
                ops->copy(blob.data(), slotPtr, info->size);
            } else {
                std::memcpy(blob.data(), slotPtr, info->size);
            }
        }
        node.componentBlobs.emplace_back(typeId, std::move(blob));
    }
    return node;
}

} // namespace

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
    if (!world.Valid(root)) {
        return asset;
    }

    std::unordered_set<std::uint64_t> visited;
    std::queue<Entity> queue;
    queue.push(root);

    while (!queue.empty()) {
        Entity entity = queue.front();
        queue.pop();
        if (!world.Valid(entity)) {
            continue;
        }
        if (!visited.insert(entity.id).second) {
            continue;
        }

        asset.entities.push_back(CaptureEntityNode(world, entity));

        if (const HierarchyComponent* hierarchy = world.TryGet<HierarchyComponent>(entity)) {
            for (Entity child = hierarchy->firstChild; child; ) {
                queue.push(child);
                const HierarchyComponent* childHier = world.TryGet<HierarchyComponent>(child);
                child = childHier ? childHier->nextSibling : Entity{};
            }
        }
    }
    return asset;
}

Entity InstantiatePrefab(World& world, PrefabId prefabId, Entity parent) {
    const PrefabAsset* asset = PrefabRegistry::Get().Find(prefabId);
    if (!asset || asset->entities.empty()) {
        return kNullEntity;
    }

    std::unordered_map<std::uint64_t, Entity> remap;
    Entity rootCreated{};

    for (const PrefabEntityNode& node : asset->entities) {
        Entity entity = world.CreateEntity();
        remap[node.sourceEntity.id] = entity;
        if (!rootCreated) {
            rootCreated = entity;
        }

        for (const auto& blob : node.componentBlobs) {
            const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(blob.first);
            if (!info) {
                continue;
            }
            world.MigrateAddComponent(entity, blob.first, blob.second.data());
        }
    }

    for (const PrefabEntityNode& node : asset->entities) {
        Entity entity = remap[node.sourceEntity.id];
        if (!world.Has<HierarchyComponent>(entity)) {
            continue;
        }
        HierarchyComponent& hierarchy = world.Get<HierarchyComponent>(entity);
        if (node.parent) {
            const auto parentIt = remap.find(node.parent.id);
            hierarchy.parent = parentIt != remap.end() ? parentIt->second : parent;
        } else if (parent) {
            hierarchy.parent = parent;
        }
    }

    return rootCreated;
}

} // namespace we::runtime::ecs
