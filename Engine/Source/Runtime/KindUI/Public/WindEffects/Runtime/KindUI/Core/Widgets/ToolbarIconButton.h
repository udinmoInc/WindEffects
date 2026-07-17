#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/Widgets/ToolbarGlyphButton.h"

#include <functional>
#include <string>

namespace WindEffects::Editor::UI {

class UI_API ToolbarIconButton : public ToolbarGlyphButton {
public:
    explicit ToolbarIconButton(const std::string& iconName, const char* tooltip = nullptr)
        : ToolbarGlyphButton(
            iconName,
            StyleRole::IconButton,
            ThemeToken::IconButtonSize,
            ThemeToken::IconSizeToolbar)
    {
        (void)tooltip;
    }

    void SetOnClicked(std::function<void()> callback) { ToolbarGlyphButton::SetOnClicked(std::move(callback)); }
    void SetEnabled(bool enabled) { Widget::SetEnabled(enabled); }
    void SetSelected(bool selected) { ToolbarGlyphButton::SetSelected(selected); }
    [[nodiscard]] bool IsEnabled() const { return Widget::IsEnabled(); }
    [[nodiscard]] bool IsSelected() const { return Widget::IsSelected(); }
};

} // namespace WindEffects::Editor::UI
