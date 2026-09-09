// ==============================================================================
// WindEffects — KindUI — PanelBuilder
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Panel/Panel.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Core/WindIcon.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace we::runtime::kindui::panels {

using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Row;

class KINDUI_API PanelBuilder {
public:
    explicit PanelBuilder(std::string_view title);
    explicit PanelBuilder(const char* title);
    explicit PanelBuilder(const std::string& title);

    static PanelBuilder Create(std::string_view title = "") { return PanelBuilder(title); }
    static PanelBuilder Create(const char* title) { return PanelBuilder(title); }
    static PanelBuilder Create(const std::string& title) { return PanelBuilder(title); }

    PanelBuilder& TabIcon(we::runtime::kindui::WindIconRef icon);
    PanelBuilder& HeaderHeight(float height);
    PanelBuilder& Transparent();
    PanelBuilder& FloatingToolbar();
    PanelBuilder& Collapsible(bool collapsible);
    PanelBuilder& WithCloseButton(std::function<void()> onClose = {});
    PanelBuilder& WithHeaderAction(we::runtime::kindui::WindIconRef icon, std::function<void()> onClick);
    PanelBuilder& AddHeaderAction(we::runtime::kindui::WindIconRef icon, std::function<void()> onClick) {
        return WithHeaderAction(icon, std::move(onClick)); }
    PanelBuilder& Toolbar(std::shared_ptr<Widget> toolbar);
    PanelBuilder& ToolbarBox(std::function<void(Row&)> build);
    PanelBuilder& ModeTabs(std::shared_ptr<Widget> modeTabs);
    PanelBuilder& Search(std::shared_ptr<Widget> search);
    PanelBuilder& ColumnHeader(std::shared_ptr<Widget> columnHeader);
    PanelBuilder& Footer(std::shared_ptr<Widget> footer);

    PanelBuilder& Content(std::shared_ptr<Widget> content);
    [[nodiscard]] std::shared_ptr<Panel> Build() const;
    operator std::shared_ptr<Panel>() const { return Build(); }

private:
    std::shared_ptr<Panel> m_Panel;
};

} // namespace we::runtime::kindui::panels
