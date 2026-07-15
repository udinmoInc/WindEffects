#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"

#include <vector>

namespace we::runtime::terrain {

class TerrainHeightmap;
class TerrainChunkManager;

// Heightfield collision queries. Extensible to PhysX/Jolt heightfields later
// without changing the sampling API used by gameplay/editor tools.
class TERRAIN_API TerrainCollision {
public:
    void Bind(const TerrainHeightmap* heightmap, const TerrainCreateInfo* info);
    void InvalidateChunk(TerrainChunkId id);
    void RebuildDirty(TerrainChunkManager& chunks);

    bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
        glm::vec3& outHit, glm::vec3& outNormal) const;

    bool SampleHeightNormal(float worldX, float worldZ, float& outHeight, glm::vec3& outNormal) const;

    // Future: streamed physx actor handles per chunk.
    struct ChunkColliderHandle {
        TerrainChunkId id{};
        bool built = false;
    };

    const std::vector<ChunkColliderHandle>& Colliders() const { return m_Colliders; }

private:
    const TerrainHeightmap* m_Heightmap = nullptr;
    const TerrainCreateInfo* m_Info = nullptr;
    std::vector<ChunkColliderHandle> m_Colliders;
};

} // namespace we::runtime::terrain

#pragma warning(pop)
