#pragma once

#include "KindUI/Core/Types.h"
#include "KindUI/Core/ColorSpace.h"

namespace we::runtime::kindui::palette {

// Compile-time fallback defaults only.
// Edit live colors in Engine/Config/Themes/GraphiteDark.json (hot-reloads; no rebuild).
// ColorToken resolves via GraphiteDarkTheme → palette::GraphiteDarkLive().
// Do not reference this struct from widgets; use ResolveColor / ds:: accessors.

struct GraphiteDark {

    // ── Base surfaces (EStyleColor) ─────────────────────────────────────────
    // Dark charcoal hierarchy:
    // Black      → deepest level
    // Input      → recessed controls
    // Foldout    → recessed control wells
    // Recessed   → tree/grid wells
    // Background → main workspace
    // Panel      → panel body
    // Header     → panel/section header
    // Dropdown   → raised popup surface

    static constexpr Color Black             = Hex("#000000");

    // Main editor chrome / title strip
    static constexpr Color Title             = Hex("#161616");

    // Main workspace — deep charcoal
    static constexpr Color Background        = Hex("#101010");

    // Very dark separators / window edges
    static constexpr Color WindowBorder      = Hex("#0C0C0C");

    // Recessed control wells
    static constexpr Color Foldout           = Hex("#111111");

    // Inputs / pressed wells
    static constexpr Color Input             = Hex("#0E0E0E");

    // Input and control borders
    static constexpr Color InputOutline      = Hex("#303030");

    // Recessed input top inner edge — darker charcoal, low contrast against Input.
    static constexpr Color InputInsetInner   = Hex("#0C0C0C66");
    // Optional outer lip for non-input chrome (panels / cards).
    static constexpr Color InputInsetOuter   = Hex("#000000A8");

    // Tree / grid wells
    static constexpr Color Recessed          = Hex("#151515");

    // Main panel body — subtle separation from #181818
    static constexpr Color Panel             = Hex("#1C1C1C");

    // Section / panel headers
    static constexpr Color Header            = Hex("#252525");

    // Raised menus / cards / popups
    static constexpr Color Dropdown          = Hex("#2D2D2D");

    // Raised control / popup edges
    static constexpr Color DropdownOutline   = Hex("#424242");


    // ── Interaction (EStyleColor) ───────────────────────────────────────────

    // Row / control hover — visible but not bright gray
    static constexpr Color Hover             = Hex("#353535");

    // Muted text / scrollbar hover
    static constexpr Color Hover2            = Hex("#707070");

    // Primary interaction blue
    static constexpr Color Highlight         = Hex("#0070E0");
    static constexpr Color Primary           = Hex("#0070E0");
    static constexpr Color PrimaryHover      = Hex("#0E86FF");
    static constexpr Color PrimaryPress      = Hex("#0050A0");

    // Legacy EStyleColor name
    static constexpr Color Secondary         = Dropdown;

    // Selection
    static constexpr Color Select            = Primary;

    // Inactive selection
    static constexpr Color SelectInactive    = Hex("#40576F");

    // Parent selection — subtle dark blue-gray
    static constexpr Color SelectParent      = Hex("#2C323A");

    // Hovered selected item
    static constexpr Color SelectHover       = Panel;


    // ── Text (EStyleColor) ──────────────────────────────────────────────────

    static constexpr Color White             = Hex("#FFFFFF");
    static constexpr Color White25           = Hex("#FFFFFF40");

    // Main editor text
    static constexpr Color Foreground        = Hex("#C0C0C0");

    // Hovered text
    static constexpr Color ForegroundHover   = White;

    // Text rendered over Input surfaces
    static constexpr Color ForegroundInverted = Input;

    // Header / section text
    static constexpr Color ForegroundHeader  = Hex("#C8C8C8");

    // Disabled / notification label text
    static constexpr Color Notifications     = Hex("#464B50");


    // ── Icons (mono atlas tint targets — separate from body text) ───────────

    // Normal inactive toolbar / panel icons — lifted for dark-surface contrast
    static constexpr Color IconNormal        = Hex("#C2C8D0");

    // Hovered icon emphasis
    static constexpr Color IconHoverTint     = Hex("#D6DBE1");

    // Selected / active / important icons
    static constexpr Color IconActiveTint    = White;

    // Disabled / subdued icons
    static constexpr Color IconSubdued       = Hex("#5C6570");

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
    static constexpr Color AccentFolder      = Hex("#C09A5A");


    // ── Composites (alpha permitted — overlays / shadows only) ──────────────

    // Tooltip / popup overlays
    static constexpr Color TooltipBg         = Hex("#2D2D2DF7");
    static constexpr Color DragGhost         = Hex("#2D2D2DE6");

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

    static constexpr Color ButtonBevelTop    = Hex("#383A3D");
    static constexpr Color ButtonBevelBottom = Hex("#111213");


    // ── Diagnostics (WE_TEXT_DEBUG glyph bounds) ────────────────────────────

    static constexpr Color DebugGlyphBounds  = Hex("#FF729C59");
};

} 