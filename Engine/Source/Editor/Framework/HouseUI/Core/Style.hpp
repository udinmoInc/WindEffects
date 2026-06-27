#pragma once

#include "Geometry.hpp"
#include "Theme.hpp"
#include <string>
#include <memory>
#include <unordered_map>

namespace HouseEngine::UI {

// Enum for visual states
enum class WidgetState {
    Normal,
    Hovered,
    Pressed,
    Focused,
    Disabled,
    Selected
};

// Border styling
struct BorderStyle {
    float width = 1.0f;
    Color color = Theme::Get().BorderDefault;
    float cornerRadius = Theme::Get().CornerRadiusSmall;
    float cornerRadiusTopLeft = Theme::Get().CornerRadiusSmall;
    float cornerRadiusTopRight = Theme::Get().CornerRadiusSmall;
    float cornerRadiusBottomLeft = 0.0f;
    float cornerRadiusBottomRight = 0.0f;
    
    // Predefined styles
    static BorderStyle None() { return BorderStyle{ 0.0f, Color{0,0,0,0}, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }; }
    static BorderStyle Thin() { return BorderStyle{ 1.0f, Theme::Get().BorderDefault, Theme::Get().CornerRadiusSmall, Theme::Get().CornerRadiusSmall, Theme::Get().CornerRadiusSmall, Theme::Get().CornerRadiusSmall, Theme::Get().CornerRadiusSmall }; }
    static BorderStyle Thick() { return BorderStyle{ 2.0f, Theme::Get().BorderDefault, Theme::Get().CornerRadiusLarge, Theme::Get().CornerRadiusLarge, Theme::Get().CornerRadiusLarge, Theme::Get().CornerRadiusLarge, Theme::Get().CornerRadiusLarge }; }
    static BorderStyle Selected() { return BorderStyle{ 1.5f, Theme::Get().SelectedAccent, Theme::Get().CornerRadiusSmall, Theme::Get().CornerRadiusSmall, Theme::Get().CornerRadiusSmall, Theme::Get().CornerRadiusSmall, Theme::Get().CornerRadiusSmall }; }
};

// Background styling
struct BackgroundStyle {
    Color color = Theme::Get().PanelBackground;
    float cornerRadius = Theme::Get().CornerRadiusSmall;
    
    // Predefined styles
    static BackgroundStyle None() { return BackgroundStyle{ Color{0,0,0,0}, 0.0f }; }
    static BackgroundStyle Panel() { return BackgroundStyle{ Theme::Get().PanelBackground, Theme::Get().CornerRadiusSmall }; }
    static BackgroundStyle Toolbar() { return BackgroundStyle{ Theme::Get().ToolbarBackground, Theme::Get().CornerRadiusSmall }; }
    static BackgroundStyle Hover() { return BackgroundStyle{ Theme::Get().HoverOverlay, Theme::Get().CornerRadiusSmall }; }
    static BackgroundStyle Selected() { return BackgroundStyle{ Theme::Get().SelectedAccent * 0.3f, Theme::Get().CornerRadiusSmall }; }
};

// Text styling
struct TextStyle {
    Color color = Theme::Get().TextPrimary;
    float size = Theme::Get().TextSizeBody;
    bool bold = false;
    bool italic = false;
    
    // Predefined styles
    static TextStyle Menu() { return TextStyle{ Theme::Get().TextPrimary, Theme::Get().TextSizeMenu, false, false }; }
    static TextStyle Toolbar() { return TextStyle{ Theme::Get().TextPrimary, Theme::Get().TextSizeToolbar, false, false }; }
    static TextStyle Header() { return TextStyle{ Theme::Get().TextPrimary, Theme::Get().TextSizeHeader, true, false }; }
    static TextStyle Body() { return TextStyle{ Theme::Get().TextPrimary, Theme::Get().TextSizeBody, false, false }; }
    static TextStyle Small() { return TextStyle{ Theme::Get().TextSecondary, Theme::Get().TextSizeSmall, false, false }; }
    static TextStyle Disabled() { return TextStyle{ Theme::Get().TextSecondary * 0.5f, Theme::Get().TextSizeBody, false, false }; }
};

// Shadow styling
struct ShadowStyle {
    Color color = Color{ 0.0f, 0.0f, 0.0f, 0.15f };
    float offsetX = 0.0f;
    float offsetY = 2.0f;
    float blur = 4.0f;
    float spread = 0.0f;
    
    // Predefined styles
    static ShadowStyle None() { return ShadowStyle{ Color{0,0,0,0}, 0, 0, 0, 0 }; }
    static ShadowStyle Small() { return ShadowStyle{ Color{0,0,0,0.1f}, 0, 1, 2, 0 }; }
    static ShadowStyle Medium() { return ShadowStyle{ Color{0,0,0,0.15f}, 0, 2, 4, 0 }; }
    static ShadowStyle Large() { return ShadowStyle{ Color{0,0,0,0.2f}, 0, 4, 8, 0 }; }
};

// Complete widget style
struct WidgetStyle {
    BackgroundStyle background;
    BorderStyle border;
    TextStyle text;
    ShadowStyle shadow;
    Margin padding = Theme::Get().PaddingPanel;
    
    // State-based style overrides
    BackgroundStyle backgroundHover;
    BackgroundStyle backgroundPressed;
    BorderStyle borderFocused;
    
    WidgetStyle() = default;
    
    // Predefined widget styles
    static WidgetStyle Panel() {
        WidgetStyle style;
        style.background = BackgroundStyle::Panel();
        style.border = BorderStyle::Thin();
        style.text = TextStyle::Body();
        style.shadow = ShadowStyle::None();
        style.padding = Theme::Get().PaddingPanel;
        return style;
    }
    
    static WidgetStyle Button() {
        WidgetStyle style;
        style.background = BackgroundStyle::Toolbar();
        style.border = BorderStyle::Thin();
        style.text = TextStyle::Toolbar();
        style.shadow = ShadowStyle::Small();
        style.padding = Theme::Get().PaddingButton;
        style.backgroundHover = BackgroundStyle::Hover();
        style.backgroundPressed = BackgroundStyle{ Theme::Get().HoverOverlay * 0.8f, Theme::Get().CornerRadiusSmall };
        return style;
    }
    
    static WidgetStyle ToolButton() {
        WidgetStyle style;
        style.background = BackgroundStyle::None();
        style.border = BorderStyle::None();
        style.text = TextStyle::Toolbar();
        style.shadow = ShadowStyle::None();
        style.padding = Theme::Get().PaddingIconBtn;
        style.backgroundHover = BackgroundStyle::Hover();
        style.backgroundPressed = BackgroundStyle{ Theme::Get().HoverOverlay * 0.8f, Theme::Get().CornerRadiusSmall };
        return style;
    }
    
    static WidgetStyle TextBox() {
        WidgetStyle style;
        style.background = BackgroundStyle{ Theme::Get().WindowBackground, Theme::Get().CornerRadiusSmall };
        style.border = BorderStyle::Thin();
        style.text = TextStyle::Body();
        style.shadow = ShadowStyle::None();
        style.padding = Margin{ 8.0f, 6.0f, 8.0f, 6.0f };
        style.borderFocused = BorderStyle::Selected();
        return style;
    }
    
    static WidgetStyle TreeItem() {
        WidgetStyle style;
        style.background = BackgroundStyle::None();
        style.border = BorderStyle::None();
        style.text = TextStyle::Body();
        style.shadow = ShadowStyle::None();
        style.padding = Margin{ 4.0f, 2.0f, 4.0f, 2.0f };
        style.backgroundHover = BackgroundStyle::Hover();
        style.backgroundPressed = BackgroundStyle::Selected();
        return style;
    }
    
    static WidgetStyle PropertyLabel() {
        WidgetStyle style;
        style.background = BackgroundStyle::None();
        style.border = BorderStyle::None();
        style.text = TextStyle::Small();
        style.shadow = ShadowStyle::None();
        style.padding = Margin{ 0.0f, 4.0f, 0.0f, 4.0f };
        return style;
    }
    
    static WidgetStyle Tab() {
        WidgetStyle style;
        style.background = BackgroundStyle::Toolbar();
        style.border = BorderStyle{ 1.0f, Theme::Get().BorderDefault, Theme::Get().CornerRadiusSmall, Theme::Get().CornerRadiusSmall, 0.0f, 0.0f };
        style.text = TextStyle::Toolbar();
        style.shadow = ShadowStyle::None();
        style.padding = Margin{ 16.0f, 6.0f, 16.0f, 6.0f };
        style.backgroundHover = BackgroundStyle::Hover();
        return style;
    }
    
    static WidgetStyle TabActive() {
        WidgetStyle style;
        style.background = BackgroundStyle::Panel();
        style.border = BorderStyle{ 1.0f, Theme::Get().BorderDefault, Theme::Get().CornerRadiusSmall, Theme::Get().CornerRadiusSmall, 0.0f, 0.0f };
        style.text = TextStyle{ Theme::Get().TextPrimary, Theme::Get().TextSizeToolbar, true, false };
        style.shadow = ShadowStyle::None();
        style.padding = Margin{ 16.0f, 6.0f, 16.0f, 6.0f };
        return style;
    }
};

// Style manager for runtime theming
class StyleManager {
public:
    static StyleManager& Get();
    
    void SetWidgetStyle(const std::string& widgetName, const WidgetStyle& style) {
        m_WidgetStyles[widgetName] = style;
    }
    
    WidgetStyle GetWidgetStyle(const std::string& widgetName) const {
        auto it = m_WidgetStyles.find(widgetName);
        if (it != m_WidgetStyles.end()) {
            return it->second;
        }
        return WidgetStyle::Panel(); // Default fallback
    }
    
private:
    std::unordered_map<std::string, WidgetStyle> m_WidgetStyles;
};

} // namespace HouseEngine::UI
