// ==============================================================================
// WindEffects — Reflection — ReflectionTests
// Public API surface for the Reflection module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Reflection/ReflectionProductionReport.h"

namespace we::runtime::reflection {

/// Alias for tooling that prefers a Tests-named entry point.
[[nodiscard]] inline ReflectionTestReport RunReflectionHardeningTests() {
    return RunReflectionTests();
}

} // namespace we::runtime::reflection
