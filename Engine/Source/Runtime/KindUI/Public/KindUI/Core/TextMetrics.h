// ==============================================================================
// WindEffects — KindUI — TextMetrics
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <functional>
#include <string_view>

namespace we::runtime::kindui {

/// Shared text measurement helpers. All widgets use these instead of ad-hoc heuristics.
struct KINDUI_API TextMetrics {
    using MeasureFn = std::function<float(std::string_view text, float fontSize, bool bold)>;

    static void SetMeasureProvider(MeasureFn provider);
    static void ClearCache();

    [[nodiscard]] static float MeasureWidth(std::string_view text, float fontSize, bool bold = false);

    /// Redirects to MeasureWidth for backward compatibility.
    [[nodiscard]] static float EstimateWidth(std::string_view text, float fontSize);

    [[nodiscard]] static float CharWidth(float fontSize);
};

} // namespace we::runtime::kindui
