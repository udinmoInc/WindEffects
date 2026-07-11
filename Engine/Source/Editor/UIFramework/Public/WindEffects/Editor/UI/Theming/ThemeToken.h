#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Core/Types.h"

#include <cstdint>

namespace WindEffects::Editor::UI {

enum class ThemeToken : uint32_t {
    WindowBackground,
    WorkspaceBackground,
    ToolbarBackground,
    PanelBackground,
    HeaderBackground,
    ViewportBackground,
    FooterBackground,
    MenuBarBackground,
    TabBackground,
    PopupBackground,
    ContentBrowserBackground,

    BorderLight,
    BorderDark,
    BorderDefault,
    BorderFocus,
    Separator,

    HoverBackground,
    ActiveBackground,
    SelectedBackground,
    DisabledBackground,
    PressedBackground,

    TextPrimary,
    TextSecondary,
    TextDisabled,
    TextMuted,
    TextWindowLabel,

    IconDefault,
    IconHover,
    IconActive,
    IconDisabled,

    AccentPrimary,
    AccentHover,
    ActiveTabLine,
    SelectionHighlight,
    Success,
    Warning,

    InputBackground,
    SearchBoxBackground,
    SearchPlaceholder,

    ViewportToolbarBackground,
    StatusBarBackground,

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

    BorderWidth,

    PanelHeaderHeight,
    ToolbarHeight,
    SearchBoxHeight,
    IconButtonSize,
    ButtonHeight,
    IconSizeSearch,
    IconSizeToolbar,
    IconSizeTree,

    Space1,
    Space2,
    Space3,
    Space4,
    Space5,
    Space6,

    PaddingPanelLeft,
    PaddingPanelTop,
    PaddingPanelRight,
    PaddingPanelBottom,
    PaddingButtonLeft,
    PaddingButtonTop,
    PaddingButtonRight,
    PaddingButtonBottom,
};

enum class StyleRole {
    Window,
    Workspace,
    Toolbar,
    Panel,
    PanelHeader,
    Tab,
    TabActive,
    Button,
    ButtonHover,
    ButtonActive,
    IconButton,
    Input,
    SearchBox,
    StatusBar,
    MenuBar,
    DockTab,
    DockTabActive,
    Splitter,
    Separator,
    TextPrimary,
    TextSecondary,
    TextCaption,
};

} // namespace WindEffects::Editor::UI
