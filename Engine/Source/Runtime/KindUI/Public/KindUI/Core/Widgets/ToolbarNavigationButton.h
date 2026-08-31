#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widgets/ToolbarGlyphButton.h"

#include <functional>
#include "KindUI/Tokens/DesignToken.h"

namespace we::runtime::kindui {

class KINDUI_API ToolbarNavigationButton : public ToolbarGlyphButton {
public:
    explicit ToolbarNavigationButton(WindIconRef icon, const char* tooltip = nullptr)
        : ToolbarGlyphButton(
            icon,
            StyleRole::NavigationButton,
            MetricToken::NavigationButtonSize,
            MetricToken::IconSizeNavigation)
    {
        (void)tooltip;
    }

    void SetOnClicked(std::function<void()> callback);
    void SetEnabled(bool enabled);
    void SetSelected(bool selected);
    [[nodiscard]] bool IsEnabled() const { return Widget::IsEnabled(); }
    [[nodiscard]] bool IsSelected() const { return Widget::IsSelected(); }
};

} // namespace we::runtime::kindui
