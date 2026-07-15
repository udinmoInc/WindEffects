#include "Terrain/TerrainRenderer.h"

#include "Core/Logger.h"

namespace we::runtime::terrain {

void TerrainRenderer::Init(we::runtime::renderer::DeviceContext* device,
    we::runtime::renderer::ResourceManager* resources) {
    m_Device = device;
    m_Resources = resources;
    m_Uploaded = 0;
    m_Initialized = true;
    HE_INFO("[Terrain] TerrainRenderer initialized (CPU draw-list path; GPU upload reserved).");
}

void TerrainRenderer::Shutdown() {
    m_Device = nullptr;
    m_Resources = nullptr;
    m_Uploaded = 0;
    m_Initialized = false;
}

void TerrainRenderer::SyncChunks(TerrainChunkManager& chunks) {
    if (!m_Initialized) {
        return;
    }
    // Extension point: upload dirty visible chunk meshes to GPU buffers / clipmap rings.
    // Until TerrainPass lands, keep CPU meshes authoritative for collision/editor.
    std::uint32_t count = 0;
    for (const TerrainChunk& chunk : chunks.Chunks()) {
        if (chunk.visible && !chunk.mesh.indices.empty()) {
            ++count;
        }
    }
    m_Uploaded = count;
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
