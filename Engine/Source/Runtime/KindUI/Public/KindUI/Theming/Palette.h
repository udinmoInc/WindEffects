#pragma once

#include "KindUI/Core/Types.h"
#include "KindUI/Core/ColorSpace.h"

namespace we::runtime::kindui::palette {

// UE5 Slate EStyleColor dark defaults — the only place HEX literals may be authored.
// ColorToken resolves semantic roles onto these entries via GraphiteDarkTheme::ResolveColor.
// Do not reference this struct from widgets; use ResolveColor / ds:: accessors.

struct GraphiteDark {
    // ── Base surfaces (EStyleColor) ─────────────────────────────────────────
    static constexpr Color Black             = Hex("#000000");
    static constexpr Color Title             = Hex("#151515"); // window chrome / title strip
    static constexpr Color Background        = Hex("#151515"); // workspace / tab strip / toolbar
    static constexpr Color WindowBorder      = Hex("#0F0F0F"); // dividers / separators
    static constexpr Color Foldout           = Hex("#0F0F0F"); // disabled control wells
    static constexpr Color Input             = Hex("#0F0F0F"); // inputs / pressed wells
    static constexpr Color InputOutline      = Hex("#383838"); // input & control borders
    static constexpr Color Recessed          = Hex("#1A1A1A"); // tree / grid wells
    static constexpr Color Panel             = Hex("#242424"); // panel body / active tab
    static constexpr Color Header            = Hex("#2F2F2F"); // section / panel headers
    static constexpr Color Dropdown          = Hex("#383838"); // menus / cards / popups
    static constexpr Color DropdownOutline   = Hex("#4C4C4C"); // raised control / popup edges

    // ── Interaction (EStyleColor) ─────────────────────────────────────────────
    static constexpr Color Hover             = Hex("#575757"); // row / control hover only
    static constexpr Color Hover2            = Hex("#808080"); // muted text / scrollbar hover
    static constexpr Color Highlight         = Hex("#0070E0");
    static constexpr Color Primary           = Hex("#0070E0");
    static constexpr Color PrimaryHover      = Hex("#0E86FF");
    static constexpr Color PrimaryPress      = Hex("#0050A0");
    static constexpr Color Secondary         = Hex("#383838"); // legacy EStyleColor name (= Dropdown)
    static constexpr Color Select            = Primary;
    static constexpr Color SelectInactive    = Hex("#40576F");
    static constexpr Color SelectParent      = Hex("#2C323A");
    static constexpr Color SelectHover       = Panel;

    // ── Text (EStyleColor) ────────────────────────────────────────────────────
    static constexpr Color White             = Hex("#FFFFFF");
    static constexpr Color White25           = Hex("#FFFFFF40");
    static constexpr Color Foreground        = Hex("#C0C0C0");
    static constexpr Color ForegroundHover   = Hex("#FFFFFF");
    static constexpr Color ForegroundInverted = Input;
    static constexpr Color ForegroundHeader  = Hex("#C8C8C8");
    static constexpr Color Notifications     = Hex("#464B50"); // disabled label text

    // ── Status & accents (EStyleColor) ────────────────────────────────────────
    static constexpr Color Warning           = Hex("#FFB800");
    static constexpr Color Error             = Hex("#EF3535");
    static constexpr Color Success           = Hex("#1FE44B");

    static constexpr Color AccentBlue        = Hex("#26BBFF");
    static constexpr Color AccentPurple      = Hex("#A139BF");
    static constexpr Color AccentPink        = Hex("#FF729C");
    static constexpr Color AccentRed         = Hex("#FF4040");
    static constexpr Color AccentOrange      = Hex("#FE9B07");
    static constexpr Color AccentYellow      = Hex("#FFDC1A");
    static constexpr Color AccentGreen       = Hex("#8BC24A");
    static constexpr Color AccentBrown       = Hex("#804D39");
    static constexpr Color AccentBlack       = Hex("#242424");
    static constexpr Color AccentGray        = Hex("#808080"); // = Hover2
    static constexpr Color AccentWhite       = Hex("#FFFFFF");
    static constexpr Color AccentFolder      = Hex("#B68F55");

    // ── Composites (alpha permitted — overlays / shadows only) ────────────────
    static constexpr Color TooltipBg         = Hex("#383838F7");
    static constexpr Color DragGhost           = Hex("#383838E6");
    static constexpr Color ActiveTabLine       = Hex("#0070E0CC");
    static constexpr Color SelectionHighlight  = Hex("#0070E0E6");
    static constexpr Color HighlightSubtle     = White25;
    static constexpr Color ModalScrim          = Hex("#0000008C");
    static constexpr Color ShadowSubtle        = Hex("#00000038");
    static constexpr Color ShadowOverlay       = Hex("#0000006B");
    static constexpr Color ShadowPopup         = Hex("#00000052");
    static constexpr Color ShadowColor         = Hex("#00000047");
    static constexpr Color FolderShadow        = Hex("#00000061");

    // ── Button bevel (toolbar / raised controls only — never full-surface fills)
    static constexpr Color ButtonBevelTop      = Hex("#383A3D");
    static constexpr Color ButtonBevelBottom   = Hex("#111213");
};

} // namespace we::runtime::kindui::palette
