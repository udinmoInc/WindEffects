#include "KindUI/Rendering/IconMetrics.h"

#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <mutex>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::kindui::IconMetrics {
namespace {

std::mutex& TierMutex() {
    static std::mutex mutex;
    return mutex;
}

std::vector<uint32_t>& TierTable() {
    static std::vector<uint32_t> tiers;
    return tiers;
}

std::unordered_map<std::string, std::vector<uint32_t>>& IconTierTable() {
    static std::unordered_map<std::string, std::vector<uint32_t>> tiers;
    return tiers;
}

std::vector<uint32_t> CopyTiers() {
    std::lock_guard lock(TierMutex());
    return TierTable();
}

std::vector<uint32_t> CopyIconTiers(const std::string_view iconName) {
    std::lock_guard lock(TierMutex());
    const auto it = IconTierTable().find(std::string(iconName));
    if (it == IconTierTable().end()) {
        return {};
    }
    return it->second;
}

uint32_t NearestInList(uint32_t requestedPx, const std::span<const uint32_t> tiers) {
    if (tiers.empty()) {
        return std::max(1u, requestedPx);
    }

    const uint32_t minTier = tiers.front();
    const uint32_t maxTier = tiers.back();
    const uint32_t clamped = std::clamp(requestedPx, minTier, maxTier);

    uint32_t bestTier = minTier;
    uint32_t bestDistance = UINT32_MAX;
    for (const uint32_t tier : tiers) {
        const uint32_t distance = tier >= clamped ? tier - clamped : clamped - tier;
        if (distance < bestDistance || (distance == bestDistance && tier < bestTier)) {
            bestDistance = distance;
            bestTier = tier;
        }
    }
    return bestTier;
}

uint32_t NearestTier(uint32_t requestedPx) {
    return NearestInList(requestedPx, CopyTiers());
}

uint32_t TierForMetricToken(MetricToken role) {
    switch (role) {
    case MetricToken::IconSizeSearch:
        return NativeIconTierPx(ResolveMetric(MetricToken::IconSizeSearch));
    case MetricToken::IconSizePrimary:
        return NativeIconTierPx(ResolveMetric(MetricToken::IconSizePrimary));
    case MetricToken::IconSizeTree:
        return NativeIconTierPx(ResolveMetric(MetricToken::IconSizeTree));
    case MetricToken::IconSizeNavigation:
        return NativeIconTierPx(ResolveMetric(MetricToken::IconSizeNavigation));
    case MetricToken::IconSizeToolbar:
    default:
        return NativeIconTierPx(ResolveMetric(MetricToken::IconSizeToolbar));
    }
}

} // namespace

void SetRegisteredAtlasTiers(std::span<const uint32_t> tiers) {
    std::vector<uint32_t> unique;
    unique.reserve(tiers.size());
    for (uint32_t tier : tiers) {
        if (tier == 0) {
            continue;
        }
        unique.push_back(tier);
    }
    std::sort(unique.begin(), unique.end());
    unique.erase(std::unique(unique.begin(), unique.end()), unique.end());

    std::lock_guard lock(TierMutex());
    TierTable() = std::move(unique);
}

std::vector<uint32_t> GetRegisteredAtlasTiers() {
    return CopyTiers();
}

void ClearIconAtlasTiers() {
    std::lock_guard lock(TierMutex());
    IconTierTable().clear();
}

void SetIconAtlasTiers(const std::string_view resolvedIconName, const std::span<const uint32_t> tiers) {
    if (resolvedIconName.empty()) {
        return;
    }

    std::vector<uint32_t> unique;
    unique.reserve(tiers.size());
    for (uint32_t tier : tiers) {
        if (tier == 0) {
            continue;
        }
        unique.push_back(tier);
    }
    std::sort(unique.begin(), unique.end());
    unique.erase(std::unique(unique.begin(), unique.end()), unique.end());

    std::lock_guard lock(TierMutex());
    if (unique.empty()) {
        IconTierTable().erase(std::string(resolvedIconName));
        return;
    }
    IconTierTable()[std::string(resolvedIconName)] = std::move(unique);
}

uint32_t MinRegisteredTierPx() {
    const auto tiers = CopyTiers();
    return tiers.empty() ? kDefaultMinTierPx : tiers.front();
}

uint32_t MaxRegisteredTierPx() {
    const auto tiers = CopyTiers();
    return tiers.empty() ? kDefaultMaxTierPx : tiers.back();
}

uint32_t SnapToTierList(uint32_t requestedPx, const std::span<const uint32_t> tiers) {
    return NearestInList(std::max(1u, requestedPx), tiers);
}

uint32_t SnapToAtlasTier(uint32_t requestedPx) {
    return NearestTier(std::max(1u, requestedPx));
}

uint32_t SnapToAtlasTier(float requestedPx) {
    return SnapToAtlasTier(static_cast<uint32_t>(std::lround(std::max(0.0f, requestedPx))));
}

bool IsAtlasTier(uint32_t px) {
    const auto tiers = CopyTiers();
    return std::binary_search(tiers.begin(), tiers.end(), px);
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
    return GlyphTierPx(MetricToken::IconSizeToolbar);
}

uint32_t GlyphTierPx(MetricToken role) {
    return TierForMetricToken(role);
}

uint32_t CompactGlyphTierPx() {
    const auto tiers = CopyTiers();
    if (std::binary_search(tiers.begin(), tiers.end(), kCompactTierPx)) {
        return kCompactTierPx;
    }
    return tiers.empty() ? kCompactTierPx : SnapToAtlasTier(kCompactTierPx);
}

float CompactDisplayPx() {
    return static_cast<float>(CompactGlyphTierPx());
}

uint32_t CompactSourceTierPx() {
    const auto tiers = CopyTiers();
    if (std::binary_search(tiers.begin(), tiers.end(), kCompactSourceTierPx)) {
        return kCompactSourceTierPx;
    }
    return CompactGlyphTierPx();
}

bool IsChevronIcon(const std::string_view resolvedIconName) {
    return resolvedIconName == "chevrondown"
        || resolvedIconName == "chevronright"
        || resolvedIconName == "chevronleft"
        || resolvedIconName == "chevronup";
}

uint32_t TierPxForIcon(const std::string_view resolvedIconName, const float requestedPx) {
    const uint32_t floored = static_cast<uint32_t>(std::lround(std::max(0.0f, requestedPx)));
    const uint32_t requested = std::max(1u, floored);

    const auto iconTiers = CopyIconTiers(resolvedIconName);
    if (!iconTiers.empty()) {
        return SnapToTierList(requested, iconTiers);
    }
    return SnapToAtlasTier(std::max(requested, MinRegisteredTierPx()));
}

float IconButtonHitPx(float uiScale) {
    return ResolveMetric(MetricToken::IconButtonSize) * std::max(1.0f, uiScale);
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
        return PlaceGlyphCentered(controlBounds, CompactGlyphTierPx());
    }
    return PlaceGlyphCentered(controlBounds, SnapToAtlasTier(logicalTierPx));
}

} // namespace we::runtime::kindui::IconMetrics
