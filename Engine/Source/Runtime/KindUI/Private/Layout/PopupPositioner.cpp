// ==============================================================================
// WindEffects — KindUI — PopupPositioner
// Implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/UI/PopupPositioner.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace we::runtime::kindui {

namespace {

enum class SingleDirection { Right, Left, Bottom, Top, Center };

struct PositionCandidate {
    Point pos{};
    SingleDirection dir = SingleDirection::Right;
    PopupPlacementMode placementMode = PopupPlacementMode::BottomPreferred;
};

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
    return std::clamp((intersectW * intersectH) / popupArea, 0.0f, 1.0f);
}

Rect PaddedViewport(const Rect& viewport, float margin) {
    return Rect{
        viewport.x + margin,
        viewport.y + margin,
        std::max(0.0f, viewport.width - margin * 2.0f),
        std::max(0.0f, viewport.height - margin * 2.0f)
    };
}

bool FitsInViewport(const Rect& popupRect, const Rect& viewport, float margin) {
    const Rect padded = PaddedViewport(viewport, margin);
    return popupRect.x >= padded.x &&
           popupRect.y >= padded.y &&
           (popupRect.x + popupRect.width) <= (padded.x + padded.width) + 0.01f &&
           (popupRect.y + popupRect.height) <= (padded.y + padded.height) + 0.01f;
}

Size ClampSizeToViewport(const Size& popupSize, const Rect& viewport, float margin) {
    const Rect padded = PaddedViewport(viewport, margin);
    return Size{
        std::clamp(popupSize.width, 0.0f, std::max(0.0f, padded.width)),
        std::clamp(popupSize.height, 0.0f, std::max(0.0f, padded.height))
    };
}

Rect ClampToViewport(const Rect& popupRect, const Rect& viewport, float margin) {
    const Size clampedSize = ClampSizeToViewport(
        Size{ popupRect.width, popupRect.height }, viewport, margin);
    const float minX = viewport.x + margin;
    const float minY = viewport.y + margin;
    const float maxX = std::max(minX, viewport.x + viewport.width - margin - clampedSize.width);
    const float maxY = std::max(minY, viewport.y + viewport.height - margin - clampedSize.height);

    return Rect{
        std::clamp(popupRect.x, minX, maxX),
        std::clamp(popupRect.y, minY, maxY),
        clampedSize.width,
        clampedSize.height
    };
}

void AlignAlongFreeAxis(
    Rect& candRect,
    SingleDirection dir,
    const Rect& viewport,
    float margin,
    float popupW,
    float popupH) {
    if (dir == SingleDirection::Right || dir == SingleDirection::Left) {
        const float minY = viewport.y + margin;
        const float maxY = std::max(minY, viewport.y + viewport.height - margin - popupH);
        candRect.y = std::clamp(candRect.y, minY, maxY);
    } else if (dir == SingleDirection::Top || dir == SingleDirection::Bottom) {
        const float minX = viewport.x + margin;
        const float maxX = std::max(minX, viewport.x + viewport.width - margin - popupW);
        candRect.x = std::clamp(candRect.x, minX, maxX);
    }
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

PopupPlacementMode PreferredModeOf(SingleDirection dir) {
    switch (dir) {
    case SingleDirection::Right: return PopupPlacementMode::SidePreferred;
    case SingleDirection::Left: return PopupPlacementMode::LeftPreferred;
    case SingleDirection::Bottom: return PopupPlacementMode::BottomPreferred;
    case SingleDirection::Top: return PopupPlacementMode::TopPreferred;
    case SingleDirection::Center: return PopupPlacementMode::AtPoint;
    }
    return PopupPlacementMode::BottomPreferred;
}

bool IsFlipOf(PopupPlacementMode preferred, SingleDirection chosen) {
    switch (preferred) {
    case PopupPlacementMode::BottomPreferred:
        return chosen == SingleDirection::Top;
    case PopupPlacementMode::TopPreferred:
        return chosen == SingleDirection::Bottom;
    case PopupPlacementMode::SidePreferred:
        return chosen == SingleDirection::Left;
    case PopupPlacementMode::LeftPreferred:
        return chosen == SingleDirection::Right;
    case PopupPlacementMode::AtPoint:
        return false;
    }
    return false;
}

} // namespace

PopupPlacementResult PopupPositioner::Calculate(
    const Size& popupSize,
    const PopupPlacementOptions& options) {
    PopupPlacementResult result{};

    const Rect viewport = (options.viewportBounds.width > 0.0f && options.viewportBounds.height > 0.0f)
        ? options.viewportBounds
        : Rect{ 0.0f, 0.0f, 0.0f, 0.0f };

    const Size fitted = ClampSizeToViewport(popupSize, viewport, options.viewportMargin);
    result.popupRect = Rect{ 0.0f, 0.0f, fitted.width, fitted.height };

    if (viewport.width <= 0.0f || viewport.height <= 0.0f || fitted.width <= 0.0f || fitted.height <= 0.0f) {
        result.position = Point{ options.anchorRect.x, options.anchorRect.y };
        result.popupRect.x = result.position.x;
        result.popupRect.y = result.position.y;
        result.chosenMode = options.mode;
        result.visibleFraction = 0.0f;
        return result;
    }

    if (options.mode == PopupPlacementMode::AtPoint) {
        Rect rawRect{ options.anchorRect.x, options.anchorRect.y, fitted.width, fitted.height };
        Rect clamped = ClampToViewport(rawRect, viewport, options.viewportMargin);
        result.position = Point{ clamped.x, clamped.y };
        result.popupRect = clamped;
        result.chosenMode = PopupPlacementMode::AtPoint;
        result.visibleFraction = CalculateVisibleFraction(clamped, viewport);
        return result;
    }

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
        dirOrder = { SingleDirection::Bottom, SingleDirection::Top, SingleDirection::Right, SingleDirection::Left };
        break;
    }

    for (const SingleDirection dir : dirOrder) {
        PositionCandidate cand = BuildCandidate(dir, fitted, options);
        Rect candRect{ cand.pos.x, cand.pos.y, fitted.width, fitted.height };
        AlignAlongFreeAxis(candRect, dir, viewport, options.viewportMargin, fitted.width, fitted.height);

        if (FitsInViewport(candRect, viewport, options.viewportMargin)) {
            result.position = Point{ candRect.x, candRect.y };
            result.popupRect = candRect;
            result.chosenMode = PreferredModeOf(dir);
            result.visibleFraction = 1.0f;
            result.flipped = IsFlipOf(options.mode, dir);
            return result;
        }
    }

    float bestFraction = -1.0f;
    PositionCandidate bestCandidate{};
    Rect bestRect{};
    SingleDirection bestDir = dirOrder[0];

    for (const SingleDirection dir : dirOrder) {
        PositionCandidate cand = BuildCandidate(dir, fitted, options);
        Rect candRect{ cand.pos.x, cand.pos.y, fitted.width, fitted.height };
        candRect = ClampToViewport(candRect, viewport, options.viewportMargin);

        const float fraction = CalculateVisibleFraction(candRect, viewport);
        if (fraction > bestFraction) {
            bestFraction = fraction;
            bestCandidate = cand;
            bestRect = candRect;
            bestDir = dir;
        }
    }

    result.position = Point{ bestRect.x, bestRect.y };
    result.popupRect = bestRect;
    result.chosenMode = PreferredModeOf(bestDir);
    result.visibleFraction = bestFraction;
    result.flipped = IsFlipOf(options.mode, bestDir);
    (void)bestCandidate;
    return result;
}

} // namespace we::runtime::kindui
