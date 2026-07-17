#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theming/IThemeProvider.h"
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
