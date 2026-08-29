#include "LandscapeWorkspaceInternal.h"
#include "LandscapeFormLayout.h"
#include "ViewportEdit/ViewportEditSession.h"

namespace we::editor::terrain {

static void ActivateOp(ILandscapeEditor& editor, runtime_terrain::TerrainBrushOp op, viewportedit::ViewportToolId tool) {
    editor.SetBrushOp(op);
    if (auto* ve = viewportedit::ViewportEditSession::Editor()) {
        ve->SetActiveMode("Landscape");
        ve->SetActiveTool(tool);
    }
}

void BuildSculptTab(const std::shared_ptr<we::runtime::kindui::Column>& layout, ILandscapeEditor& editor) {
    const auto op = editor.BrushSettings().op;

    AddFormSectionTitle(layout, "Basic");
    AddFormChipRow(layout, {
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

    AddFormSectionTitle(layout, "Advanced");
    AddFormChipRow(layout, {
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

    AddFormSectionTitle(layout, "Erosion");
    AddFormChipRow(layout, {
        {"Hydraulic", "droplet", op == runtime_terrain::TerrainBrushOp::HydraulicErosion,
            [&]() { ActivateOp(editor, runtime_terrain::TerrainBrushOp::HydraulicErosion,
                viewportedit::ViewportToolId::LandscapeSculpt); }},
        {"Thermal", "thermometer", op == runtime_terrain::TerrainBrushOp::ThermalErosion,
            [&]() { ActivateOp(editor, runtime_terrain::TerrainBrushOp::ThermalErosion,
                viewportedit::ViewportToolId::LandscapeSculpt); }},
    });

    AddFormSectionTitle(layout, "Brush Settings");
    const auto& brush = editor.BrushSettings();
    auto& ui = editor.BrushUi();
    AddFormField(layout, "Radius", FormFormatFloat(brush.radius), [&](std::string_view v) {
        editor.SetBrushRadius(FormParseFloat(v, editor.BrushSettings().radius));
    });
    AddFormField(layout, "Strength", FormFormatFloat(brush.strength), [&](std::string_view v) {
        editor.SetBrushStrength(FormParseFloat(v, editor.BrushSettings().strength));
    });
    AddFormField(layout, "Falloff", FormFormatFloat(brush.falloff), [&](std::string_view v) {
        editor.SetBrushFalloff(FormParseFloat(v, editor.BrushSettings().falloff));
    });

    AddFormSectionTitle(layout, "Brush Shape");
    AddFormChipRow(layout, {
        {"Circle", "circle", ui.shape == LandscapeBrushShape::Circle,
            [&]() { editor.BrushUi().shape = LandscapeBrushShape::Circle; }},
        {"Square", "square", ui.shape == LandscapeBrushShape::Square,
            [&]() { editor.BrushUi().shape = LandscapeBrushShape::Square; }},
    });

    AddFormField(layout, "Alpha", ui.alphaPath.empty() ? "(none)" : ui.alphaPath,
        [&](std::string_view v) {
            if (v.empty() || v == "(none)") {
                editor.ClearBrushAlpha();
            } else {
                editor.SetBrushAlphaPlaceholder(v);
            }
        });

    AddFormToggle(layout, "Invert", ui.invert, [&]() {
        editor.BrushUi().invert = !editor.BrushUi().invert;
    });
    AddFormToggle(layout, "Mirror", ui.mirror, [&]() {
        editor.BrushUi().mirror = !editor.BrushUi().mirror;
    });
    AddFormToggle(layout, "Brush Preview", ui.showPreview, [&]() {
        editor.BrushUi().showPreview = !editor.BrushUi().showPreview;
        editor.BrushPreview().visible = editor.BrushUi().showPreview;
    });
    AddFormToggle(layout, "Brush Cursor", ui.showCursor, [&]() {
        editor.BrushUi().showCursor = !editor.BrushUi().showCursor;
    });
}

} // namespace we::editor::terrain
