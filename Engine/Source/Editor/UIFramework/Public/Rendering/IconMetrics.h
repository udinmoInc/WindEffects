#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <cstdint>

namespace WindEffects::Editor::UI::IconMetrics {

// Native atlas tiers — icons are only ever drawn at these pixel sizes.
constexpr uint32_t kAtlasTiers[] = {16, 20, 24, 32, 48, 64};
constexpr uint32_t kAtlasTierCount = 6;
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

} // namespace WindEffects::Editor::UI::IconMetrics
