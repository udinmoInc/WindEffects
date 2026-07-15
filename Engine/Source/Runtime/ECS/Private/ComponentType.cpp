#include "ECS/ComponentType.h"
#include "ECS/Components/CoreComponents.h"

namespace we::runtime::ecs {

ComponentTypeRegistry::ComponentTypeRegistry() {
    m_Types.resize(static_cast<std::size_t>(CoreComponentId::Count));
}

ComponentTypeRegistry& ComponentTypeRegistry::Get() {
    static ComponentTypeRegistry instance;
    return instance;
}

const ComponentTypeInfo* ComponentTypeRegistry::Find(ComponentTypeId id) const {
    if (id >= m_Types.size()) {
        return nullptr;
    }
    return &m_Types[id];
}

} // namespace we::runtime::ecs
