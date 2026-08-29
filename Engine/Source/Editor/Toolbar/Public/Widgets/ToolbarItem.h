#pragma once

#include "Toolbar/Export.h"
#include "Widgets/Toolbar.h"
#include "Widgets/ToolButton.h"

#include <functional>
#include <memory>
#include <string>

namespace we::editor::toolbar {
namespace ToolbarItem {

TOOLBAR_API std::shared_ptr<ToolButton> Icon(
    const std::string& iconName,
    const std::string& tooltip,
    std::function<void()> onClick);

TOOLBAR_API std::shared_ptr<ToolButton> LabeledDropdown(
    const std::string& iconName,
    const std::string& label,
    const std::string& tooltip,
    std::function<void()> onClick);

TOOLBAR_API std::shared_ptr<ToolButton> Transport(
    const std::string& iconName,
    const std::string& tooltip,
    std::function<void()> onClick,
    bool isPlay = false);

TOOLBAR_API std::shared_ptr<ToolbarGroup> MakeGroup(ToolbarGroupStyle style = ToolbarGroupStyle::Transparent);

} // namespace ToolbarItem
} // namespace we::editor::toolbar
