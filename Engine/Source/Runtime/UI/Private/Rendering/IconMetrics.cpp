#include "WindEffects/Runtime/UI/Rendering/IconMetrics.h"

#include "WindEffects/Runtime/UI/Theming/ThemeAccess.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace WindEffects::Editor::UI::IconMetrics {
namespace {

uint32_t NearestTier(uint32_t requestedPx) {
    const uint32_t clamped = std::max(requestedPx, kMinTierPx);
    if (clamped >= kAtlasTiers[kAtlasTierCount - 1]) {
        return kAtlasTiers[kAtlasTierCount - 1];
    }

    uint32_t bestTier = kMinTierPx;
    uint32_t bestDistance = UINT32_MAX;
    for (const uint32_t tier : kAtlasTiers) {
        if (tier < kMinTierPx) {
            continue;
        }
        const uint32_t distance = tier >= clamped ? tier - clamped : clamped - tier;
        if (distance < bestDistance || (distance == bestDistance && tier < bestTier)) {
            bestDistance = distance;
            bestTier = tier;
        }
    }
    return bestTier;
}

uint32_t TierForThemeToken(ThemeToken role) {
    switch (role) {
    case ThemeToken::IconSizeSearch:
        return NativeIconTierPx(ResolveThemeMetric(ThemeToken::IconSizeSearch));
    case ThemeToken::IconSizePrimary:
        return NativeIconTierPx(ResolveThemeMetric(ThemeToken::IconSizePrimary));
    case ThemeToken::IconSizeTree:
        return NativeIconTierPx(ResolveThemeMetric(ThemeToken::IconSizeTree));
    case ThemeToken::IconSizeNavigation:
        return NativeIconTierPx(ResolveThemeMetric(ThemeToken::IconSizeNavigation));
    case ThemeToken::IconSizeToolbar:
    default:
        return NativeIconTierPx(ResolveThemeMetric(ThemeToken::IconSizeToolbar));
    }
}

} // namespace

uint32_t SnapToAtlasTier(uint32_t requestedPx) {
    return NearestTier(std::max(1u, requestedPx));
}

uint32_t SnapToAtlasTier(float requestedPx) {
    return SnapToAtlasTier(static_cast<uint32_t>(std::lround(std::max(0.0f, requestedPx))));
}

bool IsAtlasTier(uint32_t px) {
    for (const uint32_t tier : kAtlasTiers) {
        if (tier == px) {
            return true;
        }
    }
    return false;
}

uint32_t NativeIconTierPx(float requestedPx) {
    return SnapToAtlasTier(requestedPx);
}

uint32_t NativeIconTierPx(uint32_t requestedPx) {
    return SnapToAtlasTier(requestedPx);
}

float ClampDisplaySize(float sizePx) {
    return static_cast<float>(SnapToAtlasTier(sizePx));
}

uint32_t ClampDisplaySizePx(float sizePx) {
    return SnapToAtlasTier(sizePx);
}

uint32_t ClampDisplaySizePx(uint32_t sizePx) {
    return SnapToAtlasTier(sizePx);
}

uint32_t RasterSizeForDisplay(uint32_t displaySizePx, bool highDetail) {
    (void)highDetail;
    return SnapToAtlasTier(displaySizePx);
}

float SnapPx(float value) {
    return std::floor(value + 0.5f);
}

uint32_t StandardGlyphTierPx() {
    return GlyphTierPx(ThemeToken::IconSizeToolbar);
}

uint32_t GlyphTierPx(ThemeToken role) {
    return TierForThemeToken(role);
}

uint32_t CompactGlyphTierPx() {
    return kCompactTierPx;
}

float CompactDisplayPx() {
    return static_cast<float>(kCompactTierPx);
}

uint32_t CompactSourceTierPx() {
    return kCompactSourceTierPx;
}

bool IsChevronIcon(const std::string_view resolvedIconName) {
    return resolvedIconName == "chevrondown"
        || resolvedIconName == "chevronright"
        || resolvedIconName == "chevronleft"
        || resolvedIconName == "chevronup";
}

uint32_t TierPxForIcon(const std::string_view resolvedIconName, const float requestedPx) {
    (void)resolvedIconName;
    return SnapToAtlasTier(std::max(requestedPx, static_cast<float>(kMinTierPx)));
}

float IconButtonHitPx(float uiScale) {
    return ResolveThemeMetric(ThemeToken::IconButtonSize) * std::max(1.0f, uiScale);
}

float IconContentPaddingPx(float uiScale) {
    const float hit = IconButtonHitPx(uiScale);
    const float tier = static_cast<float>(StandardGlyphTierPx());
    return std::max(0.0f, (hit - tier) * 0.5f);
}

Rect PlaceGlyphCentered(const Rect& controlBounds, uint32_t tierPx) {
    const float drawSize = static_cast<float>(tierPx);
    const float x = SnapPx(controlBounds.x + (controlBounds.width - drawSize) * 0.5f);
    const float y = SnapPx(controlBounds.y + (controlBounds.height - drawSize) * 0.5f);
    return Rect{ x, y, drawSize, drawSize };
}

Rect PlaceGlyphCentered(const Rect& controlBounds, float logicalTierPx) {
    const uint32_t rounded = static_cast<uint32_t>(std::lround(std::max(0.0f, logicalTierPx)));
    if (rounded == kCompactTierPx) {
        return PlaceGlyphCentered(controlBounds, kCompactTierPx);
    }
    return PlaceGlyphCentered(controlBounds, SnapToAtlasTier(logicalTierPx));
}

} // namespace WindEffects::Editor::UI::IconMetrics
