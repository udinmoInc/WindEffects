#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Theming/IThemeProvider.h"
#include "WindEffects/Runtime/UI/Core/Style.h"

namespace WindEffects::Editor::UI {

class UI_API StyleFactory {
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

} // namespace WindEffects::Editor::UI
