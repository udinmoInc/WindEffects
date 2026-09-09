// ==============================================================================
// WindEffects — ViewportEdit — ViewportEditToolRegistration
// Internal implementation for the ViewportEdit module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
REGISTER_EDITOR_TOOL(SelectEssentials, SelectTool, "Select", WindIcons::ToolbarHand16, "Q", []() {
    ActivateTool(ViewportToolId::Select);
})
REGISTER_EDITOR_TOOL(SelectEssentials, MoveTool, "Move", WindIcons::MoveOutline16, "W", []() {
    ActivateTool(ViewportToolId::Move);
})
REGISTER_EDITOR_TOOL(SelectEssentials, RotateTool, "Rotate", WindIcons::ToolbarRotate16, "E", []() {
    ActivateTool(ViewportToolId::Rotate);
})
REGISTER_EDITOR_TOOL(SelectEssentials, ScaleTool, "Scale", WindIcons::ToolbarScaling16, "R", []() {
    ActivateTool(ViewportToolId::Scale);
})

} // namespace we::editor::viewportedit
