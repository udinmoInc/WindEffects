#include "ECS/Registry.h"

namespace we::runtime::ecs {

Registry::Registry() = default;

Registry::~Registry() {
    Clear();
}

Entity Registry::Create() {
    return m_World.CreateEntity();
}

Entity Registry::Create(const std::string& name) {
    return m_World.CreateEntity(name);
}

void Registry::Destroy(Entity entity) {
    m_World.DestroyEntity(entity);
}

bool Registry::Valid(Entity entity) const {
    return m_World.Valid(entity);
}

void Registry::Clear() {
    m_World.Clear();
}

void Registry::ForEachLiving(const std::function<void(Entity)>& fn) const {
    m_World.Entities().ForEachLiving(fn);
}

} // namespace we::runtime::ecs
