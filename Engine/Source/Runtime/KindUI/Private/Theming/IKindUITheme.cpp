#include "KindUI/Theming/IKindUITheme.h"

namespace we::runtime::kindui {
namespace {

bool IsStrongRole(const TypographyToken token) {
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
    case TypographyToken::BodyStrong:
    case TypographyToken::Button:
    case TypographyToken::TableHeader:
        return true;
    default:
        return false;
    }
}

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
    case TypographyToken::Caption:
    case TypographyToken::CaptionSmall:
    case TypographyToken::Status:
    case TypographyToken::StatusBar:
    case TypographyToken::Tooltip:
    case TypographyToken::PropertyLabel:
        return ColorToken::TextSecondary;
    default:
        return ColorToken::TextPrimary;
    }
}

} // namespace

TypographySpec IKindUITheme::ResolveTypography(const TypographyToken token) const {
    TypographySpec spec;
    spec.role = token;
    spec.sizePx = ResolveFontSize(token);
    // Professional body rhythm: ~1.35–1.45× size; headings slightly tighter.
    const float lhMul = (token == TypographyToken::Body || token == TypographyToken::BodyStrong)
        ? 1.40f
        : (IsStrongRole(token) ? 1.25f : 1.35f);
    spec.lineHeightPx = spec.sizePx * lhMul;
    spec.letterSpacing = 0.0f;
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
