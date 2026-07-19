#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include "Reflection/ITypeRegistrar.h"
#include "Reflection/ITypeRegistry.h"

namespace we::runtime::world {

/// Registers World Runtime reflected types (descriptors, spawn params, GUIDs).
class WORLD_API WorldTypeRegistrar final : public reflection::ITypeRegistrar {
public:
    void RegisterTypes(reflection::ITypeRegistry& registry) override;
    void UnregisterTypes(reflection::ITypeRegistry& registry) override;
};

WORLD_API void RegisterWorldReflectionTypes(reflection::ITypeRegistry& registry);

} // namespace we::runtime::world
