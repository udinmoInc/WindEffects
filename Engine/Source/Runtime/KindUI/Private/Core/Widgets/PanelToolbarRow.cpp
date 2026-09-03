#include "KindUI/Core/Widgets/PanelToolbarRow.h"
#include "KindUI/Core/Widgets/VerticalDivider.h"
#include "KindUI/Core/LayoutMetrics.h"

namespace we::runtime::kindui {

PanelToolbarRow::PanelToolbarRow(std::string searchPlaceholder)
    : m_SearchPlaceholder(std::move(searchPlaceholder)) {
    Padding(Margin{});
    Gap(ThemeMetric(MetricToken::ChromeSeparationGapWide));
    Align(AlignItems::Center);

    m_SearchBox = std::make_shared<SearchBoxControl>(m_SearchPlaceholder);
    m_SearchBox->SetToolbarInset(true);
    m_SearchBox->SetMargin(Margin{ 0.0f, 0.0f, ThemeMetric(MetricToken::Space1), 0.0f });
    m_SearchBox->SetFlexGrow(1.0f);
    m_SearchBox->SetFlexShrink(1.0f);
    m_SearchBox->SetMinWidth(ThemeMetric(MetricToken::Space6) * 4.0f);
}

Size PanelToolbarRow::Measure(const Size& availableSize) {
    const float rowH = LayoutMetrics::SearchRowHeight();
    Size childAvail = availableSize;
    if (childAvail.height > rowH) {
        childAvail.height = rowH;
    }
    Size size = Row::Measure(childAvail);
    size.height = rowH;
    m_DesiredSize = size;
    return m_DesiredSize;
}

void PanelToolbarRow::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    Row::Arrange(allottedRect);
}

void PanelToolbarRow::Finalize() {
    EnsureBuilt();
}

void PanelToolbarRow::EnsureBuilt() {
    if (m_Built) {
        return;
    }
    AddChild(m_SearchBox);
    for (const auto& item : m_TrailingItems) {
        AddChild(item);
    }
    m_Built = true;
}

void PanelToolbarRow::SetSearchText(std::string text) {
    EnsureBuilt();
    if (m_SearchBox) {
        m_SearchBox->SetText(std::move(text));
    }
}

const std::string& PanelToolbarRow::GetSearchText() const {
    static const std::string kEmpty;
    return m_SearchBox ? m_SearchBox->GetText() : kEmpty;
}

void PanelToolbarRow::SetOnSearchChanged(std::function<void(const std::string&)> callback) {
    EnsureBuilt();
    if (m_SearchBox) {
        m_SearchBox->SetOnChanged(std::move(callback));
    }
}

void PanelToolbarRow::AddSeparator() {
    auto divider = std::make_shared<VerticalDivider>();
    divider->SetFlexShrink(0.0f);
    m_TrailingItems.push_back(divider);
    if (m_Built) {
        AddChild(divider);
    }
}

void PanelToolbarRow::AddIconButton(WindIconRef icon, std::function<void()> onClicked) {
    auto btn = std::make_shared<IconButton>(icon);
    btn->SetBorderless(true);
    btn->SetFlexShrink(0.0f);
    btn->SetOnClicked(std::move(onClicked));
    m_IconButtons.push_back(btn);
    m_TrailingItems.push_back(btn);
    if (m_Built) {
        AddChild(btn);
    }
}

std::shared_ptr<IconButton> PanelToolbarRow::GetIconButton(size_t index) const {
    return index < m_IconButtons.size() ? m_IconButtons[index] : nullptr;
}

} // namespace we::runtime::kindui
