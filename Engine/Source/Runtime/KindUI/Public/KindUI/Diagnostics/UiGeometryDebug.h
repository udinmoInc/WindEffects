// ==============================================================================
// WindEffects — KindUI — UiGeometryDebug
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Core/Types.h"
#include "KindUI/Export.h"

#include <cstdint>
#include <string>

namespace we::runtime::kindui {

// Runtime geometry audit — enable with WE_UI_GEOMETRY_DEBUG=1
class KINDUI_API UiGeometryDebug {
public:
    struct RegionSample {
        std::string name;
        std::string parent;
        Rect bounds{};
        float paddingH = 0.0f;
        float paddingV = 0.0f;
        float fontSize = 0.0f;
        float iconSize = 0.0f;
        float dpiScale = 1.0f;
    };

    static UiGeometryDebug& Get();

    [[nodiscard]] static bool IsEnabled();

    void BeginFrame();
    void EndFrame();

    void TraceRegion(const RegionSample& sample);
    void TraceRegion(
        const char* name,
        const Rect& bounds,
        const char* parent = nullptr,
        float paddingH = 0.0f,
        float paddingV = 0.0f,
        float fontSize = 0.0f,
        float iconSize = 0.0f);

    void ReportClippedControl(const char* widget, const char* reason);
    void ReportBelowMinHitTarget(const char* widget, float width, float height, float minSize);

private:
    UiGeometryDebug() = default;

    uint64_t m_Frame = 0;
    bool m_LoggedSummary = false;
    uint32_t m_RegionTraces = 0;
    uint32_t m_ClipWarnings = 0;
    uint32_t m_HitTargetWarnings = 0;
};

} // namespace we::runtime::kindui
