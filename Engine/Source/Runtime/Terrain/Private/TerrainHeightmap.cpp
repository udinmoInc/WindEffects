#include "Terrain/TerrainHeightmap.h"

#include "Core/Logger.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>

#pragma warning(push)
#pragma warning(disable : 4505)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#pragma warning(pop)

namespace we::runtime::terrain {

void TerrainHeightmap::Create(int width, int height, std::uint16_t fill) {
    m_Width = std::max(1, width);
    m_Height = std::max(1, height);
    m_Samples.assign(static_cast<std::size_t>(m_Width) * static_cast<std::size_t>(m_Height), fill);
    m_Dirty = false;
    m_DirtyMinX = 0;
    m_DirtyMinZ = 0;
    m_DirtyMaxX = -1;
    m_DirtyMaxZ = -1;
}

void TerrainHeightmap::Resize(int width, int height, std::uint16_t fill) {
    Create(width, height, fill);
}

void TerrainHeightmap::Clear(std::uint16_t fill) {
    std::fill(m_Samples.begin(), m_Samples.end(), fill);
    MarkRegionDirty(0, 0, m_Width - 1, m_Height - 1);
}

std::uint16_t TerrainHeightmap::Get(int x, int z) const {
    if (x < 0 || z < 0 || x >= m_Width || z >= m_Height || m_Samples.empty()) {
        return 0;
    }
    return m_Samples[static_cast<std::size_t>(z) * static_cast<std::size_t>(m_Width)
        + static_cast<std::size_t>(x)];
}

void TerrainHeightmap::Set(int x, int z, std::uint16_t value) {
    if (x < 0 || z < 0 || x >= m_Width || z >= m_Height || m_Samples.empty()) {
        return;
    }
    m_Samples[static_cast<std::size_t>(z) * static_cast<std::size_t>(m_Width)
        + static_cast<std::size_t>(x)] = value;
    MarkRegionDirty(x, z, x, z);
}

float TerrainHeightmap::SampleNormalized(float u, float v) const {
    if (m_Width < 1 || m_Height < 1 || m_Samples.empty()) {
        return 0.5f;
    }
    u = std::clamp(u, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);
    const float fx = u * static_cast<float>(m_Width - 1);
    const float fz = v * static_cast<float>(m_Height - 1);
    const int x0 = static_cast<int>(fx);
    const int z0 = static_cast<int>(fz);
    const int x1 = std::min(x0 + 1, m_Width - 1);
    const int z1 = std::min(z0 + 1, m_Height - 1);
    const float tx = fx - static_cast<float>(x0);
    const float tz = fz - static_cast<float>(z0);
    const float h00 = static_cast<float>(Get(x0, z0)) / 65535.0f;
    const float h10 = static_cast<float>(Get(x1, z0)) / 65535.0f;
    const float h01 = static_cast<float>(Get(x0, z1)) / 65535.0f;
    const float h11 = static_cast<float>(Get(x1, z1)) / 65535.0f;
    const float hx0 = h00 + (h10 - h00) * tx;
    const float hx1 = h01 + (h11 - h01) * tx;
    return hx0 + (hx1 - hx0) * tz;
}

float TerrainHeightmap::SampleWorldHeight(float localX, float localZ, float worldSizeX, float worldSizeY,
    float heightScale, float heightOffset) const {
    const float u = (worldSizeX > 1e-6f) ? (localX / worldSizeX) : 0.0f;
    const float v = (worldSizeY > 1e-6f) ? (localZ / worldSizeY) : 0.0f;
    return heightOffset + SampleNormalized(u, v) * heightScale;
}

void TerrainHeightmap::MarkRegionDirty(int minX, int minZ, int maxX, int maxZ) {
    minX = std::clamp(minX, 0, std::max(0, m_Width - 1));
    maxX = std::clamp(maxX, 0, std::max(0, m_Width - 1));
    minZ = std::clamp(minZ, 0, std::max(0, m_Height - 1));
    maxZ = std::clamp(maxZ, 0, std::max(0, m_Height - 1));
    if (!m_Dirty) {
        m_Dirty = true;
        m_DirtyMinX = minX;
        m_DirtyMinZ = minZ;
        m_DirtyMaxX = maxX;
        m_DirtyMaxZ = maxZ;
        return;
    }
    m_DirtyMinX = std::min(m_DirtyMinX, minX);
    m_DirtyMinZ = std::min(m_DirtyMinZ, minZ);
    m_DirtyMaxX = std::max(m_DirtyMaxX, maxX);
    m_DirtyMaxZ = std::max(m_DirtyMaxZ, maxZ);
}

bool TerrainHeightmap::ConsumeDirtyRect(int& minX, int& minZ, int& maxX, int& maxZ) {
    if (!m_Dirty) {
        return false;
    }
    minX = m_DirtyMinX;
    minZ = m_DirtyMinZ;
    maxX = m_DirtyMaxX;
    maxZ = m_DirtyMaxZ;
    m_Dirty = false;
    m_DirtyMaxX = -1;
    m_DirtyMaxZ = -1;
    return true;
}

bool HeightmapIO::ImportRaw16(const std::filesystem::path& path, int width, int height,
    TerrainHeightmap& outMap, bool littleEndian) {
    if (width < 1 || height < 1) {
        return false;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        HE_ERROR("[Terrain] Failed to open RAW heightmap: " + path.string());
        return false;
    }
    const std::size_t expected = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::vector<std::uint16_t> samples(expected);
    in.read(reinterpret_cast<char*>(samples.data()),
        static_cast<std::streamsize>(expected * sizeof(std::uint16_t)));
    if (!in) {
        HE_ERROR("[Terrain] Truncated RAW heightmap: " + path.string());
        return false;
    }
    if (!littleEndian) {
        for (std::uint16_t& s : samples) {
            s = static_cast<std::uint16_t>((s << 8) | (s >> 8));
        }
    }
    outMap.Create(width, height);
    std::memcpy(outMap.Data(), samples.data(), expected * sizeof(std::uint16_t));
    outMap.MarkRegionDirty(0, 0, width - 1, height - 1);
    return true;
}

bool HeightmapIO::ExportRaw16(const std::filesystem::path& path, const TerrainHeightmap& map,
    bool littleEndian) {
    if (map.Empty()) {
        return false;
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        HE_ERROR("[Terrain] Failed to write RAW heightmap: " + path.string());
        return false;
    }
    if (littleEndian) {
        out.write(reinterpret_cast<const char*>(map.Data()),
            static_cast<std::streamsize>(map.SampleCount() * sizeof(std::uint16_t)));
        return static_cast<bool>(out);
    }
    std::vector<std::uint16_t> swapped(map.SampleCount());
    for (std::size_t i = 0; i < map.SampleCount(); ++i) {
        const std::uint16_t s = map.Data()[i];
        swapped[i] = static_cast<std::uint16_t>((s << 8) | (s >> 8));
    }
    out.write(reinterpret_cast<const char*>(swapped.data()),
        static_cast<std::streamsize>(swapped.size() * sizeof(std::uint16_t)));
    return static_cast<bool>(out);
}

bool HeightmapIO::ImportPng(const std::filesystem::path& path, TerrainHeightmap& outMap) {
    int w = 0;
    int h = 0;
    int channels = 0;
    stbi_us* pixels = stbi_load_16(path.string().c_str(), &w, &h, &channels, 0);
    if (!pixels || w < 1 || h < 1) {
        if (pixels) {
            stbi_image_free(pixels);
        }
        // Fallback: 8-bit PNG expanded to 16-bit.
        int w8 = 0, h8 = 0, c8 = 0;
        stbi_uc* p8 = stbi_load(path.string().c_str(), &w8, &h8, &c8, 1);
        if (!p8) {
            HE_ERROR("[Terrain] Failed to load PNG heightmap: " + path.string());
            return false;
        }
        outMap.Create(w8, h8);
        for (int z = 0; z < h8; ++z) {
            for (int x = 0; x < w8; ++x) {
                const std::uint8_t v = p8[static_cast<std::size_t>(z) * static_cast<std::size_t>(w8)
                    + static_cast<std::size_t>(x)];
                outMap.Set(x, z, static_cast<std::uint16_t>(v) * 257u);
            }
        }
        stbi_image_free(p8);
        return true;
    }

    outMap.Create(w, h);
    for (int z = 0; z < h; ++z) {
        for (int x = 0; x < w; ++x) {
            const stbi_us* px = pixels
                + (static_cast<std::size_t>(z) * static_cast<std::size_t>(w)
                    + static_cast<std::size_t>(x)) * static_cast<std::size_t>(channels);
            outMap.Set(x, z, px[0]);
        }
    }
    stbi_image_free(pixels);
    return true;
}

bool HeightmapIO::ExportPng8(const std::filesystem::path& path, const TerrainHeightmap& map) {
    if (map.Empty()) {
        return false;
    }
    std::vector<std::uint8_t> bytes(map.SampleCount());
    for (std::size_t i = 0; i < map.SampleCount(); ++i) {
        bytes[i] = static_cast<std::uint8_t>(map.Data()[i] >> 8);
    }
    const int ok = stbi_write_png(path.string().c_str(), map.Width(), map.Height(), 1,
        bytes.data(), map.Width());
    if (!ok) {
        HE_ERROR("[Terrain] Failed to write PNG heightmap: " + path.string());
        return false;
    }
    return true;
}

bool HeightmapIO::ExportPng16(const std::filesystem::path& path, const TerrainHeightmap& map) {
    // stb_image_write has no 16-bit PNG path; export high byte as 8-bit PNG for interchange.
    // Prefer Raw16Le / R16 for lossless 16-bit elevation export.
    HE_WARN("[Terrain] ExportPng16 writes 8-bit high-byte PNG; use Raw16 for full precision.");
    return ExportPng8(path, map);
}

bool HeightmapIO::Import(const std::filesystem::path& path, TerrainHeightmap& outMap,
    HeightmapFormat* detected) {
    const std::string ext = path.extension().string();
    std::string lower = ext;
    for (char& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    if (lower == ".png") {
        if (detected) {
            *detected = HeightmapFormat::Png16;
        }
        return ImportPng(path, outMap);
    }

    if (lower == ".r16" || lower == ".raw") {
        if (detected) {
            *detected = (lower == ".r16") ? HeightmapFormat::R16 : HeightmapFormat::Raw16Le;
        }
        // Infer square resolution from file size when possible.
        std::ifstream in(path, std::ios::binary | std::ios::ate);
        if (!in) {
            return false;
        }
        const auto bytes = static_cast<std::size_t>(in.tellg());
        in.close();
        if (bytes % 2 != 0) {
            HE_ERROR("[Terrain] RAW heightmap size is not a multiple of 2.");
            return false;
        }
        const std::size_t samples = bytes / 2;
        const int side = static_cast<int>(std::lround(std::sqrt(static_cast<double>(samples))));
        if (static_cast<std::size_t>(side) * static_cast<std::size_t>(side) != samples) {
            HE_ERROR("[Terrain] RAW heightmap is not square; use ImportRaw16 with explicit size.");
            return false;
        }
        return ImportRaw16(path, side, side, outMap, true);
    }

    HE_ERROR("[Terrain] Unsupported heightmap extension: " + path.string());
    return false;
}

bool HeightmapIO::Export(const std::filesystem::path& path, const TerrainHeightmap& map,
    HeightmapFormat format) {
    switch (format) {
    case HeightmapFormat::Png8:
        return ExportPng8(path, map);
    case HeightmapFormat::Png16:
        return ExportPng16(path, map);
    case HeightmapFormat::Raw16Le:
    case HeightmapFormat::R16:
        return ExportRaw16(path, map, true);
    }
    return false;
}

} // namespace we::runtime::terrain
