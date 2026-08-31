#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

// Semantic color tokens — one role per entry.
// Concrete values live in Palette.h (GraphiteDark) and theme ResolveColor().
// Multiple tokens may share a palette entry when roles intentionally match.

enum class ColorToken : uint32_t {
    // ── Surfaces (UE5 Slate StyleColors dark defaults) ───────────────────────
    WindowBackground,       // Title (#151515) — window frame
    WorkspaceBackground,    // Background (#151515) — dock/viewport void
    DockChromeBackground,   // Background (#151515) — dock tab strip / splitter chrome
    PanelBackground,        // Panel (#242424) — panel body
    TabActiveBackground,    // Panel (#242424) — active dock tab fill
    SecondarySurface,       // Recessed (#1A1A1A) — tree / grid wells
    CardBackground,         // Dropdown (#383838) — raised cards
    HeaderBackground,       // Header (#2F2F2F) — section headers
    ListLabelBandBackground, // Header (#2F2F2F) — column label rows, panel footer bands
    ToolbarBackground,      // Background (#151515) — main toolbar strip
    TabBackground,          // Background (#151515) — inactive dock tab
    InputBackground,        // Input (#0F0F0F) — search / property fields
    ControlBackground,      // Input (#0F0F0F) — generic control wells
    PopupBackground,        // Dropdown (#383838) — menus / context popups
    TooltipBackground,      // Tooltips (#383838 @ 97%)
    DisabledBackground,     // Foldout (#0F0F0F) — disabled control fill
    StatusBarBackground,    // Bottom status bar (#151515)
    ViewportToolbarBackground, // Floating viewport toolbar (opaque chrome)
    ScrollbarTrack,         // Scroll gutter / recessed (#1A1A1A)

    // ── Interaction states ────────────────────────────────────────────────────
    HoverBackground,
    PressedBackground,
    SelectedBackground,     // Select (#0070E0)
    SelectInactiveBackground, // Select inactive (#40576F)
    SelectParentBackground,   // Select parent (#2C323A)
    SelectHoverBackground,    // Select hover (#242424)
    ControlBackgroundHover,
    ControlBackgroundPressed,
    ControlBackgroundDisabled,
    ControlBackgroundSelected,

    // ── Borders & separators ──────────────────────────────────────────────────
    Separator,              // WindowBorder (#0F0F0F) — dividers
    BorderSubtle,           // InputOutline (#383838) — input / recessed edges
    BorderDefault,          // InputOutline (#383838) — general control borders
    BorderLight,            // DropdownOutline (#4C4C4C) — raised / popup edges
    BorderFocus,            // Focus ring (#0070E0)
    BorderError,            // Validation error (#EF3535)

    // ── Text hierarchy ──────────────────────────────────────────────────────
    TextPrimary,            // Foreground (#C0C0C0)
    TextSecondary,          // Muted metadata (#808080)
    TextHint,               // Placeholders / muted (#808080)
    TextDisabled,           // Disabled / placeholder (#464B50)
    TextOnAccent,           // Text on filled buttons (#FFFFFF)
    LinkForeground,         // Hyperlinks (#0070E0)
    SearchPlaceholder,      // Search field placeholder (alias → TextHint)

    // ── Icons ─────────────────────────────────────────────────────────────────
    IconPrimary,            // Default mono icon (#A7AFBA)
    IconSecondary,          // Default mono icon (#A7AFBA)
    IconDisabled,           // Disabled icon (#5C6570)
    IconAccent,             // Active/accent icon (#FFFFFF)
    IconHover,              // Hovered icon (#D6DBE1)
    IconActive,             // Pressed/active icon (#FFFFFF)

    // ── Accent & selection ────────────────────────────────────────────────────
    AccentPrimary,          // Primary accent (#0070E0)
    AccentHover,            // Accent hover (#0E86FF)
    AccentOrange,           // Inline code / warning accent (#FE9B07)
    ActiveTabLine,          // Active dock tab indicator (#0070E0 @ 80%)
    SelectionHighlight,     // Selection overlay (#0070E0 @ 90%)

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
    TabStripPadH,              // dock/mode tab strip left inset (align with panel content)
    TabStripPadV,              // dock tab strip top inset (gap above tabs)
    TabActiveIndicatorWidth,   // active tab left accent width
    TabPaddingH,               // dock tab horizontal inner padding
    TabPaddingV,               // dock tab vertical inner padding
    DockPanelGap,              // gutter between docked panels and workspace edge (logical px)
    ViewportToolbarHeight,     // floating viewport control strip
    StatusBarHeight,           // bottom status/command bar

    ToolbarSeparatorHeight,    // vertical separator line in toolbars
    ToolbarLabeledHeight,      // labeled toolbar button variant
    ToolbarLabeledMinWidth,

    BreadcrumbBarHeight,       // content browser path bar
    PropertyLabelColumnWidth,  // details / property inspector label column
    PropertyIndentStep,        // nested property tree indent per level
    TreeIndentWidth,           // tree view indent per depth level
    TreeExpanderHitSize,       // expand/collapse chevron hit area (20–24 logical px)

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

    SpaceXS,
    Space1,
    SpaceMD,   // compact editor rhythm (6 logical px)
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
