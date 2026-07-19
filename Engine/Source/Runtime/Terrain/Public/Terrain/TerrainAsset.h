#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/ITerrain.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace we::runtime::terrain {

/// Serializable terrain asset description — Landscape exists as a normal world asset.
struct TERRAIN_API TerrainAssetDesc {
    std::string assetPath;
    std::string displayName = "Landscape";
    std::uint64_t guidLo = 0;
    std::uint64_t guidHi = 0;
    TerrainCreateInfo createInfo{};
    /// Optional relative heightmap path for package cook — not required for runtime.
    std::string heightmapRelativePath;
    std::string materialSlot0;
    std::string materialSlot1;
    std::string materialSlot2;
    std::string materialSlot3;
    bool streamingEnabled = true;
    int maxResidentChunks = 64;
};

/// In-memory terrain asset — stores elevation samples directly (heightmap file optional).
class TERRAIN_API TerrainAsset : public ITerrainAsset {
public:
    TerrainAsset() = default;
    explicit TerrainAsset(TerrainAssetDesc desc)
        : m_Desc(std::move(desc))
    {}

    [[nodiscard]] const TerrainAssetDesc& Desc() const noexcept { return m_Desc; }
    [[nodiscard]] TerrainAssetDesc& Desc() noexcept { return m_Desc; }

    void SetHeightSamples(std::vector<std::uint16_t> samples, int width, int height) {
        m_Samples = std::move(samples);
        m_Width = width;
        m_Height = height;
        m_Dirty = true;
    }

    [[nodiscard]] const std::vector<std::uint16_t>& Samples() const noexcept { return m_Samples; }
    [[nodiscard]] int Width() const noexcept { return m_Width; }
    [[nodiscard]] int Height() const noexcept { return m_Height; }

    // ITerrainAsset
    [[nodiscard]] TerrainGuid GetGuid() const noexcept override {
        return TerrainGuid{m_Desc.guidHi, m_Desc.guidLo};
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return m_Desc.displayName; }
    [[nodiscard]] std::string_view GetAssetPath() const noexcept override {
        return m_Desc.assetPath;
    }
    [[nodiscard]] const TerrainCreateInfo& GetCreateInfo() const noexcept override {
        return m_Desc.createInfo;
    }
    [[nodiscard]] std::span<const std::uint16_t> GetElevationSamples() const noexcept override {
        return m_Samples;
    }
    [[nodiscard]] int GetWidth() const noexcept override { return m_Width; }
    [[nodiscard]] int GetHeight() const noexcept override { return m_Height; }
    [[nodiscard]] bool IsValid() const noexcept override {
        return !m_Desc.displayName.empty() && m_Desc.createInfo.resolutionX > 1
            && m_Desc.createInfo.resolutionY > 1;
    }
    [[nodiscard]] bool IsDirty() const noexcept override { return m_Dirty; }

    void SetDirty(bool dirty) noexcept { m_Dirty = dirty; }

private:
    TerrainAssetDesc m_Desc{};
    std::vector<std::uint16_t> m_Samples;
    int m_Width = 0;
    int m_Height = 0;
    bool m_Dirty = false;
};

[[nodiscard]] inline std::shared_ptr<ITerrainAsset> MakeTerrainAssetShared(TerrainAsset asset) {
    return std::make_shared<TerrainAsset>(std::move(asset));
}

} // namespace we::runtime::terrain

