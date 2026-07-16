#include "ECS/EntityManager.h"

namespace we::runtime::ecs {

EntityManager::EntityManager() {
    m_Records.push_back({});
}

Entity EntityManager::Create() {
    std::uint32_t index = 0;
    std::uint32_t generation = 1;
    if (!m_FreeList.empty()) {
        index = m_FreeList.back();
        m_FreeList.pop_back();
        generation = m_Records[index].generation;
        m_Records[index].flags = EntityFlags::Enabled;
        m_Records[index].location = {};
    } else {
        index = static_cast<std::uint32_t>(m_Records.size());
        EntityRecord record{};
        record.generation = generation;
        record.flags = EntityFlags::Enabled;
        m_Records.push_back(record);
    }
    ++m_Living;
    return Entity(index, generation);
}

void EntityManager::Destroy(Entity entity) {
    if (!Valid(entity)) {
        return;
    }
    const std::uint32_t index = entity.Index();
    EntityRecord& record = m_Records[index];
    record.flags = static_cast<EntityFlags>(
        static_cast<std::uint16_t>(record.flags) | static_cast<std::uint16_t>(EntityFlags::Destroyed));
    record.location = {};
    record.generation += 1;
    if (record.generation == 0) {
        record.generation = 1;
    }
    m_FreeList.push_back(index);
    if (m_Living > 0) {
        --m_Living;
    }
}

bool EntityManager::Valid(Entity entity) const {
    if (entity.IsNull()) {
        return false;
    }
    const std::uint32_t index = entity.Index();
    if (index == 0 || index >= m_Records.size()) {
        return false;
    }
    const EntityRecord& record = m_Records[index];
    return record.generation == entity.Generation()
        && !HasFlag(record.flags, EntityFlags::Destroyed);
}

bool EntityManager::IsEnabled(Entity entity) const {
    if (!Valid(entity)) {
        return false;
    }
    return HasFlag(m_Records[entity.Index()].flags, EntityFlags::Enabled);
}

void EntityManager::SetEnabled(Entity entity, bool enabled) {
    if (!Valid(entity)) {
        return;
    }
    EntityRecord& record = m_Records[entity.Index()];
    if (enabled) {
        record.flags = static_cast<EntityFlags>(
            static_cast<std::uint16_t>(record.flags) | static_cast<std::uint16_t>(EntityFlags::Enabled));
    } else {
        record.flags = static_cast<EntityFlags>(
            static_cast<std::uint16_t>(record.flags) & ~static_cast<std::uint16_t>(EntityFlags::Enabled));
    }
}

void EntityManager::Clear() {
    m_Records.clear();
    m_Records.push_back(EntityRecord{});
    m_FreeList.clear();
    m_Living = 0;
}

EntityLocation& EntityManager::Location(Entity entity) {
    return m_Records[entity.Index()].location;
}

const EntityLocation& EntityManager::Location(Entity entity) const {
    return m_Records[entity.Index()].location;
}

EntityRecord& EntityManager::Record(Entity entity) {
    return m_Records[entity.Index()];
}

const EntityRecord& EntityManager::Record(Entity entity) const {
    return m_Records[entity.Index()];
}

void EntityManager::ForEachLiving(const std::function<void(Entity)>& fn) const {
    for (std::uint32_t i = 1; i < static_cast<std::uint32_t>(m_Records.size()); ++i) {
        const EntityRecord& record = m_Records[i];
        if (!HasFlag(record.flags, EntityFlags::Destroyed)) {
            fn(Entity(i, record.generation));
        }
    }
}

} // namespace we::runtime::ecs
