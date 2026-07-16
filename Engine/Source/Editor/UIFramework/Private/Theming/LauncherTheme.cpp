#include "WindEffects/Editor/UI/Theming/LauncherTheme.h"

namespace WindEffects::Editor::UI {
namespace {

Color LnHex(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
    return Color{ r / 255.0f, g / 255.0f, b / 255.0f, a };
}

} // namespace

Color LauncherTheme::GetColor(ThemeToken token) const {
    switch (token) {
    // Layered surfaces: Header → Sidebar → Content → Footer (depth without shadows)
    case ThemeToken::WindowBackground:        return LnHex(0x16, 0x16, 0x16);
    case ThemeToken::HeaderBackground:        return LnHex(0x1A, 0x1A, 0x1A);
    case ThemeToken::MenuBarBackground:       return LnHex(0x1A, 0x1A, 0x1A);
    case ThemeToken::PanelBackground:         return LnHex(0x18, 0x18, 0x18); // sidebar
    case ThemeToken::PanelContentBackground:  return LnHex(0x1E, 0x1E, 0x1E); // main content
    case ThemeToken::PanelToolbarBackground:  return LnHex(0x1C, 0x1C, 0x1C);
    case ThemeToken::FooterBackground:        return LnHex(0x15, 0x15, 0x15);
    case ThemeToken::StatusBarBackground:     return LnHex(0x15, 0x15, 0x15);

    // Professional blue accent — launcher identity (no orange)
    case ThemeToken::IconAccent:              return LnHex(0x5B, 0x8D, 0xEF);
    case ThemeToken::AccentPrimary:           return LnHex(0x5B, 0x8D, 0xEF);
    case ThemeToken::AccentHover:             return LnHex(0x7A, 0xA4, 0xF5);
    case ThemeToken::BorderFocus:             return LnHex(0x5B, 0x8D, 0xEF);
    case ThemeToken::ActiveTabLine:           return LnHex(0x5B, 0x8D, 0xEF, 0.85f);
    case ThemeToken::LinkForeground:          return LnHex(0x7A, 0xA4, 0xF5);
    case ThemeToken::PlayForeground:          return LnHex(0x5B, 0x8D, 0xEF);

    // Blue primary buttons
    case ThemeToken::ButtonPrimaryBackground: return LnHex(0x3D, 0x7A, 0xE0);
    case ThemeToken::ButtonPrimaryHover:      return LnHex(0x5B, 0x8D, 0xEF);
    case ThemeToken::ButtonPrimaryPressed:    return LnHex(0x2F, 0x65, 0xC4);

    // Blue-tinted selection / focus surfaces
    case ThemeToken::SelectedBackground:      return LnHex(0x1E, 0x32, 0x4E);
    case ThemeToken::SelectionHighlight:      return LnHex(0x5B, 0x8D, 0xEF, 0.24f);

    // Subtle cool hover; dark secondary chrome
    case ThemeToken::HoverBackground:         return LnHex(0x28, 0x2C, 0x34);
    case ThemeToken::ActiveBackground:        return LnHex(0x26, 0x28, 0x2E);
    case ThemeToken::PressedBackground:       return LnHex(0x32, 0x38, 0x44);
    case ThemeToken::ContentBrowserHoverBackground: return LnHex(0x28, 0x2C, 0x34);

    // Subtle panel rules (#2B2B2B–#303030)
    case ThemeToken::BorderDefault:           return LnHex(0x30, 0x30, 0x30);
    case ThemeToken::BorderLight:             return LnHex(0x38, 0x38, 0x38);
    case ThemeToken::BorderDark:              return LnHex(0x2B, 0x2B, 0x2B);
    case ThemeToken::Separator:               return LnHex(0x2B, 0x2B, 0x2B);

    // High-contrast type on launcher surfaces
    case ThemeToken::TextPrimary:             return LnHex(0xF2, 0xF4, 0xF8);
    case ThemeToken::TextSecondary:           return LnHex(0xB0, 0xB6, 0xC0);
    case ThemeToken::TextMuted:               return LnHex(0x78, 0x7E, 0x8A);
    case ThemeToken::TextWindowLabel:         return LnHex(0xF2, 0xF4, 0xF8);

    default:
        return GraphiteDarkTheme::GetColor(token);
    }
}

} // namespace WindEffects::Editor::UI
