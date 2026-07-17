#pragma once

#include "Toolbar/Export.h"
#include "Widgets/Toolbar.h"
#include "Widgets/ToolButton.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::editor::toolbar {

struct ToolbarItemSpec {
    std::string icon;
    std::string label;
    std::string tooltip;
    std::function<void()> onClick;
    ToolButtonStyle style = ToolButtonStyle::ToolbarInline;
    bool dropdown = false;
    ToolbarAlignment alignment = ToolbarAlignment::Left;
    std::function<void(float)> onMouseWheel;
    std::function<void(std::shared_ptr<ToolButton>)> configure;
};

class TOOLBAR_API ToolbarBuilder {
public:
    ToolbarBuilder& Height(float height);
    ToolbarBuilder& IconSize(float size);
    ToolbarBuilder& Floating();

    ToolbarBuilder& Item(
        std::string_view icon,
        std::string_view label = {},
        std::function<void()> onClick = {},
        std::string_view tooltip = {},
        std::function<void(std::shared_ptr<ToolButton>)> configure = {});

    ToolbarBuilder& Dropdown(
        std::string_view icon,
        std::string_view label,
        std::function<void()> onClick = {},
        std::string_view tooltip = {},
        std::function<void(std::shared_ptr<ToolButton>)> configure = {});

    ToolbarBuilder& Separator(ToolbarAlignment alignment = ToolbarAlignment::Left);
    ToolbarBuilder& Right(std::function<void(ToolbarBuilder&)> buildRight);

    [[nodiscard]] std::shared_ptr<Toolbar> Build();

private:
    float m_Height = 28.0f;
    float m_IconSize = 16.0f;
    bool m_Floating = false;
    std::vector<ToolbarItemSpec> m_Items;
};

} // namespace we::editor::toolbar