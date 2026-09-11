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
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"

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

    return PanelBuilder("Output Log")
        .TabIcon(WindIcons::Console16)
        .WithCloseButton([]() {
            EditorWorkspaceController::Get().SetPanelVisible("OutputLog", false);
        })
        .ToolbarBox([&](Row& toolbar) {
            toolbar.Padding(Margin{6.0f, 4.0f, 6.0f, 4.0f});
            toolbar.Gap(6.0f);

            auto searchBox = std::make_shared<TextBox>("Filter output log...", [outputWidget](const std::string& text) {
                outputWidget->SetSearchQuery(text);
            });

            auto filterAll = MakeSecondaryAction("All");
            filterAll->SetOnClicked([outputWidget]() {
                outputWidget->SetMinimumLevel(we::Logger::Level::Trace);
            });

            auto filterInfo = MakeSecondaryAction("Info");
            filterInfo->SetOnClicked([outputWidget]() {
                outputWidget->SetMinimumLevel(we::Logger::Level::Info);
            });

            auto filterWarning = MakeSecondaryAction("Warning");
            filterWarning->SetOnClicked([outputWidget]() {
                outputWidget->SetMinimumLevel(we::Logger::Level::Warning);
            });

            auto filterError = MakeSecondaryAction("Error");
            filterError->SetOnClicked([outputWidget]() {
                outputWidget->SetMinimumLevel(we::Logger::Level::Error);
            });

            auto clearButton = MakeSecondaryAction("Clear");
            clearButton->SetOnClicked([outputWidget]() {
                outputWidget->Clear();
            });

            auto pauseButton = MakeSecondaryAction("Pause");
            pauseButton->SetOnClicked([outputWidget, pauseButton]() {
                const bool paused = !outputWidget->IsPaused();
                outputWidget->SetPaused(paused);
                pauseButton->SetLabel(paused ? "Resume" : "Pause");
            });

            auto autoScrollBtn = MakeSecondaryAction("Auto-Scroll");
            autoScrollBtn->SetOnClicked([outputWidget, autoScrollBtn]() {
                const bool scroll = !outputWidget->IsAutoScroll();
                outputWidget->SetAutoScroll(scroll);
                autoScrollBtn->SetLabel(scroll ? "Auto-Scroll" : "Manual Scroll");
            });

            toolbar.AddChild(searchBox);
            toolbar.AddChild(filterAll);
            toolbar.AddChild(filterInfo);
            toolbar.AddChild(filterWarning);
            toolbar.AddChild(filterError);
            toolbar.AddChild(clearButton);
            toolbar.AddChild(pauseButton);
            toolbar.AddChild(autoScrollBtn);
        })
        .Footer([&]() {
            auto footerRow = std::make_shared<Row>();
            footerRow->Padding(Margin{6.0f, 4.0f, 6.0f, 4.0f});
            footerRow->Gap(6.0f);

            auto cmdInput = std::make_shared<TextBox>("Console Commands...", [](const std::string& cmd) {
                if (!cmd.empty()) {
                    we::Logger::Log(we::Logger::Level::Info, "Cmd", "> " + cmd);
                }
            });

            auto executeBtn = MakePrimaryAction("Execute");
            executeBtn->SetOnClicked([cmdInput]() {
                const std::string cmd = cmdInput->GetText();
                if (!cmd.empty()) {
                    we::Logger::Log(we::Logger::Level::Info, "Cmd", "> " + cmd);
                    cmdInput->SetText("");
                }
            });

            footerRow->AddChild(cmdInput);
            footerRow->AddChild(executeBtn);
            return footerRow;
        }())
        .Content(outputWidget);
}

REGISTER_UI_PANEL(OutputLog,
    WE_PANEL(OutputLog).Title("Output Log").Icon("output-log").Zone(DockZone::Bottom).SortOrder(5),
    CreateOutputLogPanel)

} // namespace we::programs::editor
