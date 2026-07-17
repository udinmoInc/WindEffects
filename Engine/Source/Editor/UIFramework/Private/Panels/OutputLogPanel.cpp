#include "WindEffects/Editor/EditorSDK.h"
#include "Widgets/OutputLogWidget.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Widgets/Button.h"

namespace we::programs::editor {
using namespace we::runtime::kindui;

std::shared_ptr<Panel> CreateOutputLogPanel() {
    auto outputWidget = std::make_shared<OutputLogWidget>();

    return PanelBuilder("Output Log")
        .TabIcon(Icons::OutputLogName)
        .ToolbarBox([&](HorizontalBox& toolbar) {
            toolbar.SetPadding(Margin{6.0f, 4.0f, 6.0f, 4.0f});

            auto searchBox = std::make_shared<TextBox>("", [outputWidget](const std::string& text) {
                outputWidget->SetSearchQuery(text);
            });

            auto clearButton = std::make_shared<Button>("Clear", [outputWidget]() {
                outputWidget->Clear();
            });

            auto pauseButton = std::make_shared<Button>("Pause");
            pauseButton->SetOnClicked([outputWidget, pauseButton]() {
                const bool paused = !outputWidget->IsPaused();
                outputWidget->SetPaused(paused);
                pauseButton->SetText(paused ? "Resume" : "Pause");
            });

            toolbar.AddChild(searchBox);
            toolbar.AddChild(clearButton);
            toolbar.AddChild(pauseButton);
        })
        .Content(outputWidget);
}

REGISTER_UI_PANEL(OutputLog,
    WE_PANEL(OutputLog).Title("Output Log").Icon("output-log").Zone(DockZone::Floating).Hidden().SortOrder(5),
    CreateOutputLogPanel)

} // namespace we::programs::editor
