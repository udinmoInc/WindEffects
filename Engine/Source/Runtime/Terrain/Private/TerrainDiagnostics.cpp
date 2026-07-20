#include "Terrain/TerrainDiagnostics.h"

#include <cstring>
#include <sstream>

namespace we::runtime::terrain {

TerrainDiagnostics& TerrainDiagnostics::Get() noexcept {
    static TerrainDiagnostics instance;
    return instance;
}

void TerrainDiagnostics::Reset() noexcept {
    m_Created = 0;
    m_Destroyed = 0;
    m_Alive = 0;
    m_Generations = 0;
    m_Imports = 0;
    m_Exports = 0;
    m_Brush = 0;
    m_Chunks = 0;
    m_Collision = 0;
    m_Lod = 0;
    m_GenerateMicros = 0;
    m_BrushMicros = 0;
    m_UpdateMicros = 0;
    m_GpuUploadMicros = 0;
    m_MeshRebuildMicros = 0;
    m_CollisionMicros = 0;
    m_RenderMicros = 0;
    m_DirtyChunks = 0;
    m_VisibleChunks = 0;
    m_StreamedChunks = 0;
    m_RenderedChunks = 0;
    m_DrawCalls = 0;
    m_MemoryBytes = 0;
    m_LightingValid = 0;
    m_CameraSpeedBits = 0;
}

TerrainDiagnosticsSnapshot TerrainDiagnostics::Snapshot() const noexcept {
    TerrainDiagnosticsSnapshot s;
    s.landscapesCreated = m_Created.load();
    s.landscapesDestroyed = m_Destroyed.load();
    s.landscapesAlive = m_Alive.load();
    s.generations = m_Generations.load();
    s.heightmapImports = m_Imports.load();
    s.heightmapExports = m_Exports.load();
    s.brushApplies = m_Brush.load();
    s.chunksRebuilt = m_Chunks.load();
    s.collisionRebuilds = m_Collision.load();
    s.lodUpdates = m_Lod.load();
    s.generateMicros = m_GenerateMicros.load();
    s.brushMicros = m_BrushMicros.load();
    s.updateMicros = m_UpdateMicros.load();
    s.gpuUploadMicros = m_GpuUploadMicros.load();
    s.meshRebuildMicros = m_MeshRebuildMicros.load();
    s.collisionMicros = m_CollisionMicros.load();
    s.renderMicros = m_RenderMicros.load();
    s.dirtyChunks = m_DirtyChunks.load();
    s.visibleChunks = m_VisibleChunks.load();
    s.streamedChunks = m_StreamedChunks.load();
    s.renderedChunks = m_RenderedChunks.load();
    s.drawCalls = m_DrawCalls.load();
    s.memoryBytes = m_MemoryBytes.load();
    s.lightingValid = m_LightingValid.load() != 0;
    const std::uint64_t bits = m_CameraSpeedBits.load();
    float speed = 0.0f;
    std::memcpy(&speed, &bits, sizeof(speed));
    s.cameraSpeed = speed;
    return s;
}

std::string TerrainDiagnostics::FormatSummary() const {
    const auto s = Snapshot();
    std::ostringstream oss;
    oss << "Terrain diagnostics:"
        << " dirty=" << s.dirtyChunks
        << " rebuild_us=" << s.meshRebuildMicros
        << " gpu_us=" << s.gpuUploadMicros
        << " collision_us=" << s.collisionMicros
        << " render_us=" << s.renderMicros
        << " brush_us=" << s.brushMicros
        << " lighting=" << (s.lightingValid ? "ok" : "fallback")
        << " cam_speed=" << s.cameraSpeed
        << " visible=" << s.visibleChunks
        << " rendered=" << s.renderedChunks
        << " draws=" << s.drawCalls
        << " chunks_rebuilt=" << s.chunksRebuilt
        << " collision=" << s.collisionRebuilds;
    return oss.str();
}

void TerrainDiagnostics::OnCreated() noexcept {
    ++m_Created;
    ++m_Alive;
}

void TerrainDiagnostics::OnDestroyed() noexcept {
    ++m_Destroyed;
    if (m_Alive > 0) {
        --m_Alive;
    }
}

void TerrainDiagnostics::OnGenerated(std::uint64_t micros) noexcept {
    ++m_Generations;
    m_GenerateMicros = micros;
}

void TerrainDiagnostics::OnHeightmapImported() noexcept { ++m_Imports; }
void TerrainDiagnostics::OnHeightmapExported() noexcept { ++m_Exports; }

void TerrainDiagnostics::OnBrush(std::uint64_t micros) noexcept {
    ++m_Brush;
    m_BrushMicros = micros;
}

void TerrainDiagnostics::OnChunksRebuilt(std::uint64_t count) noexcept {
    m_Chunks += count;
}

void TerrainDiagnostics::OnCollisionRebuilt(std::uint64_t count) noexcept {
    m_Collision += count;
}

void TerrainDiagnostics::OnLodUpdated(std::uint64_t count) noexcept {
    m_Lod += count;
}

void TerrainDiagnostics::OnUpdate(std::uint64_t micros) noexcept {
    m_UpdateMicros = micros;
}

void TerrainDiagnostics::OnGpuUpload(std::uint64_t micros, std::uint64_t /*chunksUploaded*/) noexcept {
    m_GpuUploadMicros = micros;
}

void TerrainDiagnostics::OnMeshRebuild(std::uint64_t micros, std::uint64_t dirtyCount) noexcept {
    m_MeshRebuildMicros = micros;
    m_DirtyChunks = dirtyCount;
}

void TerrainDiagnostics::OnCollisionTiming(std::uint64_t micros) noexcept {
    m_CollisionMicros = micros;
}

void TerrainDiagnostics::OnRender(std::uint64_t micros, bool lightingValid) noexcept {
    m_RenderMicros = micros;
    m_LightingValid = lightingValid ? 1ull : 0ull;
}

void TerrainDiagnostics::OnDirtyChunks(std::uint64_t count) noexcept {
    m_DirtyChunks = count;
}

void TerrainDiagnostics::SetCameraSpeed(float speed) noexcept {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &speed, sizeof(speed));
    m_CameraSpeedBits = bits;
}

void TerrainDiagnostics::OnFrameRenderStats(
    std::uint64_t visible,
    std::uint64_t streamed,
    std::uint64_t rendered,
    std::uint64_t drawCalls,
    std::uint64_t memoryBytes) noexcept
{
    m_VisibleChunks = visible;
    m_StreamedChunks = streamed;
    m_RenderedChunks = rendered;
    m_DrawCalls = drawCalls;
    m_MemoryBytes = memoryBytes;
}

} // namespace we::runtime::terrain
