#include "UI/Theming/LauncherTheme.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::programs::welauncher {
namespace {

using we::runtime::kindui::Color;
using we::runtime::kindui::ColorToken;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::PaddingToken;

Color LnHex(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
    return Color{ r / 255.0f, g / 255.0f, b / 255.0f, a };
}

} // namespace

Color LauncherTheme::ResolveColor(ColorToken token) const {
    switch (token) {
    // Elevation ladder aligned with Graphite (product accent overrides below)
    case ColorToken::WindowBackground:        return LnHex(0x14, 0x14, 0x16);
    case ColorToken::WorkspaceBackground:     return LnHex(0x14, 0x14, 0x16);
    case ColorToken::PanelContentBackground:  return LnHex(0x1A, 0x1A, 0x1C);
    case ColorToken::PanelBackground:         return LnHex(0x1A, 0x1A, 0x1C);
    case ColorToken::PrimarySurface:          return LnHex(0x1A, 0x1A, 0x1C);
    case ColorToken::CardBackground:          return LnHex(0x22, 0x22, 0x24);
    case ColorToken::SecondarySurface:        return LnHex(0x22, 0x22, 0x24);
    case ColorToken::TertiarySurface:         return LnHex(0x28, 0x28, 0x2A);
    case ColorToken::HeaderBackground:        return LnHex(0x18, 0x18, 0x1A);
    case ColorToken::MenuBarBackground:       return LnHex(0x16, 0x16, 0x18);
    case ColorToken::PanelToolbarBackground:  return LnHex(0x18, 0x18, 0x1A);
    case ColorToken::FooterBackground:        return LnHex(0x16, 0x16, 0x18);
    case ColorToken::StatusBarBackground:     return LnHex(0x16, 0x16, 0x18);
    case ColorToken::InputBackground:         return LnHex(0x1E, 0x1E, 0x20);
    case ColorToken::SearchBoxBackground:     return LnHex(0x1E, 0x1E, 0x20);
    case ColorToken::ControlBackground:       return LnHex(0x1E, 0x1E, 0x20);
    case ColorToken::DialogBackground:        return LnHex(0x2A, 0x2A, 0x2C);
    case ColorToken::OverlayBackground:       return LnHex(0x2A, 0x2A, 0x2C);
    case ColorToken::PopupBackground:         return LnHex(0x2E, 0x2E, 0x30);
    case ColorToken::TooltipBackground:       return LnHex(0x2E, 0x2E, 0x30);

    case ColorToken::Separator:               return LnHex(0xFF, 0xFF, 0xFF, 0.08f);
    case ColorToken::BorderSubtle:            return LnHex(0xFF, 0xFF, 0xFF, 0.07f);
    case ColorToken::BorderDark:              return LnHex(0xFF, 0xFF, 0xFF, 0.05f);
    case ColorToken::BorderDefault:           return LnHex(0x36, 0x36, 0x3A);
    case ColorToken::BorderLight:             return LnHex(0xFF, 0xFF, 0xFF, 0.14f);

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

    case ColorToken::ModalScrim:              return LnHex(0x00, 0x00, 0x00, 0.50f);

    case ColorToken::HoverBackground:         return LnHex(0x2C, 0x2C, 0x2E);
    case ColorToken::ActiveBackground:        return LnHex(0x26, 0x26, 0x28);
    case ColorToken::PressedBackground:       return LnHex(0x34, 0x34, 0x38);
    case ColorToken::DisabledBackground:      return LnHex(0x14, 0x14, 0x16);
    case ColorToken::ContentBrowserHoverBackground: return LnHex(0x2C, 0x2C, 0x2E);

    case ColorToken::TextPrimary:             return LnHex(0xEC, 0xEC, 0xEC);
    case ColorToken::TextSecondary:           return LnHex(0xB4, 0xB4, 0xB4);
    case ColorToken::TextCaption:             return LnHex(0x9A, 0x9A, 0x9A);
    case ColorToken::TextMuted:               return LnHex(0x85, 0x85, 0x85);
    case ColorToken::TextHint:                return LnHex(0x85, 0x85, 0x85);
    case ColorToken::SearchPlaceholder:       return LnHex(0x85, 0x85, 0x85);
    case ColorToken::TextDisabled:            return LnHex(0x5C, 0x5C, 0x5C);
    case ColorToken::TextWindowLabel:         return LnHex(0xEC, 0xEC, 0xEC);

    case ColorToken::Success:                 return LnHex(0x4C, 0xAF, 0x50);
    case ColorToken::IconPrimary:             return LnHex(0xD4, 0xD4, 0xD4);
    case ColorToken::IconSecondary:           return LnHex(0xA8, 0xA8, 0xA8);

    default:
        return GraphiteDarkTheme::ResolveColor(token);
    }
}

float LauncherTheme::ResolveMetric(MetricToken token) const {
    switch (token) {
    case MetricToken::ButtonHeight: return 34.0f;
    case MetricToken::ControlHeightCompact: return 28.0f;
    case MetricToken::ControlHeightLarge: return 40.0f;
    case MetricToken::FormRowHeight: return 40.0f;
    case MetricToken::MenuItemHeight: return 28.0f;
    case MetricToken::PageMargin: return 16.0f;
    case MetricToken::SectionGap: return 24.0f;
    case MetricToken::CardPadding: return 12.0f;
    case MetricToken::ContentGap: return 12.0f;
    case MetricToken::FormRowGap: return 8.0f;
    case MetricToken::LabelHintGap: return 2.0f;
    case MetricToken::CornerRadiusSmall: return 8.0f;
    case MetricToken::CornerRadiusMedium: return 8.0f;
    case MetricToken::ButtonPaddingHorizontal: return 12.0f;
    case MetricToken::ListRowHeight: return 44.0f;
    case MetricToken::SearchBoxHeight: return 34.0f;
    case MetricToken::HeaderControlHeight: return 34.0f;
    case MetricToken::TextSizeTitle: return 22.0f;
    case MetricToken::TextSizeHeader: return 15.0f;
    case MetricToken::TextSizeBody: return 13.0f;
    case MetricToken::TextSizeSmall: return 12.0f;
    case MetricToken::TextSizeCaption: return 11.0f;
    case MetricToken::TextSizeToolbar: return 12.0f;
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
