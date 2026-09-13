// ==============================================================================
// WindEffects — KindUI — UiPathDiagnostics
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace we::runtime::kindui {

/// Per-frame UI path counters (layout/paint/invalidation). Enable with WE_UI_PATH_DIAG=1.
struct KINDUI_API UiPathFrameSample {
    uint32_t layoutPasses = 0;
    uint32_t paintPasses = 0;
    uint32_t invalidateCount = 0;
    uint32_t widgetsVisited = 0;
    uint32_t paintCommands = 0;
    uint32_t geometryVertices = 0;
    uint64_t layoutInvalidations = 0;
    uint64_t paintInvalidations = 0;
};

class KINDUI_API UiPathDiagnostics {
public:
    static UiPathDiagnostics& Get();

    static bool IsEnabled();
    static void SetBenchmarkActive(bool active) noexcept;

    void BeginFrame();
    void EndFrame();

    void OnLayoutPass() noexcept;
    void OnPaintPass() noexcept;
    void OnLayoutInvalidation() noexcept;
    void OnPaintInvalidation() noexcept;

    void SetWidgetsVisited(uint32_t count) noexcept;
    void SetPaintCommands(uint32_t count) noexcept;
    void SetGeometryVertices(uint32_t count) noexcept;

    [[nodiscard]] const UiPathFrameSample& Last() const noexcept { return m_Last; }
    [[nodiscard]] const UiPathFrameSample& Peak() const noexcept { return m_Peak; }

    void ResetPeak() noexcept;

private:
    UiPathDiagnostics() = default;

    UiPathFrameSample m_Last{};
    UiPathFrameSample m_Peak{};
};

/// RAII scope tag for interaction benchmark attribution.
class KINDUI_API UiPathScope {
public:
    explicit UiPathScope(const char* label);
    ~UiPathScope();

    [[nodiscard]] double ElapsedMs() const;

    uint32_t layoutPassesStart = 0;
    uint32_t paintPassesStart = 0;
    uint32_t invalidateStart = 0;
    uint64_t layoutInvStart = 0;
    uint64_t paintInvStart = 0;
    const char* label = nullptr;
    double startMs = 0.0;
};

} // namespace we::runtime::kindui
