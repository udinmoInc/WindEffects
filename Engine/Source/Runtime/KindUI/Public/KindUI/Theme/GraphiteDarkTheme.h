// ==============================================================================
// WindEffects — KindUI — GraphiteDarkTheme
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theme/IKindUITheme.h"

namespace we::runtime::kindui {

/// Canonical token → palette/metrics theme. Product themes subclass for deltas only.
class KINDUI_API GraphiteDarkTheme : public IKindUITheme {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "GraphiteDark"; }

    [[nodiscard]] Color ResolveColor(ColorToken token) const override;
    [[nodiscard]] float ResolveMetric(MetricToken token) const override;
    [[nodiscard]] Margin ResolvePadding(PaddingToken token) const override;
    [[nodiscard]] float ResolveSpacing(SpacingToken token) const override;
    [[nodiscard]] float ResolveRadius(RadiusToken token) const override;
    [[nodiscard]] float ResolveFontSize(TypographyToken token) const override;
    [[nodiscard]] int ResolveElevation(ElevationToken token) const override;
    [[nodiscard]] float ResolveAnimationDuration(AnimationToken token) const override;
};

} // namespace we::runtime::kindui
