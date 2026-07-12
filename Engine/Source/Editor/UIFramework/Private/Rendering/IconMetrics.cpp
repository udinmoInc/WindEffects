#include "Rendering/IconMetrics.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace WindEffects::Editor::UI::IconMetrics {
namespace {

uint32_t NearestTier(uint32_t requestedPx) {
    if (requestedPx <= kAtlasTiers[0]) {
        return kAtlasTiers[0];
    }
    if (requestedPx >= kAtlasTiers[kAtlasTierCount - 1]) {
        return kAtlasTiers[kAtlasTierCount - 1];
    }

    uint32_t bestTier = kAtlasTiers[0];
    uint32_t bestDistance = UINT32_MAX;
    for (const uint32_t tier : kAtlasTiers) {
        const uint32_t distance = tier >= requestedPx ? tier - requestedPx : requestedPx - tier;
        if (distance < bestDistance || (distance == bestDistance && tier < bestTier)) {
            bestDistance = distance;
            bestTier = tier;
        }
    }
    return bestTier;
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

} // namespace WindEffects::Editor::UI::IconMetrics
