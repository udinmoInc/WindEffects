#include "KindUI/Core/Widgets/ToolbarIconButton.h"

namespace we::runtime::kindui {

void ToolbarIconButton::SetOnClicked(std::function<void()> callback) {
    ToolbarGlyphButton::SetOnClicked(std::move(callback));
}

} // namespace we::runtime::kindui
