// ==============================================================================
// WindEffects — ToolsPanel — EditorModeSelector
// Public API surface for the ToolsPanel module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "ToolsPanel/Export.h"

#include <KindUI/EditorUI.h>
#include <memory>
#include <string>
namespace we::programs::editor {

/// Compact toolbar control: shows active editor mode and opens a registry-driven mode menu.
class TOOLSPANEL_API EditorModeSelector : public we::runtime::kindui::Widget {
public:
    EditorModeSelector();
    ~EditorModeSelector() override;

    EditorModeSelector(const EditorModeSelector&) = delete;
    EditorModeSelector& operator=(const EditorModeSelector&) = delete;

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override { (void)position; return true; }

    void Refresh();

    void InitializeCallbacks(const std::shared_ptr<EditorModeSelector>& self);

private:
    void OpenModeMenu();

    float m_HoverAnim = 0.0f;
    std::string m_Label;
    we::runtime::kindui::WindIconRef m_Icon = we::runtime::kindui::kWindIconNone;
};

} // namespace we::programs::editor
