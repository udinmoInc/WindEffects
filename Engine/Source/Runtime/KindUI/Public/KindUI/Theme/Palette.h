// ==============================================================================
// WindEffects — KindUI — Palette
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Core/Types.h"
#include "KindUI/Core/ColorSpace.h"

namespace we::runtime::kindui::palette {

// Compile-time fallback defaults only.
// Edit live colors in Engine/Config/Themes/GraphiteDark.json (hot-reloads; no rebuild).
// Do not reference this struct from widgets; use ResolveColor / ds:: accessors.

struct GraphiteDark {

    // AppBackground vs one shared PanelSurface.
    // Nested container roles alias Panel. Input/Button are controls only.
    static constexpr Color Black             = Hex("#050505");
    static constexpr Color Title             = Hex("#0A0A0A");
    static constexpr Color Background        = Hex("#0C0C0C"); // AppBackground
    static constexpr Color WindowBorder      = Hex("#070707");
    static constexpr Color Foldout           = Hex("#202020"); // Category / foldout headers (distinct from Panel)
    static constexpr Color Category          = Hex("#202020"); // alias → Foldout — property categorizer bars
    static constexpr Color Input             = Hex("#0E0E0E"); // InputSurface
    static constexpr Color InputOutline      = Hex("#262626"); // dark gray hairline, subtle edge feel
    static constexpr Color BorderSeparator   = Hex("#0F0F0F");
    static constexpr Color BorderSubtle      = Hex("#222222");
    static constexpr Color BorderDefault     = Hex("#262626");
    static constexpr Color BorderLight       = Hex("#303030");
    static constexpr Color BorderFocus       = Hex("#0068D0");
    static constexpr Color BorderError       = Hex("#963B3B");
    static constexpr Color AxisX            = Hex("#E54B4B");
    static constexpr Color AxisY            = Hex("#4CAF50");
    static constexpr Color AxisZ            = Hex("#2196F3");
    static constexpr Color InputInsetInner   = Hex("#00000038");
    static constexpr Color InputInsetOuter   = Hex("#00000059");
    static constexpr Color Recessed          = Hex("#121212"); // legacy alias → InnerPanel
    static constexpr Color InnerPanel        = Hex("#121212"); // PanelInner — nested rows / trees / wells
    static constexpr Color Panel             = Hex("#181818"); // shared PanelSurface
    static constexpr Color Header            = Hex("#181818"); // same as Panel
    static constexpr Color Dropdown          = Hex("#262626"); // Popup / dropdown elevated surface
    static constexpr Color DropdownOutline   = Hex("#3C3C3C"); // Popup border outline
    static constexpr Color Hover             = Hex("#2C2C2C"); // HoverSurface
    static constexpr Color Hover2            = Hex("#2E2E2E");
    static constexpr Color Highlight         = Hex("#0068D0");
    static constexpr Color Primary           = Hex("#0068D0");
    static constexpr Color PrimaryHover      = Hex("#0878E5");
    static constexpr Color PrimaryPress      = Hex("#004A96");
    static constexpr Color ButtonPrimary      = Hex("#242424"); // ButtonSurface
    static constexpr Color ButtonPrimaryHover = Hex("#2D2D2D");
    static constexpr Color ButtonPrimaryPress = Hex("#1D1D1D");
    static constexpr Color Secondary         = Hex("#141414"); // PanelInner alias

    static constexpr Color Select            = Hex("#2C2C2C");
    static constexpr Color SelectInactive    = Hex("#262626");
    static constexpr Color SelectParent      = Hex("#222222");
    static constexpr Color SelectHover       = Hex("#2E2E2E");

    // ── Text — only two general UI text colors (neutral) ─────────────────────

    static constexpr Color White             = Hex("#D0D0D0");
    static constexpr Color White25           = Hex("#FFFFFF33");

    // PrimaryText — labels, titles, values, active readable content
    static constexpr Color PrimaryText       = Hex("#B8B8B8");
    static constexpr Color Foreground        = PrimaryText; // alias

    // Brighter text only when drawn on accent/filled controls (not a 3rd body color)
    static constexpr Color ForegroundHover   = Hex("#D2D2D2");

    // Text rendered over light Input surfaces
    static constexpr Color ForegroundInverted = Input;

    // Header text shares PrimaryText (no separate chroma)
    static constexpr Color ForegroundHeader  = PrimaryText;

    // SecondaryText — hints, metadata, descriptions, supporting copy
    static constexpr Color SecondaryText     = Hex("#929292");
    static constexpr Color Notifications     = SecondaryText; // alias

    // ── Icons (mono atlas tint — no separate "normal"; default = PrimaryText) ─

    // Hovered icon emphasis
    static constexpr Color IconHoverTint     = Hex("#D8D8D8");

    // Selected / active / important icons
    static constexpr Color IconActiveTint    = Hex("#D8D8D8");

    // Disabled / subdued / secondary icons
    static constexpr Color IconSubdued       = Hex("#8A8A8A");

    // 1px contact silhouette under glyphs (alpha; not a glow/box)
    static constexpr Color IconContactShadow = Hex("#00000073");

    // ── Status & accents (EStyleColor) ──────────────────────────────────────

    static constexpr Color Warning           = Hex("#FFB800");
    static constexpr Color Error             = Hex("#EF3535");
    static constexpr Color Success           = Hex("#1FE44B");

    // ── Accent colors ───────────────────────────────────────────────────────

    static constexpr Color AccentBlue        = Hex("#26BBFF");
    static constexpr Color AccentPurple      = Hex("#A139BF");
    static constexpr Color AccentPink        = Hex("#FF729C");
    static constexpr Color AccentRed         = Hex("#FF4040");
    static constexpr Color AccentOrange      = Hex("#FE9B07");
    static constexpr Color AccentYellow      = Hex("#FFDC1A");
    static constexpr Color AccentGreen       = Hex("#8BC24A");
    static constexpr Color AccentBrown       = Hex("#804D39");

    // Dark charcoal accent
    static constexpr Color AccentBlack       = Hex("#202020");

    // Neutral gray accent
    static constexpr Color AccentGray        = Hex("#707070");

    static constexpr Color AccentWhite       = White;
    static constexpr Color AccentFolder      = Hex("#A8844A");

    // ── Composites (alpha permitted — overlays / shadows only) ──────────────

    // Tooltip / popup overlays
    static constexpr Color TooltipBg         = Hex("#1A1A1AF5");
    static constexpr Color DragGhost         = Hex("#141416E6");

    // Selection / active tab
    static constexpr Color ActiveTabLine     = Hex("#0070E0CC");
    static constexpr Color SelectionHighlight = Hex("#0070E0E6");

    // Subtle white overlay
    static constexpr Color HighlightSubtle   = White25;

    // Modal / shadow layers
    static constexpr Color ModalScrim        = Hex("#0000008C");
    static constexpr Color ShadowSubtle      = Hex("#00000038");
    static constexpr Color ShadowOverlay     = Hex("#0000006B");
    static constexpr Color ShadowPopup       = Hex("#00000052");
    static constexpr Color ShadowColor       = Hex("#00000047");
    static constexpr Color FolderShadow      = Hex("#00000061");

    // ── Button bevel ────────────────────────────────────────────────────────
    // Toolbar / raised controls only — never full-surface fills.

    // Cool slate face — distinct from Panel/Header/Dropdown, not a darker clone.
    static constexpr Color ButtonFace        = Hex("#32353C");
    static constexpr Color ButtonFaceHover   = Hex("#3E424A");
    static constexpr Color ButtonFacePress   = Hex("#262930");
    // Near-black outer rim (alpha; drawn on the outside edge of the face).
    static constexpr Color ButtonInset       = Hex("#000000D0");

    static constexpr Color ButtonBevelTop    = Hex("#383A3D");
    static constexpr Color ButtonBevelBottom = Hex("#111213");

    // ── Diagnostics (WE_TEXT_DEBUG glyph bounds) ────────────────────────────

    static constexpr Color DebugGlyphBounds  = Hex("#FF729C59");
};

}
