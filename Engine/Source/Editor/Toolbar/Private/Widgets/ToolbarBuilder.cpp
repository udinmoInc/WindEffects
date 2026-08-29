#include "Widgets/ToolbarBuilder.h"
#include "Widgets/ToolbarItem.h"

namespace we::editor::toolbar {

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

ToolbarBuilder& ToolbarBuilder::LeftInset(float inset) {
    m_LeftInset = inset;
    return *this;
}

ToolbarBuilder& ToolbarBuilder::RightInset(float inset) {
    m_RightInset = inset;
    return *this;
}

ToolbarBuilder& ToolbarBuilder::EdgePadding(float padding) {
    m_EdgePadding = padding;
    return *this;
}

void ToolbarBuilder::PushItem(ToolbarItemSpec spec) {
    m_Items.push_back(std::move(spec));
}

ToolbarBuilder& ToolbarBuilder::AddWidget(const std::shared_ptr<Widget>& widget, ToolbarAlignment alignment) {
    ToolbarItemSpec spec;
    spec.customWidget = widget;
    spec.alignment = alignment;
    PushItem(std::move(spec));
    return *this;
}

ToolbarBuilder& ToolbarBuilder::Group(
    ToolbarAlignment alignment,
    ToolbarGroupStyle style,
    const std::function<void(ToolbarBuilder&)>& buildGroup)
{
    if (!buildGroup) {
        return *this;
    }

    ToolbarBuilder groupBuilder;
    buildGroup(groupBuilder);

    auto group = ToolbarItem::MakeGroup(style);
    for (const auto& childSpec : groupBuilder.m_Items) {
        if (childSpec.icon == "__separator__" || childSpec.customWidget || childSpec.group) {
            continue;
        }
        std::shared_ptr<ToolButton> button;
        if (childSpec.style == ToolButtonStyle::TransportButton || childSpec.isPlayTransport) {
            button = ToolbarItem::Transport(
                childSpec.icon,
                childSpec.tooltip,
                childSpec.onClick,
                childSpec.isPlayTransport);
        } else if (childSpec.dropdown) {
            button = ToolbarItem::LabeledDropdown(
                childSpec.icon,
                childSpec.label,
                childSpec.tooltip,
                childSpec.onClick);
        } else {
            button = ToolbarItem::Icon(childSpec.icon, childSpec.tooltip, childSpec.onClick);
        }
        if (childSpec.configure) {
            childSpec.configure(button);
        }
        if (childSpec.onMouseWheel) {
            button->SetOnMouseWheel(childSpec.onMouseWheel);
        }
        group->AddChildWidget(button);
    }

    ToolbarItemSpec spec;
    spec.group = std::move(group);
    spec.alignment = alignment;
    PushItem(std::move(spec));
    return *this;
}

ToolbarBuilder& ToolbarBuilder::IconItem(
    std::string_view icon,
    std::string_view tooltip,
    std::function<void()> onClick,
    std::function<void(std::shared_ptr<ToolButton>)> configure)
{
    ToolbarItemSpec spec;
    spec.icon = std::string(icon);
    spec.tooltip = std::string(tooltip);
    spec.onClick = std::move(onClick);
    spec.style = ToolButtonStyle::ToolbarIconOnly;
    spec.configure = std::move(configure);
    PushItem(std::move(spec));
    return *this;
}

ToolbarBuilder& ToolbarBuilder::DropdownItem(
    std::string_view icon,
    std::string_view label,
    std::function<void()> onClick,
    std::string_view tooltip,
    std::function<void(std::shared_ptr<ToolButton>)> configure)
{
    ToolbarItemSpec spec;
    spec.icon = std::string(icon);
    spec.label = std::string(label);
    spec.tooltip = tooltip.empty() ? spec.label : std::string(tooltip);
    spec.onClick = std::move(onClick);
    spec.style = ToolButtonStyle::ToolbarInline;
    spec.dropdown = true;
    spec.configure = std::move(configure);
    PushItem(std::move(spec));
    return *this;
}

ToolbarBuilder& ToolbarBuilder::TransportItem(
    std::string_view icon,
    std::string_view tooltip,
    std::function<void()> onClick,
    bool isPlay)
{
    ToolbarItemSpec spec;
    spec.icon = std::string(icon);
    spec.tooltip = std::string(tooltip);
    spec.onClick = std::move(onClick);
    spec.style = ToolButtonStyle::TransportButton;
    spec.isPlayTransport = isPlay;
    PushItem(std::move(spec));
    return *this;
}

ToolbarBuilder& ToolbarBuilder::Item(
    std::string_view icon,
    std::string_view label,
    std::function<void()> onClick,
    std::string_view tooltip,
    std::function<void(std::shared_ptr<ToolButton>)> configure)
{
    return IconItem(icon, tooltip.empty() ? label : tooltip, std::move(onClick), std::move(configure));
}

ToolbarBuilder& ToolbarBuilder::Dropdown(
    std::string_view icon,
    std::string_view label,
    std::function<void()> onClick,
    std::string_view tooltip,
    std::function<void(std::shared_ptr<ToolButton>)> configure)
{
    return DropdownItem(icon, label, std::move(onClick), tooltip, std::move(configure));
}

ToolbarBuilder& ToolbarBuilder::Separator(ToolbarAlignment alignment) {
    ToolbarItemSpec spec;
    spec.alignment = alignment;
    spec.icon = "__separator__";
    PushItem(std::move(spec));
    return *this;
}

ToolbarBuilder& ToolbarBuilder::Right(const std::function<void(ToolbarBuilder&)>& buildRight) {
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
    if (m_Height > 0.0f) {
        toolbar->SetHeight(m_Height);
    }
    if (m_IconSize > 0.0f) {
        toolbar->SetIconSize(m_IconSize);
    }
    toolbar->SetFloating(m_Floating);
    if (m_LeftInset > 0.0f) {
        toolbar->SetLeftInset(m_LeftInset);
    }
    if (m_RightInset > 0.0f) {
        toolbar->SetRightInset(m_RightInset);
    }
    if (m_EdgePadding > 0.0f) {
        toolbar->SetEdgePadding(m_EdgePadding);
    }

    for (const auto& spec : m_Items) {
        if (spec.icon == "__separator__") {
            toolbar->AddSeparator(spec.alignment);
            continue;
        }
        if (spec.customWidget) {
            toolbar->AddWidget(spec.customWidget, spec.alignment);
            continue;
        }
        if (spec.group) {
            toolbar->AddGroup(spec.group, spec.alignment);
            continue;
        }

        std::shared_ptr<ToolButton> button;
        if (spec.style == ToolButtonStyle::TransportButton || spec.isPlayTransport) {
            button = ToolbarItem::Transport(spec.icon, spec.tooltip, spec.onClick, spec.isPlayTransport);
        } else if (spec.dropdown || spec.style == ToolButtonStyle::ToolbarInline) {
            button = ToolbarItem::LabeledDropdown(spec.icon, spec.label, spec.tooltip, spec.onClick);
        } else {
            button = ToolbarItem::Icon(spec.icon, spec.tooltip, spec.onClick);
        }
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
        toolbar->AddWidget(button, spec.alignment);
    }

    return toolbar;
}

} // namespace we::editor::toolbar
