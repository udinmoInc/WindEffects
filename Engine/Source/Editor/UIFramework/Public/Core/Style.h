#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "Core/Geometry.h"
#include <string>
#include <memory>

namespace WindEffects::Editor::UI {

enum class WidgetState {
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
        return TextStyle{ Color{0.722f, 0.745f, 0.780f, 1.0f}, 13.0f, false, false };
    }
};

struct ShadowStyle {
    Color color = Color{0.0f, 0.0f, 0.0f, 0.15f};
    float offsetX = 0.0f;
    float offsetY = 2.0f;
    float blur = 4.0f;
    float spread = 0.0f;

    static ShadowStyle None() { return ShadowStyle{Color::Transparent(), 0, 0, 0, 0}; }
    static ShadowStyle Small() { return ShadowStyle{Color{0, 0, 0, 0.1f}, 0, 1, 2, 0}; }
    static ShadowStyle Medium() { return ShadowStyle{Color{0, 0, 0, 0.15f}, 0, 2, 4, 0}; }
    static ShadowStyle Large() { return ShadowStyle{Color{0, 0, 0, 0.2f}, 0, 4, 8, 0}; }
};

struct WidgetStyle {
    BackgroundStyle background;
    BorderStyle border;
    TextStyle text;
    ShadowStyle shadow;
    Margin padding{};

    BackgroundStyle backgroundHover;
    BackgroundStyle backgroundPressed;
    BorderStyle borderFocused;

    static WidgetStyle Panel() { return WidgetStyle{}; }
    static WidgetStyle Button() { return WidgetStyle{}; }
    static WidgetStyle ToolButton() { return WidgetStyle{}; }
    static WidgetStyle TextBox() { return WidgetStyle{}; }
    static WidgetStyle TreeItem() { return WidgetStyle{}; }
};

} // namespace WindEffects::Editor::UI
