#include "WindEffects/Editor/EditorSDK.h"
#include "WindEffects/Editor/UI/Widgets/OutputLogWidget.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"

namespace we::programs::editor {
using namespace ::we::runtime::kindui;
using ::we::editor::panels::Panel;
using ::we::editor::panels::PanelBuilder;
using ::we::editor::panels::OutputLogWidget;
using ::we::editor::docking::DockZone;

std::shared_ptr<Panel> CreateOutputLogPanel() {
    auto outputWidget = std::make_shared<OutputLogWidget>();

    return PanelBuilder("Output Log")
        .TabIcon(Icons::OutputLogName)
        .ToolbarBox([&](Row& toolbar) {
            toolbar.Padding(Margin{6.0f, 4.0f, 6.0f, 4.0f});

            auto searchBox = std::make_shared<TextBox>("", [outputWidget](const std::string& text) {
                outputWidget->SetSearchQuery(text);
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
