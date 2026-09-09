// ==============================================================================
// WindEffects — Terrain — TerrainReflection
// Public API surface for the Terrain module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Terrain/Export.h"

namespace we::runtime::reflection {
class ITypeRegistry;
}

namespace we::runtime::terrain {

/// Registers TerrainCreateInfo / brush settings for Property Editor + Undo diffs.
TERRAIN_API void RegisterTerrainReflection(reflection::ITypeRegistry& registry);

} // namespace we::runtime::terrain
