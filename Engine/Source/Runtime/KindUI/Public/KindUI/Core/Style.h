#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Theming/ThemeAccess.h"
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
    bool bold = false;
    bool italic = false;

    static TextStyle Body() {
        return TextStyle{
            ResolveThemeColor(ThemeToken::TextPrimary),
            ResolveThemeMetric(ThemeToken::TextSizeBody),
            false,
            false
        };
    }
};

struct ShadowStyle {
    Color color = ResolveThemeColor(ThemeToken::ShadowSubtle);
    float offsetX = 0.0f;
    float offsetY = 2.0f;
    float blur = 4.0f;
    float spread = 0.0f;

    static ShadowStyle None() { return ShadowStyle{Color::Transparent(), 0, 0, 0, 0}; }
    static ShadowStyle Small() { return ShadowStyle{ResolveThemeColor(ThemeToken::ShadowPopup), 0, 1, 2, 0}; }
    static ShadowStyle Medium() { return ShadowStyle{ResolveThemeColor(ThemeToken::ShadowSubtle), 0, 2, 4, 0}; }
    static ShadowStyle Large() { return ShadowStyle{ResolveThemeColor(ThemeToken::ShadowOverlay), 0, 4, 8, 0}; }
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
