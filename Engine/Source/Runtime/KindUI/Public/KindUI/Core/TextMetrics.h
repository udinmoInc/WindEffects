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
#include <string>
#include <string_view>

namespace we::runtime::kindui {

enum class TruncateMode {
    End,
    Middle,
};

struct FontMetricsSpec {
    float ascender = 0.0f;
    float descender = 0.0f;
    float capHeight = 0.0f;
    float xHeight = 0.0f;
    float lineHeight = 0.0f;
};

/// Shared text measurement and truncation helpers. All widgets use these instead of ad-hoc heuristics.
struct KINDUI_API TextMetrics {
    using MeasureFn = std::function<float(std::string_view text, float fontSize, bool bold)>;

    static void SetMeasureProvider(MeasureFn provider);
    static void ClearCache();

    [[nodiscard]] static FontMetricsSpec GetFontMetrics(float fontSize);
    [[nodiscard]] static float MeasureWidth(std::string_view text, float fontSize, bool bold = false);

    /// Redirects to MeasureWidth for backward compatibility.
    [[nodiscard]] static float EstimateWidth(std::string_view text, float fontSize);

    [[nodiscard]] static float CharWidth(float fontSize);

    /// Truncates text to fit within maxWidth.
    /// If mode is TruncateMode::Middle, preserves beginning and meaningful file extension (e.g. "M_Sky_Def...mat").
    [[nodiscard]] static std::string TruncateText(
        std::string_view text,
        float maxWidth,
        float fontSize,
        bool bold = false,
        TruncateMode mode = TruncateMode::Middle);
};

}
