// ==============================================================================
// WindEffects — Terrain — TerrainFoliage
// Public API surface for the Terrain module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/TerrainCollision.h"

#include <vector>

namespace we::runtime::terrain {

class TerrainHeightmap;
class TerrainMaterialSystem;

class TERRAIN_API TerrainFoliageSystem {
public:
    void Clear();
    const std::vector<FoliageInstance>& Instances() const { return m_Instances; }

    // Procedural placement filtered by slope/height and optional layer weight.
    int Spawn(const TerrainCollision& collision, const TerrainCreateInfo& info,
        const TerrainMaterialSystem* materials, const FoliageSpawnParams& params,
        const we::math::Vec2& regionMinLocal, const we::math::Vec2& regionMaxLocal);

private:
    std::vector<FoliageInstance> m_Instances;
};

} // namespace we::runtime::terrain

