#include "Terrain/TerrainSystem.h"

#include "Core/Logger.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include <algorithm>
#include <cmath>

#include "Core/Math/GlmInterop.h"
namespace we::runtime::terrain {

TerrainSystem& TerrainSystem::Get() {
    static TerrainSystem instance;
    return instance;
}

bool TerrainSystem::Create(const TerrainCreateInfo& info) {
    Destroy();

    m_Info = info;
    m_Info.resolutionX = std::max(2, m_Info.resolutionX);
    m_Info.resolutionY = std::max(2, m_Info.resolutionY);
    m_Info.chunkQuads = std::max(1, m_Info.chunkQuads);

    // Snap resolution so chunks tile: (N * quads) + 1
    const int qx = m_Info.chunkQuads;
    if (((m_Info.resolutionX - 1) % qx) != 0) {
        m_Info.resolutionX = ((m_Info.resolutionX - 1) / qx) * qx + 1;
    }
    if (((m_Info.resolutionY - 1) % qx) != 0) {
        m_Info.resolutionY = ((m_Info.resolutionY - 1) / qx) * qx + 1;
    }

    m_Heightmap.Create(m_Info.resolutionX, m_Info.resolutionY, 32768);
    m_Chunks.Initialize(m_Info);
    m_Materials.Initialize(m_Info.resolutionX, m_Info.resolutionY);
    m_Collision.Bind(&m_Heightmap, &m_Info);
    m_Foliage.Clear();
    m_Created = true;

    RebuildDirty();
    HE_INFO("[Terrain] Landscape created "
        + std::to_string(m_Info.resolutionX) + "x" + std::to_string(m_Info.resolutionY)
        + " (" + std::to_string(m_Chunks.ChunkCountX()) + "x"
        + std::to_string(m_Chunks.ChunkCountZ()) + " chunks).");
    return true;
}

void TerrainSystem::Destroy() {
    if (!m_Created) {
        return;
    }
    m_Renderer.Shutdown();
    m_Materials.Shutdown();
    m_Chunks.Shutdown();
    m_Heightmap.Create(0, 0);
    m_Foliage.Clear();
    m_LandscapeEntityId = 0;
    m_Created = false;
}

void TerrainSystem::BindScene(we::runtime::scene::Scene* scene) {
    m_Scene = scene;
}

void TerrainSystem::BindRenderer(we::rhi::IRHIDevice* device) {
    m_Renderer.Init(device);
}

std::uint64_t TerrainSystem::SpawnLandscapeActor(const char* name) {
    if (!m_Scene) {
        HE_ERROR("[Terrain] Cannot spawn Landscape — scene is not bound.");
        return 0;
    }
    if (!m_Created) {
        Create(TerrainCreateInfo{});
    }

    using we::runtime::scene::EntityType;
    if (m_Scene->HasEntityOfType(EntityType::Landscape)) {
        const int index = m_Scene->FindEntityIndexByName(name ? name : "Landscape");
        if (index >= 0) {
            m_LandscapeEntityId = m_Scene->GetEntities()[static_cast<std::size_t>(index)].Id;
            return m_LandscapeEntityId;
        }
    }

    m_Scene->CreateEntity(name ? name : "Landscape", EntityType::Landscape);
    auto& entities = m_Scene->GetEntities();
    if (!entities.empty()) {
        we::runtime::scene::Entity& e = entities.back();
        e.Position = m_Info.worldOrigin;
        e.Scale = we::math::Vec3(m_Info.worldSizeX, m_Info.heightScale, m_Info.worldSizeY);
        m_LandscapeEntityId = e.Id;
        m_Scene->SetSelectedEntityIndex(static_cast<int>(entities.size()) - 1);
    }
    return m_LandscapeEntityId;
}

bool TerrainSystem::ImportHeightmap(const std::filesystem::path& path) {
    TerrainHeightmap imported;
    if (!HeightmapIO::Import(path, imported)) {
        return false;
    }

    TerrainCreateInfo info = m_Info;
    info.resolutionX = imported.Width();
    info.resolutionY = imported.Height();
    if (!m_Created) {
        Create(info);
    } else {
        m_Info = info;
        m_Chunks.Initialize(m_Info);
        m_Materials.Initialize(m_Info.resolutionX, m_Info.resolutionY);
        m_Collision.Bind(&m_Heightmap, &m_Info);
    }

    m_Heightmap = std::move(imported);
    // Resize if Create snapped resolution.
    if (m_Heightmap.Width() != m_Info.resolutionX || m_Heightmap.Height() != m_Info.resolutionY) {
        TerrainHeightmap resized;
        resized.Create(m_Info.resolutionX, m_Info.resolutionY, 32768);
        for (int z = 0; z < resized.Height(); ++z) {
            for (int x = 0; x < resized.Width(); ++x) {
                const float u = static_cast<float>(x) / static_cast<float>(std::max(1, resized.Width() - 1));
                const float v = static_cast<float>(z) / static_cast<float>(std::max(1, resized.Height() - 1));
                const float n = m_Heightmap.SampleNormalized(u, v);
                resized.Set(x, z, static_cast<std::uint16_t>(std::clamp(n * 65535.0f, 0.0f, 65535.0f)));
            }
        }
        m_Heightmap = std::move(resized);
    }
    m_Heightmap.MarkRegionDirty(0, 0, m_Heightmap.Width() - 1, m_Heightmap.Height() - 1);
    m_Chunks.MarkAllDirty();
    RebuildDirty();
    return true;
}

bool TerrainSystem::ExportHeightmap(const std::filesystem::path& path, HeightmapFormat format) {
    return HeightmapIO::Export(path, m_Heightmap, format);
}

bool TerrainSystem::ApplyBrushWorld(float worldX, float worldZ) {
    if (!m_Created) {
        return false;
    }
    const float localX = worldX - m_Info.worldOrigin.x;
    const float localZ = worldZ - m_Info.worldOrigin.z;
    const float sx = (m_Info.worldSizeX > 1e-6f)
        ? (localX / m_Info.worldSizeX) * static_cast<float>(m_Info.resolutionX - 1)
        : 0.0f;
    const float sz = (m_Info.worldSizeY > 1e-6f)
        ? (localZ / m_Info.worldSizeY) * static_cast<float>(m_Info.resolutionY - 1)
        : 0.0f;

    if (!m_Brushes.Apply(m_Heightmap, sx, sz)) {
        return false;
    }

    int minX = 0, minZ = 0, maxX = 0, maxZ = 0;
    if (m_Heightmap.ConsumeDirtyRect(minX, minZ, maxX, maxZ)) {
        m_Chunks.MarkDirtyRect(minX, minZ, maxX, maxZ);
        m_Materials.NotifyVirtualTexturePagesDirty(minX, minZ, maxX, maxZ);
    }
    RebuildDirty();
    return true;
}

void TerrainSystem::RebuildDirty() {
    // Refresh bounds for dirty chunks before LOD / streaming.
    for (TerrainChunk& chunk : m_Chunks.Chunks()) {
        if (!chunk.meshDirty) {
            continue;
        }
        TerrainMeshCPU mesh;
        if (TerrainLODManager::BuildChunkMesh(m_Heightmap, m_Info, chunk, chunk.lod, mesh)) {
            chunk.mesh = std::move(mesh);
            if (!chunk.mesh.positions.empty()) {
                we::math::Vec3 bmin = chunk.mesh.positions.front();
                we::math::Vec3 bmax = bmin;
                for (const we::math::Vec3& p : chunk.mesh.positions) {
                    bmin = we::math::Vec3(std::min(bmin.x, p.x), std::min(bmin.y, p.y), std::min(bmin.z, p.z));
                    bmax = we::math::Vec3(std::max(bmax.x, p.x), std::max(bmax.y, p.y), std::max(bmax.z, p.z));
                }
                chunk.bounds.min = bmin;
                chunk.bounds.max = bmax;
            }
            chunk.meshDirty = false;
        }
    }
    m_Collision.RebuildDirty(m_Chunks);
    m_Renderer.SyncChunks(m_Chunks);
}

void TerrainSystem::Tick(float /*deltaSeconds*/, const we::math::Vec3& cameraWorldPos,
    const we::math::Mat4* viewProj) {
    if (!m_Created) {
        return;
    }

    TerrainFrustump frustum{};
    const TerrainFrustump* frustumPtr = nullptr;
    if (viewProj) {
        frustum = TerrainFrustump::FromViewProj(*viewProj);
        frustumPtr = &frustum;
    }

    m_Lod.UpdateChunkLods(m_Chunks, cameraWorldPos, m_Info, m_Heightmap.Width());
    m_Streaming.Update(m_Chunks, cameraWorldPos, m_Info, frustumPtr);

    // Rebuild any LOD-dirtied chunks.
    bool anyDirty = false;
    for (const TerrainChunk& chunk : m_Chunks.Chunks()) {
        if (chunk.meshDirty) {
            anyDirty = true;
            break;
        }
    }
    if (anyDirty) {
        RebuildDirty();
    } else {
        m_Renderer.SyncChunks(m_Chunks);
    }
}

} // namespace we::runtime::terrain
