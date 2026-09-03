#include "LandscapeFormLayout.h"

#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Widgets/Components.h"
#include "KindUI/Widgets/Label.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Tokens/ChromeSeparation.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

#include "KindUI/Core/TextMetrics.h"
#include "Text/Layout/TextStyle.h"

#include <cstdio>

namespace we::editor::terrain {
namespace {

using namespace we::runtime::kindui;
namespace PanelChrome = we::editor::panels::PanelChrome;

class FormSectionTitleWidget final : public Widget {
public:
    explicit FormSectionTitleWidget(std::string title, bool leadingGap)
        : m_Title(std::move(title))
        , m_LeadingGap(leadingGap) {}

    Size Measure(const Size& availableSize) override {
        const float gap = m_LeadingGap ? ChromeSeparation::GapWide() : 0.0f;
        const float bandH = PropertyPanelChrome::SectionHeight();
        m_DesiredSize = Size{ availableSize.width, gap + bandH };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
        const float gap = m_LeadingGap ? ChromeSeparation::GapWide() : 0.0f;
        m_TitleBand = Rect{
            allottedRect.x,
            allottedRect.y + gap,
            allottedRect.width,
            std::max(0.0f, allottedRect.height - gap)
        };
    }

    void Paint(PaintContext& context) override {
        if (m_TitleBand.IsEmpty()) {
            return;
        }
        PanelChrome::PaintListLabelBand(context, m_TitleBand);
        const float scale = std::max(1.0f, DPIContext::GetScale());
        const float fontSize = ResolveMetric(MetricToken::TextSizeCategory) * scale;
        // Match form-row label left edge (column padding already applied).
        const float textY = m_TitleBand.y + (m_TitleBand.height - fontSize) * 0.5f;
        context.DrawText(
            m_Title,
            Point{ m_TitleBand.x, textY },
            ResolveColor(ColorToken::TextPrimary),
            fontSize,
            we::runtime::text::layout::FontWeight::Medium);
    }

private:
    std::string m_Title;
    bool m_LeadingGap = false;
    Rect m_TitleBand;
};

} // namespace

void ConfigureLandscapeFormColumn(const std::shared_ptr<Column>& layout) {
    if (!layout) {
        return;
    }
    LayoutMetrics::ConfigurePropertyFormColumn(*layout);
}

void AddFormSectionTitle(const std::shared_ptr<Column>& layout, std::string_view title) {
    const bool leadingGap = layout && !layout->GetChildren().empty();
    layout->AddChild(std::make_shared<FormSectionTitleWidget>(std::string(title), leadingGap));
}

void AddFormField(
    const std::shared_ptr<Column>& layout,
    const std::string& label,
    const std::string& value,
    std::function<void(std::string_view)> onCommit)
{
    layout->AddChild(LayoutMetrics::MakeTextFormRow(label, value, std::move(onCommit)));
}

void AddFormChipRow(
    const std::shared_ptr<Column>& layout,
    const std::vector<FormChip>& chips,
    size_t maxPerRow)
{
    std::shared_ptr<Row> currentRow = nullptr;
    size_t countInRow = 0;

    for (const auto& [label, icon, selected, onClick] : chips) {
        if (!currentRow || countInRow >= maxPerRow) {
            currentRow = MakeRow();
            currentRow->Gap(ResolveMetric(MetricToken::Space1));
            currentRow->SetFlexShrink(0.0f);
            layout->AddChild(currentRow);
            countInRow = 0;
        }
        auto btn = MakeSecondaryAction(label, icon);
        btn->SetFlexGrow(1.0f);
        btn->SetFlexShrink(1.0f);
        btn->SetMinWidth(LayoutMetrics::FormChipButtonMinWidth());
        btn->SetOnClicked(onClick);
        currentRow->AddChild(btn);
        ++countInRow;
    }
}

void AddFormToggle(
    const std::shared_ptr<Column>& layout,
    const std::string& label,
    bool on,
    std::function<void()> onClick)
{
    auto btn = MakeSecondaryAction(label + (on ? " : ON" : " : OFF"));
    btn->SetOnClicked(std::move(onClick));
    btn->SetHorizontalAlignment(HorizontalAlignment::Fill);
    btn->SetFlexShrink(0.0f);
    layout->AddChild(btn);
}

void AddFormButton(
    const std::shared_ptr<Column>& layout,
    const std::string& label,
    std::function<void()> onClick,
    bool primary)
{
    std::shared_ptr<DesignButton> btn;
    if (primary) {
        btn = MakePrimaryAction(label);
    } else {
        btn = MakeSecondaryAction(label);
    }
    btn->SetOnClicked(std::move(onClick));
    layout->AddChild(btn);
}

void AddFormInfoRow(
    const std::shared_ptr<Column>& layout,
    const std::string& label,
    const std::string& value)
{
    auto valueLabel = std::make_shared<Label>(value, TypographyToken::PropertyValue);
    layout->AddChild(LayoutMetrics::MakeFormRow(label, valueLabel));
}

std::string FormFormatFloat(float value) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.3g", static_cast<double>(value));
    return buf;
}

std::string FormFormatInt(int value) {
    return std::to_string(value);
}

float FormParseFloat(std::string_view text, float fallback) {
    try {
        return std::stof(std::string(text));
    } catch (...) {
        return fallback;
    }
}

int FormParseInt(std::string_view text, int fallback) {
    try {
        return std::stoi(std::string(text));
    } catch (...) {
        return fallback;
    }
}

} // namespace we::editor::terrain
