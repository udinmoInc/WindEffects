#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

// Platform-neutral semantic design tokens.
// Applications supply concrete values through themes; KindUI never hardcodes branding.

enum class ColorToken : uint32_t {
    PrimarySurface,
    SecondarySurface,
    TertiarySurface,
    AccentSurface,
    WindowBackground,
    ControlBackground,
    ControlBackgroundHover,
    ControlBackgroundPressed,
    ControlBackgroundDisabled,
    ControlBackgroundSelected,
    BorderDefault,
    BorderSubtle,
    BorderFocused,
    BorderError,
    TextPrimary,
    TextSecondary,
    TextDisabled,
    TextOnAccent,
    TextLink,
    IconPrimary,
    IconSecondary,
    IconDisabled,
    SuccessColor,
    WarningColor,
    ErrorColor,
    InfoColor,
    ScrimOverlay,
    ShadowColor,
};

enum class SpacingToken : uint32_t {
    None,
    ExtraSmall,
    Small,
    Medium,
    Large,
    ExtraLarge,
};

enum class RadiusToken : uint32_t {
    None,
    Small,
    Medium,
    Large,
    Full,
};

enum class TypographyToken : uint32_t {
    Display,
    Heading,
    Title,
    Subtitle,
    Body,
    Caption,
    Button,
    Monospace,
};

enum class ElevationToken : uint32_t {
    None,
    Card,
    Popup,
    Window,
    Overlay,
};

enum class AnimationToken : uint32_t {
    Instant,
    Fast,
    Normal,
    Slow,
};

} // namespace we::runtime::kindui
