// ==============================================================================
// WindEffects — ECS — Reflection
// Internal implementation for the ECS module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "ECS/Reflection.h"
#include "ECS/ComponentOps.h"

namespace we::runtime::ecs {

ReflectionRegistry& ReflectionRegistry::Get() {
    static ReflectionRegistry instance;
    return instance;
}

void ReflectionRegistry::RegisterComponent(const ComponentReflection& reflection) {
    if (reflection.typeId >= m_Components.size()) {
        m_Components.resize(static_cast<std::size_t>(reflection.typeId) + 1u);
    }
    m_Components[reflection.typeId] = reflection;
}

const ComponentReflection* ReflectionRegistry::Find(ComponentTypeId typeId) const {
    if (typeId >= m_Components.size()) {
        return nullptr;
    }
    return m_Components[typeId].typeId == kInvalidComponentType ? nullptr : &m_Components[typeId];
}

std::vector<std::uint8_t> ReflectionRegistry::SerializeComponent(
    ComponentTypeId typeId,
    const void* componentData) const
{
    std::vector<std::uint8_t> bytes;
    const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(typeId);
    if (!info || !componentData) {
        return bytes;
    }
    bytes.resize(info->size);
    if (const ComponentOps* ops = FindComponentOps(typeId)) {
        if (ops->copy) {
            ops->copy(bytes.data(), componentData, info->size);
        }
    } else {
        std::memcpy(bytes.data(), componentData, info->size);
    }
    return bytes;
}

bool ReflectionRegistry::DeserializeComponent(
    ComponentTypeId typeId,
    void* componentData,
    const std::uint8_t* bytes,
    std::size_t byteCount) const
{
    const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(typeId);
    if (!info || !componentData || !bytes || byteCount < info->size) {
        return false;
    }
    if (const ComponentOps* ops = FindComponentOps(typeId)) {
        if (ops->copy) {
            ops->copy(componentData, bytes, info->size);
        }
    } else {
        std::memcpy(componentData, bytes, info->size);
    }
    return true;
}

} // namespace we::runtime::ecs
