#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widgets/ToolbarGlyphButton.h"

#include <functional>
#include <string>
#include "KindUI/Tokens/DesignToken.h"

namespace we::runtime::kindui {

class KINDUI_API ToolbarIconButton : public ToolbarGlyphButton {
public:
    explicit ToolbarIconButton(const std::string& iconName, const char* tooltip = nullptr)
        : ToolbarGlyphButton(
            iconName,
            StyleRole::IconButton,
            MetricToken::IconButtonSize,
            MetricToken::IconSizeToolbar)
    {
        (void)tooltip;
    }

    void SetOnClicked(std::function<void()> callback) { ToolbarGlyphButton::SetOnClicked(std::move(callback)); }
    void SetEnabled(bool enabled) { Widget::SetEnabled(enabled); }
    void SetSelected(bool selected) { ToolbarGlyphButton::SetSelected(selected); }
    [[nodiscard]] bool IsEnabled() const { return Widget::IsEnabled(); }
    [[nodiscard]] bool IsSelected() const { return Widget::IsSelected(); }
};

} // namespace we::runtime::kindui
