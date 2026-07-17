#include "KindUI/Widgets/Components.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <cmath>


namespace we::runtime::kindui {

EmptyState::EmptyState(std::string title, std::string subtitle) {
    SetStyleClass("Page");
    Align(AlignItems::Center);
    Justify(JustifyContent::Center);
    Gap(8.0f);

    auto titleLabel = std::make_shared<Label>(std::move(title));
    titleLabel->SetHorizontalAlignment(HorizontalAlignment::Center);
    AddChild(titleLabel);

    if (!subtitle.empty()) {
        auto sub = std::make_shared<Label>(std::move(subtitle));
        sub->SetHorizontalAlignment(HorizontalAlignment::Center);
        AddChild(sub);
    }
}

StatusBadge::StatusBadge(std::string text) : m_Text(std::move(text)) {
    SetStyleClass("Card");
    SetMinSize({ 48.0f, 20.0f });
}

void StatusBadge::SetText(std::string text) {
    m_Text = std::move(text);
    InvalidateLayout();
}

Size StatusBadge::Measure(const Size& availableSize) {
    (void)availableSize;
    m_DesiredSize = ClampDesiredSize({ std::max(48.0f, static_cast<float>(m_Text.size()) * 7.0f + 16.0f), 20.0f });
    return m_DesiredSize;
}

void StatusBadge::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    ClearLayoutDirty();
}

void StatusBadge::Paint(PaintContext& context) {
    ClearPaintDirty();
    const Color bg = ThemeColor(ColorToken::AccentPrimary);
    const float radius = ThemeMetric(MetricToken::CornerRadiusSmall);
    context.DrawRoundedRect(m_Geometry, bg, radius);
    const float fontSize = ThemeMetric(MetricToken::TextSizeSmall);
    const float textW = context.GetTextWidth(m_Text, fontSize);
    const Point textPos{
        m_Geometry.x + (m_Geometry.width - textW) * 0.5f,
        m_Geometry.y + (m_Geometry.height - fontSize) * 0.5f
    };
    context.DrawText(m_Text, textPos, ThemeColor(ColorToken::TextPrimary), fontSize);
}

DialogChrome::DialogChrome(std::string title, const std::shared_ptr<Widget>& body) {
    SetStyleClass("Card");
    Gap(12.0f);
    Padding(Margin{ 16.0f, 16.0f, 16.0f, 16.0f });
    AddChild(std::make_shared<SectionHeader>(std::move(title)));
    if (body) {
        body->SetFlexGrow(1.0f);
        AddChild(body);
    }
}

ToolbarBar::ToolbarBar() {
    SetStyleClass("ToolbarButton");
    Gap(6.0f);
    Align(AlignItems::Center);
    SetMinSize({ 0.0f, 36.0f });
}

SkeletonBlock::SkeletonBlock() {
    SetMinSize({ 40.0f, 16.0f });
    SetFlexGrow(1.0f);
}

Size SkeletonBlock::Measure(const Size& availableSize) {
    m_DesiredSize = ClampDesiredSize({
        availableSize.width > 0.0f ? availableSize.width : 120.0f,
        std::max(m_MinSize.height, 16.0f)
    });
    return m_DesiredSize;
}

void SkeletonBlock::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    ClearLayoutDirty();
}

void SkeletonBlock::Tick(float deltaTime) {
    m_Pulse += deltaTime * 2.5f;
    InvalidatePaint();
    Widget::Tick(deltaTime);
}

void SkeletonBlock::Paint(PaintContext& context) {
    ClearPaintDirty();
    Color c = ThemeColor(ColorToken::HoverBackground);
    const float wave = 0.55f + 0.25f * std::sin(m_Pulse);
    c.a *= wave;
    context.DrawRoundedRect(m_Geometry, c, ThemeMetric(MetricToken::CornerRadiusSmall));
}

std::shared_ptr<SearchBoxControl> MakeSearchBar(std::string placeholder) {
    auto box = std::make_shared<SearchBoxControl>(std::move(placeholder));
    box->SetStyleClass("SearchBar");
    return box;
}

std::shared_ptr<EmptyState> MakeEmptyState(std::string title, std::string subtitle) {
    return std::make_shared<EmptyState>(std::move(title), std::move(subtitle));
}

std::shared_ptr<StatusBadge> MakeStatusBadge(std::string text) {
    return std::make_shared<StatusBadge>(std::move(text));
}

std::shared_ptr<SectionHeader> MakeSectionHeader(std::string title) {
    return std::make_shared<SectionHeader>(std::move(title));
}

std::shared_ptr<Card> MakeCard() {
    auto card = std::make_shared<Card>();
    card->SetStyleClass("Card");
    return card;
}

std::shared_ptr<SkeletonBlock> MakeSkeleton() {
    return std::make_shared<SkeletonBlock>();
}

} // namespace we::runtime::kindui
