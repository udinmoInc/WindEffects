// ==============================================================================
// WindEffects — KindUI — StyleRole
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

namespace we::runtime::kindui {

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
    ButtonPrimary,
    ButtonSecondary,
    ButtonGhost,
    ButtonDanger,
    ToolbarButton,
    IconButton,
    IconButtonHover,
    IconButtonPressed,
    NavigationButton,
    Input,
    SearchBox,
    StatusBar,
    MenuBar,
    MenuItem,
    DockTab,
    DockTabActive,
    Splitter,
    Separator,
    Popup,
    Tooltip,
    Modal,
    Gizmo,
    ContentBrowser,
    TextPrimary,
    TextSecondary,
    TextCaption,
    TextHint,
    TextDisabled,
    Card,
    CardHover,
    TableHeader,
    TableRow,
    TableRowHover,
    TableRowSelected,
    SectionHeader,
    PropertyRow,
    SidebarItem,
    SidebarItemActive,
    WindowHeader,
    Checkbox,
    ToggleSwitch,
    Scrollbar,
    TreeItem,
    TreeItemSelected,
};

} // namespace we::runtime::kindui
