// ==============================================================================
// WindEffects — Toolbar — ToolbarItem
// UI widget used by the Toolbar module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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

 
