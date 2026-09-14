// ==============================================================================
// WindEffects — TerrainEditor — LandscapeFormLayout
// UI widget used by the TerrainEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "LandscapeFormLayout.h"

#include <KindUI/EditorUI.h>
#include <cstdio>
#include <cmath>

namespace we::editor::terrain {

using namespace we::runtime::kindui;

namespace {

    void AddChildToContainer(const std::shared_ptr<Widget>& container, const std::shared_ptr<Widget>& child) {
        if (!container || !child) return;
        if (auto group = std::dynamic_pointer_cast<CollapsibleGroup>(container)) {
            group->AddContentChild(child);
        } else if (auto col = std::dynamic_pointer_cast<Column>(container)) {
            col->AddChild(child);
        }
    }

    class AxisInputWidget : public TextBox {
    public:
        AxisInputWidget(Color accentColor, const std::string& initialVal, std::function<void(const std::string&)> onCommit)
            : TextBox(initialVal, std::move(onCommit)), m_AccentColor(accentColor)
        {
            SetMinWidth(30.0f);
            SetFlexGrow(1.0f);
            SetFlexShrink(1.0f);
        }

        Size Measure(const Size& availableSize) override {
            const float h = ResolveMetric(MetricToken::SearchBoxHeight);
            m_DesiredSize = Size{ availableSize.width, h };
            return m_DesiredSize;
        }

        void Paint(PaintContext& context) override {
            if (!IsVisible()) return;
            TextBox::Paint(context);

            const float scale = DPIContext::GetScale();
            const float inset = 3.0f * scale;
            const float accentW = 3.0f * scale;
            const Rect accent{
                m_Geometry.x + inset,
                m_Geometry.y + inset,
                accentW,
                std::max(0.0f, m_Geometry.height - inset * 2.0f)
            };
            context.DrawRoundedRect(accent, m_AccentColor, 1.5f);
        }

    private:
        Color m_AccentColor;
    };

} // namespace

void ConfigureLandscapeFormColumn(const std::shared_ptr<Column>& layout) {
    if (!layout) {
        return;
    }
    layout->Align(AlignItems::Stretch);
    layout->Padding(Margin{ 0.0f, 0.0f, 0.0f, 0.0f });
    layout->Gap(0.0f);
}

std::shared_ptr<CollapsibleGroup> AddFormSection(
    const std::shared_ptr<Column>& layout,
    std::string_view title,
    bool expanded)
{
    auto group = std::make_shared<CollapsibleGroup>(std::string(title), expanded);
    group->SetFlexShrink(0.0f);
    if (layout) {
        layout->AddChild(group);
    }
    return group;
}

void AddFormSectionTitle(const std::shared_ptr<Widget>& container, std::string_view title) {
    const bool leadingGap = container != nullptr;
    AddChildToContainer(container, std::make_shared<FormSectionTitle>(std::string(title), leadingGap));
}

void AddFormField(
    const std::shared_ptr<Widget>& container,
    const std::string& label,
    const std::string& value,
    std::function<void(std::string_view)> onCommit)
{
    auto input = std::make_shared<TextBox>(value, [onCommit](const std::string& v) {
        if (onCommit) onCommit(v);
    });
    auto row = std::make_shared<PropertyRowLayout>(label, input);
    AddChildToContainer(container, row);
}

void AddFormVector3Field(
    const std::shared_ptr<Widget>& container,
    const std::string& label,
    float x, float y, float z,
    std::function<void(float, float, float)> onCommit)
{
    auto vecRow = MakeRow();
    vecRow->Align(AlignItems::Center);
    vecRow->Gap(4.0f);
    vecRow->SetFlexGrow(1.0f);
    vecRow->SetFlexShrink(1.0f);

    struct VecState { float x; float y; float z; };
    auto state = std::make_shared<VecState>(VecState{ x, y, z });

    // Inspector-style colored axis accents: Red for X, Green for Y, Blue for Z
    const Color colorX{ 0.88f, 0.28f, 0.28f, 1.0f };
    const Color colorY{ 0.28f, 0.78f, 0.28f, 1.0f };
    const Color colorZ{ 0.28f, 0.48f, 0.88f, 1.0f };

    auto fieldX = std::make_shared<AxisInputWidget>(colorX, FormFormatFloat(x), [state, onCommit](const std::string& v) {
        state->x = FormParseFloat(v, state->x);
        if (onCommit) onCommit(state->x, state->y, state->z);
    });
    auto fieldY = std::make_shared<AxisInputWidget>(colorY, FormFormatFloat(y), [state, onCommit](const std::string& v) {
        state->y = FormParseFloat(v, state->y);
        if (onCommit) onCommit(state->x, state->y, state->z);
    });
    auto fieldZ = std::make_shared<AxisInputWidget>(colorZ, FormFormatFloat(z), [state, onCommit](const std::string& v) {
        state->z = FormParseFloat(v, state->z);
        if (onCommit) onCommit(state->x, state->y, state->z);
    });

    vecRow->AddChild(fieldX);
    vecRow->AddChild(fieldY);
    vecRow->AddChild(fieldZ);

    auto row = std::make_shared<PropertyRowLayout>(label, vecRow);
    AddChildToContainer(container, row);
}

void AddFormChipRow(
    const std::shared_ptr<Widget>& container,
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
            AddChildToContainer(container, currentRow);
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
    const std::shared_ptr<Widget>& container,
    const std::string& label,
    bool on,
    std::function<void(bool)> onChange)
{
    auto cb = std::make_shared<CheckBox>("", on);
    cb->SetOnChanged(std::move(onChange));
    auto row = std::make_shared<PropertyRowLayout>(label, cb);
    AddChildToContainer(container, row);
}

void AddFormButton(
    const std::shared_ptr<Widget>& container,
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
    AddChildToContainer(container, btn);
}

void AddFormInfoRow(
    const std::shared_ptr<Widget>& container,
    const std::string& label,
    const std::string& value)
{
    auto valueLabel = std::make_shared<Label>(value, TypographyToken::PropertyValue);
    auto row = std::make_shared<PropertyRowLayout>(label, valueLabel);
    AddChildToContainer(container, row);
}

std::string FormFormatFloat(float value) {
    if (std::abs(value - std::round(value)) < 1e-4f) {
        return std::to_string(static_cast<int>(std::round(value)));
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.4g", static_cast<double>(value));
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
