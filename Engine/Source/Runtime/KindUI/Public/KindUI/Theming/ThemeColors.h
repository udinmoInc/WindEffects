#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Types.h"

namespace we::runtime::kindui {

class IThemeProvider;

// Semantic editor palette — single source for UI chrome and icon tinting.
struct KINDUI_API ThemeColors {
    Color background = Color::Transparent();
    Color panel = Color::Transparent();
    Color border = Color::Transparent();
    Color hover = Color::Transparent();
    Color pressed = Color::Transparent();
    Color selection = Color::Transparent();

    Color textPrimary = Color::White();
    Color textSecondary = Color::White();
    Color textDisabled = Color::White();

    Color iconPrimary = Color::White();
    Color iconSecondary = Color::White();
    Color iconAccent = Color::White();
    Color iconDisabled = Color::White();

    [[nodiscard]] static ThemeColors Resolve(const IThemeProvider& theme);
    [[nodiscard]] static ThemeColors Resolve();
};

enum class IconColorRole {
    Primary,
    Secondary,
    Accent,
    Disabled,
};

// Resolve mono icon tint: Secondary (gray) at rest; brightens on hover; Accent only when active.
[[nodiscard]] KINDUI_API Color ResolveIconColor(
    IconColorRole role,
    float hoverAnim = 0.0f,
    float pressStrength = 0.0f,
    bool accent = false);

[[nodiscard]] KINDUI_API Color ResolveIconColorForState(
    bool hovered,
    bool accent,
    bool disabled = false,
    bool secondary = false);

} // namespace we::runtime::kindui
