// DEPRECATED: This file has been removed. Use KindUI::SearchBoxControl directly.
// This file is kept for build compatibility and will be removed in a future update.
#pragma once

#include "ContentBrowser/Export.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include <functional>
#include <string>

namespace we::editor::widgets {

// DEPRECATED: Use KindUI::SearchBoxControl directly
class CONTENTBROWSER_API SearchBox : public we::runtime::kindui::SearchBoxControl {
public:
    using OnTextChanged = std::function<void(const std::string&)>;

    SearchBox() : we::runtime::kindui::SearchBoxControl("Search...") {}
    explicit SearchBox(std::string placeholder) : we::runtime::kindui::SearchBoxControl(std::move(placeholder)) {}
    ~SearchBox() override = default;
};

}