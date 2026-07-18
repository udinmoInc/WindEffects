#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

// Platform-neutral semantic design tokens.
// Applications supply concrete values through IKindUITheme; KindUI never hardcodes branding.

enum class ColorToken : uint32_t {
    // Semantic elevation surfaces (darkest → lightest for dark themes)
    // Window → Panel → Card → Control → Overlay → Popup
    PrimarySurface,   // Panel-level content surface
    SecondarySurface, // Card / raised content surface
    TertiarySurface,  // Elevated chrome / nested wells
    AccentSurface,
    WindowBackground,
    ControlBackground,
    ControlBackgroundHover,
    ControlBackgroundPressed,
    ControlBackgroundDisabled,
    ControlBackgroundSelected,
    CardBackground, // Explicit card fill (aliases SecondarySurface in themes)

    // App chrome surfaces
    WorkspaceBackground,
    ToolbarBackground,
    PanelBackground,
    HeaderBackground,
    ViewportBackground,
    FooterBackground,
    MenuBarBackground,
    TabBackground,
    PopupBackground,
    OverlayBackground, // Modal/dialog elevation fill
    ContentBrowserBackground,
    PanelContentBackground,
    PanelToolbarBackground,
    PanelTabActiveBackground,
    PanelTabInactiveBackground,
    DockAreaBackground,
    StatusBarBackground,
    ViewportToolbarBackground,
    DialogBackground,
    TooltipBackground,
    InputBackground,
    SearchBoxBackground,

    // Borders
    BorderDefault,
    BorderSubtle,
    BorderFocused,
    BorderError,
    BorderLight,
    BorderDark,
    BorderFocus, // alias of BorderFocused for product themes
    Separator,

    // Interaction
    HoverBackground,
    ActiveBackground,
    SelectedBackground,
    DisabledBackground,
    PressedBackground,
    ContentBrowserHoverBackground,

    // Text — semantic hierarchy (Primary > Secondary > Caption/Muted > Hint > Disabled)
    TextPrimary,   // PageTitle, SectionTitle, Body, labels
    TextSecondary, // Descriptions, supporting copy
    TextCaption,   // Captions / meta (between Secondary and Hint)
    TextDisabled,
    TextOnAccent,
    TextLink,
    TextMuted,     // Hint / placeholder (aliases TextHint in themes)
    TextHint,      // Explicit hint/placeholder role
    TextWindowLabel,
    SearchPlaceholder,
    CodeForeground,
    LinkForeground,

    // Icons
    IconPrimary,
    IconSecondary,
    IconDisabled,
    IconAccent,
    IconDefault,
    IconHover,
    IconActive,

    // Accent / semantic
    AccentPrimary,
    AccentHover,
    ActiveTabLine,
    SelectionHighlight,
    SuccessColor,
    WarningColor,
    ErrorColor,
    InfoColor,
    Success,
    Warning,
    ErrorForeground,
    PlayForeground,
    CloseButtonHover,

    // Buttons
    ButtonPrimaryBackground,
    ButtonPrimaryHover,
    ButtonPrimaryPressed,
    ButtonDangerBackground,
    ButtonDangerHover,
    ButtonDangerPressed,

    // Gizmo
    GizmoBackground,
    GizmoAxisX,
    GizmoAxisY,
    GizmoAxisZ,

    // Depth
    HighlightSubtle,
    ShadowSubtle,
    ShadowOverlay,
    ShadowPopup,
    ShadowColor,
    ScrimOverlay,
    ModalScrim,
    DragGhostBackground,

    // Content Browser folder art
    ContentBrowserFolderShadow,
    ContentBrowserFolderEdge,
    ContentBrowserFolderHighlight,
    ContentBrowserFolderTab,
    ContentBrowserFolderPrimary,
    ContentBrowserFolderBody,

    // Scrollbar
    ScrollbarTrack,
    ScrollbarThumb,
    ScrollbarThumbHover,
};

enum class SpacingToken : uint32_t {
    None,
    ExtraSmall,
    Small,
    Medium,
    Large,
    ExtraLarge,
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
