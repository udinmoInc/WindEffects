// ==============================================================================
// WindEffects — KindUI — PopupPositioner
// Implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Layout/PopupPositioner.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace we::runtime::kindui {

namespace {

enum class SingleDirection { Right, Left, Bottom, Top, Center };

struct PositionCandidate {
    Point pos{};
    SingleDirection dir = SingleDirection::Right;
    PopupPlacementMode placementMode = PopupPlacementMode::SidePreferred;
};

// Calculates visible overlap area fraction of popupRect inside viewport
float CalculateVisibleFraction(const Rect& popupRect, const Rect& viewport) {
    const float popupArea = popupRect.width * popupRect.height;
    if (popupArea <= 0.0001f) {
        return 0.0f;
    }

    const float intersectX0 = std::max(popupRect.x, viewport.x);
    const float intersectY0 = std::max(popupRect.y, viewport.y);
    const float intersectX1 = std::min(popupRect.x + popupRect.width, viewport.x + viewport.width);
    const float intersectY1 = std::min(popupRect.y + popupRect.height, viewport.y + viewport.height);

    const float intersectW = std::max(0.0f, intersectX1 - intersectX0);
    const float intersectH = std::max(0.0f, intersectY1 - intersectY0);
    const float intersectArea = intersectW * intersectH;

    return std::clamp(intersectArea / popupArea, 0.0f, 1.0f);
}

// Check if popup fits 100% inside viewport with margin
bool FitsInViewport(const Rect& popupRect, const Rect& viewport, float margin) {
    const Rect paddedViewport{
        viewport.x + margin,
        viewport.y + margin,
        std::max(0.0f, viewport.width - margin * 2.0f),
        std::max(0.0f, viewport.height - margin * 2.0f)
    };
    return popupRect.x >= paddedViewport.x &&
           popupRect.y >= paddedViewport.y &&
           (popupRect.x + popupRect.width) <= (paddedViewport.x + paddedViewport.width) &&
           (popupRect.y + popupRect.height) <= (paddedViewport.y + paddedViewport.height);
}

// Clamps popupRect inside padded viewport
Rect ClampToViewport(const Rect& popupRect, const Rect& viewport, float margin) {
    const float minX = viewport.x + margin;
    const float minY = viewport.y + margin;
    const float maxX = std::max(minX, viewport.x + viewport.width - margin - popupRect.width);
    const float maxY = std::max(minY, viewport.y + viewport.height - margin - popupRect.height);

    Rect result = popupRect;
    result.x = std::clamp(popupRect.x, minX, maxX);
    result.y = std::clamp(popupRect.y, minY, maxY);
    return result;
}

PositionCandidate BuildCandidate(
    SingleDirection dir,
    const Size& popupSize,
    const PopupPlacementOptions& options) {
    const Rect& anchor = options.anchorRect;
    const float gap = options.gap;

    PositionCandidate candidate{};
    candidate.dir = dir;

    switch (dir) {
    case SingleDirection::Right:
        candidate.pos.x = anchor.x + anchor.width + gap;
        candidate.pos.y = anchor.y;
        candidate.placementMode = PopupPlacementMode::SidePreferred;
        break;
    case SingleDirection::Left:
        candidate.pos.x = anchor.x - popupSize.width - gap;
        candidate.pos.y = anchor.y;
        candidate.placementMode = PopupPlacementMode::LeftPreferred;
        break;
    case SingleDirection::Bottom:
        candidate.pos.x = anchor.x;
        candidate.pos.y = anchor.y + anchor.height + gap;
        candidate.placementMode = PopupPlacementMode::BottomPreferred;
        break;
    case SingleDirection::Top:
        candidate.pos.x = anchor.x;
        candidate.pos.y = anchor.y - popupSize.height - gap;
        candidate.placementMode = PopupPlacementMode::TopPreferred;
        break;
    case SingleDirection::Center:
        candidate.pos.x = anchor.x + (anchor.width - popupSize.width) * 0.5f;
        candidate.pos.y = anchor.y + (anchor.height - popupSize.height) * 0.5f;
        candidate.placementMode = PopupPlacementMode::AtPoint;
        break;
    }

    return candidate;
}

} // namespace

PopupPlacementResult PopupPositioner::Calculate(
    const Size& popupSize,
    const PopupPlacementOptions& options) {
    PopupPlacementResult result{};
    result.popupRect = Rect{ 0.0f, 0.0f, popupSize.width, popupSize.height };

    const Rect viewport = (options.viewportBounds.width > 0.0f && options.viewportBounds.height > 0.0f)
        ? options.viewportBounds
        : Rect{ 0.0f, 0.0f, 1920.0f, 1080.0f };

    if (options.mode == PopupPlacementMode::AtPoint) {
        Rect rawRect{ options.anchorRect.x, options.anchorRect.y, popupSize.width, popupSize.height };
        Rect clamped = ClampToViewport(rawRect, viewport, options.viewportMargin);
        result.position = Point{ clamped.x, clamped.y };
        result.popupRect = clamped;
        result.chosenMode = PopupPlacementMode::AtPoint;
        result.visibleFraction = CalculateVisibleFraction(clamped, viewport);
        return result;
    }

    // Determine candidate direction evaluation order based on requested mode
    std::array<SingleDirection, 4> dirOrder{};
    switch (options.mode) {
    case PopupPlacementMode::SidePreferred:
        dirOrder = { SingleDirection::Right, SingleDirection::Left, SingleDirection::Bottom, SingleDirection::Top };
        break;
    case PopupPlacementMode::BottomPreferred:
        dirOrder = { SingleDirection::Bottom, SingleDirection::Top, SingleDirection::Right, SingleDirection::Left };
        break;
    case PopupPlacementMode::TopPreferred:
        dirOrder = { SingleDirection::Top, SingleDirection::Bottom, SingleDirection::Right, SingleDirection::Left };
        break;
    case PopupPlacementMode::LeftPreferred:
        dirOrder = { SingleDirection::Left, SingleDirection::Right, SingleDirection::Bottom, SingleDirection::Top };
        break;
    default:
        dirOrder = { SingleDirection::Right, SingleDirection::Left, SingleDirection::Bottom, SingleDirection::Top };
        break;
    }

    // First pass: check if candidate fits 100% inside viewport
    for (const SingleDirection dir : dirOrder) {
        PositionCandidate cand = BuildCandidate(dir, popupSize, options);
        Rect candRect{ cand.pos.x, cand.pos.y, popupSize.width, popupSize.height };

        // For Side/Left placements, align top with anchor but clamp vertically inside viewport
        if (dir == SingleDirection::Right || dir == SingleDirection::Left) {
            const float minY = viewport.y + options.viewportMargin;
            const float maxY = std::max(minY, viewport.y + viewport.height - options.viewportMargin - popupSize.height);
            candRect.y = std::clamp(candRect.y, minY, maxY);
        }
        // For Top/Bottom placements, align left with anchor but clamp horizontally inside viewport
        else if (dir == SingleDirection::Top || dir == SingleDirection::Bottom) {
            const float minX = viewport.x + options.viewportMargin;
            const float maxX = std::max(minX, viewport.x + viewport.width - options.viewportMargin - popupSize.width);
            candRect.x = std::clamp(candRect.x, minX, maxX);
        }

        if (FitsInViewport(candRect, viewport, options.viewportMargin)) {
            result.position = Point{ candRect.x, candRect.y };
            result.popupRect = candRect;
            result.chosenMode = cand.placementMode;
            result.visibleFraction = 1.0f;
            return result;
        }
    }

    // Second pass: if none fit 100% without overlap, pick candidate with maximum visible fraction
    float bestFraction = -1.0f;
    PositionCandidate bestCandidate{};
    Rect bestRect{};

    for (const SingleDirection dir : dirOrder) {
        PositionCandidate cand = BuildCandidate(dir, popupSize, options);
        Rect candRect{ cand.pos.x, cand.pos.y, popupSize.width, popupSize.height };
        candRect = ClampToViewport(candRect, viewport, options.viewportMargin);

        const float fraction = CalculateVisibleFraction(candRect, viewport);
        if (fraction > bestFraction) {
            bestFraction = fraction;
            bestCandidate = cand;
            bestRect = candRect;
        }
    }

    result.position = Point{ bestRect.x, bestRect.y };
    result.popupRect = bestRect;
    result.chosenMode = bestCandidate.placementMode;
    result.visibleFraction = bestFraction;
    return result;
}

} // namespace we::runtime::kindui
