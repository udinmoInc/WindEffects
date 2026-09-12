// ==============================================================================
// WindEffects — ContentBrowser — SearchBox
// Public API surface for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "ContentBrowser/Export.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include <functional>
#include <string>

namespace we::editor::widgets {

/// Search box control derived directly from KindUI's shared SearchBoxControl.
class CONTENTBROWSER_API SearchBox : public we::runtime::kindui::SearchBoxControl {
public:
    using OnTextChanged = std::function<void(const std::string&)>;

    SearchBox() : we::runtime::kindui::SearchBoxControl("Search...") {}
    explicit SearchBox(std::string placeholder) : we::runtime::kindui::SearchBoxControl(std::move(placeholder)) {}
    ~SearchBox() override = default;
};

}
