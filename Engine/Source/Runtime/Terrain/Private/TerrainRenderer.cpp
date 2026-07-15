#include "Terrain/TerrainRenderer.h"

#include "Core/Logger.h"

namespace we::runtime::terrain {

void TerrainRenderer::Init(we::rhi::IRHIDevice* device) {
    m_Device = device;
    m_Uploaded = 0;
    m_Initialized = true;
    HE_INFO("[Terrain] TerrainRenderer initialized (IRHI device; CPU draw-list path).");
}

void TerrainRenderer::Shutdown() {
    m_Device = nullptr;
    m_Uploaded = 0;
    m_Initialized = false;
}

void TerrainRenderer::SyncChunks(TerrainChunkManager& chunks) {
    if (!m_Initialized) {
        return;
    }
    // Extension point: upload dirty visible chunk meshes through IRHI buffers.
    std::uint32_t count = 0;
    for (const TerrainChunk& chunk : chunks.Chunks()) {
        if (chunk.visible && !chunk.mesh.indices.empty()) {
            ++count;
        }
    }
    m_Uploaded = count;
    (void)m_Device;
}

void TerrainRenderer::BuildDrawList(const TerrainChunkManager& chunks,
    std::vector<DrawItem>& outDraws) const {
    outDraws.clear();
    for (const TerrainChunk& chunk : chunks.Chunks()) {
        if (!chunk.visible || chunk.mesh.indices.empty()) {
            continue;
        }
        DrawItem item{};
        item.id = chunk.id;
        item.mesh = &chunk.mesh;
        item.lod = chunk.lod;
        outDraws.push_back(item);
    }
}

} // namespace we::runtime::terrain
