// ==============================================================================
// WindEffects — KindUI — StyleFactory
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Theming/ResolvedStyle.h"
#include "KindUI/Core/Style.h"

namespace we::runtime::kindui {

class KINDUI_API StyleFactory {
public:
    static BorderStyle BorderNone();
    static BorderStyle BorderThin(const IStyleResolver& styles);
    static BorderStyle BorderSelected(const IStyleResolver& styles);

    static BackgroundStyle BackgroundNone();
    static BackgroundStyle BackgroundPanel(const IStyleResolver& styles);
    static BackgroundStyle BackgroundToolbar(const IStyleResolver& styles);
    static BackgroundStyle BackgroundHover(const IStyleResolver& styles);
    static BackgroundStyle BackgroundSelected(const IStyleResolver& styles);
    static BackgroundStyle BackgroundInput(const IStyleResolver& styles);

    static TextStyle TextMenu(const IStyleResolver& styles);
    static TextStyle TextToolbar(const IStyleResolver& styles);
    static TextStyle TextHeader(const IStyleResolver& styles);
    static TextStyle TextBody(const IStyleResolver& styles);
    static TextStyle TextSmall(const IStyleResolver& styles);

    static WidgetStyle Panel(const IStyleResolver& styles);
    static WidgetStyle Button(const IStyleResolver& styles);
    static WidgetStyle ToolButton(const IStyleResolver& styles);
    static WidgetStyle TextBox(const IStyleResolver& styles);
    static WidgetStyle TreeItem(const IStyleResolver& styles);
    static WidgetStyle PropertyLabel(const IStyleResolver& styles);
    static WidgetStyle Tab(const IStyleResolver& styles);
    static WidgetStyle TabActive(const IStyleResolver& styles);
};

} // namespace we::runtime::kindui
