// ==============================================================================
// WindEffects — KindUI — PopupPositioner
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"

namespace we::runtime::kindui {

/// Placement mode preference for contextual popups/popovers.
enum class PopupPlacementMode {
    /// Try Right side of anchor, flip to Left, then Below, then Above.
    SidePreferred,
    /// Try Below anchor, flip to Above, then Right, then Left.
    BottomPreferred,
    /// Try Above anchor, flip to Below, then Right, then Left.
    TopPreferred,
    /// Try Left side of anchor, flip to Right, then Below, then Above.
    LeftPreferred,
    /// Exact target point placement clamped inside viewport bounds.
    AtPoint
};

/// Parameters for contextual popup placement.
struct PopupPlacementOptions {
    /// Rect of the anchor widget/control in window/viewport coordinate space.
    Rect anchorRect{};
    /// Preferred placement mode relative to the anchor.
    PopupPlacementMode mode = PopupPlacementMode::SidePreferred;
    /// Gap in pixels between the anchor edge and the popup edge.
    float gap = 4.0f;
    /// Safety padding margin from the screen/viewport boundaries.
    float viewportMargin = 8.0f;
    /// Optional explicit viewport bounds (if zero/empty, uses default screen bounds).
    Rect viewportBounds{};
};

/// Result of popup placement calculation.
struct PopupPlacementResult {
    /// Final top-left position for arranging the popup in overlay space.
    Point position{};
    /// Computed target rect of the popup.
    Rect popupRect{};
    /// Final direction chosen for placement (Right, Left, Bottom, Top, AtPoint).
    PopupPlacementMode chosenMode = PopupPlacementMode::SidePreferred;
    /// Fraction of popup area that remains visible within the viewport [0..1].
    float visibleFraction = 1.0f;
};

class KINDUI_API PopupPositioner {
public:
    /// Calculates the optimal top-left position for a popup of size `popupSize`
    /// relative to an `anchorRect` within `viewportBounds`.
    [[nodiscard]] static PopupPlacementResult Calculate(
        const Size& popupSize,
        const PopupPlacementOptions& options);
};

} // namespace we::runtime::kindui
