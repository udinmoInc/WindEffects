#include "ECS/ComponentType.h"

namespace we::runtime::ecs {

ComponentTypeRegistry::ComponentTypeRegistry() {
    m_Types.resize(static_cast<std::size_t>(CoreComponentId::Count));
}

ComponentTypeRegistry& ComponentTypeRegistry::Get() {
    static ComponentTypeRegistry instance;
    return instance;
}

ComponentTypeId ComponentTypeRegistry::RegisterType(
    ComponentTypeId id,
    const char* name,
    std::size_t size,
    std::size_t alignment,
    ComponentCategory category,
    bool enableable)
{
    if (id >= kMaxComponentTypes) {
        return kInvalidComponentType;
    }
    if (id >= m_Types.size()) {
        m_Types.resize(static_cast<std::size_t>(id) + 1u);
    }
    ComponentTypeInfo& info = m_Types[id];
    if (info.id == kInvalidComponentType) {
        info.id = id;
        info.name = name ? name : "Component";
        info.size = size;
        info.alignment = alignment;
        info.category = category;
        info.enableable = enableable;
        info.mask.Set(id);
    }
    return id;
}

ComponentTypeId ComponentTypeRegistry::RegisterDynamic(
    const char* name,
    std::size_t size,
    std::size_t alignment,
    ComponentCategory category,
    bool enableable)
{
    if (m_NextDynamicId >= kMaxComponentTypes) {
        return kInvalidComponentType;
    }
    return RegisterType(m_NextDynamicId++, name, size, alignment, category, enableable);
}

const ComponentTypeInfo* ComponentTypeRegistry::Find(ComponentTypeId id) const {
    if (id >= m_Types.size()) {
        return nullptr;
    }
    if (m_Types[id].id == kInvalidComponentType) {
        return nullptr;
    }
    return &m_Types[id];
}

} // namespace we::runtime::ecs
