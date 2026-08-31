#pragma once

#include "Toolbar/Export.h"
#include "Widgets/Toolbar.h"
#include "Widgets/ToolButton.h"
#include "KindUI/Core/WindIcon.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::editor::toolbar {

struct ToolbarItemSpec {
    we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone;
    std::string label;
    std::string tooltip;
    std::function<void()> onClick;
    ToolButtonStyle style = ToolButtonStyle::ToolbarIconOnly;
    bool dropdown = false;
    bool isPlayTransport = false;
    bool isSeparator = false;
    ToolbarAlignment alignment = ToolbarAlignment::Left;
    std::function<void(float)> onMouseWheel;
    std::function<void(std::shared_ptr<ToolButton>)> configure;
    std::shared_ptr<Widget> customWidget;
    std::shared_ptr<ToolbarGroup> group;
};

class TOOLBAR_API ToolbarBuilder {
public:
    ToolbarBuilder& Height(float height);
    ToolbarBuilder& IconSize(float size);
    ToolbarBuilder& Floating();
    ToolbarBuilder& LeftInset(float inset);
    ToolbarBuilder& RightInset(float inset);
    ToolbarBuilder& EdgePadding(float padding);

    ToolbarBuilder& AddWidget(const std::shared_ptr<Widget>& widget, ToolbarAlignment alignment = ToolbarAlignment::Left);
    ToolbarBuilder& Group(
        ToolbarAlignment alignment,
        ToolbarGroupStyle style,
        const std::function<void(ToolbarBuilder&)>& buildGroup);

    ToolbarBuilder& IconItem(
        we::runtime::kindui::WindIconRef icon,
        std::string_view tooltip,
        std::function<void()> onClick = {},
        std::function<void(std::shared_ptr<ToolButton>)> configure = {});

    ToolbarBuilder& DropdownItem(
        we::runtime::kindui::WindIconRef icon,
        std::string_view label,
        std::function<void()> onClick = {},
        std::string_view tooltip = {},
        std::function<void(std::shared_ptr<ToolButton>)> configure = {});

    ToolbarBuilder& TransportItem(
        we::runtime::kindui::WindIconRef icon,
        std::string_view tooltip,
        std::function<void()> onClick = {},
        bool isPlay = false);

    ToolbarBuilder& Item(
        we::runtime::kindui::WindIconRef icon,
        std::string_view label = {},
        std::function<void()> onClick = {},
        std::string_view tooltip = {},
        std::function<void(std::shared_ptr<ToolButton>)> configure = {});

    ToolbarBuilder& Dropdown(
        we::runtime::kindui::WindIconRef icon,
        std::string_view label,
        std::function<void()> onClick = {},
        std::string_view tooltip = {},
        std::function<void(std::shared_ptr<ToolButton>)> configure = {});

    ToolbarBuilder& Separator(ToolbarAlignment alignment = ToolbarAlignment::Left);
    ToolbarBuilder& Right(const std::function<void(ToolbarBuilder&)>& buildRight);

    [[nodiscard]] std::shared_ptr<Toolbar> Build();

private:
    void PushItem(ToolbarItemSpec spec);

    float m_Height = 0.0f;
    float m_IconSize = 0.0f;
    float m_LeftInset = 0.0f;
    float m_RightInset = 0.0f;
    float m_EdgePadding = 0.0f;
    bool m_Floating = false;
    std::vector<ToolbarItemSpec> m_Items;
};

} // namespace we::editor::toolbar
