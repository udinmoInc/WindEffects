#include "LandscapeWorkspaceInternal.h"
#include "LandscapePanelChrome.h"
#include "ViewportEdit/ViewportEditSession.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace we::editor::terrain {
namespace {

namespace Chrome = LandscapePanelChrome;

void AddSectionTitle(LandscapeLayoutBuilder& layout, std::string_view title) {
    LandscapeHitTarget t{};
    t.kind = LandscapeHitKind::None;
    t.geometry = {layout.contentX, layout.cursorY, layout.contentW, Chrome::RowHeight()};
    t.label = std::string(title);
    t.id = -1;
    layout.targets.push_back(std::move(t));
    layout.Advance(Chrome::RowHeight() + 4.f);
}

void AddChipRow(
    LandscapeLayoutBuilder& layout,
    const std::vector<std::tuple<std::string, const char*, bool, std::function<void()>>>& chips)
{
    const float gap = 6.f;
    const float chipH = Chrome::ChipHeight();
    float x = layout.contentX;
    float y = layout.cursorY;
    for (const auto& [label, icon, selected, onClick] : chips) {
        const float chipW = std::min(110.f, (layout.contentW - gap) * 0.5f);
        if (x + chipW > layout.contentX + layout.contentW + 0.5f) {
            x = layout.contentX;
            y += chipH + gap;
        }
        LandscapeHitTarget t{};
        t.kind = LandscapeHitKind::Chip;
        t.geometry = {x, y, chipW, chipH};
        t.label = label;
        t.iconHook = icon;
        t.selected = selected;
        t.onClick = onClick;
        layout.targets.push_back(std::move(t));
        x += chipW + gap;
    }
    layout.cursorY = y + chipH + Chrome::SectionGap();
    layout.contentHeight = std::max(layout.contentHeight, layout.cursorY);
}

void AddField(
    LandscapeLayoutBuilder& layout,
    std::string label,
    std::string value,
    std::function<void(std::string_view)> onCommit)
{
    const float h = Chrome::RowHeight();
    LandscapeHitTarget t{};
    t.kind = LandscapeHitKind::Field;
    t.geometry = {
        layout.contentX + Chrome::LabelColumnWidth() + 8.f,
        layout.cursorY,
        layout.contentW - Chrome::LabelColumnWidth() - 8.f,
        h};
    t.label = std::move(label);
    t.value = std::move(value);
    t.editable = true;
    t.onTextCommit = std::move(onCommit);
    layout.targets.push_back(std::move(t));
    layout.Advance(h + 4.f);
}

void AddToggle(
    LandscapeLayoutBuilder& layout,
    std::string label,
    bool on,
    std::function<void()> onClick)
{
    LandscapeHitTarget t{};
    t.kind = LandscapeHitKind::Toggle;
    t.geometry = {layout.contentX, layout.cursorY, layout.contentW, Chrome::RowHeight()};
    t.label = std::move(label);
    t.toggled = on;
    t.onClick = std::move(onClick);
    layout.targets.push_back(std::move(t));
    layout.Advance(Chrome::RowHeight() + 4.f);
}

std::string FormatFloat(float v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.3g", static_cast<double>(v));
    return buf;
}

float ParseFloat(std::string_view s, float fallback) {
    try {
        return std::stof(std::string(s));
    } catch (...) {
        return fallback;
    }
}

void ActivateOp(ILandscapeEditor& editor, runtime_terrain::TerrainBrushOp op,
    viewportedit::ViewportToolId tool)
{
    editor.InstallViewportMode();
    editor.SetBrushOp(op);
    (void)editor.EnsureLandscape();
    if (auto* ve = viewportedit::ViewportEditSession::Editor()) {
        ve->SetActiveMode("Landscape");
        ve->SetActiveTool(tool);
    }
}

} // namespace

void BuildSculptTab(LandscapeLayoutBuilder& layout, ILandscapeEditor& editor) {
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
