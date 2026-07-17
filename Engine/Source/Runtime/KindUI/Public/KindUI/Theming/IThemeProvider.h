#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theming/ThemeToken.h"
#include "KindUI/Core/Types.h"

#include <string_view>

namespace we::runtime::kindui {

struct ResolvedStyle {
    Color background = Color::Transparent();
    Color foreground = Color::White();
    Color border = Color::Transparent();
    Color icon = Color::White();
    float cornerRadius = 0.0f;
    float borderWidth = 1.0f;
    float fontSize = 13.0f;
    Margin padding{};
    float height = 0.0f;
    float iconSize = 16.0f;
    bool bold = false;
    int elevation = 0; // 0=none, 1=small, 2=medium, 3=large
};

class KINDUI_API IThemeProvider {
public:
    virtual ~IThemeProvider() = default;

    [[nodiscard]] virtual std::string_view GetThemeId() const = 0;
    [[nodiscard]] virtual Color GetColor(ThemeToken token) const = 0;
    [[nodiscard]] virtual float GetMetric(ThemeToken token) const = 0;
    [[nodiscard]] virtual Margin GetPadding(ThemeToken token) const = 0;

    [[nodiscard]] virtual Color InteractiveBackground(float hoverAnim, float pressAnim, bool selected) const = 0;
    [[nodiscard]] virtual Color IconForState(bool hovered, bool active = false) const = 0;
    [[nodiscard]] virtual Color TextForState(bool hovered, bool active = false) const = 0;
};

class KINDUI_API IStyleResolver {
public:
    virtual ~IStyleResolver() = default;

    [[nodiscard]] virtual ResolvedStyle Resolve(StyleRole role) const = 0;
    [[nodiscard]] virtual ResolvedStyle ResolveClass(std::string_view className) const = 0;
    [[nodiscard]] virtual float Scaled(float logicalValue) const = 0;
    [[nodiscard]] virtual float GetDpiScale() const = 0;
    virtual void SetDpiScale(float scale) = 0;
};

} // namespace we::runtime::kindui
