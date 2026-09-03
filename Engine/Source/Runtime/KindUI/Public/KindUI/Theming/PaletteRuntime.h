#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Types.h"

namespace we::runtime::kindui::palette {

/// Mutable GraphiteDark colors — defaults from Palette.h, overridable via
/// Engine/Config/Themes/GraphiteDark.json (hot-reloads on save; no rebuild).
struct GraphiteDarkColors {
    Color Black{};
    Color Title{};
    Color Background{};
    Color WindowBorder{};
    Color Foldout{};
    Color Input{};
    Color InputOutline{};
    Color InputInsetInner{};
    Color InputInsetOuter{};
    Color Recessed{};
    Color Panel{};
    Color Header{};
    Color Dropdown{};
    Color DropdownOutline{};
    Color Hover{};
    Color Hover2{};
    Color Highlight{};
    Color Primary{};
    Color PrimaryHover{};
    Color PrimaryPress{};
    Color Secondary{};
    Color Select{};
    Color SelectInactive{};
    Color SelectParent{};
    Color SelectHover{};
    Color White{};
    Color White25{};
    Color Foreground{};
    Color ForegroundHover{};
    Color ForegroundInverted{};
    Color ForegroundHeader{};
    Color Notifications{};
    Color IconNormal{};
    Color IconHoverTint{};
    Color IconActiveTint{};
    Color IconSubdued{};
    Color Warning{};
    Color Error{};
    Color Success{};
    Color AccentBlue{};
    Color AccentPurple{};
    Color AccentPink{};
    Color AccentRed{};
    Color AccentOrange{};
    Color AccentYellow{};
    Color AccentGreen{};
    Color AccentBrown{};
    Color AccentBlack{};
    Color AccentGray{};
    Color AccentWhite{};
    Color AccentFolder{};
    Color TooltipBg{};
    Color DragGhost{};
    Color ActiveTabLine{};
    Color SelectionHighlight{};
    Color HighlightSubtle{};
    Color ModalScrim{};
    Color ShadowSubtle{};
    Color ShadowOverlay{};
    Color ShadowPopup{};
    Color ShadowColor{};
    Color FolderShadow{};
    Color ButtonBevelTop{};
    Color ButtonBevelBottom{};
    Color DebugGlyphBounds{};
};

/// Live palette used by GraphiteDarkTheme / Color::White|Black.
[[nodiscard]] KINDUI_API GraphiteDarkColors& GraphiteDarkLive();

/// Load JSON if needed; poll mtime and reload when the file changes.
/// Returns true when colors were reloaded this call.
KINDUI_API bool ReloadGraphiteDarkPaletteIfChanged();

} // namespace we::runtime::kindui::palette
