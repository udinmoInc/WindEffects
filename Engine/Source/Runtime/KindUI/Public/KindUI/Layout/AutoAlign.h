// ==============================================================================
// WindEffects — KindUI — AutoAlign
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Tokens/DesignToken.h"

#include <string_view>

namespace we::runtime::kindui {

/// Universal layout alignment rules.
enum class AlignRule {
    Start,          // Align to start edge (left in Row, top in Column)
    End,            // Align to end edge
    Center,         // Center along axis
    Stretch,        // Expand to fill cross axis
    Baseline,       // Align font text baselines and control centers
    IconText,       // Normalize icon bounds and align icon glyph center with text cap-height midline
    LabelControl,   // Align property/form labels and input controls sharing visual midline
    GroupCenter     // Calculate combined group width of multi-element controls and center the group
};

namespace Align {
    using Rule = AlignRule;
}

/// Universal rule-based layout engine helper.
struct KINDUI_API AutoAlign {
    /// Calculate exact vertical center Y coordinate for a given content height inside container
    static float ComputeVerticalCenter(const Rect& container, float contentHeight);

    /// Calculate exact horizontal X coordinate for a given content width inside container
    static float ComputeHorizontalAlign(const Rect& container, float contentWidth, HorizontalAlignment align =
        HorizontalAlignment::Left);

    /// Calculate baseline top Y for font text layout so visual cap-height midline aligns with centerY
    static float ComputeBaselineY(float centerY, float fontSizePx);

    /// Calculate top Y for text within container bounds
    static float AlignTextTopY(const Rect& bounds, float fontSizePx);

    /// Calculate top Y for text centered at centerY
    static float AlignTextTopAtCenterY(float centerY, float fontSizePx);

    /// Normalize icon bounds centered in control container (handles 12px, 14px, 16px, 20px icons)
    static Rect NormalizeIconBounds(const Rect& controlBounds, float iconSizePx);

    /// Universal Icon + Text rule layout calculation
    struct IconTextResult {
        Rect iconRect{};
        Point textPos{};
        float totalWidth = 0.0f;
        float totalHeight = 0.0f;
    };
    static IconTextResult ComputeIconTextLayout(
        const Rect& container,
        float iconSizePx,
        bool hasIcon,
        std::string_view text,
        float fontSizePx,
        float gapPx = -1.0f);

    /// Universal Label + Control rule layout calculation
    struct LabelControlResult {
        Point labelPos{};
        Rect controlRect{};
        float labelWidth = 0.0f;
    };
    static LabelControlResult ComputeLabelControlLayout(
        const Rect& container,
        float labelWidth,
        float controlWidth,
        float controlHeight,
        float fontSizePx);

    /// Universal Compound Group Center rule layout calculation (e.g. Button with Icon + Label + Chevron)
    struct GroupCenterResult {
        Rect groupBounds{};
        Rect iconRect{};
        Point textPos{};
        Rect chevronRect{};
        float totalWidth = 0.0f;
    };
    static GroupCenterResult ComputeGroupCenterLayout(
        const Rect& container,
        float iconSizePx,
        bool hasIcon,
        std::string_view text,
        float fontSizePx,
        float chevronSizePx,
        bool hasChevron,
        float gapPx = -1.0f);

    /// Universal Search/Text Input Content rule layout calculation
    struct InputContentResult {
        Rect iconRect{};
        Point textPos{};
        Rect clearBtnRect{};
        float textMaxW = 0.0f;
    };
    static InputContentResult ComputeInputContentLayout(
        const Rect& container,
        float iconSizePx,
        bool hasSearchIcon,
        std::string_view text,
        float fontSizePx,
        bool showClearBtn,
        float padH);

    /// Universal Tree View Row rule layout calculation
    struct TreeRowResult {
        Rect expanderRect{};
        Rect iconRect{};
        Point labelPos{};
        Rect eyeRect{};
        Rect lockRect{};
        float textX = 0.0f;
        float textMaxW = 0.0f;
    };
    static TreeRowResult ComputeTreeRowLayout(
        const Rect& rowBounds,
        float indentWidth,
        int depth,
        bool hasExpander,
        float expanderSizePx,
        bool hasIcon,
        float iconSizePx,
        std::string_view labelText,
        float fontSizePx,
        bool hasEye,
        bool hasLock);
};

} // namespace we::runtime::kindui
