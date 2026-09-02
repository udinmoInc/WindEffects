#pragma once

#include "KindUI/Core/Types.h"
#include "KindUI/Export.h"
#include "KindUI/Tokens/DesignToken.h"

namespace we::runtime::kindui {

// Phase 1 — semantic surface roles. Widgets resolve colors only through these roles.
enum class SurfaceRole : uint8_t {
    None = 0,
    Window,
    Workspace,
    Toolbar,
    DockChrome,
    TabActive,
    TabInactive,
    Panel,
    PanelHeader,
    Recessed,
    Input,
    InputBorder,
    Control,
    ControlHover,
    ControlPressed,
    Selected,
    SelectedInactive,
    Text,
    TextSecondary,
    TextHint,
    TextDisabled,
    Separator,
    Border,
    StatusBar,
    ViewportToolbar,
    Accent,
    Disabled,
    Popup,
    Transparent,
};

enum class TextRole : uint8_t {
    Primary,
    Secondary,
    Hint,
    Disabled,
    OnAccent,
    Header,
};

enum class ControlState : uint8_t {
    Normal,
    Hover,
    Pressed,
    Checked,
    Selected,
    SelectedInactive,
    Disabled,
    Focused,
};

enum class ControlKind : uint8_t {
    Generic,
    Button,
    TreeRow,
    Tab,
    Input,
};

/// Maps legacy ColorToken values to semantic surface roles (for migration).
[[nodiscard]] KINDUI_API SurfaceRole SurfaceRoleFromColorToken(ColorToken token);

/// Single central mapping: SurfaceRole → ColorToken (→ GraphiteDark palette).
[[nodiscard]] KINDUI_API ColorToken SurfaceRoleToColorToken(SurfaceRole role);
[[nodiscard]] KINDUI_API TextRole TextRoleFromSurfaceRole(SurfaceRole role);
[[nodiscard]] KINDUI_API const char* SurfaceRoleName(SurfaceRole role);
[[nodiscard]] KINDUI_API const char* TextRoleName(TextRole role);
[[nodiscard]] KINDUI_API const char* ControlStateName(ControlState state);

[[nodiscard]] KINDUI_API Color ResolveSurfaceColor(SurfaceRole role);
[[nodiscard]] KINDUI_API Color ResolveTextColor(TextRole role);
[[nodiscard]] KINDUI_API Color ResolveControlColor(ControlKind kind, ControlState state);

/// Interactive blend on top of a semantic surface (hover/press/selected).
[[nodiscard]] KINDUI_API Color ResolveInteractiveSurfaceColor(
    SurfaceRole baseRole,
    float hoverAnim,
    float pressAnim,
    bool selected = false);

} // namespace we::runtime::kindui
