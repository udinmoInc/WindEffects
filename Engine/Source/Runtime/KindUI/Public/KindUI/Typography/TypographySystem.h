// ==============================================================================
// WindEffects — KindUI — TypographySystem
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/TypographySpec.h"

#include <string_view>

namespace we::runtime::kindui {

/// Single central source of truth for all typography definitions across the editor UI.
/// Every text element across themes, controls, and editor panels must query its font family,
/// font size, line height, weight, and neutral color role from TypographySystem.
class KINDUI_API TypographySystem {
public:
    /// Global font family name.
    static constexpr std::string_view kFontFamily = "Roboto";

    /// Standard line height ratio matching Roboto font metrics (32.0 / 24.0).
    static constexpr float kLineHeightRatio = 32.0f / 24.0f;

    /// Cap-height midline ratio for font character vertical centering (matches AutoAlign).
    static constexpr float kCapHeightMidlineRatio = 0.61f;

    /// Visual midline ratio for DrawText top placement (matches AutoAlign::ComputeBaselineY).
    static constexpr float kVisualMidlineRatio = 0.61f;

    /// Resolve complete typography spec for a semantic role.
    [[nodiscard]] static TypographySpec GetSpec(TypographyToken role, float scale = 1.0f);

    /// Resolve unscaled base font size in logical pixels for a semantic role.
    [[nodiscard]] static float GetFontSize(TypographyToken role);

    /// Resolve font weight (400 Regular, 500 Medium, 600 SemiBold) for a semantic role.
    [[nodiscard]] static uint16_t GetFontWeight(TypographyToken role);

    /// Resolve line height in logical pixels for a semantic role.
    [[nodiscard]] static float GetLineHeight(TypographyToken role);

    /// Resolve neutral text color token (TextPrimary vs TextSecondary) for a semantic role.
    [[nodiscard]] static ColorToken GetColorToken(TypographyToken role);

    /// Calculate top Y for context.DrawText so text character midline aligns with centerY.
    [[nodiscard]] static float AlignTextTopAtCenterY(float centerY, float fontSizePx);

    /// Calculate top Y for context.DrawText centered vertically within bounds.
    [[nodiscard]] static float AlignTextTopY(const Rect& bounds, float fontSizePx);
};

} // namespace we::runtime::kindui
