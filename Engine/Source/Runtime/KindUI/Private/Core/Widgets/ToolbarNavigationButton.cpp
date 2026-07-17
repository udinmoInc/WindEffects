#include "KindUI/Core/Widgets/ToolbarNavigationButton.h"

namespace we::runtime::kindui {

void ToolbarNavigationButton::SetOnClicked(std::function<void()> callback) {
    ToolbarGlyphButton::SetOnClicked(std::move(callback));
}

void ToolbarNavigationButton::SetEnabled(bool enabled) {
    Widget::SetEnabled(enabled);
}

void ToolbarNavigationButton::SetSelected(bool selected) {
    ToolbarGlyphButton::SetSelected(selected);
}

} // namespace we::runtime::kindui
