#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"

#include <cmath>
#include <cstdint>

namespace we::runtime::kindui::IconMetrics {

constexpr uint32_t kCompactGlyphPx = 16;

constexpr float kSizeCompact = 16.0f;
constexpr float kSizeSmall = 20.0f;
constexpr float kSizeMedium = 24.0f;
constexpr float kSizeLarge = 32.0f;
constexpr float kSizeXLarge = 48.0f;

KINDUI_API float SnapPx(float value);

KINDUI_API float IconButtonHitPx(float uiScale);
KINDUI_API float IconContentPaddingPx(float uiScale);

KINDUI_API Rect PlaceGlyphCentered(const Rect& controlBounds, uint32_t sizePx);
KINDUI_API Rect PlaceGlyphCentered(const Rect& controlBounds, float logicalSizePx);

[[nodiscard]] KINDUI_API Rect CompactGlyphBand(const Rect& controlBounds, float x);

} // namespace we::runtime::kindui::IconMetrics
