#include "LandscapeWorkspaceInternal.h"
#include "KindUI/Widgets/Components.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "TerrainEditor/ILandscapeEditor.h"
#include "ViewportEdit/ViewportEditSession.h"

using namespace we::runtime::kindui;

namespace we::editor::terrain {

namespace {

static void AddLayerRow(const std::shared_ptr<Column>& layout, int index, const std::string& label, bool selected, bool danger, std::function<void()> onClick, std::function<void()> onDelete) {
    auto row = MakeRow();
    row->Align(AlignItems::Center);
    row->Gap(4.0f);
    
    std::shared_ptr<DesignButton> btn;
    if(danger) btn = MakePrimaryAction(label);
    else btn = MakeSecondaryAction(label);
    
    btn->SetSelected(selected);
    btn->SetOnClicked(onClick);
    row->AddChild(btn);
    layout->AddChild(row);
}

std::string FormatFloat(float v) { char buf[64]; std::snprintf(buf, sizeof(buf), "%.3g", static_cast<double>(v)); return buf; }
std::string FormatInt(int v) { return std::to_string(v); }
float ParseFloat(std::string_view s, float fallback) { try { return std::stof(std::string(s)); } catch (...) { return fallback; } }
int ParseInt(std::string_view s, int fallback) { try { return std::stoi(std::string(s)); } catch (...) { return fallback; } }

static void AddField(const std::shared_ptr<Column>& layout, std::string label, std::string value, std::function<void(std::string_view)> onCommit) {
    auto row = MakeRow();
    row->Align(AlignItems::Center);
    row->Gap(8.0f);
    auto lbl = std::make_shared<Label>(label);
    lbl->SetMinWidth(120.0f);
    lbl->SetMaxWidth(120.0f);
    lbl->SetFlexShrink(0.0f);
    lbl->SetFlexGrow(0.0f);
    auto input = std::make_shared<SearchBoxControl>("");
    input->SetText(value);
    input->SetFlexGrow(1.0f);
    input->SetFlexShrink(1.0f);
    input->SetMinWidth(60.0f);
    input->SetOnChanged([onCommit](const std::string& v) { onCommit(v); });
    row->AddChild(lbl);
    row->AddChild(input);
    layout->AddChild(row);
}

static void AddSectionTitle(const std::shared_ptr<Column>& layout, std::string title) {
    auto lbl = std::make_shared<Label>(title);
    layout->AddChild(lbl);
}

} // namespace

void BuildPaintTab(const std::shared_ptr<Column>& layout, ILandscapeEditor& editor) {
    AddSectionTitle(layout, "Landscape Layers");

    const int count = editor.GetLayerCount();
    for (int i = 0; i < count; ++i) {
        AddLayerRow(layout, i, editor.GetLayerName(i), editor.ActivePaintLayer() == i, false,
            [&editor, i]() {
                editor.SetPaintLayer(i);
                editor.SetBrushOp(runtime_terrain::TerrainBrushOp::Paint);
                if (auto* ve = viewportedit::ViewportEditSession::Editor()) {
                    ve->SetActiveMode("Landscape");
                    ve->SetActiveTool(viewportedit::ViewportToolId::LandscapePaint);
                }
            },
            []() {}
        );

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
}

} // namespace we::editor::terrain

