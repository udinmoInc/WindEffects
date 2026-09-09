// ==============================================================================
// WindEffects — KindUI — UiMetrics
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::runtime::kindui {

/// Centralized interaction thresholds derived from design tokens.
namespace UiMetrics {

[[nodiscard]] inline float DragThresholdPx() {
    return ResolveMetric(MetricToken::DragThreshold);
}

[[nodiscard]] inline float DragThresholdSquared() {
    const float t = DragThresholdPx();
    return t * t;
}

[[nodiscard]] inline bool ExceedsDragThreshold(float dx, float dy) {
    return (dx * dx + dy * dy) >= DragThresholdSquared();
}

[[nodiscard]] inline float MenuPadding() {
    return ResolveMetric(MetricToken::MenuPadding);
}

[[nodiscard]] inline float MenuItemHeight() {
    return ResolveMetric(MetricToken::MenuItemHeight);
}

[[nodiscard]] inline float CheckMarkSize() {
    return ResolveMetric(MetricToken::CheckMarkSize);
}

[[nodiscard]] inline float MenuTextIndent() {
    return ResolveMetric(MetricToken::MenuTextIndent);
}

} // namespace UiMetrics

} // namespace we::runtime::kindui
