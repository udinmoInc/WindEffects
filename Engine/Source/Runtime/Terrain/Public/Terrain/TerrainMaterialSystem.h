#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Core/Math/Types.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace we::runtime::terrain {

struct TerrainLayerDesc {
    std::string name = "Layer";
    std::string albedoPath;
    float tiling = 8.0f;
    float roughness = kDefaultLandscapeRoughness;
    float metallic = kDefaultLandscapeMetallic;
    we::math::Vec4 baseColor{
        kDefaultLandscapeAlbedoR,
        kDefaultLandscapeAlbedoG,
        kDefaultLandscapeAlbedoB,
        1.0f};
};

struct TerrainMaterialParams {
    we::math::Vec4 albedo{
        kDefaultLandscapeAlbedoR,
        kDefaultLandscapeAlbedoG,
        kDefaultLandscapeAlbedoB,
        1.0f};
    float roughness = kDefaultLandscapeRoughness;
    float metallic = kDefaultLandscapeMetallic;
    /// World-space grid (meters). Zero opacity disables the grid (project materials).
    float gridSpacing = kDefaultLandscapeGridSpacing;
    float gridLineWidth = kDefaultLandscapeGridLineWidth;
    we::math::Vec3 gridColor{
        kDefaultLandscapeGridColorR,
        kDefaultLandscapeGridColorG,
        kDefaultLandscapeGridColorB};
    float gridOpacity = kDefaultLandscapeGridOpacity;
    float gridFadeStart = kDefaultLandscapeGridFadeStart;
    float gridFadeEnd = kDefaultLandscapeGridFadeEnd;
    std::string materialPath = kDefaultLandscapeMaterialPath;
    bool loadedFromEngineContent = false;
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

    /// Bind Engine/Content M_DefaultLandscapeEditor when no project material is assigned.
    void EnsureDefaultLandscapeMaterial(TerrainCreateInfo& createInfo);
    /// Resolve materialSlot0 (engine or project) into GPU-ready shading params.
    [[nodiscard]] TerrainMaterialParams ResolveShadingParams(
        const TerrainCreateInfo& createInfo) const;
    [[nodiscard]] const TerrainMaterialParams& ActiveParams() const noexcept {
        return m_ActiveParams;
    }
    void SetActiveParams(const TerrainMaterialParams& params) { m_ActiveParams = params; }

    // Future hooks (no-ops today) — keep ABI ready for VT / streaming.
    void NotifyVirtualTexturePagesDirty(int /*minX*/, int /*minZ*/, int /*maxX*/, int /*maxZ*/) {}
    bool WantsVirtualTexturing() const { return m_UseVirtualTexturing; }
    void SetVirtualTexturingEnabled(bool enabled) { m_UseVirtualTexturing = enabled; }

private:
    [[nodiscard]] static bool TryLoadWemat(
        const std::filesystem::path& path,
        TerrainMaterialParams& outParams);

    int m_Width = 0;
    int m_Height = 0;
    std::vector<std::uint8_t> m_Weights; // RGBA8, row-major
    std::array<TerrainLayerDesc, kMaxPaintLayers> m_Layers{};
    TerrainMaterialParams m_ActiveParams{};
    bool m_UseVirtualTexturing = false;
};

} // namespace we::runtime::terrain

