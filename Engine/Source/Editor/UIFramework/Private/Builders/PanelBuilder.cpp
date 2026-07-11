#include "WindEffects/Editor/UI/Builders/PanelBuilder.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"

namespace WindEffects::Editor::UI {

PanelBuilder::PanelBuilder(std::string_view title)
    : m_Panel(std::make_shared<Panel>(std::string(title))) {
    m_Panel->SetHeaderHeight(ResolveThemeMetric(ThemeToken::PanelTabHeight));
}

PanelBuilder& PanelBuilder::TabIcon(std::string_view iconName) {
    m_Panel->SetTabIcon(std::string(iconName));
    return *this;
}

PanelBuilder& PanelBuilder::HeaderHeight(float height) {
    m_Panel->SetHeaderHeight(height);
    return *this;
}

PanelBuilder& PanelBuilder::Transparent() {
    m_Panel->SetTransparentBackground(true);
    return *this;
}

PanelBuilder& PanelBuilder::Collapsible(bool collapsible) {
    m_Panel->SetCollapsible(collapsible);
    return *this;
}

PanelBuilder& PanelBuilder::WithCloseButton(std::function<void()> onClose) {
    m_Panel->AddHeaderAction(Icons::XName, std::move(onClose));
    return *this;
}

PanelBuilder& PanelBuilder::WithHeaderAction(std::string_view iconName, std::function<void()> onClick) {
    m_Panel->AddHeaderAction(std::string(iconName), std::move(onClick));
    return *this;
}

PanelBuilder& PanelBuilder::Toolbar(std::shared_ptr<Widget> toolbar) {
    m_Panel->SetToolbar(std::move(toolbar));
    return *this;
}

PanelBuilder& PanelBuilder::ToolbarBox(std::function<void(HorizontalBox&)> build) {
    auto toolbarBox = std::make_shared<HorizontalBox>();
    toolbarBox->SetPadding(Margin{
        ResolveThemeMetric(ThemeToken::Space3),
        ResolveThemeMetric(ThemeToken::Space1),
        ResolveThemeMetric(ThemeToken::Space3),
        ResolveThemeMetric(ThemeToken::Space1)
    });
    if (build) {
        build(*toolbarBox);
    }
    m_Panel->SetToolbar(toolbarBox);
    return *this;
}

std::shared_ptr<Panel> PanelBuilder::Content(std::shared_ptr<Widget> content) {
    m_Panel->SetContent(std::move(content));
    return m_Panel;
}

std::shared_ptr<Panel> PanelBuilder::Build() {
    return m_Panel;
}

} // namespace WindEffects::Editor::UI
