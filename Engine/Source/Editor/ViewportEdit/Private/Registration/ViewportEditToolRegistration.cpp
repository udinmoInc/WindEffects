#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"
#include "ViewportEdit/ViewportEditSession.h"

namespace we::editor::viewportedit {
using ::we::runtime::kindui::kWindIconNone;
namespace WindIcons = ::we::runtime::kindui::WindIcons;

namespace {

void ActivateTool(ViewportToolId tool) {
    if (auto* editor = ViewportEditSession::Editor()) {
        editor->SetActiveTool(tool);
    }
}

} // namespace

// Overrides empty Select-mode stubs in ToolsPanel DefaultEditorModes.
REGISTER_EDITOR_TOOL(SelectEssentials, SelectTool, "Select", WindIcons::BoxSolid16, "Q", []() {
    ActivateTool(ViewportToolId::Select);
})
REGISTER_EDITOR_TOOL(SelectEssentials, MoveTool, "Move", WindIcons::AdjustHorizon16, "W", []() {
    ActivateTool(ViewportToolId::Move);
})
REGISTER_EDITOR_TOOL(SelectEssentials, RotateTool, "Rotate", WindIcons::RedoAlt16, "E", []() {
    ActivateTool(ViewportToolId::Rotate);
})
REGISTER_EDITOR_TOOL(SelectEssentials, ScaleTool, "Scale", WindIcons::ToolbarScaling16, "R", []() {
    ActivateTool(ViewportToolId::Scale);
})

} // namespace we::editor::viewportedit
