// ==============================================================================
// WindEffects — ECS — Registry
// Internal implementation for the ECS module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "ECS/Registry.h"

// Rebuild after World pimpl (accessors moved out of header).

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
