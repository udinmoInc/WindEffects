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

void AddButton(
    LandscapeLayoutBuilder& layout,
    std::string label,
    std::function<void()> onClick,
    bool danger = false)
{
    LandscapeHitTarget t{};
    t.kind = LandscapeHitKind::Button;
    t.geometry = {layout.contentX, layout.cursorY, layout.contentW, Chrome::RowHeight() + 4.f};
    t.label = std::move(label);
    t.onClick = std::move(onClick);
    t.isDanger = danger;
    layout.targets.push_back(std::move(t));
    layout.Advance(Chrome::RowHeight() + 10.f);
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

} // namespace

void BuildPaintTab(LandscapeLayoutBuilder& layout, ILandscapeEditor& editor) {
    AddSectionTitle(layout, "Landscape Layers");

    const int count = editor.GetLayerCount();
    for (int i = 0; i < count; ++i) {
        LandscapeHitTarget row{};
        row.kind = LandscapeHitKind::LayerRow;
        row.geometry = {layout.contentX, layout.cursorY, layout.contentW, Chrome::RowHeight() + 2.f};
        row.label = editor.GetLayerName(i);
        row.id = i;
        row.selected = (editor.ActivePaintLayer() == i);
        row.onClick = [&editor, i]() {
            editor.SetPaintLayer(i);
            editor.SetBrushOp(runtime_terrain::TerrainBrushOp::Paint);
            if (auto* ve = viewportedit::ViewportEditSession::Editor()) {
                ve->SetActiveMode("Landscape");
                ve->SetActiveTool(viewportedit::ViewportToolId::LandscapePaint);
            }
        };
        layout.targets.push_back(std::move(row));
        layout.Advance(Chrome::RowHeight() + 6.f);

        if (editor.ActivePaintLayer() == i) {
            AddField(layout, "Rename", editor.GetLayerName(i), [&editor, i](std::string_view v) {
                editor.SetLayerName(i, v);
            });
            AddField(layout, "Material",
                editor.GetLayerMaterialPath(i).empty() ? "None" : editor.GetLayerMaterialPath(i),
                [&editor, i](std::string_view v) {
                    editor.SetLayerMaterialPath(i, (v == "None") ? "" : v);
                });
            AddField(layout, "Layer Weight", FormatFloat(editor.BrushSettings().strength),
                [&](std::string_view v) {
                    editor.SetBrushStrength(ParseFloat(v, editor.BrushSettings().strength));
                });
            AddField(layout, "Layer Blend", FormatFloat(editor.BrushSettings().falloff),
                [&](std::string_view v) {
                    editor.SetBrushFalloff(ParseFloat(v, editor.BrushSettings().falloff));
                });
        }
    }

    AddButton(layout, "Create Layer", [&]() {
        (void)editor.AddLayer("Layer");
    });
    AddButton(layout, "Delete Layer", [&]() {
        (void)editor.RemoveLayer(editor.ActivePaintLayer());
    }, true);

    AddSectionTitle(layout, "Brush Settings");
    AddField(layout, "Radius", FormatFloat(editor.BrushSettings().radius), [&](std::string_view v) {
        editor.SetBrushRadius(ParseFloat(v, editor.BrushSettings().radius));
    });
    AddField(layout, "Strength", FormatFloat(editor.BrushSettings().strength), [&](std::string_view v) {
        editor.SetBrushStrength(ParseFloat(v, editor.BrushSettings().strength));
    });
    AddField(layout, "Falloff", FormatFloat(editor.BrushSettings().falloff), [&](std::string_view v) {
        editor.SetBrushFalloff(ParseFloat(v, editor.BrushSettings().falloff));
    });
    AddToggle(layout, "Brush Preview", editor.BrushUi().showPreview, [&]() {
        editor.BrushUi().showPreview = !editor.BrushUi().showPreview;
    });
}

} // namespace we::editor::terrain
