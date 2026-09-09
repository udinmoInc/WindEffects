// ==============================================================================
// WindEffects — World — WorldReflection
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
