#include "TerrainEditor/TerrainEditorDiagnostics.h"

#include <sstream>

namespace we::editor::terrain {

TerrainEditorDiagnostics& TerrainEditorDiagnostics::Get() noexcept {
    static TerrainEditorDiagnostics instance;
    return instance;
}

void TerrainEditorDiagnostics::Reset() noexcept {
    m_Created.store(0);
    m_Imported.store(0);
    m_Exported.store(0);
    m_Strokes.store(0);
    m_Samples.store(0);
    m_Undo.store(0);
    m_ModeEnters.store(0);
}

TerrainEditorDiagnosticsSnapshot TerrainEditorDiagnostics::Snapshot() const noexcept {
    TerrainEditorDiagnosticsSnapshot s;
    s.landscapesCreated = m_Created.load();
    s.heightmapsImported = m_Imported.load();
    s.heightmapsExported = m_Exported.load();
    s.brushStrokes = m_Strokes.load();
    s.brushSamples = m_Samples.load();
    s.undoTransactions = m_Undo.load();
    s.modeEnters = m_ModeEnters.load();
    return s;
}

std::string TerrainEditorDiagnostics::FormatSummary() const {
    const auto s = Snapshot();
    std::ostringstream oss;
    oss << "TerrainEditor created=" << s.landscapesCreated
        << " import=" << s.heightmapsImported
        << " export=" << s.heightmapsExported
        << " strokes=" << s.brushStrokes
        << " samples=" << s.brushSamples
        << " undo=" << s.undoTransactions
        << " modeEnters=" << s.modeEnters;
    return oss.str();
}

void TerrainEditorDiagnostics::OnLandscapeCreated() noexcept { m_Created.fetch_add(1); }
void TerrainEditorDiagnostics::OnHeightmapImported() noexcept { m_Imported.fetch_add(1); }
void TerrainEditorDiagnostics::OnHeightmapExported() noexcept { m_Exported.fetch_add(1); }
void TerrainEditorDiagnostics::OnBrushStroke() noexcept { m_Strokes.fetch_add(1); }
void TerrainEditorDiagnostics::OnBrushSample() noexcept { m_Samples.fetch_add(1); }
void TerrainEditorDiagnostics::OnUndoTransaction() noexcept { m_Undo.fetch_add(1); }
void TerrainEditorDiagnostics::OnModeEnter() noexcept { m_ModeEnters.fetch_add(1); }

} // namespace we::editor::terrain
