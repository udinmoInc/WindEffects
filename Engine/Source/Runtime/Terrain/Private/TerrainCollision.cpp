#include "Terrain/TerrainCollision.h"
#include "Terrain/TerrainHeightmap.h"
#include "Terrain/TerrainChunkManager.h"

#include <algorithm>
#include <cmath>

#include "Core/Math/GlmInterop.h"
namespace we::runtime::terrain {

void TerrainCollision::Bind(const TerrainHeightmap* heightmap, const TerrainCreateInfo* info) {
    m_Heightmap = heightmap;
    m_Info = info;
    m_Colliders.clear();
}

void TerrainCollision::InvalidateChunk(TerrainChunkId id) {
    for (ChunkColliderHandle& handle : m_Colliders) {
        if (handle.id == id) {
            handle.built = false;
            return;
        }
    }
    m_Colliders.push_back(ChunkColliderHandle{ id, false });
}

void TerrainCollision::RebuildDirty(TerrainChunkManager& chunks) {
    for (TerrainChunk& chunk : chunks.Chunks()) {
        if (!chunk.collisionDirty) {
            continue;
        }
        InvalidateChunk(chunk.id);
        for (ChunkColliderHandle& handle : m_Colliders) {
            if (handle.id == chunk.id) {
                handle.built = true;
                break;
            }
        }
        chunk.collisionDirty = false;
    }
}

bool TerrainCollision::SampleHeightNormal(float worldX, float worldZ, float& outHeight,
    we::math::Vec3& outNormal) const {
    if (!m_Heightmap || !m_Info || m_Heightmap->Empty()) {
        return false;
    }

    const float localX = worldX - m_Info->worldOrigin.x;
    const float localZ = worldZ - m_Info->worldOrigin.z;
    outHeight = m_Heightmap->SampleWorldHeight(localX, localZ, m_Info->worldSizeX, m_Info->worldSizeY,
        m_Info->heightScale, m_Info->heightOffset);

    const float eps = m_Info->worldSizeX / static_cast<float>(std::max(1, m_Heightmap->Width() - 1));
    const float hL = m_Heightmap->SampleWorldHeight(localX - eps, localZ, m_Info->worldSizeX,
        m_Info->worldSizeY, m_Info->heightScale, m_Info->heightOffset);
    const float hR = m_Heightmap->SampleWorldHeight(localX + eps, localZ, m_Info->worldSizeX,
        m_Info->worldSizeY, m_Info->heightScale, m_Info->heightOffset);
    const float hD = m_Heightmap->SampleWorldHeight(localX, localZ - eps, m_Info->worldSizeX,
        m_Info->worldSizeY, m_Info->heightScale, m_Info->heightOffset);
    const float hU = m_Heightmap->SampleWorldHeight(localX, localZ + eps, m_Info->worldSizeX,
        m_Info->worldSizeY, m_Info->heightScale, m_Info->heightOffset);

    outNormal = we::math::FromGlm(glm::normalize(glm::vec3(hL - hR, 2.0f * eps, hD - hU)));
    return true;
}

bool TerrainCollision::Raycast(const we::math::Vec3& origin, const we::math::Vec3& direction, float maxDistance,
    we::math::Vec3& outHit, we::math::Vec3& outNormal) const {
    if (!m_Heightmap || !m_Info || m_Heightmap->Empty()) {
        return false;
    }

    const glm::vec3 dir = glm::normalize(we::math::ToGlm(direction));
    const float step = std::max(0.25f,
        m_Info->worldSizeX / static_cast<float>(std::max(1, m_Heightmap->Width() - 1)));
    float t = 0.0f;
    float prevY = origin.y;
    float prevTerrain = 0.0f;
    we::math::Vec3 prevN(0.0f, 1.0f, 0.0f);
    bool havePrev = false;

    while (t <= maxDistance) {
        const glm::vec3 p = we::math::ToGlm(origin) + dir * t;
        float terrainY = 0.0f;
        we::math::Vec3 n(0.0f, 1.0f, 0.0f);
        if (!SampleHeightNormal(p.x, p.z, terrainY, n)) {
            return false;
        }

        if (havePrev) {
            const float d0 = prevY - prevTerrain;
            const float d1 = p.y - terrainY;
            if (d0 >= 0.0f && d1 <= 0.0f) {
                const float denom = (d0 - d1);
                const float alpha = (std::abs(denom) > 1e-6f) ? (d0 / denom) : 0.0f;
                outHit = we::math::FromGlm(glm::mix(we::math::ToGlm(origin) + dir * (t - step), p, std::clamp(alpha, 0.0f, 1.0f)));
                outNormal = we::math::FromGlm(glm::normalize(glm::mix(we::math::ToGlm(prevN), we::math::ToGlm(n), std::clamp(alpha, 0.0f, 1.0f))));
                return true;
            }
        }

        havePrev = true;
        prevY = p.y;
        prevTerrain = terrainY;
        prevN = n;
        t += step;
    }
    return false;
}

} // namespace we::runtime::terrain
