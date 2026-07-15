#include "ECS/Registry.h"
#include "ECS/Components/CoreComponents.h"

namespace we::runtime::ecs {

Registry::Registry() {
    // Index 0 reserved for null Entity.
    m_Entities.push_back(EntityMeta{});
}

Registry::~Registry() {
    Clear();
}

Entity Registry::Create() {
    std::uint32_t index = 0;
    std::uint32_t generation = 1;
    if (!m_FreeList.empty()) {
        index = m_FreeList.back();
        m_FreeList.pop_back();
        generation = m_Entities[index].generation;
        m_Entities[index].alive = true;
        m_Entities[index].entity = Entity(index, generation);
    } else {
        index = static_cast<std::uint32_t>(m_Entities.size());
        generation = 1;
        EntityMeta meta{};
        meta.alive = true;
        meta.generation = generation;
        meta.entity = Entity(index, generation);
        m_Entities.push_back(meta);
    }
    ++m_Living;
    NotifyChanged();

    const Entity entity = m_Entities[index].entity;
    Add<UuidComponent>(entity, UuidComponent{ Uuid::Generate() });
    Add<TransformComponent>(entity);
    Add<HierarchyComponent>(entity);
    Add<VisibilityComponent>(entity);
    return entity;
}

Entity Registry::Create(const std::string& name) {
    Entity entity = Create();
    Add<NameComponent>(entity, NameComponent{ name });
    return entity;
}

void Registry::Destroy(Entity entity) {
    if (!Valid(entity)) {
        return;
    }
    const std::uint32_t index = entity.Index();

    // Remove from all component pools.
    for (auto& pool : m_Pools) {
        if (pool) {
            pool->Remove(entity);
        }
    }

    EntityMeta& meta = m_Entities[index];
    meta.alive = false;
    meta.generation += 1;
    if (meta.generation == 0) {
        meta.generation = 1; // never wrap to 0 with live confusion
    }
    meta.entity = Entity(index, meta.generation);
    m_FreeList.push_back(index);
    if (m_Living > 0) {
        --m_Living;
    }
    NotifyChanged();
}

bool Registry::Valid(Entity entity) const {
    if (entity.IsNull()) {
        return false;
    }
    const std::uint32_t index = entity.Index();
    if (index == 0 || index >= m_Entities.size()) {
        return false;
    }
    const EntityMeta& meta = m_Entities[index];
    return meta.alive && meta.generation == entity.Generation();
}

void Registry::Clear() {
    for (auto& pool : m_Pools) {
        if (pool) {
            pool->Clear();
        }
    }
    m_Entities.clear();
    m_Entities.push_back(EntityMeta{});
    m_FreeList.clear();
    m_Living = 0;
    NotifyChanged();
}

IComponentPool* Registry::PoolById(ComponentTypeId id) {
    if (id >= m_Pools.size()) {
        return nullptr;
    }
    return m_Pools[id].get();
}

const IComponentPool* Registry::PoolById(ComponentTypeId id) const {
    if (id >= m_Pools.size()) {
        return nullptr;
    }
    return m_Pools[id].get();
}

void Registry::ForEachLiving(const std::function<void(Entity)>& fn) const {
    for (std::size_t i = 1; i < m_Entities.size(); ++i) {
        if (m_Entities[i].alive) {
            fn(Entity(static_cast<std::uint32_t>(i), m_Entities[i].generation));
        }
    }
}

void Registry::EnsurePoolCapacity(ComponentTypeId id) {
    if (id >= m_Pools.size()) {
        m_Pools.resize(static_cast<std::size_t>(id) + 1u);
    }
}

} // namespace we::runtime::ecs
