#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::terrain {

struct TerrainLayerDesc {
    std::string name = "Layer";
    std::string albedoPath;
    float tiling = 8.0f;
    float roughness = 0.85f;
};

// RGBA weight maps at heightmap resolution. Architecture ready for virtual texturing
// (virtual page table + physical cache) without changing this interface.
class TERRAIN_API TerrainMaterialSystem {
public:
    void Initialize(int width, int height);
    void Shutdown();

    int Width() const { return m_Width; }
    int Height() const { return m_Height; }

    std::uint8_t* WeightData() { return m_Weights.data(); }
    const std::uint8_t* WeightData() const { return m_Weights.data(); }

    TerrainLayerDesc& Layer(int index);
    const TerrainLayerDesc& Layer(int index) const;

    void PaintWeight(int x, int z, int layerIndex, float strength, float radius);

    // Future hooks (no-ops today) — keep ABI ready for VT / streaming.
    void NotifyVirtualTexturePagesDirty(int /*minX*/, int /*minZ*/, int /*maxX*/, int /*maxZ*/) {}
    bool WantsVirtualTexturing() const { return m_UseVirtualTexturing; }
    void SetVirtualTexturingEnabled(bool enabled) { m_UseVirtualTexturing = enabled; }

private:
    int m_Width = 0;
    int m_Height = 0;
    std::vector<std::uint8_t> m_Weights; // RGBA8, row-major
    std::array<TerrainLayerDesc, kMaxPaintLayers> m_Layers{};
    bool m_UseVirtualTexturing = false;
};

} // namespace we::runtime::terrain

#pragma warning(pop)
