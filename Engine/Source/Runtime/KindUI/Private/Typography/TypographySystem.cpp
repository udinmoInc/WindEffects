// ==============================================================================
// WindEffects — KindUI — TypographySystem
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Typography/TypographySystem.h"
#include "KindUI/Layout/AutoAlign.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {

float TypographySystem::GetFontSize(TypographyToken role) {
    switch (role) {
    case TypographyToken::WindowTitle:
    case TypographyToken::Display:
    case TypographyToken::PageTitle:
    case TypographyToken::Heading1:
        return 18.0f;

    case TypographyToken::SectionTitle:
    case TypographyToken::Heading:
    case TypographyToken::Heading2:
    case TypographyToken::Heading3:
    case TypographyToken::CardTitle:
    case TypographyToken::DialogTitle:
        return 14.0f;

    case TypographyToken::Title:
    case TypographyToken::Subtitle:
    case TypographyToken::Body:
    case TypographyToken::BodyStrong:
    case TypographyToken::Heading4:
    case TypographyToken::Heading5:
    case TypographyToken::Heading6:
    case TypographyToken::Label:
    case TypographyToken::Button:
    case TypographyToken::Tab:
    case TypographyToken::Toolbar:
    case TypographyToken::Menu:
    case TypographyToken::Navigation:
    case TypographyToken::PropertyLabel:
    case TypographyToken::PropertyValue:
    case TypographyToken::Code:
    case TypographyToken::Console:
    case TypographyToken::Monospace:
    case TypographyToken::TableHeader:
    case TypographyToken::Link:
        return 13.0f;

    case TypographyToken::Caption:
    case TypographyToken::CaptionSmall:
    case TypographyToken::Hint:
    case TypographyToken::Tooltip:
    case TypographyToken::Status:
    case TypographyToken::StatusBar:
    case TypographyToken::Error:
    case TypographyToken::Warning:
    case TypographyToken::Success:
    case TypographyToken::Disabled:
    default:
        return 12.0f;
    }
}

uint16_t TypographySystem::GetFontWeight(TypographyToken role) {
    switch (role) {
    case TypographyToken::WindowTitle:
    case TypographyToken::Display:
    case TypographyToken::PageTitle:
    case TypographyToken::Heading1:
        return 600; // SemiBold for display headers

    default:
        return 400; // Regular for all standard content, tabs, selectors, buttons, labels, property values
    }
}

float TypographySystem::GetLineHeight(TypographyToken role) {
    const float fontSize = GetFontSize(role);
    switch (role) {
    case TypographyToken::WindowTitle:
    case TypographyToken::Display:
    case TypographyToken::PageTitle:
    case TypographyToken::Heading1:
        return std::round(fontSize * 1.25f);
    default:
        return std::round(fontSize * kLineHeightRatio);
    }
}

ColorToken TypographySystem::GetColorToken(TypographyToken role) {
    switch (role) {
    case TypographyToken::Link:
        return ColorToken::LinkForeground;
    case TypographyToken::Error:
        return ColorToken::ErrorForeground;
    case TypographyToken::Warning:
        return ColorToken::Warning;
    case TypographyToken::Success:
        return ColorToken::Success;
    case TypographyToken::Disabled:
        return ColorToken::TextDisabled;

    // Neutral Secondary hierarchy
    case TypographyToken::Subtitle:
    case TypographyToken::Status:
    case TypographyToken::StatusBar:
    case TypographyToken::PropertyLabel:
    case TypographyToken::Toolbar:
    case TypographyToken::Caption:
    case TypographyToken::Hint:
    case TypographyToken::CaptionSmall:
    case TypographyToken::Tooltip:
        return ColorToken::TextSecondary;

    // Neutral Primary hierarchy
    case TypographyToken::Label:
    case TypographyToken::Body:
    case TypographyToken::BodyStrong:
    case TypographyToken::Title:
    case TypographyToken::SectionTitle:
    case TypographyToken::PageTitle:
    case TypographyToken::WindowTitle:
    case TypographyToken::Display:
    case TypographyToken::Heading:
    case TypographyToken::Heading1:
    case TypographyToken::Heading2:
    case TypographyToken::Heading3:
    case TypographyToken::Heading4:
    case TypographyToken::Heading5:
    case TypographyToken::Heading6:
    case TypographyToken::Button:
    case TypographyToken::Menu:
    case TypographyToken::Navigation:
    case TypographyToken::PropertyValue:
    case TypographyToken::Code:
    case TypographyToken::Console:
    case TypographyToken::Monospace:
    case TypographyToken::TableHeader:
    default:
        return ColorToken::TextPrimary;
    }
}

TypographySpec TypographySystem::GetSpec(TypographyToken role, float scale) {
    TypographySpec spec;
    spec.role = role;
    spec.sizePx = GetFontSize(role) * (std::max)(1.0f, scale);
    spec.lineHeightPx = GetLineHeight(role) * (std::max)(1.0f, scale);
    spec.letterSpacing = 0.0f;
    spec.weight = GetFontWeight(role);
    spec.bold = spec.weight >= 600;
    spec.italic = false;
    spec.color = ResolveColor(GetColorToken(role));
    return spec;
}

float TypographySystem::AlignTextTopAtCenterY(float centerY, float fontSizePx) {
    return AutoAlign::AlignTextTopAtCenterY(centerY, fontSizePx);
}

float TypographySystem::AlignTextTopY(const Rect& bounds, float fontSizePx) {
    return AutoAlign::AlignTextTopY(bounds, fontSizePx);
}

} // namespace we::runtime::kindui
