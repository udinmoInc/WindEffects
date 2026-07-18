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
    case TypographyToken::Button:
    case TypographyToken::TableHeader:
        return true;
    default:
        return false;
    }
}

bool IsMediumRole(const TypographyToken token) {
    switch (token) {
    case TypographyToken::Label:
    case TypographyToken::BodyStrong:
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

    // Secondary / help copy — descriptions, captions, hints
    case TypographyToken::Subtitle:
    case TypographyToken::Status:
    case TypographyToken::StatusBar:
    case TypographyToken::PropertyLabel:
    case TypographyToken::Toolbar:
    case TypographyToken::Menu:
    case TypographyToken::Navigation:
    case TypographyToken::Caption:
    case TypographyToken::Hint:
    case TypographyToken::CaptionSmall:
    case TypographyToken::Tooltip:
        return ColorToken::TextSecondary;

    // Control labels — primary scan weight
    case TypographyToken::Label:
        return ColorToken::TextPrimary;

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

    // Vertical rhythm: body/hints ~1.4×; titles tighter; captions open.
    float lhMul = 1.35f;
    if (token == TypographyToken::Body
        || token == TypographyToken::BodyStrong
        || token == TypographyToken::Hint
        || token == TypographyToken::Caption
        || token == TypographyToken::CaptionSmall
        || token == TypographyToken::Subtitle) {
        lhMul = 1.40f;
    } else if (IsTitleRole(token)) {
        lhMul = 1.25f;
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

    if (IsStrongRole(token)) {
        spec.weight = 600; // SemiBold
    } else if (IsMediumRole(token)) {
        spec.weight = 500; // Medium
    } else {
        spec.weight = 400; // Regular
    }
    spec.bold = spec.weight >= 600;
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
