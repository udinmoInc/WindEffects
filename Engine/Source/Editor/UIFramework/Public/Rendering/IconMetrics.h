#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "Core/Geometry.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"

#include <cstdint>
#include <string_view>

namespace WindEffects::Editor::UI::IconMetrics {

// Native atlas tiers — icons are only ever drawn at these pixel sizes.
constexpr uint32_t kAtlasTiers[] = {12, 16, 20, 24, 32, 48, 64};
constexpr uint32_t kAtlasTierCount = 7;
constexpr uint32_t kCompactTierPx = 12; // compact layout size (dock-tab close, dropdown chevrons)
constexpr uint32_t kCompactSourceTierPx = 16; // atlas tier used for compact glyphs
constexpr uint32_t kMinTierPx = 16;
constexpr uint32_t kMaxTierPx = 64;

constexpr float kSizeCompact = 16.0f;
constexpr float kSizeSmall = 20.0f;
constexpr float kSizeMedium = 24.0f;
constexpr float kSizeLarge = 32.0f;
constexpr float kSizeXLarge = 48.0f;

// Snap any requested size to the nearest atlas tier (ties prefer the smaller tier).
UIFRAMEWORK_API uint32_t SnapToAtlasTier(uint32_t requestedPx);
UIFRAMEWORK_API uint32_t SnapToAtlasTier(float requestedPx);

// True when px is exactly one of the atlas tiers.
UIFRAMEWORK_API bool IsAtlasTier(uint32_t px);

// Resolve a layout float to an integer atlas tier (no resampling, no fractional sizes).
UIFRAMEWORK_API uint32_t NativeIconTierPx(float requestedPx);
UIFRAMEWORK_API uint32_t NativeIconTierPx(uint32_t requestedPx);

// Legacy clamp — result is always snapped to a tier.
UIFRAMEWORK_API float ClampDisplaySize(float sizePx);
UIFRAMEWORK_API uint32_t ClampDisplaySizePx(float sizePx);
UIFRAMEWORK_API uint32_t ClampDisplaySizePx(uint32_t sizePx);

// Alias: returns the snapped tier for the requested display size (never resamples).
UIFRAMEWORK_API uint32_t RasterSizeForDisplay(uint32_t displaySizePx, bool highDetail = false);

// Snap a UI-space coordinate to the nearest integer pixel.
UIFRAMEWORK_API float SnapPx(float value);

// Standard toolbar / panel glyph tier (never DPI-scaled).
UIFRAMEWORK_API uint32_t StandardGlyphTierPx();
UIFRAMEWORK_API uint32_t GlyphTierPx(ThemeToken role);

// Compact layout (12px) — dock-tab close and dropdown chevrons only.
UIFRAMEWORK_API uint32_t CompactGlyphTierPx();
UIFRAMEWORK_API float CompactDisplayPx();
UIFRAMEWORK_API uint32_t CompactSourceTierPx();
UIFRAMEWORK_API bool IsChevronIcon(std::string_view resolvedIconName);
UIFRAMEWORK_API uint32_t TierPxForIcon(std::string_view resolvedIconName, float requestedPx);

// Square icon-button hit area (DPI-scaled control chrome only).
UIFRAMEWORK_API float IconButtonHitPx(float uiScale);

// Padding between hit area edge and native-tier glyph: (hit - tier) / 2.
UIFRAMEWORK_API float IconContentPaddingPx(float uiScale);

// Place a native-tier glyph centered inside a control bounds (layout only; no artwork changes).
UIFRAMEWORK_API Rect PlaceGlyphCentered(const Rect& controlBounds, uint32_t tierPx);
UIFRAMEWORK_API Rect PlaceGlyphCentered(const Rect& controlBounds, float logicalTierPx);

} // namespace WindEffects::Editor::UI::IconMetrics
