// ==============================================================================
// WindEffects — Reflection — TypeRegistrar
// Internal implementation for the Reflection module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Reflection/ITypeRegistrar.h"

namespace we::runtime::reflection {

void ApplyTypeRegistrar(ITypeRegistrar& registrar) {
    registrar.RegisterTypes(GetTypeRegistry());
}

void RemoveTypeRegistrar(ITypeRegistrar& registrar) {
    registrar.UnregisterTypes(GetTypeRegistry());
}

} // namespace we::runtime::reflection
