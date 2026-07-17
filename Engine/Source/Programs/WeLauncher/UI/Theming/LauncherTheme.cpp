#include "UI/Theming/LauncherTheme.h"

namespace we::programs::welauncher {
namespace {

using we::runtime::kindui::Color;
using we::runtime::kindui::ColorToken;

Color LnHex(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
    return Color{ r / 255.0f, g / 255.0f, b / 255.0f, a };
}

} // namespace

Color LauncherTheme::ResolveColor(ColorToken token) const {
    switch (token) {
    case ColorToken::WindowBackground:        return LnHex(0x1E, 0x1E, 0x1E);
    case ColorToken::WorkspaceBackground:     return LnHex(0x1E, 0x1E, 0x1E);
    case ColorToken::PanelContentBackground:  return LnHex(0x1E, 0x1E, 0x1E);
    case ColorToken::PanelBackground:         return LnHex(0x20, 0x20, 0x20);
    case ColorToken::HeaderBackground:        return LnHex(0x25, 0x25, 0x26);
    case ColorToken::MenuBarBackground:       return LnHex(0x25, 0x25, 0x26);
    case ColorToken::PanelToolbarBackground:  return LnHex(0x25, 0x25, 0x26);
    case ColorToken::FooterBackground:        return LnHex(0x1A, 0x1A, 0x1A);
    case ColorToken::StatusBarBackground:     return LnHex(0x1A, 0x1A, 0x1A);
    case ColorToken::InputBackground:         return LnHex(0x25, 0x25, 0x26);
    case ColorToken::SearchBoxBackground:     return LnHex(0x25, 0x25, 0x26);

    case ColorToken::Separator:               return LnHex(0x2B, 0x2B, 0x2B);
    case ColorToken::BorderDark:              return LnHex(0x2B, 0x2B, 0x2B);
    case ColorToken::BorderDefault:           return LnHex(0x2B, 0x2B, 0x2B);
    case ColorToken::BorderLight:             return LnHex(0x3A, 0x3A, 0x3A);

    case ColorToken::AccentPrimary:           return LnHex(0x3B, 0x82, 0xF6);
    case ColorToken::AccentHover:             return LnHex(0x60, 0xA5, 0xFA);
    case ColorToken::IconAccent:              return LnHex(0x3B, 0x82, 0xF6);
    case ColorToken::BorderFocus:             return LnHex(0x3B, 0x82, 0xF6);
    case ColorToken::ActiveTabLine:           return LnHex(0x3B, 0x82, 0xF6, 0.85f);
    case ColorToken::LinkForeground:          return LnHex(0x60, 0xA5, 0xFA);
    case ColorToken::PlayForeground:          return LnHex(0x3B, 0x82, 0xF6);

    case ColorToken::ButtonPrimaryBackground: return LnHex(0x3B, 0x82, 0xF6);
    case ColorToken::ButtonPrimaryHover:      return LnHex(0x60, 0xA5, 0xFA);
    case ColorToken::ButtonPrimaryPressed:    return LnHex(0x25, 0x63, 0xEB);

    case ColorToken::SelectedBackground:      return LnHex(0x3B, 0x82, 0xF6, 0.28f);
    case ColorToken::SelectionHighlight:      return LnHex(0x3B, 0x82, 0xF6, 0.20f);

    case ColorToken::DialogBackground:        return LnHex(0x17, 0x17, 0x17);
    case ColorToken::ModalScrim:              return LnHex(0x00, 0x00, 0x00, 0.45f);

    case ColorToken::HoverBackground:         return LnHex(0x2A, 0x2A, 0x2C);
    case ColorToken::ActiveBackground:        return LnHex(0x2A, 0x2A, 0x2E);
    case ColorToken::PressedBackground:       return LnHex(0x32, 0x32, 0x36);
    case ColorToken::DisabledBackground:      return LnHex(0x1E, 0x1E, 0x1E);
    case ColorToken::ContentBrowserHoverBackground: return LnHex(0x2A, 0x2A, 0x2C);

    case ColorToken::TextPrimary:             return LnHex(0xF3, 0xF3, 0xF3);
    case ColorToken::TextSecondary:           return LnHex(0xA8, 0xA8, 0xA8);
    case ColorToken::TextMuted:               return LnHex(0x70, 0x70, 0x70);
    case ColorToken::TextDisabled:            return LnHex(0x70, 0x70, 0x70);
    case ColorToken::TextWindowLabel:         return LnHex(0xF3, 0xF3, 0xF3);

    case ColorToken::Success:                 return LnHex(0x4C, 0xAF, 0x50);
    case ColorToken::IconPrimary:             return LnHex(0xF3, 0xF3, 0xF3);
    case ColorToken::IconSecondary:           return LnHex(0xA8, 0xA8, 0xA8);

    default:
        return GraphiteDarkTheme::ResolveColor(token);
    }
}

float LauncherTheme::ResolveMetric(MetricToken token) const {
    switch (token) {
    case MetricToken::ButtonHeight: return 34.0f;
    case MetricToken::CornerRadiusSmall: return 8.0f;
    case MetricToken::CornerRadiusMedium: return 8.0f;
    case MetricToken::ButtonPaddingHorizontal: return 12.0f;
    case MetricToken::ListRowHeight: return 44.0f;
    case MetricToken::SearchBoxHeight: return 34.0f;
    case MetricToken::HeaderControlHeight: return 34.0f;
    case MetricToken::TextSizeTitle: return 30.0f;
    case MetricToken::TextSizeHeader: return 18.0f;
    case MetricToken::TextSizeBody: return 14.0f;
    case MetricToken::TextSizeCaption: return 12.0f;
    case MetricToken::TextSizeToolbar: return 13.0f;
    case MetricToken::IconSizeToolbar: return 16.0f;
    case MetricToken::IconSizeNavigation: return 16.0f;
    case MetricToken::IconSizeSearch: return 16.0f;
    case MetricToken::HoverAnimationDamping: return 14.0f;
    case MetricToken::PressAnimationDamping: return 18.0f;
    case MetricToken::TitleBarHeight: return 36.0f;
    default:
        return GraphiteDarkTheme::ResolveMetric(token);
    }
}

} // namespace we::programs::welauncher
