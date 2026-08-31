#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widgets/ToolbarGlyphButton.h"

#include <functional>

namespace we::runtime::kindui {

class KINDUI_API ToolbarIconButton : public ToolbarGlyphButton {
public:
    explicit ToolbarIconButton(WindIconRef icon, const char* tooltip = nullptr)
        : ToolbarGlyphButton(
            icon,
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
