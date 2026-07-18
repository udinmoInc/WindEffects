#include "KindUI/Theming/IKindUITheme.h"

namespace we::runtime::kindui {
namespace {

bool IsTitleRole(const TypographyToken token) {
    switch (token) {
    case TypographyToken::WindowTitle:
    case TypographyToken::PageTitle:
    case TypographyToken::SectionTitle:
    case TypographyToken::CardTitle:
    case TypographyToken::DialogTitle:
    case TypographyToken::Display:
    case TypographyToken::Heading1:
    case TypographyToken::Heading2:
    case TypographyToken::Heading3:
    case TypographyToken::Heading4:
    case TypographyToken::Heading:
    case TypographyToken::Title:
        return true;
    default:
        return false;
    }
}

bool IsStrongRole(const TypographyToken token) {
    if (IsTitleRole(token)) {
        return true;
    }
    switch (token) {
    case TypographyToken::BodyStrong:
    case TypographyToken::Button:
    case TypographyToken::TableHeader:
        return true;
    default:
        return false;
    }
}

// Semantic text roles → color tokens (hierarchy, not arbitrary hex).
ColorToken ColorForRole(const TypographyToken token) {
    switch (token) {
    case TypographyToken::Link:
        return ColorToken::TextLink;
    case TypographyToken::Error:
        return ColorToken::ErrorForeground;
    case TypographyToken::Warning:
        return ColorToken::Warning;
    case TypographyToken::Success:
        return ColorToken::Success;
    case TypographyToken::Disabled:
        return ColorToken::TextDisabled;

    // SecondaryText — supporting descriptions at readable contrast
    case TypographyToken::Subtitle:
    case TypographyToken::Status:
    case TypographyToken::StatusBar:
    case TypographyToken::PropertyLabel:
    case TypographyToken::Toolbar:
    case TypographyToken::Menu:
    case TypographyToken::Navigation:
        return ColorToken::TextSecondary;

    // Caption — meta / secondary labels, still clearly readable
    case TypographyToken::Caption:
        return ColorToken::TextCaption;

    // Control labels sit with SecondaryText for scanability
    case TypographyToken::Label:
        return ColorToken::TextSecondary;

    // Hint — placeholders and helper copy (lowest readable rung)
    case TypographyToken::Hint:
    case TypographyToken::CaptionSmall:
    case TypographyToken::Tooltip:
        return ColorToken::TextHint;

    // PrimaryText — titles, body, interactive labels
    default:
        return ColorToken::TextPrimary;
    }
}

} // namespace

TypographySpec IKindUITheme::ResolveTypography(const TypographyToken token) const {
    TypographySpec spec;
    spec.role = token;
    spec.sizePx = ResolveFontSize(token);

    // Vertical rhythm: body ~1.4×; titles tighter; captions slightly open.
    float lhMul = 1.35f;
    if (token == TypographyToken::Body || token == TypographyToken::BodyStrong) {
        lhMul = 1.40f;
    } else if (IsTitleRole(token)) {
        lhMul = 1.25f;
    } else if (token == TypographyToken::Caption
        || token == TypographyToken::CaptionSmall
        || token == TypographyToken::Hint) {
        lhMul = 1.35f;
    }
    spec.lineHeightPx = spec.sizePx * lhMul;

    // Optical tracking: titles slightly tight; hints slightly open.
    if (IsTitleRole(token) && token != TypographyToken::Title) {
        spec.letterSpacing = -0.2f;
    } else if (token == TypographyToken::Hint || token == TypographyToken::CaptionSmall) {
        spec.letterSpacing = 0.15f;
    } else {
        spec.letterSpacing = 0.0f;
    }

    spec.bold = IsStrongRole(token);
    spec.italic = false;
    spec.color = ResolveColor(ColorForRole(token));
    return spec;
}

Color IKindUITheme::InteractiveBackground(float hoverAnim, float pressAnim, bool selected) const {
    if (pressAnim > 0.01f) {
        auto c = ResolveColor(ColorToken::PressedBackground);
        c.a *= pressAnim;
        return c;
    }
    if (hoverAnim > 0.01f) {
        return Color::Transparent().Lerp(ResolveColor(ColorToken::HoverBackground), hoverAnim);
    }
    if (selected) {
        return ResolveColor(ColorToken::SelectedBackground);
    }
    return Color::Transparent();
}

Color IKindUITheme::IconForState(bool hovered, bool active) const {
    if (active) {
        return ResolveColor(ColorToken::IconAccent);
    }
    Color base = ResolveColor(ColorToken::IconSecondary);
    if (hovered) {
        return Color::Lerp(base, ResolveColor(ColorToken::IconPrimary), 1.0f);
    }
    return base;
}

Color IKindUITheme::TextForState(bool hovered, bool active) const {
    return (active || hovered)
        ? ResolveColor(ColorToken::TextPrimary)
        : ResolveColor(ColorToken::TextSecondary);
}

} // namespace we::runtime::kindui
