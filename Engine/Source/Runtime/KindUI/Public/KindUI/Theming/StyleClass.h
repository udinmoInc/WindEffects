#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Theming/ThemeToken.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace we::runtime::kindui {

/// Declarative style class with optional inheritance (Button -> PrimaryButton).
struct KINDUI_API StyleClass {
    std::string name;
    std::string parentName;

    ThemeToken background = ThemeToken::PanelBackground;
    ThemeToken foreground = ThemeToken::TextPrimary;
    ThemeToken border = ThemeToken::BorderDefault;
    ThemeToken hoverBackground = ThemeToken::HoverBackground;
    ThemeToken pressedBackground = ThemeToken::PressedBackground;
    ThemeToken disabledBackground = ThemeToken::DisabledBackground;

    ThemeToken paddingToken = ThemeToken::Space3;
    ThemeToken radiusToken = ThemeToken::CornerRadiusMedium;
    ThemeToken fontSizeToken = ThemeToken::TextSizeBody;
    ThemeToken heightToken = ThemeToken::ButtonHeight;
    ThemeToken animDurationToken = ThemeToken::HoverAnimationDamping;

    float opacity = 1.0f;
    bool bold = false;
};

class KINDUI_API StyleClassRegistry {
public:
    static StyleClassRegistry& Get();

    void Register(StyleClass styleClass);
    void RegisterDefaults();
    [[nodiscard]] const StyleClass* Find(std::string_view name) const;
    [[nodiscard]] StyleClass Resolve(std::string_view name) const;

private:
    StyleClassRegistry() = default;
    std::unordered_map<std::string, StyleClass> m_Classes;
};

} // namespace we::runtime::kindui
