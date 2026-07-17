#include "WindEffects/Runtime/UI/Theming/LauncherTheme.h"

namespace WindEffects::Editor::UI {
namespace {

Color LnHex(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
    return Color{ r / 255.0f, g / 255.0f, b / 255.0f, a };
}

} // namespace

Color LauncherTheme::GetColor(ThemeToken token) const {
    switch (token) {
    // Master palette — commercial desktop launcher
    case ThemeToken::WindowBackground:        return LnHex(0x1E, 0x1E, 0x1E);
    case ThemeToken::WorkspaceBackground:     return LnHex(0x1E, 0x1E, 0x1E);
    case ThemeToken::PanelContentBackground:  return LnHex(0x1E, 0x1E, 0x1E);
    case ThemeToken::PanelBackground:         return LnHex(0x20, 0x20, 0x20); // sidebar
    case ThemeToken::HeaderBackground:        return LnHex(0x25, 0x25, 0x26);
    case ThemeToken::MenuBarBackground:       return LnHex(0x25, 0x25, 0x26);
    case ThemeToken::PanelToolbarBackground:  return LnHex(0x25, 0x25, 0x26); // table header
    case ThemeToken::FooterBackground:        return LnHex(0x1A, 0x1A, 0x1A);
    case ThemeToken::StatusBarBackground:     return LnHex(0x1A, 0x1A, 0x1A);
    case ThemeToken::InputBackground:         return LnHex(0x25, 0x25, 0x26);
    case ThemeToken::SearchBoxBackground:     return LnHex(0x25, 0x25, 0x26);

    case ThemeToken::Separator:               return LnHex(0x2B, 0x2B, 0x2B);
    case ThemeToken::BorderDark:              return LnHex(0x2B, 0x2B, 0x2B);
    case ThemeToken::BorderDefault:           return LnHex(0x2B, 0x2B, 0x2B);
    case ThemeToken::BorderLight:             return LnHex(0x3A, 0x3A, 0x3A);

    case ThemeToken::AccentPrimary:           return LnHex(0x3B, 0x82, 0xF6);
    case ThemeToken::AccentHover:             return LnHex(0x60, 0xA5, 0xFA);
    case ThemeToken::IconAccent:              return LnHex(0x3B, 0x82, 0xF6);
    case ThemeToken::BorderFocus:             return LnHex(0x3B, 0x82, 0xF6);
    case ThemeToken::ActiveTabLine:           return LnHex(0x3B, 0x82, 0xF6, 0.85f);
    case ThemeToken::LinkForeground:          return LnHex(0x60, 0xA5, 0xFA);
    case ThemeToken::PlayForeground:          return LnHex(0x3B, 0x82, 0xF6);

    case ThemeToken::ButtonPrimaryBackground: return LnHex(0x3B, 0x82, 0xF6);
    case ThemeToken::ButtonPrimaryHover:      return LnHex(0x60, 0xA5, 0xFA);
    case ThemeToken::ButtonPrimaryPressed:    return LnHex(0x25, 0x63, 0xEB);

    case ThemeToken::SelectedBackground:      return LnHex(0x3B, 0x82, 0xF6, 0.28f);
    case ThemeToken::SelectionHighlight:      return LnHex(0x3B, 0x82, 0xF6, 0.20f);

    case ThemeToken::DialogBackground:        return LnHex(0x17, 0x17, 0x17);
    case ThemeToken::ModalScrim:              return LnHex(0x00, 0x00, 0x00, 0.45f);

    case ThemeToken::HoverBackground:         return LnHex(0x2A, 0x2A, 0x2C);
    case ThemeToken::ActiveBackground:        return LnHex(0x2A, 0x2A, 0x2E);
    case ThemeToken::PressedBackground:       return LnHex(0x32, 0x32, 0x36);
    case ThemeToken::DisabledBackground:      return LnHex(0x1E, 0x1E, 0x1E);
    case ThemeToken::ContentBrowserHoverBackground: return LnHex(0x2A, 0x2A, 0x2C);

    case ThemeToken::TextPrimary:             return LnHex(0xF3, 0xF3, 0xF3);
    case ThemeToken::TextSecondary:           return LnHex(0xA8, 0xA8, 0xA8);
    case ThemeToken::TextMuted:               return LnHex(0x70, 0x70, 0x70);
    case ThemeToken::TextDisabled:            return LnHex(0x70, 0x70, 0x70);
    case ThemeToken::TextWindowLabel:         return LnHex(0xF3, 0xF3, 0xF3);

    case ThemeToken::Success:                 return LnHex(0x4C, 0xAF, 0x50);
    case ThemeToken::IconPrimary:             return LnHex(0xF3, 0xF3, 0xF3);
    case ThemeToken::IconSecondary:           return LnHex(0xA8, 0xA8, 0xA8);

    default:
        return GraphiteDarkTheme::GetColor(token);
    }
}

float LauncherTheme::GetMetric(ThemeToken token) const {
    switch (token) {
    case ThemeToken::ButtonHeight: return 34.0f;
    case ThemeToken::CornerRadiusSmall: return 8.0f;
    case ThemeToken::CornerRadiusMedium: return 8.0f;
    case ThemeToken::ButtonPaddingHorizontal: return 12.0f;
    case ThemeToken::ListRowHeight: return 44.0f;
    case ThemeToken::SearchBoxHeight: return 34.0f;
    case ThemeToken::HeaderControlHeight: return 34.0f;
    case ThemeToken::TextSizeTitle: return 30.0f;
    case ThemeToken::TextSizeHeader: return 18.0f;
    case ThemeToken::TextSizeBody: return 14.0f;
    case ThemeToken::TextSizeCaption: return 12.0f;
    case ThemeToken::TextSizeToolbar: return 13.0f;
    case ThemeToken::IconSizeToolbar: return 16.0f;
    case ThemeToken::IconSizeNavigation: return 16.0f;
    case ThemeToken::IconSizeSearch: return 16.0f;
    case ThemeToken::HoverAnimationDamping: return 14.0f; // ~150 ms feel
    case ThemeToken::PressAnimationDamping: return 18.0f; // ~100 ms feel
    default:
        return GraphiteDarkTheme::GetMetric(token);
    }
}

} // namespace WindEffects::Editor::UI
