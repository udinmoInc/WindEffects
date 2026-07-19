#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/TerrainHeightmap.h"

namespace we::runtime::terrain {

/// CPU sculpt / paint / erosion brushes. Regenerates only dirty chunk bounds.
class TERRAIN_API TerrainBrushSystem {
public:
    void SetSettings(const TerrainBrushSettings& settings) { m_Settings = settings; }
    const TerrainBrushSettings& Settings() const { return m_Settings; }

    /// Apply brush at heightfield sample coordinates. Returns true if any sample changed.
    bool Apply(TerrainHeightmap& heightmap, float centerX, float centerZ);

    bool RaiseLower(TerrainHeightmap& map, float cx, float cz, bool raise);
    bool Smooth(TerrainHeightmap& map, float cx, float cz);
    bool Flatten(TerrainHeightmap& map, float cx, float cz);
    bool Noise(TerrainHeightmap& map, float cx, float cz);
    bool Ramp(TerrainHeightmap& map, float cx, float cz);
    bool Terrace(TerrainHeightmap& map, float cx, float cz);
    bool CustomAlpha(TerrainHeightmap& map, float cx, float cz);
    bool HydraulicErosion(TerrainHeightmap& map, float cx, float cz);
    bool ThermalErosion(TerrainHeightmap& map, float cx, float cz);

    void SetCustomAlpha(const std::uint8_t* mask, int width, int height);

private:
    float FalloffWeight(float dist, float radius) const;
    float AlphaWeight(float localX, float localZ, float radius) const;

    TerrainBrushSettings m_Settings{};
};

} // namespace we::runtime::terrain

