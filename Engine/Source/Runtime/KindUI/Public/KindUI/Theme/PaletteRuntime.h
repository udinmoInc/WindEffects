// ==============================================================================
// WindEffects — KindUI — PaletteRuntime
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
    Color Category{};
    Color Input{};
    Color InputOutline{};
    Color InputInsetInner{};
    Color InputInsetOuter{};
    Color Recessed{};
    Color InnerPanel{};
    Color BorderSeparator{};
    Color BorderSubtle{};
    Color BorderDefault{};
    Color BorderLight{};
    Color BorderFocus{};
    Color BorderError{};
    Color AxisX{};
    Color AxisY{};
    Color AxisZ{};
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
    Color ButtonPrimary{};
    Color ButtonPrimaryHover{};
    Color ButtonPrimaryPress{};
    Color Secondary{};
    Color Select{};
    Color SelectInactive{};
    Color SelectParent{};
    Color SelectHover{};
    Color White{};
    Color White25{};
    Color PrimaryText{};
    Color SecondaryText{};
    Color Foreground{};
    Color ForegroundHover{};
    Color ForegroundInverted{};
    Color ForegroundHeader{};
    Color Notifications{};
    // Legacy alias of PrimaryText (IconNormal theme key removed).
    Color IconNormal{};
    Color IconHoverTint{};
    Color IconActiveTint{};
    Color IconSubdued{};
    Color IconContactShadow{};
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
    Color ScrollbarTrack{};
    Color ScrollbarThumb{};
    Color ScrollbarThumbHover{};
    Color DebugGlyphBounds{};
};

/// Hot-reloadable metrics from GraphiteDark.json "Metrics" object.
struct GraphiteDarkMetrics {
    float CornerRadiusSmall = 4.0f;
    float CornerRadiusMedium = 4.0f;
    float CornerRadiusLarge = 10.0f;
    float WindowCornerRadius = 10.0f;
    float PanelCornerRadius = 0.0f;
    float BorderWidth = 1.0f;
    float PanelDividerWidth = 1.0f;
    float SplitterThickness = 1.0f;
    float FocusRingWidth = 1.0f;
    float PanelHeaderHeight = 28.0f;
    float PanelTabHeight = 28.0f;
    float PanelToolbarHeight = 26.0f;
    float ToolbarHeight = 26.0f;
    float ViewportToolbarHeight = 26.0f;
    float BreadcrumbBarHeight = 26.0f;
    float HeaderControlHeight = 24.0f;
    float IconButtonSize = 24.0f;
    float ButtonHeight = 24.0f;
    float SearchBoxHeight = 24.0f;
    float NavigationButtonSize = 24.0f;
    float ToolbarLabeledHeight = 24.0f;
    float ControlHeightCompact = 24.0f;
    float ControlHeightLarge = 34.0f;
    float InputWidthCompact = 80.0f;
    float InputWidthDefault = 120.0f;
    float InputWidthLarge = 160.0f;
    float FormRowHeight = 36.0f;
    float MenuItemHeight = 26.0f;
    float PageMargin = 16.0f;
    float SectionGap = 12.0f;
    float CardPadding = 12.0f;
    float ContentGap = 8.0f;
    float FormRowGap = 2.0f;
    float LabelHintGap = 4.0f;
    float ListRowHeight = 22.0f;
    float CategoryHeaderHeight = 36.0f;
    float TitleBarHeight = 32.0f;
    float WindowControlWidth = 40.0f;
    float IconSizeSearch = 16.0f;
    float IconSizeTree = 16.0f;
    float IconSizeToolbar = 16.0f;
    float IconSizeNavigation = 16.0f;
    float IconSizePrimary = 16.0f;
    float IconSizeVerySmall = 16.0f;
    float IconSizeWindowControl = 16.0f;
    float IconButtonRadius = 3.0f;
    float ButtonPaddingHorizontal = 6.0f;
    float ButtonSpacing = 2.0f;
    float ButtonGroupSpacing = 8.0f;
    float ScrollbarWidth = 14.0f;
    float ScrollbarThumbMinHeight = 20.0f;
    float TabTopRadius = 6.0f;
    float TabActiveIndicatorHeight = 2.0f;
    float StatusBarHeight = 34.0f;
    float TabGap = 3.0f;
    float TabIconGap = 6.0f;
    float TabCloseGap = 10.0f;
    float TabMinWidth = 160.0f;
    float CloseGlyphSize = 12.0f;
    float TabStripPadH = 0.0f;
    float TabStripPadV = 0.0f;
    float TabActiveIndicatorWidth = 0.0f;
    float TabPaddingH = 10.0f;
    float TabPaddingV = 4.0f;
    float DockPanelGap = 8.0f;
    float ChromeSeparationGap = 2.0f;
    float ChromeSeparationGapWide = 2.0f;
    float ToolbarSeparatorWidth = 2.0f;
    float ToolbarSeparatorHeight = 20.0f;
    float ToolbarLabeledMinWidth = 36.0f;
    float PropertyLabelColumnWidth = 120.0f;
    float PropertyIndentStep = 16.0f;
    float TreeIndentWidth = 16.0f;
    float TreeExpanderHitSize = 18.0f;
    float PopupMinWidth = 140.0f;
    float PopupMaxWidth = 340.0f;
    float PopupMaxHeight = 360.0f;
    float TooltipMinWidth = 180.0f;
    float ToggleTrackWidth = 34.0f;
    float ToggleTrackHeight = 18.0f;
    float CheckboxGlyphSize = 14.0f;
    float PrimaryButtonHeight = 24.0f;
    float ContentBrowserGridPadding = 16.0f;
    float ContentBrowserGridHSpacing = 12.0f;
    float ContentBrowserGridVSpacing = 14.0f;
    float ContentBrowserThumbLarge = 96.0f;
    float ContentBrowserThumbMedium = 72.0f;
    float ContentBrowserThumbSmall = 48.0f;
    float ContentBrowserCellLarge = 112.0f;
    float ContentBrowserCellMedium = 80.0f;
    float ContentBrowserCellSmall = 56.0f;
    float DragThreshold = 6.0f;
    float MenuPadding = 3.0f;
    float CheckMarkSize = 16.0f;
    float MenuTextIndent = 20.0f;
    float SpaceXS = 2.0f;
    float Space1 = 4.0f;
    float SpaceMD = 5.0f;
    float Space2 = 8.0f;
    float Space3 = 12.0f;
    float Space4 = 16.0f;
    float Space5 = 20.0f;
    float Space6 = 24.0f;
    float HoverAnimationDamping = 28.0f;
    float PressAnimationDamping = 36.0f;
    float PressOffset = 1.0f;
    float ShadowBlurSmall = 4.0f;
    float ShadowBlurMedium = 8.0f;
    float ShadowSpreadMedium = 16.0f;
    float FontSizeTitle = 18.0f;
    float FontSizeHeader = 14.0f;
    float FontSizeNormal = 13.0f;
    float FontSizeCaption = 12.0f;
};

/// Live palette used by GraphiteDarkTheme / Color::White|Black.
[[nodiscard]] KINDUI_API GraphiteDarkColors& GraphiteDarkLive();

/// Live metrics (TabTopRadius, …) — edit Metrics in GraphiteDark.json; no rebuild.
[[nodiscard]] KINDUI_API GraphiteDarkMetrics& GraphiteDarkLiveMetrics();

/// Load JSON if needed; poll mtime and reload when the file changes.
/// Returns true when colors/metrics were reloaded this call.
KINDUI_API bool ReloadGraphiteDarkPaletteIfChanged();

/// Force reload of the active theme JSON regardless of mtime.
KINDUI_API bool ForceReloadActiveThemePalette();

}
