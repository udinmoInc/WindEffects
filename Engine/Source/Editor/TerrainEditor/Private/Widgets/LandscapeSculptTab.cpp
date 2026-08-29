#include "LandscapeWorkspaceInternal.h"
#include "KindUI/Widgets/Components.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"

#include "LandscapePanelChrome.h"
#include "ViewportEdit/ViewportEditSession.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace we::editor::terrain {
namespace {

using namespace we::runtime::kindui;

static void AddSectionTitle(const std::shared_ptr<Column>& layout, std::string_view title) {
    auto header = MakeSectionHeader(std::string(title));
    layout->AddChild(header);
}

static void AddChipRow(
    const std::shared_ptr<Column>& layout,
    const std::vector<std::tuple<std::string, const char*, bool, std::function<void()>>>& chips)
{
    const size_t maxPerRow = 4;
    std::shared_ptr<Row> currentRow = nullptr;
    size_t countInRow = 0;

    for (const auto& [label, icon, selected, onClick] : chips) {
        if (!currentRow || countInRow >= maxPerRow) {
            currentRow = MakeRow();
            currentRow->Gap(we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space1));
            currentRow->SetFlexShrink(0.0f);
            layout->AddChild(currentRow);
            countInRow = 0;
        }
        auto btn = MakeSecondaryAction(label, icon ? icon : "");
        btn->SetFlexGrow(1.0f);
        btn->SetFlexShrink(1.0f);
        btn->SetMinWidth(ChipButtonMinWidth());
        btn->SetOnClicked(onClick);
        currentRow->AddChild(btn);
        ++countInRow;
    }
}

static void AddField(
    const std::shared_ptr<Column>& layout,
    std::string label,
    std::string value,
    std::function<void(std::string_view)> onCommit)
{
    AddFormField(layout, label, value, std::move(onCommit));
}

static void AddToggle(
    const std::shared_ptr<Column>& layout,
    std::string label,
    bool on,
    std::function<void()> onClick)
{
    auto btn = MakeSecondaryAction(label + (on ? " : ON" : " : OFF"));
    btn->SetOnClicked(onClick);
    layout->AddChild(btn);
}



std::string FormatFloat(float v) { char buf[64]; std::snprintf(buf, sizeof(buf), "%.3g", static_cast<double>(v)); return buf; }
std::string FormatInt(int v) { return std::to_string(v); }
float ParseFloat(std::string_view s, float fallback) { try { return std::stof(std::string(s)); } catch (...) { return fallback; } }
int ParseInt(std::string_view s, int fallback) { try { return std::stoi(std::string(s)); } catch (...) { return fallback; } }

} // namespace


static void ActivateOp(ILandscapeEditor& editor, runtime_terrain::TerrainBrushOp op, viewportedit::ViewportToolId tool) {
    editor.SetBrushOp(op);
    if (auto* ve = viewportedit::ViewportEditSession::Editor()) {
        ve->SetActiveMode("Landscape");
        ve->SetActiveTool(tool);
    }
}

void BuildSculptTab(const std::shared_ptr<we::runtime::kindui::Column>& layout, ILandscapeEditor& editor) {
    const auto op = editor.BrushSettings().op;

    AddSectionTitle(layout, "Basic");
    AddChipRow(layout, {
        {"Raise", "plus", op == runtime_terrain::TerrainBrushOp::Raise,
            [&]() { ActivateOp(editor, runtime_terrain::TerrainBrushOp::Raise,
                viewportedit::ViewportToolId::LandscapeSculpt); }},
        {"Lower", "minus", op == runtime_terrain::TerrainBrushOp::Lower,
            [&]() { ActivateOp(editor, runtime_terrain::TerrainBrushOp::Lower,
                viewportedit::ViewportToolId::LandscapeSculpt); }},
        {"Smooth", "refresh", op == runtime_terrain::TerrainBrushOp::Smooth,
            [&]() { ActivateOp(editor, runtime_terrain::TerrainBrushOp::Smooth,
                viewportedit::ViewportToolId::LandscapeSmooth); }},
        {"Flatten", "plane", op == runtime_terrain::TerrainBrushOp::Flatten,
            [&]() { ActivateOp(editor, runtime_terrain::TerrainBrushOp::Flatten,
                viewportedit::ViewportToolId::LandscapeFlatten); }},
    });

    AddSectionTitle(layout, "Advanced");
    AddChipRow(layout, {
        {"Noise", "star", op == runtime_terrain::TerrainBrushOp::Noise,
            [&]() { ActivateOp(editor, runtime_terrain::TerrainBrushOp::Noise,
                viewportedit::ViewportToolId::LandscapeNoise); }},
        {"Ramp", "trending-up", op == runtime_terrain::TerrainBrushOp::Ramp,
            [&]() { ActivateOp(editor, runtime_terrain::TerrainBrushOp::Ramp,
                viewportedit::ViewportToolId::LandscapeSculpt); }},
        {"Terrace", "layers", op == runtime_terrain::TerrainBrushOp::Terrace,
            [&]() { ActivateOp(editor, runtime_terrain::TerrainBrushOp::Terrace,
                viewportedit::ViewportToolId::LandscapeSculpt); }},
    });

    AddSectionTitle(layout, "Erosion");
    AddChipRow(layout, {
        {"Hydraulic", "droplet", op == runtime_terrain::TerrainBrushOp::HydraulicErosion,
            [&]() { ActivateOp(editor, runtime_terrain::TerrainBrushOp::HydraulicErosion,
                viewportedit::ViewportToolId::LandscapeSculpt); }},
        {"Thermal", "thermometer", op == runtime_terrain::TerrainBrushOp::ThermalErosion,
            [&]() { ActivateOp(editor, runtime_terrain::TerrainBrushOp::ThermalErosion,
                viewportedit::ViewportToolId::LandscapeSculpt); }},
    });

    AddSectionTitle(layout, "Brush Settings");
    const auto& brush = editor.BrushSettings();
    auto& ui = editor.BrushUi();
    AddField(layout, "Radius", FormatFloat(brush.radius), [&](std::string_view v) {
        editor.SetBrushRadius(ParseFloat(v, editor.BrushSettings().radius));
    });
    AddField(layout, "Strength", FormatFloat(brush.strength), [&](std::string_view v) {
        editor.SetBrushStrength(ParseFloat(v, editor.BrushSettings().strength));
    });
    AddField(layout, "Falloff", FormatFloat(brush.falloff), [&](std::string_view v) {
        editor.SetBrushFalloff(ParseFloat(v, editor.BrushSettings().falloff));
    });

    AddSectionTitle(layout, "Brush Shape");
    AddChipRow(layout, {
        {"Circle", "circle", ui.shape == LandscapeBrushShape::Circle,
            [&]() { editor.BrushUi().shape = LandscapeBrushShape::Circle; }},
        {"Square", "square", ui.shape == LandscapeBrushShape::Square,
            [&]() { editor.BrushUi().shape = LandscapeBrushShape::Square; }},
    });

    AddField(layout, "Alpha", ui.alphaPath.empty() ? "(none)" : ui.alphaPath,
        [&](std::string_view v) {
            if (v.empty() || v == "(none)") {
                editor.ClearBrushAlpha();
            } else {
                editor.SetBrushAlphaPlaceholder(v);
            }
        });

    AddToggle(layout, "Invert", ui.invert, [&]() {
        editor.BrushUi().invert = !editor.BrushUi().invert;
    });
    AddToggle(layout, "Mirror", ui.mirror, [&]() {
        editor.BrushUi().mirror = !editor.BrushUi().mirror;
    });
    AddToggle(layout, "Brush Preview", ui.showPreview, [&]() {
        editor.BrushUi().showPreview = !editor.BrushUi().showPreview;
        editor.BrushPreview().visible = editor.BrushUi().showPreview;
    });
    AddToggle(layout, "Brush Cursor", ui.showCursor, [&]() {
        editor.BrushUi().showCursor = !editor.BrushUi().showCursor;
    });
}

} // namespace we::editor::terrain
