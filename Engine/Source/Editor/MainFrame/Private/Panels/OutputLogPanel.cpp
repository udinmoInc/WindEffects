// ==============================================================================
// WindEffects — MainFrame — OutputLogPanel
// Internal implementation for the MainFrame module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/EditorSDK.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "Widgets/OutputLogWidget.h"
#include "KindUI/EditorWidgets.h"

namespace we::programs::editor {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;
using namespace ::we::runtime::kindui;
using ::we::runtime::kindui::panels::Panel;
using ::we::runtime::kindui::panels::PanelBuilder;
using ::we::editor::panels::OutputLogWidget;
using ::we::editor::docking::DockZone;

std::shared_ptr<Panel> CreateOutputLogPanel() {
    auto outputWidget = std::make_shared<OutputLogWidget>();

    return we::editor::dsl::Panel("Output Log", [&](we::editor::dsl::PanelContext& p) {
        p.TabIcon(WindIcons::SettingsV224)
         .WithCloseButton([]() {
             EditorWorkspaceController::Get().SetPanelVisible("OutputLog", false);
         })
         .Content(outputWidget);

        p.Toolbar([&](we::editor::dsl::ToolbarContext& t) {
            t.Search("Filter output log...", [outputWidget](const std::string& text) {
                outputWidget->SetSearchQuery(text);
            });
            t.Button("All", [outputWidget]() { outputWidget->SetMinimumLevel(we::Logger::Level::Trace); });
            t.Button("Info", [outputWidget]() { outputWidget->SetMinimumLevel(we::Logger::Level::Info); });
            t.Button("Warning", [outputWidget]() { outputWidget->SetMinimumLevel(we::Logger::Level::Warning); });
            t.Button("Error", [outputWidget]() { outputWidget->SetMinimumLevel(we::Logger::Level::Error); });
            t.Button("Clear", [outputWidget]() { outputWidget->Clear(); });
            t.Button("Pause", [outputWidget]() { outputWidget->SetPaused(!outputWidget->IsPaused()); });
        });
    });
}

REGISTER_UI_PANEL(OutputLog,
    WE_PANEL(OutputLog).Title("Output Log").Icon("output-log").Zone(DockZone::Bottom).SortOrder(5),
    CreateOutputLogPanel)

}
