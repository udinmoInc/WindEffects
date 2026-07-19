#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace we::runtime::kindui::IconMetrics {

// Layout semantics (not a closed set of atlas files — those are discovered at runtime).
constexpr uint32_t kCompactTierPx = 12; // compact layout size (dock-tab close, dropdown chevrons)
constexpr uint32_t kCompactSourceTierPx = 16; // preferred atlas tier for compact glyphs when present
constexpr uint32_t kDefaultMinTierPx = 12;
constexpr uint32_t kDefaultMaxTierPx = 256;

constexpr float kSizeCompact = 16.0f;
constexpr float kSizeSmall = 20.0f;
constexpr float kSizeMedium = 24.0f;
constexpr float kSizeLarge = 32.0f;
constexpr float kSizeXLarge = 48.0f;

/// Replace the runtime atlas tier table (sorted ascending). Called when icon meta/atlases load.
KINDUI_API void SetRegisteredAtlasTiers(std::span<const uint32_t> tiers);

/// Currently registered atlas tiers (empty until SetRegisteredAtlasTiers / meta load).
[[nodiscard]] KINDUI_API std::vector<uint32_t> GetRegisteredAtlasTiers();

/// Replace per-icon tier tables. Folder-only packs (13/19/101/…) must not steal snaps for square UI glyphs.
KINDUI_API void ClearIconAtlasTiers();
KINDUI_API void SetIconAtlasTiers(std::string_view resolvedIconName, std::span<const uint32_t> tiers);

[[nodiscard]] KINDUI_API uint32_t MinRegisteredTierPx();
[[nodiscard]] KINDUI_API uint32_t MaxRegisteredTierPx();

// Snap any requested size to the nearest registered atlas tier (ties prefer the smaller tier).
KINDUI_API uint32_t SnapToAtlasTier(uint32_t requestedPx);
KINDUI_API uint32_t SnapToAtlasTier(float requestedPx);
KINDUI_API uint32_t SnapToTierList(uint32_t requestedPx, std::span<const uint32_t> tiers);

// True when px is exactly one of the registered atlas tiers.
KINDUI_API bool IsAtlasTier(uint32_t px);

// Resolve a layout float to an integer atlas tier (no resampling, no fractional sizes).
KINDUI_API uint32_t NativeIconTierPx(float requestedPx);
KINDUI_API uint32_t NativeIconTierPx(uint32_t requestedPx);

// Legacy clamp — result is always snapped to a tier.
KINDUI_API float ClampDisplaySize(float sizePx);
KINDUI_API uint32_t ClampDisplaySizePx(float sizePx);
KINDUI_API uint32_t ClampDisplaySizePx(uint32_t sizePx);

// Alias: returns the snapped tier for the requested display size (never resamples).
KINDUI_API uint32_t RasterSizeForDisplay(uint32_t displaySizePx, bool highDetail = false);

// Snap a UI-space coordinate to the nearest integer pixel.
KINDUI_API float SnapPx(float value);

// Standard toolbar / panel glyph tier (never DPI-scaled).
KINDUI_API uint32_t StandardGlyphTierPx();
KINDUI_API uint32_t GlyphTierPx(MetricToken role);

// Compact layout (12px) — dock-tab close and dropdown chevrons only.
KINDUI_API uint32_t CompactGlyphTierPx();
KINDUI_API float CompactDisplayPx();
KINDUI_API uint32_t CompactSourceTierPx();
KINDUI_API bool IsChevronIcon(std::string_view resolvedIconName);

/// Snap among tiers that actually pack this icon (falls back to global tiers if unknown).
KINDUI_API uint32_t TierPxForIcon(std::string_view resolvedIconName, float requestedPx);

// Square icon-button hit area (DPI-scaled control chrome only).
KINDUI_API float IconButtonHitPx(float uiScale);

// Padding between hit area edge and native-tier glyph: (hit - tier) / 2.
KINDUI_API float IconContentPaddingPx(float uiScale);

// Place a native-tier glyph centered inside a control bounds (layout only; no artwork changes).
KINDUI_API Rect PlaceGlyphCentered(const Rect& controlBounds, uint32_t tierPx);
KINDUI_API Rect PlaceGlyphCentered(const Rect& controlBounds, float logicalTierPx);

} // namespace we::runtime::kindui::IconMetrics
