#include "KindUI/Core/Widgets/PanelToolbarRow.h"

namespace we::runtime::kindui {

PanelToolbarRow::PanelToolbarRow(std::string searchPlaceholder)
    : m_SearchPlaceholder(std::move(searchPlaceholder)) {
    const float padH = ThemeMetric(MetricToken::Space2);
    const float padV = 4.0f;
    Padding(Margin{padH, padV, padH, padV});
    Gap(ThemeMetric(MetricToken::Space1));
    Align(AlignItems::Center);

    m_SearchBox = std::make_shared<SearchBoxControl>(m_SearchPlaceholder);
    m_SearchBox->SetFlexGrow(1.0f);
    m_SearchBox->SetFlexShrink(1.0f);
    m_SearchBox->SetMinWidth(ThemeMetric(MetricToken::Space6) * 4.0f);
}

void PanelToolbarRow::Finalize() {
    EnsureBuilt();
}

void PanelToolbarRow::EnsureBuilt() {
    if (m_Built) {
        return;
    }
    AddChild(m_SearchBox);
    for (const auto& btn : m_IconButtons) {
        AddChild(btn);
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

void PanelToolbarRow::AddIconButton(const char* icon, std::function<void()> onClicked) {
    auto btn = std::make_shared<IconButton>(icon);
    btn->SetFlexShrink(0.0f);
    btn->SetOnClicked(std::move(onClicked));
    m_IconButtons.push_back(btn);
    if (m_Built) {
        AddChild(btn);
    }
}

std::shared_ptr<IconButton> PanelToolbarRow::GetIconButton(size_t index) const {
    return index < m_IconButtons.size() ? m_IconButtons[index] : nullptr;
}

} // namespace we::runtime::kindui
