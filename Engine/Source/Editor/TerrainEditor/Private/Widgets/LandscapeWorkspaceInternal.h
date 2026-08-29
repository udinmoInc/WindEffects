#pragma once

#include "TerrainEditor/ILandscapeEditor.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include <memory>
#include <string>
#include <functional>

namespace we::editor::terrain {

void BuildCreateTab(const std::shared_ptr<we::runtime::kindui::Column>& layout, ILandscapeEditor& editor);
void BuildSculptTab(const std::shared_ptr<we::runtime::kindui::Column>& layout, ILandscapeEditor& editor);
void BuildPaintTab(const std::shared_ptr<we::runtime::kindui::Column>& layout, ILandscapeEditor& editor);
void BuildManageTab(
    const std::shared_ptr<we::runtime::kindui::Column>& layout,
    ILandscapeEditor& editor,
    std::string& importPath,
    std::string& exportPath,
    int& resizeX,
    int& resizeY);

inline void AddFormField(
    const std::shared_ptr<we::runtime::kindui::Column>& layout,
    const std::string& label,
    const std::string& value,
    std::function<void(std::string_view)> onCommit)
{
    auto input = std::make_shared<we::runtime::kindui::SearchBoxControl>("");
    input->SetText(value);
    input->SetOnChanged([onCommit](const std::string& v) { onCommit(v); });
    layout->AddChild(we::runtime::kindui::LayoutMetrics::MakeFormRow(label, input));
}

inline float ChipButtonMinWidth() {
    return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::PrimaryButtonHeight)
        + we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space2) * 2.0f;
}

} // namespace we::editor::terrain
