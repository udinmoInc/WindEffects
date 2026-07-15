#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/TerrainHeightmap.h"

namespace we::runtime::terrain {

struct TerrainBrushSettings {
    TerrainBrushOp op = TerrainBrushOp::Raise;
    float radius = 16.0f;       // heightmap samples
    float strength = 0.35f;     // 0..1
    float falloff = 0.5f;       // soft edge
    float targetHeight = 0.5f;  // normalized flatten target
    float noiseScale = 0.15f;
    int   erosionIterations = 8;
};

// CPU sculpt / erosion brushes. Regenerates only dirty chunk bounds via Heightmap dirty rect.
class TERRAIN_API TerrainBrushSystem {
public:
    void SetSettings(const TerrainBrushSettings& settings) { m_Settings = settings; }
    const TerrainBrushSettings& Settings() const { return m_Settings; }

    // Apply brush at heightmap sample coordinates. Returns true if any sample changed.
    bool Apply(TerrainHeightmap& heightmap, float centerX, float centerZ);

    bool RaiseLower(TerrainHeightmap& map, float cx, float cz, bool raise);
    bool Smooth(TerrainHeightmap& map, float cx, float cz);
    bool Flatten(TerrainHeightmap& map, float cx, float cz);
    bool Noise(TerrainHeightmap& map, float cx, float cz);
    bool HydraulicErosion(TerrainHeightmap& map, float cx, float cz);
    bool ThermalErosion(TerrainHeightmap& map, float cx, float cz);

private:
    float FalloffWeight(float dist, float radius) const;

    TerrainBrushSettings m_Settings{};
};

} // namespace we::runtime::terrain

#pragma warning(pop)
