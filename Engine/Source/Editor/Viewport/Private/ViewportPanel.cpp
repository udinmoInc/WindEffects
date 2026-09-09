// ==============================================================================
// WindEffects — Viewport — ViewportPanel
// Internal implementation for the Viewport module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/EditorSDK.h"
#include "ViewportToolbar.h"
#include "ViewportToolbarState.h"
#include "KindUI/Widgets/Label.h"

// Re-compiled for PanelBuilder API updates
namespace we::programs::editor {
using ::we::runtime::kindui::panels::Panel;
using ::we::runtime::kindui::panels::PanelBuilder;
using ::we::editor::docking::DockZone;
using ::we::runtime::kindui::Label;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

std::shared_ptr<Panel> CreateViewportPanel() {
    auto toolbar = CreateViewportToolbar();

    return PanelBuilder("Viewport")
        .TabIcon(WindIcons::Viewport16)
        .Transparent()
        .WithCloseButton()
        .Toolbar(toolbar)
        .Content(std::make_shared<Label>(""));
}

REGISTER_UI_PANEL(Viewport,
    WE_PANEL(Viewport).Title("Viewport").Icon("viewport").Zone(DockZone::Center).WindowMenu("Viewport").SortOrder(1),
    CreateViewportPanel)

} // namespace we::programs::editor
