#include "Widgets/ToolbarItem.h"
#include "Widgets/ToolButton.h"
#include "Widgets/Toolbar.h"

namespace we::editor::toolbar {
namespace ToolbarItem {

std::shared_ptr<ToolButton> Icon(
    const std::string& iconName,
    const std::string& tooltip,
    std::function<void()> onClick)
{
    auto button = std::make_shared<ToolButton>(iconName, "", std::move(onClick), tooltip);
    button->SetButtonStyle(ToolButtonStyle::ToolbarIconOnly);
    return button;
}

std::shared_ptr<ToolButton> LabeledDropdown(
    const std::string& iconName,
    const std::string& label,
    const std::string& tooltip,
    std::function<void()> onClick)
{
    auto button = std::make_shared<ToolButton>(iconName, label, std::move(onClick), tooltip);
    button->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    button->SetIsDropdown(true);
    return button;
}

std::shared_ptr<ToolButton> Transport(
    const std::string& iconName,
    const std::string& tooltip,
    std::function<void()> onClick,
    bool isPlay)
{
    auto button = std::make_shared<ToolButton>(iconName, "", std::move(onClick), tooltip);
    button->SetButtonStyle(isPlay ? ToolButtonStyle::PlayButton : ToolButtonStyle::TransportButton);
    return button;
}

std::shared_ptr<ToolbarGroup> MakeGroup(ToolbarGroupStyle style) {
    auto group = std::make_shared<ToolbarGroup>();
    group->SetStyle(style);
    return group;
}

} // namespace ToolbarItem
} // namespace we::editor::toolbar
