#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/TypographySpec.h"
#include <string>
#include <memory>

namespace we::runtime::kindui {

enum class PseudoState {
    Normal,
    Hovered,
    Pressed,
    Focused,
    Disabled,
    Selected
};

struct BorderStyle {
    float width = 1.0f;
    Color color{};
    float cornerRadius = 4.0f;
    float cornerRadiusTopLeft = 4.0f;
    float cornerRadiusTopRight = 4.0f;
    float cornerRadiusBottomLeft = 0.0f;
    float cornerRadiusBottomRight = 0.0f;
};

struct BackgroundStyle {
    Color color{};
    float cornerRadius = 4.0f;
};

struct TextStyle {
    Color color = Color::White();
    float size = 13.0f;
    uint16_t weight = 400;
    bool bold = false;
    bool italic = false;
    TypographyToken role = TypographyToken::Body;

    static TextStyle FromRole(TypographyToken token) {
        const TypographySpec spec = ResolveTypography(token);
        TextStyle style;
        style.role = token;
        style.size = spec.sizePx;
        style.color = spec.color;
        style.weight = spec.weight;
        style.bold = spec.bold;
        style.italic = spec.italic;
        return style;
    }

    static TextStyle PageTitle() { return FromRole(TypographyToken::PageTitle); }
    static TextStyle SectionTitle() { return FromRole(TypographyToken::SectionTitle); }
    static TextStyle CardTitle() { return FromRole(TypographyToken::CardTitle); }
    static TextStyle Primary() { return FromRole(TypographyToken::Body); }
    static TextStyle Secondary() { return FromRole(TypographyToken::Subtitle); }
    static TextStyle Caption() { return FromRole(TypographyToken::Caption); }
    static TextStyle Hint() { return FromRole(TypographyToken::Hint); }
    static TextStyle Disabled() { return FromRole(TypographyToken::Disabled); }
    static TextStyle Body() { return FromRole(TypographyToken::Body); }
};

struct ShadowStyle {
    Color color = ResolveColor(ColorToken::ShadowSubtle);
    float offsetX = 0.0f;
    float offsetY = 2.0f;
    float blur = 4.0f;
    float spread = 0.0f;

    static ShadowStyle None() { return ShadowStyle{Color::Transparent(), 0, 0, 0, 0}; }
    static ShadowStyle Small() { return ShadowStyle{ResolveColor(ColorToken::ShadowPopup), 0, 1, 2, 0}; }
    static ShadowStyle Medium() { return ShadowStyle{ResolveColor(ColorToken::ShadowSubtle), 0, 2, 4, 0}; }
    static ShadowStyle Large() { return ShadowStyle{ResolveColor(ColorToken::ShadowOverlay), 0, 4, 8, 0}; }
};

struct KINDUI_API WidgetStyle {
    BackgroundStyle background;
    BorderStyle border;
    TextStyle text;
    ShadowStyle shadow;
    Margin padding{};

    BackgroundStyle backgroundHover;
    BackgroundStyle backgroundPressed;
    BorderStyle borderFocused;

    // Built from ThemeManager StyleRoles (see StyleFactory).
    static WidgetStyle Panel();
    static WidgetStyle Button();
    static WidgetStyle ToolButton();
    static WidgetStyle TextBox();
    static WidgetStyle TreeItem();
};

} // namespace we::runtime::kindui
