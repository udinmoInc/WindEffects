#include "Widgets/ToolbarBuilder.h"

namespace WindEffects::Editor::UI {

ToolbarBuilder& ToolbarBuilder::Height(float height) {
    m_Height = height;
    return *this;
}

ToolbarBuilder& ToolbarBuilder::IconSize(float size) {
    m_IconSize = size;
    return *this;
}

ToolbarBuilder& ToolbarBuilder::Floating() {
    m_Floating = true;
    return *this;
}

ToolbarBuilder& ToolbarBuilder::Item(
    std::string_view icon,
    std::string_view label,
    std::function<void()> onClick,
    std::string_view tooltip,
    std::function<void(std::shared_ptr<ToolButton>)> configure) {
    ToolbarItemSpec spec;
    spec.icon = std::string(icon);
    spec.label = std::string(label);
    spec.tooltip = tooltip.empty() ? spec.label : std::string(tooltip);
    spec.onClick = std::move(onClick);
    spec.configure = std::move(configure);
    m_Items.push_back(std::move(spec));
    return *this;
}

ToolbarBuilder& ToolbarBuilder::Dropdown(
    std::string_view icon,
    std::string_view label,
    std::function<void()> onClick,
    std::string_view tooltip,
    std::function<void(std::shared_ptr<ToolButton>)> configure) {
    ToolbarItemSpec spec;
    spec.icon = std::string(icon);
    spec.label = std::string(label);
    spec.tooltip = tooltip.empty() ? spec.label : std::string(tooltip);
    spec.onClick = std::move(onClick);
    spec.dropdown = true;
    spec.configure = std::move(configure);
    m_Items.push_back(std::move(spec));
    return *this;
}

ToolbarBuilder& ToolbarBuilder::Separator(ToolbarAlignment alignment) {
    ToolbarItemSpec spec;
    spec.alignment = alignment;
    spec.icon = "__separator__";
    m_Items.push_back(std::move(spec));
    return *this;
}

ToolbarBuilder& ToolbarBuilder::Right(std::function<void(ToolbarBuilder&)> buildRight) {
    if (buildRight) {
        ToolbarBuilder rightBuilder;
        buildRight(rightBuilder);
        for (auto& item : rightBuilder.m_Items) {
            item.alignment = ToolbarAlignment::Right;
            m_Items.push_back(std::move(item));
        }
    }
    return *this;
}

std::shared_ptr<Toolbar> ToolbarBuilder::Build() {
    auto toolbar = std::make_shared<Toolbar>();
    toolbar->SetHeight(m_Height);
    toolbar->SetIconSize(m_IconSize);
    toolbar->SetFloating(m_Floating);

    for (const auto& spec : m_Items) {
        if (spec.icon == "__separator__") {
            toolbar->AddSeparator(spec.alignment);
            continue;
        }

        auto button = toolbar->AddTool(
            spec.icon,
            spec.label,
            spec.onClick,
            spec.tooltip,
            false,
            spec.alignment);
        button->SetButtonStyle(spec.style);
        if (spec.dropdown) {
            button->SetIsDropdown(true);
        }
        if (spec.onMouseWheel) {
            button->SetOnMouseWheel(spec.onMouseWheel);
        }
        if (spec.configure) {
            spec.configure(button);
        }
    }

    return toolbar;
}

} // namespace WindEffects::Editor::UI
