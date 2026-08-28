#include "KindUI/Theming/DefaultTheme.h"

namespace we::runtime::kindui {

Color DefaultTheme::ResolveColor(ColorToken token) const {
    switch (token) {
    // Elevation: Window → Panel → Card → Control → Overlay → Popup
    case ColorToken::WindowBackground:
    case ColorToken::WorkspaceBackground:
    case ColorToken::ViewportBackground:
    case ColorToken::DockAreaBackground: return {0.067f, 0.071f, 0.078f, 1.0f}; // #111214

    case ColorToken::PrimarySurface:
    case ColorToken::PanelBackground:
    case ColorToken::PanelContentBackground:
    case ColorToken::ContentBrowserBackground: return {0.094f, 0.098f, 0.106f, 1.0f}; // #18191B

    case ColorToken::CardBackground:
    case ColorToken::SecondarySurface: return {0.114f, 0.118f, 0.125f, 1.0f}; // #1D1E20

    case ColorToken::ControlBackground:
    case ColorToken::InputBackground:
    case ColorToken::SearchBoxBackground: return {0.078f, 0.082f, 0.090f, 1.0f}; // #141517

    case ColorToken::TertiarySurface: return {0.133f, 0.137f, 0.149f, 1.0f}; // #222326

    case ColorToken::OverlayBackground:
    case ColorToken::DialogBackground: return {0.133f, 0.137f, 0.149f, 1.0f}; // #222326

    case ColorToken::PopupBackground:
    case ColorToken::TooltipBackground: return {0.133f, 0.137f, 0.149f, 1.0f}; // #222326

    case ColorToken::HeaderBackground:
    case ColorToken::ToolbarBackground:
    case ColorToken::PanelToolbarBackground: return {0.110f, 0.114f, 0.122f, 1.0f}; // #1C1D1F
    case ColorToken::MenuBarBackground:
    case ColorToken::StatusBarBackground:
    case ColorToken::FooterBackground: return {0.067f, 0.071f, 0.078f, 1.0f}; // #111214

    case ColorToken::TabBackground:
    case ColorToken::PanelTabInactiveBackground: return {0.161f, 0.165f, 0.176f, 1.0f}; // #292A2D
    case ColorToken::PanelTabActiveBackground: return {0.110f, 0.114f, 0.122f, 1.0f}; // #1C1D1F

    case ColorToken::AccentSurface:
    case ColorToken::AccentPrimary:      return {0.220f, 0.224f, 0.235f, 1.0f}; // #38393C
    case ColorToken::AccentHover:        return {0.251f, 0.267f, 0.290f, 1.0f};

    case ColorToken::ControlBackgroundHover:
    case ColorToken::HoverBackground:    return {0.153f, 0.157f, 0.169f, 1.0f}; // #27282B
    case ColorToken::ControlBackgroundPressed:
    case ColorToken::PressedBackground:  return {0.078f, 0.082f, 0.090f, 1.0f}; // #141517
    case ColorToken::ControlBackgroundDisabled:
    case ColorToken::DisabledBackground: return {0.067f, 0.071f, 0.078f, 1.0f};
    case ColorToken::ControlBackgroundSelected:
    case ColorToken::SelectedBackground:
    case ColorToken::ActiveBackground:   return {0.196f, 0.200f, 0.212f, 1.0f}; // #323336

    case ColorToken::BorderDefault:      return {0.161f, 0.165f, 0.176f, 1.0f}; // #292A2D
    case ColorToken::BorderSubtle:
    case ColorToken::BorderDark:         return {0.137f, 0.141f, 0.153f, 1.0f}; // #232427
    case ColorToken::BorderFocused:
    case ColorToken::BorderFocus:
    case ColorToken::ActiveTabLine:      return {0.220f, 0.224f, 0.235f, 1.0f}; // #38393C
    case ColorToken::BorderError:
    case ColorToken::ErrorColor:
    case ColorToken::ErrorForeground:    return {0.878f, 0.322f, 0.322f, 1.0f}; // #E05252
    case ColorToken::BorderLight:
    case ColorToken::Separator:          return {0.137f, 0.141f, 0.153f, 1.0f}; // #232427

    case ColorToken::TextPrimary:
    case ColorToken::TextWindowLabel:
    case ColorToken::CodeForeground:     return {0.902f, 0.902f, 0.902f, 1.0f}; // #E6E6E6
    case ColorToken::TextSecondary:      return {0.627f, 0.631f, 0.639f, 1.0f}; // #A0A1A3
    case ColorToken::TextCaption:        return {0.627f, 0.631f, 0.639f, 1.0f}; // #A0A1A3
    case ColorToken::TextMuted:
    case ColorToken::TextHint:
    case ColorToken::SearchPlaceholder:  return {0.439f, 0.443f, 0.455f, 1.0f}; // #707174
    case ColorToken::TextDisabled:       return {0.310f, 0.314f, 0.325f, 1.0f}; // #4F5053
    case ColorToken::TextOnAccent:       return {0.902f, 0.902f, 0.902f, 1.0f};
    case ColorToken::TextLink:
    case ColorToken::LinkForeground:
    case ColorToken::PlayForeground:     return {0.627f, 0.631f, 0.639f, 1.0f};

    case ColorToken::IconPrimary:
    case ColorToken::IconDefault:        return {0.627f, 0.631f, 0.639f, 1.0f};
    case ColorToken::IconSecondary:      return {0.627f, 0.631f, 0.639f, 1.0f};
    case ColorToken::IconDisabled:       return {0.310f, 0.314f, 0.325f, 1.0f};
    case ColorToken::IconAccent:
    case ColorToken::IconHover:
    case ColorToken::IconActive:         return {0.902f, 0.902f, 0.902f, 1.0f};

    case ColorToken::Success:
    case ColorToken::SuccessColor:       return {0.298f, 0.686f, 0.314f, 1.0f}; // #4CAF50
    case ColorToken::Warning:
    case ColorToken::WarningColor:       return {0.839f, 0.651f, 0.165f, 1.0f}; // #D6A62A
    case ColorToken::InfoColor:          return {0.627f, 0.631f, 0.639f, 1.0f};

    case ColorToken::ScrimOverlay:
    case ColorToken::ModalScrim:         return {0.0f, 0.0f, 0.0f, 0.55f};
    case ColorToken::ShadowColor:
    case ColorToken::ShadowSubtle:       return {0.0f, 0.0f, 0.0f, 0.22f};
    case ColorToken::ShadowOverlay:      return {0.0f, 0.0f, 0.0f, 0.42f};
    case ColorToken::ShadowPopup:        return {0.0f, 0.0f, 0.0f, 0.32f};
    default:                             return {0.5f, 0.5f, 0.5f, 1.0f};
    }
}

float DefaultTheme::ResolveMetric(MetricToken token) const {
    switch (token) {
    case MetricToken::CornerRadiusSmall: return 3.0f;
    case MetricToken::CornerRadiusMedium: return 6.0f;
    case MetricToken::CornerRadiusLarge:
    case MetricToken::WindowCornerRadius: return 10.0f;
    case MetricToken::TextSizeMenu:
    case MetricToken::TextSizeToolbar:
    case MetricToken::TextSizeTabs:
    case MetricToken::TextSizeProperty:
    case MetricToken::TextSizeCategory: return 12.0f;
    case MetricToken::TextSizeNormal:
    case MetricToken::TextSizeWindow:
    case MetricToken::TextSizeBody: return 14.0f;
    case MetricToken::TextSizeCaption: return 12.0f;
    case MetricToken::TextSizeSmall: return 12.0f;
    case MetricToken::TextSizeHeader: return 16.5f;
    case MetricToken::TextSizeTitle: return 33.0f;
    case MetricToken::TextCharWidthRatio: return 0.55f;
    case MetricToken::BorderWidth: return 1.0f;
    case MetricToken::FocusRingWidth: return 2.0f;
    case MetricToken::PanelHeaderHeight:
    case MetricToken::PanelTabHeight:
    case MetricToken::PanelToolbarHeight:
    case MetricToken::HeaderControlHeight:
    case MetricToken::IconButtonSize:
    case MetricToken::ButtonHeight:
    case MetricToken::NavigationButtonSize: return 32.0f;
    case MetricToken::ControlHeightCompact: return 28.0f;
    case MetricToken::ControlHeightLarge: return 40.0f;
    case MetricToken::FormRowHeight: return 44.0f;
    case MetricToken::MenuItemHeight: return 28.0f;
    case MetricToken::PageMargin: return 16.0f;
    case MetricToken::SectionGap: return 24.0f;
    case MetricToken::CardPadding: return 12.0f;
    case MetricToken::ContentGap: return 12.0f;
    case MetricToken::FormRowGap: return 13.0f;
    case MetricToken::LabelHintGap: return 5.0f;
    case MetricToken::ListRowHeight: return 48.0f;
    case MetricToken::CategoryHeaderHeight: return 28.0f;
    case MetricToken::TitleBarHeight: return 34.0f;
    case MetricToken::WindowControlWidth: return 40.0f;
    case MetricToken::ToolbarHeight: return 44.0f;
    case MetricToken::SearchBoxHeight: return 28.0f;
    case MetricToken::IconSizeSearch:
    case MetricToken::IconSizeToolbar:
    case MetricToken::IconSizePrimary:
    case MetricToken::IconSizeTree:
    case MetricToken::IconSizeNavigation: return 16.0f;
    case MetricToken::IconButtonRadius: return 6.0f;
    case MetricToken::ButtonPaddingHorizontal:
    case MetricToken::Space2: return 8.0f;
    case MetricToken::ButtonSpacing:
    case MetricToken::Space1: return 4.0f;
    case MetricToken::ButtonGroupSpacing: return 10.0f;
    case MetricToken::ScrollbarWidth: return 10.0f;
    case MetricToken::DragThreshold: return 6.0f;
    case MetricToken::MenuPadding: return 4.0f;
    case MetricToken::CheckMarkSize: return 16.0f;
    case MetricToken::MenuTextIndent: return 24.0f;
    case MetricToken::Space3: return 12.0f;
    case MetricToken::Space4: return 16.0f;
    case MetricToken::Space5: return 20.0f;
    case MetricToken::Space6: return 24.0f;
    case MetricToken::HoverAnimationDamping: return 10.0f;
    case MetricToken::PressAnimationDamping: return 14.0f;
    case MetricToken::PressOffset: return 1.0f;
    case MetricToken::ShadowBlurSmall: return 4.0f;
    case MetricToken::ShadowBlurMedium: return 8.0f;
    case MetricToken::ShadowSpreadMedium: return 16.0f;
    default: return 0.0f;
    }
}

Margin DefaultTheme::ResolvePadding(PaddingToken token) const {
    switch (token) {
    case PaddingToken::Panel:
    case PaddingToken::PaddingPanelLeft:
    case PaddingToken::PaddingPanelTop:
    case PaddingToken::PaddingPanelRight:
    case PaddingToken::PaddingPanelBottom:
        return {12.0f, 12.0f, 12.0f, 12.0f};
    case PaddingToken::Card: {
        const float p = ResolveMetric(MetricToken::CardPadding);
        return {p, p, p, p};
    }
    case PaddingToken::Page: {
        const float p = ResolveMetric(MetricToken::PageMargin);
        return {p, p, p, p};
    }
    case PaddingToken::Input: {
        const float h = ResolveMetric(MetricToken::Space2);
        const float v = ResolveMetric(MetricToken::Space1);
        return {h, v, h, v};
    }
    case PaddingToken::FormRow: {
        const float g = ResolveMetric(MetricToken::FormRowGap);
        return {0.0f, g * 0.5f, 0.0f, g * 0.5f};
    }
    case PaddingToken::Button:
    case PaddingToken::PaddingButtonLeft:
    case PaddingToken::PaddingButtonTop:
    case PaddingToken::PaddingButtonRight:
    case PaddingToken::PaddingButtonBottom:
        return {8.0f, 4.0f, 8.0f, 4.0f};
    default:
        return {};
    }
}

float DefaultTheme::ResolveSpacing(SpacingToken token) const {
    switch (token) {
    case SpacingToken::None:       return 0.0f;
    case SpacingToken::ExtraSmall: return 2.0f;
    case SpacingToken::Small:      return 4.0f;
    case SpacingToken::Medium:     return 8.0f;
    case SpacingToken::Large:      return 16.0f;
    case SpacingToken::ExtraLarge: return 24.0f;
    case SpacingToken::Huge:       return 32.0f;
    default:                       return 8.0f;
    }
}

float DefaultTheme::ResolveRadius(RadiusToken token) const {
    switch (token) {
    case RadiusToken::None:   return 0.0f;
    case RadiusToken::Small:   return 3.0f;
    case RadiusToken::Medium:  return 6.0f;
    case RadiusToken::Large:   return 10.0f;
    case RadiusToken::Full:    return 999.0f;
    default:                   return 4.0f;
    }
}

float DefaultTheme::ResolveFontSize(TypographyToken token) const {
    switch (token) {
    case TypographyToken::WindowTitle:
    case TypographyToken::Display:
        return ResolveMetric(MetricToken::TextSizeTitle) + 1.0f;
    case TypographyToken::PageTitle:
    case TypographyToken::Heading1:
        return ResolveMetric(MetricToken::TextSizeTitle);
    case TypographyToken::SectionTitle:
    case TypographyToken::DialogTitle:
    case TypographyToken::Heading2:
    case TypographyToken::Heading:
        return ResolveMetric(MetricToken::TextSizeHeader);
    case TypographyToken::CardTitle:
    case TypographyToken::Heading3:
        return ResolveMetric(MetricToken::TextSizeHeader) - 1.0f;
    case TypographyToken::Heading4:
    case TypographyToken::Title:
        return ResolveMetric(MetricToken::TextSizeBody) + 1.0f;
    case TypographyToken::Heading5:
    case TypographyToken::Subtitle:
    case TypographyToken::Heading6:
    case TypographyToken::Body:
    case TypographyToken::BodyStrong:
    case TypographyToken::Button:
    case TypographyToken::Link:
    case TypographyToken::Label:
        return ResolveMetric(MetricToken::TextSizeBody);
    case TypographyToken::Menu:
    case TypographyToken::Toolbar:
    case TypographyToken::Navigation:
    case TypographyToken::TableHeader:
    case TypographyToken::PropertyValue:
    case TypographyToken::Caption:
    case TypographyToken::Code:
    case TypographyToken::Console:
    case TypographyToken::Monospace:
        return ResolveMetric(MetricToken::TextSizeSmall);
    case TypographyToken::Status:
    case TypographyToken::StatusBar:
    case TypographyToken::PropertyLabel:
    case TypographyToken::Error:
    case TypographyToken::Warning:
    case TypographyToken::Success:
    case TypographyToken::Hint:
    case TypographyToken::Tooltip:
    case TypographyToken::Disabled:
    case TypographyToken::CaptionSmall:
        return ResolveMetric(MetricToken::TextSizeCaption);
    default:
        return ResolveMetric(MetricToken::TextSizeBody);
    }
}

int DefaultTheme::ResolveElevation(ElevationToken token) const {
    switch (token) {
    case ElevationToken::None:
    case ElevationToken::Window:
    case ElevationToken::Panel:
    case ElevationToken::Control:
        return 0;
    case ElevationToken::Card:
        return 1;
    case ElevationToken::Popup:
        return 2;
    case ElevationToken::Overlay:
        return 3;
    default:
        return 0;
    }
}

float DefaultTheme::ResolveAnimationDuration(AnimationToken token) const {
    switch (token) {
    case AnimationToken::Instant: return 0.0f;
    case AnimationToken::Fast:    return 0.12f;
    case AnimationToken::Normal:  return 0.20f;
    case AnimationToken::Slow:    return 0.35f;
    default:                      return 0.15f;
    }
}

} // namespace we::runtime::kindui
