#include "KindUI/Theming/DefaultTheme.h"

namespace we::runtime::kindui {

Color DefaultTheme::ResolveColor(ColorToken token) const {
    switch (token) {
    case ColorToken::PrimarySurface:     return {0.12f, 0.12f, 0.13f, 1.0f};
    case ColorToken::SecondarySurface:   return {0.16f, 0.16f, 0.17f, 1.0f};
    case ColorToken::TertiarySurface:    return {0.20f, 0.20f, 0.21f, 1.0f};
    case ColorToken::AccentSurface:
    case ColorToken::AccentPrimary:      return {0.25f, 0.45f, 0.85f, 1.0f};
    case ColorToken::AccentHover:        return {0.35f, 0.55f, 0.90f, 1.0f};
    case ColorToken::WindowBackground:
    case ColorToken::WorkspaceBackground:
    case ColorToken::ViewportBackground:
    case ColorToken::DockAreaBackground: return {0.10f, 0.10f, 0.11f, 1.0f};
    case ColorToken::PanelBackground:
    case ColorToken::PanelContentBackground:
    case ColorToken::ContentBrowserBackground: return {0.12f, 0.12f, 0.13f, 1.0f};
    case ColorToken::HeaderBackground:
    case ColorToken::ToolbarBackground:
    case ColorToken::PanelToolbarBackground:
    case ColorToken::MenuBarBackground:
    case ColorToken::StatusBarBackground:
    case ColorToken::FooterBackground:   return {0.11f, 0.11f, 0.12f, 1.0f};
    case ColorToken::TabBackground:
    case ColorToken::PanelTabActiveBackground:
    case ColorToken::PanelTabInactiveBackground: return {0.14f, 0.14f, 0.15f, 1.0f};
    case ColorToken::PopupBackground:
    case ColorToken::DialogBackground:
    case ColorToken::TooltipBackground:  return {0.18f, 0.18f, 0.19f, 1.0f};
    case ColorToken::ControlBackground:
    case ColorToken::InputBackground:
    case ColorToken::SearchBoxBackground: return {0.18f, 0.18f, 0.19f, 1.0f};
    case ColorToken::ControlBackgroundHover:
    case ColorToken::HoverBackground:    return {0.22f, 0.22f, 0.24f, 1.0f};
    case ColorToken::ControlBackgroundPressed:
    case ColorToken::PressedBackground:  return {0.14f, 0.14f, 0.15f, 1.0f};
    case ColorToken::ControlBackgroundDisabled:
    case ColorToken::DisabledBackground: return {0.14f, 0.14f, 0.15f, 0.5f};
    case ColorToken::ControlBackgroundSelected:
    case ColorToken::SelectedBackground:
    case ColorToken::ActiveBackground:   return {0.25f, 0.45f, 0.85f, 0.3f};
    case ColorToken::BorderDefault:      return {0.30f, 0.30f, 0.32f, 1.0f};
    case ColorToken::BorderSubtle:
    case ColorToken::BorderDark:         return {0.22f, 0.22f, 0.24f, 1.0f};
    case ColorToken::BorderFocused:
    case ColorToken::BorderFocus:
    case ColorToken::ActiveTabLine:      return {0.25f, 0.45f, 0.85f, 1.0f};
    case ColorToken::BorderError:
    case ColorToken::ErrorColor:
    case ColorToken::ErrorForeground:    return {0.85f, 0.25f, 0.25f, 1.0f};
    case ColorToken::BorderLight:
    case ColorToken::Separator:          return {1.0f, 1.0f, 1.0f, 0.08f};
    case ColorToken::TextPrimary:
    case ColorToken::TextWindowLabel:
    case ColorToken::CodeForeground:     return {0.92f, 0.92f, 0.93f, 1.0f};
    case ColorToken::TextSecondary:      return {0.65f, 0.65f, 0.67f, 1.0f};
    case ColorToken::TextDisabled:
    case ColorToken::TextMuted:
    case ColorToken::SearchPlaceholder:  return {0.45f, 0.45f, 0.47f, 1.0f};
    case ColorToken::TextOnAccent:       return {1.0f, 1.0f, 1.0f, 1.0f};
    case ColorToken::TextLink:
    case ColorToken::LinkForeground:
    case ColorToken::PlayForeground:     return {0.40f, 0.65f, 1.0f, 1.0f};
    case ColorToken::IconPrimary:
    case ColorToken::IconDefault:        return {0.85f, 0.85f, 0.87f, 1.0f};
    case ColorToken::IconSecondary:      return {0.60f, 0.60f, 0.62f, 1.0f};
    case ColorToken::IconDisabled:       return {0.40f, 0.40f, 0.42f, 1.0f};
    case ColorToken::IconAccent:
    case ColorToken::IconHover:
    case ColorToken::IconActive:         return {0.40f, 0.65f, 1.0f, 1.0f};
    case ColorToken::Success:
    case ColorToken::SuccessColor:       return {0.20f, 0.70f, 0.35f, 1.0f};
    case ColorToken::Warning:
    case ColorToken::WarningColor:       return {0.90f, 0.65f, 0.15f, 1.0f};
    case ColorToken::InfoColor:          return {0.25f, 0.55f, 0.90f, 1.0f};
    case ColorToken::ScrimOverlay:
    case ColorToken::ModalScrim:         return {0.0f, 0.0f, 0.0f, 0.5f};
    case ColorToken::ShadowColor:
    case ColorToken::ShadowSubtle:
    case ColorToken::ShadowOverlay:
    case ColorToken::ShadowPopup:        return {0.0f, 0.0f, 0.0f, 0.3f};
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
    case MetricToken::TextSizeNormal:
    case MetricToken::TextSizeProperty:
    case MetricToken::TextSizeCategory: return 12.0f;
    case MetricToken::TextSizeCaption:
    case MetricToken::TextSizeSmall: return 11.0f;
    case MetricToken::TextSizeWindow:
    case MetricToken::TextSizeBody: return 13.0f;
    case MetricToken::TextSizeHeader: return 18.0f;
    case MetricToken::TextSizeTitle: return 22.0f;
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
        return {8.0f, 8.0f, 8.0f, 8.0f};
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
    case TypographyToken::Display:   return 28.0f;
    case TypographyToken::Heading:  return 22.0f;
    case TypographyToken::Title:     return 16.0f;
    case TypographyToken::Subtitle:  return 14.0f;
    case TypographyToken::Body:      return 13.0f;
    case TypographyToken::Caption:   return 11.0f;
    case TypographyToken::Button:    return 13.0f;
    case TypographyToken::Monospace: return 12.0f;
    default:                         return 13.0f;
    }
}

int DefaultTheme::ResolveElevation(ElevationToken token) const {
    switch (token) {
    case ElevationToken::None:    return 0;
    case ElevationToken::Card:    return 1;
    case ElevationToken::Popup:   return 2;
    case ElevationToken::Window:  return 3;
    case ElevationToken::Overlay: return 4;
    default:                      return 0;
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
