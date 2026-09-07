#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WindIcon.h"
#include "Widgets/MenuBar.h"
#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace we::editor::factory {

/// Centralized factory for creating developer-friendly, standardized editor UI components.
class UIFRAMEWORK_API EditorUIFactory {
public:
    /// Creates a standard panel search input box.
    static std::shared_ptr<we::runtime::kindui::Widget> CreateSearchInput(
        std::function<void(const std::string& query)> onQueryChanged = nullptr,
        const std::string& placeholder = "Search...");

    /// Creates a standardized panel header or toolbar icon button.
    static std::shared_ptr<we::runtime::kindui::Widget> CreateIconButton(
        we::runtime::kindui::WindIconRef icon,
        std::function<void()> onClick,
        const std::string& tooltip = "");

    /// Creates a standardized dropdown menu button.
    static std::shared_ptr<we::runtime::kindui::Widget> CreateDropdownButton(
        const std::string& label,
        we::runtime::kindui::WindIconRef icon,
        std::vector<std::shared_ptr<we::editor::menus::MenuItem>> items,
        const std::string& tooltip = "");

    /// Creates a standard vertical divider line for toolbars and headers.
    static std::shared_ptr<we::runtime::kindui::Widget> CreateVerticalDivider();
};

} // namespace we::editor::factory
