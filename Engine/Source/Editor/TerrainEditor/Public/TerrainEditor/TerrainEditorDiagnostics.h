// ==============================================================================
// WindEffects — TerrainEditor — TerrainEditorDiagnostics
// Public API surface for the TerrainEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "TerrainEditor/Export.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace we::editor::terrain {

struct TERRAINEDITOR_API TerrainEditorDiagnosticsSnapshot {
    std::uint64_t landscapesCreated = 0;
    std::uint64_t heightmapsImported = 0;
    std::uint64_t heightmapsExported = 0;
    std::uint64_t brushStrokes = 0;
    std::uint64_t brushSamples = 0;
    std::uint64_t undoTransactions = 0;
    std::uint64_t modeEnters = 0;
};

class TERRAINEDITOR_API TerrainEditorDiagnostics {
public:
    static TerrainEditorDiagnostics& Get() noexcept;
    void Reset() noexcept;
    [[nodiscard]] TerrainEditorDiagnosticsSnapshot Snapshot() const noexcept;
    [[nodiscard]] std::string FormatSummary() const;

    void OnLandscapeCreated() noexcept;
    void OnHeightmapImported() noexcept;
    void OnHeightmapExported() noexcept;
    void OnBrushStroke() noexcept;
    void OnBrushSample() noexcept;
    void OnUndoTransaction() noexcept;
    void OnModeEnter() noexcept;

private:
    std::atomic<std::uint64_t> m_Created{0};
    std::atomic<std::uint64_t> m_Imported{0};
    std::atomic<std::uint64_t> m_Exported{0};
    std::atomic<std::uint64_t> m_Strokes{0};
    std::atomic<std::uint64_t> m_Samples{0};
    std::atomic<std::uint64_t> m_Undo{0};
    std::atomic<std::uint64_t> m_ModeEnters{0};
};

} // namespace we::editor::terrain
