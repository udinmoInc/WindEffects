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

