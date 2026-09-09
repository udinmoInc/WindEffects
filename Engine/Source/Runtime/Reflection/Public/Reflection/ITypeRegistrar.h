// ==============================================================================
// WindEffects — Reflection — ITypeRegistrar
// Public API surface for the Reflection module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Reflection/Export.h"
#include "Reflection/ITypeRegistry.h"

namespace we::runtime::reflection {

/// Plugin / module extension point for registering reflected types.
/// External modules implement this and call RegisterTypes during their StartupModule.
class REFLECTION_API ITypeRegistrar {
public:
    virtual ~ITypeRegistrar() = default;
    virtual void RegisterTypes(ITypeRegistry& registry) = 0;
    virtual void UnregisterTypes(ITypeRegistry& registry) = 0;
};

/// Applies a registrar against the process-wide registry.
REFLECTION_API void ApplyTypeRegistrar(ITypeRegistrar& registrar);
REFLECTION_API void RemoveTypeRegistrar(ITypeRegistrar& registrar);

} // namespace we::runtime::reflection
