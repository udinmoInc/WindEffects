// ==============================================================================
// WindEffects — ToolsPanel — ToolsPanelPanel
// Internal implementation for the ToolsPanel module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/EditorSDK.h"
#include "WindEffects/Editor/UI/Shell/EditorModeController.h"
#include "KindUI/EditorWidgets.h"
#include "Widgets/ToolsPanel.h"

namespace we::programs::editor {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

using namespace ::we::runtime::kindui;
using ::we::runtime::kindui::panels::Panel;
using ::we::runtime::kindui::panels::PanelBuilder;
using ::we::editor::docking::DockZone;
using ::we::editor::shell::EditorModeController;
using ::we::editor::toolspanel::EditorToolsRegistry;

namespace {

void SyncPanelTitle(const std::shared_ptr<Panel>& panel) {
    if (!panel) {
        return;
    }

    const std::string activeModeId = EditorModeController::Get().GetActiveModeId();
    const auto* mode = EditorToolsRegistry::Get().FindMode(activeModeId);
    if (mode && !mode->opensToolDrawerByDefault) {
        if (const auto* actors = EditorToolsRegistry::Get().FindMode("Actors")) {
            if (actors->customContent) {
                panel->SetTitle(actors->label);
                panel->SetTabIcon(actors->icon);
                return;
            }
        }
    }

    if (!mode) {
        panel->SetTitle("Actors");
        panel->SetTabIcon(kWindIconNone);
        return;
    }

    panel->SetTitle(mode->label);
    panel->SetTabIcon(mode->icon);
}

}

std::shared_ptr<Panel> CreateToolsPanel() {
    auto toolsContent = std::make_shared<ToolsPanel>();
    toolsContent->InitializeFromRegistry(toolsContent);

    auto panel = we::editor::dsl::Panel("Creation Palette", [&](we::editor::dsl::PanelContext& p) {
        p.TabIcon(WindIcons::SettingsV224)
         .WithCloseButton([]() {
             EditorModeController::Get().SetDrawerVisible(false);
         })
         .Content(toolsContent);
    });

    SyncPanelTitle(panel);
    std::weak_ptr<Panel> weakPanel = panel;
    std::weak_ptr<ToolsPanel> weakTools = toolsContent;
    EditorModeController::Get().AddModeChangedListener([weakPanel, weakTools](const std::string&) {
        auto panel = weakPanel.lock();
        auto tools = weakTools.lock();
        if (!panel || !tools) {
            return;
        }
        SyncPanelTitle(panel);
        tools->OnModeChanged();
    });

    panel->SetVisible(EditorModeController::Get().IsDrawerVisible());
    return panel;
}

REGISTER_UI_PANEL(Tools,
    WE_PANEL(Tools).Title("Creation Palette").Icon("tools-panel").Zone(DockZone::Left).SortOrder(0),
    CreateToolsPanel)

}
