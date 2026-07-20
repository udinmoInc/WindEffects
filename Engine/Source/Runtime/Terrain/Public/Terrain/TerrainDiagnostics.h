#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace we::runtime::terrain {

struct TERRAIN_API TerrainDiagnosticsSnapshot {
    std::uint64_t landscapesCreated = 0;
    std::uint64_t landscapesDestroyed = 0;
    std::uint64_t landscapesAlive = 0;
    std::uint64_t generations = 0;
    std::uint64_t heightmapImports = 0;
    std::uint64_t heightmapExports = 0;
    std::uint64_t brushApplies = 0;
    std::uint64_t chunksRebuilt = 0;
    std::uint64_t collisionRebuilds = 0;
    std::uint64_t lodUpdates = 0;
    std::uint64_t generateMicros = 0;
    std::uint64_t brushMicros = 0;
    std::uint64_t updateMicros = 0;
    std::uint64_t gpuUploadMicros = 0;
    std::uint64_t meshRebuildMicros = 0;
    std::uint64_t collisionMicros = 0;
    std::uint64_t renderMicros = 0;
    std::uint64_t dirtyChunks = 0;
    std::uint64_t visibleChunks = 0;
    std::uint64_t streamedChunks = 0;
    std::uint64_t renderedChunks = 0;
    std::uint64_t drawCalls = 0;
    std::uint64_t memoryBytes = 0;
    bool lightingValid = false;
    float cameraSpeed = 0.0f;
};

class TERRAIN_API TerrainDiagnostics {
public:
    static TerrainDiagnostics& Get() noexcept;
    void Reset() noexcept;
    [[nodiscard]] TerrainDiagnosticsSnapshot Snapshot() const noexcept;
    [[nodiscard]] std::string FormatSummary() const;

    void OnCreated() noexcept;
    void OnDestroyed() noexcept;
    void OnGenerated(std::uint64_t micros) noexcept;
    void OnHeightmapImported() noexcept;
    void OnHeightmapExported() noexcept;
    void OnBrush(std::uint64_t micros) noexcept;
    void OnChunksRebuilt(std::uint64_t count) noexcept;
    void OnCollisionRebuilt(std::uint64_t count) noexcept;
    void OnLodUpdated(std::uint64_t count) noexcept;
    void OnUpdate(std::uint64_t micros) noexcept;
    void OnGpuUpload(std::uint64_t micros, std::uint64_t chunksUploaded) noexcept;
    void OnMeshRebuild(std::uint64_t micros, std::uint64_t dirtyCount) noexcept;
    void OnCollisionTiming(std::uint64_t micros) noexcept;
    void OnRender(std::uint64_t micros, bool lightingValid) noexcept;
    void OnDirtyChunks(std::uint64_t count) noexcept;
    void SetCameraSpeed(float speed) noexcept;
    void OnFrameRenderStats(
        std::uint64_t visible,
        std::uint64_t streamed,
        std::uint64_t rendered,
        std::uint64_t drawCalls,
        std::uint64_t memoryBytes) noexcept;

private:
    std::atomic<std::uint64_t> m_Created{0};
    std::atomic<std::uint64_t> m_Destroyed{0};
    std::atomic<std::uint64_t> m_Alive{0};
    std::atomic<std::uint64_t> m_Generations{0};
    std::atomic<std::uint64_t> m_Imports{0};
    std::atomic<std::uint64_t> m_Exports{0};
    std::atomic<std::uint64_t> m_Brush{0};
    std::atomic<std::uint64_t> m_Chunks{0};
    std::atomic<std::uint64_t> m_Collision{0};
    std::atomic<std::uint64_t> m_Lod{0};
    std::atomic<std::uint64_t> m_GenerateMicros{0};
    std::atomic<std::uint64_t> m_BrushMicros{0};
    std::atomic<std::uint64_t> m_UpdateMicros{0};
    std::atomic<std::uint64_t> m_GpuUploadMicros{0};
    std::atomic<std::uint64_t> m_MeshRebuildMicros{0};
    std::atomic<std::uint64_t> m_CollisionMicros{0};
    std::atomic<std::uint64_t> m_RenderMicros{0};
    std::atomic<std::uint64_t> m_DirtyChunks{0};
    std::atomic<std::uint64_t> m_VisibleChunks{0};
    std::atomic<std::uint64_t> m_StreamedChunks{0};
    std::atomic<std::uint64_t> m_RenderedChunks{0};
    std::atomic<std::uint64_t> m_DrawCalls{0};
    std::atomic<std::uint64_t> m_MemoryBytes{0};
    std::atomic<std::uint64_t> m_LightingValid{0};
    std::atomic<std::uint64_t> m_CameraSpeedBits{0};
};

} // namespace we::runtime::terrain
