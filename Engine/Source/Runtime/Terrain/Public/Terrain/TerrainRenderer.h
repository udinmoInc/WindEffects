#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/TerrainChunkManager.h"

#include <cstdint>
#include <vector>

namespace we::runtime::renderer {
class DeviceContext;
class ResourceManager;
}

namespace we::runtime::terrain {

// Submits visible chunk meshes. Designed so GPU clipmaps / VT / compute tessellation
// can replace the CPU mesh path without changing TerrainSystem ownership.
class TERRAIN_API TerrainRenderer {
public:
    void Init(we::runtime::renderer::DeviceContext* device,
        we::runtime::renderer::ResourceManager* resources);
    void Shutdown();

    // Upload / refresh GPU buffers for dirty visible chunks (stub uploads CPU-side for now
    // until a dedicated TerrainPass lands next to PBRPass).
    void SyncChunks(TerrainChunkManager& chunks);

    // Collect draw list after streaming + LOD / frustum.
    struct DrawItem {
        TerrainChunkId id{};
        const TerrainMeshCPU* mesh = nullptr;
        int lod = 0;
    };
    void BuildDrawList(const TerrainChunkManager& chunks, std::vector<DrawItem>& outDraws) const;

    std::uint32_t UploadedChunkCount() const { return m_Uploaded; }

    // Future: clipmap rings, VT feedback buffer, compute cull args.
    void SetClipmapsEnabled(bool enabled) { m_Clipmaps = enabled; }
    bool ClipmapsEnabled() const { return m_Clipmaps; }

private:
    we::runtime::renderer::DeviceContext* m_Device = nullptr;
    we::runtime::renderer::ResourceManager* m_Resources = nullptr;
    std::uint32_t m_Uploaded = 0;
    bool m_Clipmaps = false;
    bool m_Initialized = false;
};

} // namespace we::runtime::terrain

#pragma warning(pop)
