#include "LandscapeWorkspaceInternal.h"
#include "LandscapeFormLayout.h"

#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Widgets/Components.h"
#include "ViewportEdit/ViewportEditSession.h"

namespace we::editor::terrain {
namespace {

using namespace we::runtime::kindui;

void AddLayerRow(
    const std::shared_ptr<Column>& layout,
    const std::string& label,
    bool selected,
    bool danger,
    std::function<void()> onClick)
{
    auto row = MakeRow();
    row->Align(AlignItems::Center);
    row->Gap(ResolveMetric(MetricToken::Space1));

    std::shared_ptr<DesignButton> btn;
    if (danger) {
        btn = MakePrimaryAction(label);
    } else {
        btn = MakeSecondaryAction(label);
    }

    btn->SetSelected(selected);
    btn->SetOnClicked(std::move(onClick));
    row->AddChild(btn);
    layout->AddChild(row);
}

} // namespace

void BuildPaintTab(const std::shared_ptr<Column>& layout, ILandscapeEditor& editor) {
    AddFormSectionTitle(layout, "Landscape Layers");

    const int count = editor.GetLayerCount();
    for (int i = 0; i < count; ++i) {
        AddLayerRow(layout, editor.GetLayerName(i), editor.ActivePaintLayer() == i, false,
            [&editor, i]() {
                editor.SetPaintLayer(i);
                editor.SetBrushOp(runtime_terrain::TerrainBrushOp::Paint);
                if (auto* ve = viewportedit::ViewportEditSession::Editor()) {
                    ve->SetActiveMode("Landscape");
                    ve->SetActiveTool(viewportedit::ViewportToolId::LandscapePaint);
                }
            });

        if (editor.ActivePaintLayer() == i) {
            AddFormField(layout, "Rename", editor.GetLayerName(i), [&editor, i](std::string_view v) {
                editor.SetLayerName(i, v);
            });
            AddFormField(layout, "Material",
                editor.GetLayerMaterialPath(i).empty() ? "None" : editor.GetLayerMaterialPath(i),
                [&editor, i](std::string_view v) {
                    editor.SetLayerMaterialPath(i, (v == "None") ? "" : v);
                });
            AddFormField(layout, "Layer Weight", FormFormatFloat(editor.BrushSettings().strength),
                [&](std::string_view v) {
                    editor.SetBrushStrength(FormParseFloat(v, editor.BrushSettings().strength));
                });
            AddFormField(layout, "Layer Blend", FormFormatFloat(editor.BrushSettings().falloff),
                [&](std::string_view v) {
                    editor.SetBrushFalloff(FormParseFloat(v, editor.BrushSettings().falloff));
                });
        }
    }
}

} // namespace we::editor::terrain
