// ==============================================================================
// WindEffects — Reflection — TypeId
// Internal implementation for the Reflection module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Reflection/TypeId.h"
#include "HashUtils.h"

namespace we::runtime::reflection {

TypeId HashTypeName(std::string_view qualifiedName) noexcept {
    return detail::Fnv1a64(qualifiedName);
}

} // namespace we::runtime::reflection
