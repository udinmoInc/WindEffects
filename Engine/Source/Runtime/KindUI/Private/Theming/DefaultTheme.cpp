#include "KindUI/Theming/DefaultTheme.h"

namespace we::runtime::kindui {

Color DefaultTheme::ResolveColor(ColorToken token) const {
    switch (token) {
    case ColorToken::PrimarySurface:     return {0.12f, 0.12f, 0.13f, 1.0f};
    case ColorToken::SecondarySurface:   return {0.16f, 0.16f, 0.17f, 1.0f};
    case ColorToken::TertiarySurface:    return {0.20f, 0.20f, 0.21f, 1.0f};
    case ColorToken::AccentSurface:      return {0.25f, 0.45f, 0.85f, 1.0f};
    case ColorToken::WindowBackground:   return {0.10f, 0.10f, 0.11f, 1.0f};
    case ColorToken::ControlBackground:  return {0.18f, 0.18f, 0.19f, 1.0f};
    case ColorToken::ControlBackgroundHover:    return {0.22f, 0.22f, 0.24f, 1.0f};
    case ColorToken::ControlBackgroundPressed:  return {0.14f, 0.14f, 0.15f, 1.0f};
    case ColorToken::ControlBackgroundDisabled: return {0.14f, 0.14f, 0.15f, 0.5f};
    case ColorToken::ControlBackgroundSelected: return {0.25f, 0.45f, 0.85f, 0.3f};
    case ColorToken::BorderDefault:      return {0.30f, 0.30f, 0.32f, 1.0f};
    case ColorToken::BorderSubtle:       return {0.22f, 0.22f, 0.24f, 1.0f};
    case ColorToken::BorderFocused:      return {0.25f, 0.45f, 0.85f, 1.0f};
    case ColorToken::BorderError:        return {0.85f, 0.25f, 0.25f, 1.0f};
    case ColorToken::TextPrimary:        return {0.92f, 0.92f, 0.93f, 1.0f};
    case ColorToken::TextSecondary:      return {0.65f, 0.65f, 0.67f, 1.0f};
    case ColorToken::TextDisabled:       return {0.45f, 0.45f, 0.47f, 1.0f};
    case ColorToken::TextOnAccent:       return {1.0f, 1.0f, 1.0f, 1.0f};
    case ColorToken::TextLink:           return {0.40f, 0.65f, 1.0f, 1.0f};
    case ColorToken::IconPrimary:        return {0.85f, 0.85f, 0.87f, 1.0f};
    case ColorToken::IconSecondary:      return {0.60f, 0.60f, 0.62f, 1.0f};
    case ColorToken::IconDisabled:       return {0.40f, 0.40f, 0.42f, 1.0f};
    case ColorToken::SuccessColor:       return {0.20f, 0.70f, 0.35f, 1.0f};
    case ColorToken::WarningColor:       return {0.90f, 0.65f, 0.15f, 1.0f};
    case ColorToken::ErrorColor:         return {0.85f, 0.25f, 0.25f, 1.0f};
    case ColorToken::InfoColor:          return {0.25f, 0.55f, 0.90f, 1.0f};
    case ColorToken::ScrimOverlay:       return {0.0f, 0.0f, 0.0f, 0.5f};
    case ColorToken::ShadowColor:        return {0.0f, 0.0f, 0.0f, 0.3f};
    default:                             return {0.5f, 0.5f, 0.5f, 1.0f};
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
