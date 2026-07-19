#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"
#include "ViewportEdit/ViewportEditSession.h"

namespace we::editor::viewportedit {
namespace {

void ActivateTool(ViewportToolId tool) {
    if (auto* editor = ViewportEditSession::Editor()) {
        editor->SetActiveTool(tool);
    }
}

} // namespace

// Overrides empty Select-mode stubs in ToolsPanel DefaultEditorModes.
REGISTER_EDITOR_TOOL(SelectEssentials, SelectTool, "Select", "cursor", "Q", []() {
    ActivateTool(ViewportToolId::Select);
})
REGISTER_EDITOR_TOOL(SelectEssentials, MoveTool, "Move", "move", "W", []() {
    ActivateTool(ViewportToolId::Move);
})
REGISTER_EDITOR_TOOL(SelectEssentials, RotateTool, "Rotate", "rotate", "E", []() {
    ActivateTool(ViewportToolId::Rotate);
})
REGISTER_EDITOR_TOOL(SelectEssentials, ScaleTool, "Scale", "scale", "R", []() {
    ActivateTool(ViewportToolId::Scale);
})

} // namespace we::editor::viewportedit
