#pragma once

#include "Terrain/Terrain.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Core/Logger.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace we::runtime::terrain {
namespace detail {

[[nodiscard]] inline TerrainGuid MakeGuid(std::uint64_t hi, std::uint64_t lo) noexcept {
    return TerrainGuid{hi, lo};
}

[[nodiscard]] inline TerrainGuid GenerateTerrainGuid() {
    static std::atomic<std::uint64_t> counter{1};
    const auto t = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return MakeGuid(t ^ 0x9e3779b97f4a7c15ull, counter.fetch_add(1));
}

[[nodiscard]] inline TerrainId NextTerrainId() {
    static std::atomic<std::uint64_t> next{1};
    return TerrainId{next.fetch_add(1)};
}

/// Legacy bridge for TerrainSystem facade — concrete subsystem pointers for the active landscape.
struct ActiveTerrainAccess {
    TerrainId id{};
    bool created = false;
    TerrainCreateInfo* info = nullptr;
    TerrainHeightmap* heightfield = nullptr;
    TerrainChunkManager* chunks = nullptr;
    TerrainLODManager* lod = nullptr;
    TerrainCollision* collision = nullptr;
    TerrainMaterialSystem* materials = nullptr;
    TerrainBrushSystem* brushes = nullptr;
    TerrainStreaming* streaming = nullptr;
    TerrainRenderer* renderer = nullptr;
    TerrainFoliageSystem* foliage = nullptr;
    std::uint64_t* entityId = nullptr;

    using RebuildFn = void (*)(void*);
    void* rebuildCtx = nullptr;
    RebuildFn rebuildDirty = nullptr;
    using ApplyGenFn = bool (*)(void*, const TerrainGeneratorParams&);
    ApplyGenFn applyGenerator = nullptr;
    using CaptureFn = std::shared_ptr<ITerrainAsset> (*)(void*, std::string_view);
    CaptureFn captureAsset = nullptr;
    using ApplyAssetFn = bool (*)(void*, const ITerrainAsset&);
    ApplyAssetFn applyAsset = nullptr;
    using CaptureSamplesFn = std::vector<std::uint16_t> (*)(void*);
    CaptureSamplesFn captureSamples = nullptr;
    using RestoreSamplesFn = bool (*)(void*, const std::vector<std::uint16_t>&);
    RestoreSamplesFn restoreSamples = nullptr;
    using BrushWorldFn = bool (*)(void*, float, float);
    BrushWorldFn brushWorld = nullptr;
    void* instance = nullptr;
};

[[nodiscard]] ActiveTerrainAccess QueryActiveTerrainAccess();

class TerrainLayerProxy final : public ITerrainLayer {
public:
    TerrainLayerProxy(TerrainMaterialSystem& materials, int index)
        : m_Materials(materials)
        , m_Index(index)
    {}

    [[nodiscard]] int GetIndex() const noexcept override { return m_Index; }
    [[nodiscard]] std::string_view GetName() const noexcept override {
        return m_Materials.Layer(m_Index).name;
    }
    void SetName(std::string_view name) override {
        m_Materials.Layer(m_Index).name = std::string(name);
    }
    void SetAlbedoPath(std::string_view path) override {
        m_Materials.Layer(m_Index).albedoPath = std::string(path);
    }
    [[nodiscard]] std::string_view GetAlbedoPath() const noexcept override {
        return m_Materials.Layer(m_Index).albedoPath;
    }

private:
    TerrainMaterialSystem& m_Materials;
    int m_Index = 0;
};

class TerrainChunkProxy final : public ITerrainChunk {
public:
    explicit TerrainChunkProxy(TerrainChunk& chunk)
        : m_Chunk(chunk)
    {}

    [[nodiscard]] TerrainChunkId GetId() const noexcept override { return m_Chunk.id; }
    [[nodiscard]] int GetLod() const noexcept override { return m_Chunk.lod; }
    [[nodiscard]] bool IsVisible() const noexcept override { return m_Chunk.visible; }
    [[nodiscard]] bool IsMeshDirty() const noexcept override { return m_Chunk.meshDirty; }
    [[nodiscard]] const TerrainAABB& GetBounds() const noexcept override { return m_Chunk.bounds; }
    [[nodiscard]] const TerrainMeshCPU* GetMesh() const noexcept override { return &m_Chunk.mesh; }

    TerrainChunk& m_Chunk;
};

} // namespace detail
} // namespace we::runtime::terrain
