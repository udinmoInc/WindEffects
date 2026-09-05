#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::runtime::kindui {
namespace {

bool IsDisplayTitleRole(const TypographyToken token) {
    switch (token) {
    case TypographyToken::WindowTitle:
    case TypographyToken::PageTitle:
    case TypographyToken::Display:
    case TypographyToken::Heading1:
        return true;
    default:
        return false;
    }
}

bool IsTitleRole(const TypographyToken token) {
    if (IsDisplayTitleRole(token)) {
        return true;
    }
    switch (token) {
    case TypographyToken::SectionTitle:
    case TypographyToken::CardTitle:
    case TypographyToken::DialogTitle:
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

bool IsMediumRole(const TypographyToken token) {
    // Roboto Medium for tabs/headers/toolbar labels and emphasized chrome text.
    switch (token) {
    case TypographyToken::SectionTitle:
    case TypographyToken::CardTitle:
    case TypographyToken::DialogTitle:
    case TypographyToken::Heading:
    case TypographyToken::Heading2:
    case TypographyToken::Heading3:
    case TypographyToken::Heading4:
    case TypographyToken::Title:
    case TypographyToken::Button:
    case TypographyToken::TableHeader:
    case TypographyToken::Label:
    case TypographyToken::BodyStrong:
    case TypographyToken::Toolbar:
    case TypographyToken::Menu:
    case TypographyToken::Navigation:
        return true;
    default:
        return false;
    }
}

// Semantic text roles → color tokens (hierarchy, not arbitrary hex).
ColorToken ColorForRole(const TypographyToken token) {
    switch (token) {
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

    // Match Roboto wefont face metrics (lineHeight 32 / bakeSize 24).
    float lhMul = 32.0f / 24.0f;
    if (IsDisplayTitleRole(token)) {
        lhMul = 1.20f;
    }
    spec.lineHeightPx = spec.sizePx * lhMul;

    // Keep tracking neutral for compact editor chrome readability.
    spec.letterSpacing = 0.0f;

    if (IsDisplayTitleRole(token)) {
        spec.weight = 600; // SemiBold for large display titles only
    } else if (IsMediumRole(token)) {
        spec.weight = 500; // Medium — tabs, section headers, toolbar, emphasized UI
    } else {
        spec.weight = 400; // Regular — default editor text
    }
    spec.bold = spec.weight >= 600;
    spec.italic = false;
    spec.color = ResolveColor(ColorForRole(token));
    return spec;
}

Color IKindUITheme::InteractiveBackground(float hoverAnim, float pressAnim, bool selected) const {
    return ResolveInteractiveBackground(hoverAnim, pressAnim, selected, ColorToken::PanelBackground);
}

Color IKindUITheme::IconForState(bool hovered, bool active) const {
    if (active) {
        return ResolveColor(ColorToken::IconActive);
    }
    if (hovered) {
        return ResolveColor(ColorToken::IconHover);
    }
    return ResolveColor(ColorToken::IconSecondary);
}

Color IKindUITheme::TextForState(bool hovered, bool active) const {
    return (active || hovered)
        ? ResolveColor(ColorToken::TextPrimary)
        : ResolveColor(ColorToken::TextSecondary);
}

} // namespace we::runtime::kindui
