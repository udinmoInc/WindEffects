#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "Widgets/Panel.h"
#include "Layout/Box.h"
#include "Core/Icon.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace WindEffects::Editor::UI {

class UIFRAMEWORK_API PanelBuilder {
public:
    explicit PanelBuilder(std::string_view title);

    PanelBuilder& TabIcon(std::string_view iconName);
    PanelBuilder& HeaderHeight(float height);
    PanelBuilder& Transparent();
    PanelBuilder& Collapsible(bool collapsible);
    PanelBuilder& WithCloseButton(std::function<void()> onClose = {});
    PanelBuilder& WithHeaderAction(std::string_view iconName, std::function<void()> onClick);
    PanelBuilder& Toolbar(std::shared_ptr<Widget> toolbar);
    PanelBuilder& ToolbarBox(std::function<void(HorizontalBox&)> build);

    [[nodiscard]] std::shared_ptr<Panel> Content(std::shared_ptr<Widget> content);
    [[nodiscard]] std::shared_ptr<Panel> Build();

private:
    std::shared_ptr<Panel> m_Panel;
};

} // namespace WindEffects::Editor::UI
