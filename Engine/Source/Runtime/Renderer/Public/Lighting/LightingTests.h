// ==============================================================================
// WindEffects — Renderer — LightingTests
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Renderer/Export.h"
#include "Renderer/Scalability/ScalabilityTests.h"

#pragma warning(push)
#pragma warning(disable : 4251)

namespace we::runtime::renderer {

/// Reuses ScalabilityTestReport shape for the lighting hardening harness.
[[nodiscard]] RENDERER_API ScalabilityTestReport RunLightingRuntimeTests();

} // namespace we::runtime::renderer

#pragma warning(pop)
