// ==============================================================================
// WindEffects — KindUI — DesignToken
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

// Semantic color tokens — one role per entry.
// Concrete values live in Palette.h (GraphiteDark) and theme ResolveColor().
// Multiple tokens may share a palette entry when roles intentionally match.

enum class ColorToken : uint32_t {
    // ── Surfaces (dark panel defaults) ───────────────────────
    WindowBackground,
    WorkspaceBackground,
    DockChromeBackground,
    PanelBackground,
    TabActiveBackground,
    SecondarySurface,
    CardBackground,
    HeaderBackground,
    ListLabelBandBackground,
    ToolbarBackground,
    TabBackground,
    InputBackground,
    ControlBackground,
    PopupBackground,
    TooltipBackground,
    DisabledBackground,
    StatusBarBackground,
    ViewportToolbarBackground,
    ScrollbarTrack,

    // ── Interaction states ────────────────────────────────────────────────────
    HoverBackground,
    PressedBackground,
    SelectedBackground,
    SelectInactiveBackground,
    SelectParentBackground,
    SelectHoverBackground,
    ControlBackgroundHover,
    ControlBackgroundPressed,
    ControlBackgroundDisabled,
    ControlBackgroundSelected,

    // ── Borders & separators ──────────────────────────────────────────────────
    Separator,
    BorderSubtle,
    BorderDefault,
    BorderLight,
    BorderFocus,
    BorderError,

    // ── Axis colors ───────────────────────────────────────────────────────────
    AxisX,
    AxisY,
    AxisZ,

    // ── Text hierarchy ──────────────────────────────────────────────────────
    TextPrimary,
    TextSecondary,
    TextHint,
    TextDisabled,
    TextOnAccent,
    LinkForeground,
    SearchPlaceholder,

    // ── Icons ─────────────────────────────────────────────────────────────────
    IconPrimary,
    IconSecondary,
    IconDisabled,
    IconAccent,
    IconHover,
    IconActive,
    IconContactShadow,

    // ── Accent & selection ────────────────────────────────────────────────────
    AccentPrimary,
    AccentHover,
    AccentOrange,
    ActiveTabLine,
    SelectionHighlight,

    // ── Semantic status ───────────────────────────────────────────────────────
    Success,
    Warning,
    ErrorForeground,
    InfoColor,
    PlayForeground,
    CloseButtonHover,

    // ── Buttons ───────────────────────────────────────────────────────────────
    ButtonPrimaryBackground,
    ButtonPrimaryHover,
    ButtonPrimaryPressed,
    ButtonDangerBackground,
    ButtonDangerHover,
    ButtonDangerPressed,

    // ── Gizmo / viewport helpers ──────────────────────────────────────────────
    GizmoBackground,
    GizmoAxisX,
    GizmoAxisY,
    GizmoAxisZ,

    // ── Button chrome (bevel edges — not surface fills) ───────────────────────
    ButtonBevelHighlight,
    ButtonBevelShadow,

    // ── Input recessed edge chrome (charcoal only — never white/highlight) ────
    InputInsetInner,
    InputInsetOuter,

    // ── Depth & overlays ──────────────────────────────────────────────────────
    HighlightSubtle,
    ShadowSubtle,
    ShadowOverlay,
    ShadowPopup,
    ShadowColor,
    ModalScrim,
    DragGhostBackground,

    // ── Content-browser folder art ────────────────────────────────────────────
    ContentBrowserFolderShadow,
    ContentBrowserFolderEdge,
    ContentBrowserFolderHighlight,
    ContentBrowserFolderTab,
    ContentBrowserFolderPrimary,
    ContentBrowserFolderBody,

    // ── Scrollbar ─────────────────────────────────────────────────────────────
    ScrollbarThumb,
    ScrollbarThumbHover,

    // ── Diagnostics ───────────────────────────────────────────────────────────
    DebugGlyphBounds,       // Text debug glyph wireframe fill
};

enum class SpacingToken : uint32_t {
    None,
    ExtraSmall,
    Small,
    Medium,
    Large,
    ExtraLarge,
    Huge,
};

// Semantic control height roles (maps to MetricToken heights).
enum class ControlSize : uint32_t {
    Compact,
    Default,
    Large,
};

enum class RadiusToken : uint32_t {
    None,
    Small,
    Medium,
    Large,
    Full,
};

enum class TypographyToken : uint32_t {
    WindowTitle,
    PageTitle,
    SectionTitle,
    CardTitle,
    DialogTitle,

    Display,
    Heading1,
    Heading2,
    Heading3,
    Heading4,
    Heading5,
    Heading6,
    Heading,

    // Body — PrimaryText / SecondaryText / Caption / Hint
    Title,
    Subtitle,
    Body,
    BodyStrong,
    Caption,
    CaptionSmall,
    Hint,

    Label,
    Button,
    Tab,
    Toolbar,
    Menu,
    Tooltip,
    TableHeader,
    Status,
    StatusBar,
    Navigation,
    PropertyLabel,
    PropertyValue,

    Code,
    Console,
    Monospace,
    Link,
    Error,
    Warning,
    Success,
    Disabled,
};

// Semantic elevation: Window < Panel < Card < Control < Overlay < Popup (visual brightness).
// ResolveElevation returns shadow intensity (0 = flat), not brightness order.
enum class ElevationToken : uint32_t {
    None,
    Window,
    Panel,
    Card,
    Control,
    Overlay,
    Popup,
};

enum class AnimationToken : uint32_t {
    Instant,
    Fast,
    Normal,
    Slow,
};

// Layout / chrome metrics not covered by Spacing/Radius/Typography.
enum class MetricToken : uint32_t {
    CornerRadiusSmall,
    CornerRadiusMedium,
    CornerRadiusLarge,
    WindowCornerRadius,

    TextSizeMenu,
    TextSizeToolbar,
    TextSizeTabs,
    TextSizeNormal,
    TextSizeProperty,
    TextSizeCaption,
    TextSizeWindow,
    TextSizeHeader,
    TextSizeBody,
    TextSizeSmall,
    TextSizeCategory,
    TextSizeTitle,
    TextCharWidthRatio,

    BorderWidth,
    PanelDividerWidth,
    SplitterThickness,
    FocusRingWidth,

    PanelHeaderHeight,
    PanelTabHeight,
    PanelToolbarHeight,
    ListRowHeight,
    CategoryHeaderHeight,
    TitleBarHeight,
    HeaderControlHeight,
    WindowControlWidth,
    ToolbarHeight,
    SearchBoxHeight,
    IconButtonSize,
    ButtonHeight,
    ControlHeightCompact,
    ControlHeightLarge,
    InputWidthCompact,
    InputWidthDefault,
    InputWidthLarge,
    FormRowHeight,
    MenuItemHeight,
    PageMargin,
    SectionGap,
    CardPadding,          // inner card / group padding
    ContentGap,           // default stack gap between content blocks
    FormRowGap,           // vertical padding around form rows
    LabelHintGap,
    NavigationButtonSize,
    IconSizeSearch,
    IconSizeToolbar,
    IconSizePrimary,
    IconSizeTree,
    IconSizeNavigation,
    IconSizeVerySmall,
    IconSizeWindowControl,
    IconButtonRadius,
    ButtonPaddingHorizontal,
    ButtonSpacing,
    ButtonGroupSpacing,
    ScrollbarWidth,
    ScrollbarThumbMinHeight,

    TabTopRadius,
    TabActiveIndicatorHeight,
    TabGap,
    TabIconGap,
    TabCloseGap,
    TabMinWidth,
    CloseGlyphSize,
    TabStripPadH,
    TabStripPadV,
    TabActiveIndicatorWidth,
    TabPaddingH,               // dock tab horizontal inner padding
    TabPaddingV,               // dock tab vertical inner padding
    DockPanelGap,              // legacy alias — dock gutter (logical px, use ChromeSeparationGap)
    ChromeSeparationGap,
    ChromeSeparationGapWide,
    ViewportToolbarHeight,
    StatusBarHeight,

    ToolbarSeparatorHeight,
    ToolbarLabeledHeight,
    ToolbarLabeledMinWidth,

    BreadcrumbBarHeight,
    PropertyLabelColumnWidth,
    PropertyIndentStep,
    TreeIndentWidth,
    TreeExpanderHitSize,

    PopupMinWidth,
    PopupMaxWidth,
    PopupMaxHeight,
    TooltipMinWidth,
    ToggleTrackWidth,
    ToggleTrackHeight,
    CheckboxGlyphSize,
    PrimaryButtonHeight,

    ContentBrowserGridPadding,
    ContentBrowserGridHSpacing,
    ContentBrowserGridVSpacing,
    ContentBrowserThumbLarge,
    ContentBrowserThumbMedium,
    ContentBrowserThumbSmall,
    ContentBrowserCellLarge,
    ContentBrowserCellMedium,
    ContentBrowserCellSmall,

    DragThreshold,
    MenuPadding,
    CheckMarkSize,
    MenuTextIndent,

    SpaceXS,
    Space1,
    SpaceMD,
    Space2,
    Space3,
    Space4,
    Space5,
    Space6,

    HoverAnimationDamping,
    PressAnimationDamping,
    PressOffset,

    ShadowBlurSmall,
    ShadowBlurMedium,
    ShadowSpreadMedium,

    ToolbarSeparatorWidth,
};

enum class PaddingToken : uint32_t {
    Panel,
    Button,
    Card,
    Page,
    Input,
    FormRow,
    PaddingPanelLeft,
    PaddingPanelTop,
    PaddingPanelRight,
    PaddingPanelBottom,
    PaddingButtonLeft,
    PaddingButtonTop,
    PaddingButtonRight,
    PaddingButtonBottom,
};

}
