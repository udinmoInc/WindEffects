#include "Widgets/ToolbarItem.h"

#include "Widgets/ToolButton.h"
#include "Widgets/Toolbar.h"

namespace we::editor::toolbar {
namespace ToolbarItem {

std::shared_ptr<ToolButton> Icon(
    we::runtime::kindui::WindIconRef icon,
    const std::string& tooltip,
    std::function<void()> onClick)
{
    auto button = std::make_shared<ToolButton>(icon, "", std::move(onClick), tooltip);
    button->SetButtonStyle(ToolButtonStyle::ToolbarIconOnly);
    return button;
}

std::shared_ptr<ToolButton> LabeledDropdown(
    we::runtime::kindui::WindIconRef icon,
    const std::string& label,
    const std::string& tooltip,
    std::function<void()> onClick)
{
    auto button = std::make_shared<ToolButton>(icon, label, std::move(onClick), tooltip);
    button->SetButtonStyle(ToolButtonStyle::ToolbarInline);
    button->SetIsDropdown(true);
    return button;
}

std::shared_ptr<ToolButton> Transport(
    we::runtime::kindui::WindIconRef icon,
    const std::string& tooltip,
    std::function<void()> onClick,
    bool isPlay)
{
    auto button = std::make_shared<ToolButton>(icon, "", std::move(onClick), tooltip);
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
