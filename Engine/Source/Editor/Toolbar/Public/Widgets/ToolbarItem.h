// ==============================================================================
// WindEffects — Toolbar — ToolbarItem
// Public API surface for the Toolbar module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
    we::runtime::kindui::WindIconRef icon,
    const std::string& tooltip,
    std::function<void()> onClick);

TOOLBAR_API std::shared_ptr<ToolButton> LabeledDropdown(
    we::runtime::kindui::WindIconRef icon,
    const std::string& label,
    const std::string& tooltip,
    std::function<void()> onClick);

TOOLBAR_API std::shared_ptr<ToolButton> Transport(
    we::runtime::kindui::WindIconRef icon,
    const std::string& tooltip,
    std::function<void()> onClick,
    bool isPlay = false);

TOOLBAR_API std::shared_ptr<ToolbarGroup> MakeGroup(ToolbarGroupStyle style = ToolbarGroupStyle::Transparent);

} // namespace ToolbarItem
} // namespace we::editor::toolbar
