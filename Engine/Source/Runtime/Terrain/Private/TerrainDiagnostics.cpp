#include "Terrain/TerrainDiagnostics.h"

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
    m_VisibleChunks = 0;
    m_StreamedChunks = 0;
    m_RenderedChunks = 0;
    m_DrawCalls = 0;
    m_MemoryBytes = 0;
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
    s.visibleChunks = m_VisibleChunks.load();
    s.streamedChunks = m_StreamedChunks.load();
    s.renderedChunks = m_RenderedChunks.load();
    s.drawCalls = m_DrawCalls.load();
    s.memoryBytes = m_MemoryBytes.load();
    return s;
}

std::string TerrainDiagnostics::FormatSummary() const {
    const auto s = Snapshot();
    std::ostringstream oss;
    oss << "Terrain diagnostics:"
        << " created=" << s.landscapesCreated
        << " alive=" << s.landscapesAlive
        << " update_us=" << s.updateMicros
        << " brush_us=" << s.brushMicros
        << " gpu_us=" << s.gpuUploadMicros
        << " chunks_rebuilt=" << s.chunksRebuilt
        << " collision=" << s.collisionRebuilds
        << " lod=" << s.lodUpdates
        << " visible=" << s.visibleChunks
        << " streamed=" << s.streamedChunks
        << " rendered=" << s.renderedChunks
        << " draws=" << s.drawCalls
        << " mem_bytes=" << s.memoryBytes;
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
    m_GenerateMicros += micros;
}

void TerrainDiagnostics::OnHeightmapImported() noexcept { ++m_Imports; }
void TerrainDiagnostics::OnHeightmapExported() noexcept { ++m_Exports; }

void TerrainDiagnostics::OnBrush(std::uint64_t micros) noexcept {
    ++m_Brush;
    m_BrushMicros += micros;
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
