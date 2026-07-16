#pragma once

namespace we::runtime::ecs {

template <typename T>
void CommandBuffer::AddComponent(Entity entity, const T& value) {
    StructuralCommand cmd{};
    cmd.type = StructuralChangeType::AddComponent;
    cmd.entity = entity;
    cmd.componentType = ComponentTypeRegistry::Get().Id<T>();
    cmd.payload.resize(sizeof(T));
    std::memcpy(cmd.payload.data(), &value, sizeof(T));
    m_Commands.push_back(std::move(cmd));
}

template <typename T>
void CommandBuffer::RemoveComponent(Entity entity) {
    StructuralCommand cmd{};
    cmd.type = StructuralChangeType::RemoveComponent;
    cmd.entity = entity;
    cmd.componentType = ComponentTypeRegistry::Get().Id<T>();
    m_Commands.push_back(std::move(cmd));
}

template <typename T>
void CommandBuffer::SetComponentEnabled(Entity entity, bool enabled) {
    StructuralCommand cmd{};
    cmd.type = StructuralChangeType::SetComponentEnabled;
    cmd.entity = entity;
    cmd.componentType = ComponentTypeRegistry::Get().Id<T>();
    cmd.boolValue = enabled;
    m_Commands.push_back(std::move(cmd));
}

} // namespace we::runtime::ecs
