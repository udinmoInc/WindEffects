// ==============================================================================
// WindEffects — KindUI — SurfaceRole
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Core/Types.h"
#include "KindUI/Export.h"
#include "KindUI/Theme/DesignToken.h"

namespace we::runtime::kindui {

// Semantic surface roles for KindUI chrome.
// AppBackground → PanelSurface (headers/toolbars/body). PanelInner / InnerPanel
// is the recessed well for nested property rows, trees, and navigation panes.
// Input/Control are controls only. Hierarchy: Panel → PanelInner → Input.
enum class SurfaceRole : uint8_t {
    None = 0,
    Window,            // AppBackground
    Workspace,         // AppBackground
    Toolbar,           // PanelSurface
    DockChrome,        // AppBackground
    TabActive,
    TabInactive,
    Panel,             // shared PanelSurface
    PanelInner,        // InnerPanel — nested properties / trees / wells (one step darker)
    PanelRaised,       // alias → PanelSurface
    PanelHeader,       // alias → PanelSurface
    Category,          // Category / Foldout — property categorizer headers (distinct)
    Recessed,          // alias → PanelInner
    Input,             // InputSurface — controls only
    InputBorder,
    Control,           // ButtonSurface — controls only
    ControlHover,      // HoverSurface
    ControlPressed,
    Selected,          // SelectedSurface
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
    Primary,     // PrimaryText — labels, titles, values
    Secondary,   // SecondaryText — hints, metadata, supporting
    Hint,        // alias → Secondary
    Disabled,    // state; same color as Secondary
    OnAccent,    // contrast on accent fills only
    Header,      // alias → Primary
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
