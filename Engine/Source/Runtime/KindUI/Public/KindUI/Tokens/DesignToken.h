#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

// Semantic color tokens — one role per entry.
// Concrete values live in Palette.h (GraphiteDark) and theme ResolveColor().
// Multiple tokens may share a palette entry when roles intentionally match.

enum class ColorToken : uint32_t {
    // ── Surfaces (elevation: Window → Panel → Card → Control → Popup) ───────
    WindowBackground,       // Outermost chrome (#151515)
    WorkspaceBackground,    // Dock/viewport void (#151515)
    PanelBackground,        // Panel body fill (#242424)
    SecondarySurface,       // Recessed secondary panels (#1D1E20)
    CardBackground,         // Raised cards / popups (#222326)
    HeaderBackground,       // Panel header / active tab (#1C1D1F)
    ToolbarBackground,      // Toolbar strips (#1C1D1F)
    TabBackground,          // Inactive dock tab (#292A2D)
    InputBackground,        // Text/numeric fields (#141517)
    ControlBackground,      // Generic control wells (#141517)
    PopupBackground,        // Dropdowns / menus (#222326)
    TooltipBackground,      // Tooltips (#222326 @ 97%)
    DisabledBackground,     // Disabled control fill (#111214)
    StatusBarBackground,    // Bottom status bar (#151515)
    ViewportToolbarBackground, // Floating viewport toolbar (#1C1D1F @ 96%)
    ScrollbarTrack,         // Scroll gutter (#1A1A1A)

    // ── Interaction states ────────────────────────────────────────────────────
    HoverBackground,
    PressedBackground,
    SelectedBackground,
    ControlBackgroundHover,
    ControlBackgroundPressed,
    ControlBackgroundDisabled,
    ControlBackgroundSelected,

    // ── Borders & separators ──────────────────────────────────────────────────
    Separator,              // 1px panel edges (#232427)
    BorderSubtle,           // Subtle panel border (#292A2D)
    BorderDefault,          // Default control border (#292A2D)
    BorderLight,            // Emphasized border (#38393C)
    BorderFocus,            // Focus ring (#38393C)
    BorderError,            // Validation error (#E05252)

    // ── Text hierarchy ──────────────────────────────────────────────────────
    TextPrimary,            // Body / labels (#E6E6E6)
    TextSecondary,          // Supporting copy (#A0A1A3)
    TextHint,               // Placeholders / muted (#707174)
    TextDisabled,           // Disabled text (#4F5053)
    TextOnAccent,           // Text on filled buttons (#E6E6E6)
    LinkForeground,         // Hyperlinks (#E6E6E6)
    SearchPlaceholder,      // Search field placeholder (alias → TextHint)

    // ── Icons ─────────────────────────────────────────────────────────────────
    IconPrimary,            // Default icon (#A0A1A3)
    IconSecondary,          // De-emphasized icon (#A0A1A3)
    IconDisabled,           // Disabled icon (#4F5053)
    IconAccent,             // Active/accent icon (#E6E6E6)
    IconHover,              // Hovered icon (#E6E6E6)
    IconActive,             // Pressed/active icon (#E6E6E6)

    // ── Accent & selection ────────────────────────────────────────────────────
    AccentPrimary,          // Primary accent (#38393C)
    AccentHover,            // Accent hover (#40444A)
    ActiveTabLine,          // Active dock tab indicator (#38393C @ 80%)
    SelectionHighlight,     // Selection overlay (#323336 @ 90%)

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
};

enum class SpacingToken : uint32_t {
    None,
    ExtraSmall, // 2
    Small,      // 4
    Medium,     // 8
    Large,      // 16
    ExtraLarge, // 24
    Huge,       // 32 — page / section breathing room
};

// Semantic control height roles (maps to MetricToken heights).
enum class ControlSize : uint32_t {
    Compact, // denser inputs / toggles / menu items
    Default, // buttons, search, header controls
    Large,   // prominent CTAs / list rows
};

enum class RadiusToken : uint32_t {
    None,
    Small,
    Medium,
    Large,
    Full,
};

enum class TypographyToken : uint32_t {
    // Window / page chrome
    WindowTitle,
    PageTitle,
    SectionTitle,
    CardTitle,
    DialogTitle,

    // Headings
    Display,
    Heading1,
    Heading2,
    Heading3,
    Heading4,
    Heading5,
    Heading6,
    Heading, // maps to Heading2 in themes

    // Body — PrimaryText / SecondaryText / Caption / Hint
    Title,
    Subtitle,    // Secondary supporting text at body size
    Body,        // PrimaryText
    BodyStrong,
    Caption,
    CaptionSmall,
    Hint,        // Placeholders, helper text (lowest readable emphasis)

    // Controls
    Label,
    Button,
    Toolbar,
    Menu,
    Tooltip,
    TableHeader,
    Status,
    StatusBar,
    Navigation,
    PropertyLabel,
    PropertyValue,

    // Specialized
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
    ControlHeightCompact, // denser form controls (toggle, spin, compact input)
    ControlHeightLarge,   // prominent CTAs
    FormRowHeight,        // label + control settings/property row
    MenuItemHeight,       // popup / dropdown option row
    PageMargin,           // page content inset
    SectionGap,           // gap between titled sections
    CardPadding,          // inner card / group padding
    ContentGap,           // default stack gap between content blocks
    FormRowGap,           // vertical padding around form rows
    LabelHintGap,         // gap between label and hint in a form row
    NavigationButtonSize,
    IconSizeSearch,
    IconSizeToolbar,
    IconSizePrimary,
    IconSizeTree,
    IconSizeNavigation,
    IconButtonRadius,
    ButtonPaddingHorizontal,
    ButtonSpacing,
    ButtonGroupSpacing,
    ScrollbarWidth,
    ScrollbarThumbMinHeight,

    TabTopRadius,              // dock tab upper corner radius
    TabActiveIndicatorHeight,  // accent line on active dock tab
    TabGap,                    // horizontal gap between dock tabs
    StatusBarHeight,           // bottom status/command bar

    ToolbarSeparatorHeight,    // vertical separator line in toolbars
    ToolbarLabeledHeight,      // labeled toolbar button variant
    ToolbarLabeledMinWidth,

    BreadcrumbBarHeight,       // content browser path bar
    PropertyLabelColumnWidth,  // details / property inspector label column
    PropertyIndentStep,        // nested property tree indent per level
    TreeIndentWidth,           // tree view indent per depth level

    PopupMinWidth,
    PopupMaxWidth,
    PopupMaxHeight,            // scrollable dropdown / context menu cap
    TooltipMinWidth,
    ToggleTrackWidth,          // toggle switch track
    ToggleTrackHeight,
    CheckboxGlyphSize,         // checkbox inner mark
    PrimaryButtonHeight,       // prominent panel CTA (e.g. Create Landscape)

    ContentBrowserGridPadding,
    ContentBrowserGridHSpacing,
    ContentBrowserGridVSpacing,
    ContentBrowserThumbLarge,
    ContentBrowserThumbMedium,
    ContentBrowserThumbSmall,
    ContentBrowserCellLarge,
    ContentBrowserCellMedium,
    ContentBrowserCellSmall,

    DragThreshold,      // pointer movement before drag gesture starts
    MenuPadding,        // popup / dropdown inner inset
    CheckMarkSize,      // checkbox / menu check glyph
    MenuTextIndent,     // label offset after check column in menus

    Space1,
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

} // namespace we::runtime::kindui
