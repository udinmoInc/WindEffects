#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace we::runtime::terrain {

/// In-memory 16-bit elevation grid (row-major, Z-major rows, X columns).
/// This is the terrain foundation — NOT a heightmap file. PNG/RAW I/O is optional.
class TERRAIN_API TerrainHeightmap {
public:
    void Create(int width, int height, std::uint16_t fill = kMidHeightSample);
    void Resize(int width, int height, std::uint16_t fill = kMidHeightSample);
    void Clear(std::uint16_t fill = kMidHeightSample);

    int Width() const { return m_Width; }
    int Height() const { return m_Height; }
    bool Empty() const { return m_Samples.empty(); }

    std::uint16_t* Data() { return m_Samples.data(); }
    const std::uint16_t* Data() const { return m_Samples.data(); }
    std::size_t SampleCount() const { return m_Samples.size(); }

    std::uint16_t Get(int x, int z) const;
    void Set(int x, int z, std::uint16_t value);

    /// Bilinear sample in normalized [0,1] UV space.
    float SampleNormalized(float u, float v) const;
    /// World-space height in meters using create info scales.
    float SampleWorldHeight(
        float localX,
        float localZ,
        float worldSizeX,
        float worldSizeY,
        float heightScale,
        float heightOffset) const;

    void MarkRegionDirty(int minX, int minZ, int maxX, int maxZ);
    bool ConsumeDirtyRect(int& minX, int& minZ, int& maxX, int& maxZ);

private:
    int m_Width = 0;
    int m_Height = 0;
    std::vector<std::uint16_t> m_Samples;
    bool m_Dirty = false;
    int m_DirtyMinX = 0;
    int m_DirtyMinZ = 0;
    int m_DirtyMaxX = -1;
    int m_DirtyMaxZ = -1;
};

/// Preferred alias — elevation grid, independent of heightmap files.
using TerrainHeightfield = TerrainHeightmap;

/// Optional heightmap file I/O (PNG / RAW / 16-bit). Never required to create a landscape.
class TERRAIN_API HeightmapIO {
public:
    static bool Import(
        const std::filesystem::path& path,
        TerrainHeightmap& outMap,
        HeightmapFormat* detected = nullptr);
    static bool Export(
        const std::filesystem::path& path,
        const TerrainHeightmap& map,
        HeightmapFormat format);

    static bool ImportRaw16(
        const std::filesystem::path& path,
        int width,
        int height,
        TerrainHeightmap& outMap,
        bool littleEndian = true);
    static bool ExportRaw16(
        const std::filesystem::path& path,
        const TerrainHeightmap& map,
        bool littleEndian = true);

    static bool ImportPng(const std::filesystem::path& path, TerrainHeightmap& outMap);
    static bool ExportPng8(const std::filesystem::path& path, const TerrainHeightmap& map);
    static bool ExportPng16(const std::filesystem::path& path, const TerrainHeightmap& map);
};

} // namespace we::runtime::terrain

