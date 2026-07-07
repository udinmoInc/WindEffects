#include "EditorRegistry.h"
#include "Widgets/Panel.h"
#include "Widgets/OutputLogWidget.h"
#include "Widgets/TextBox.h"
#include "Widgets/Button.h"
#include "Layout/Box.h"

namespace we::programs::editor {

std::shared_ptr<we::UI::Panel> CreateOutputLogPanel() {
    auto panel = std::make_shared<we::UI::Panel>("Output Log");
    panel->SetHeaderHeight(30.0f);

    auto toolbar = std::make_shared<we::UI::HorizontalBox>();
    toolbar->SetPadding(we::UI::Margin{ 6.0f, 4.0f, 6.0f, 4.0f });

    auto outputWidget = std::make_shared<we::UI::OutputLogWidget>();

    auto searchBox = std::make_shared<we::UI::TextBox>("", [outputWidget](const std::string& text) {
        outputWidget->SetSearchQuery(text);
    });

    auto clearButton = std::make_shared<we::UI::Button>("Clear", [outputWidget]() {
        outputWidget->Clear();
    });

    auto pauseButton = std::make_shared<we::UI::Button>("Pause");
    pauseButton->SetOnClicked([outputWidget, pauseButton]() {
        const bool paused = !outputWidget->IsPaused();
        outputWidget->SetPaused(paused);
        pauseButton->SetText(paused ? "Resume" : "Pause");
    });

    toolbar->AddChild(searchBox);
    toolbar->AddChild(clearButton);
    toolbar->AddChild(pauseButton);
    panel->SetToolbar(toolbar);
    panel->SetContent(outputWidget);
    return panel;
}

REGISTER_EDITOR_PANEL(OutputLog, CreateOutputLogPanel)

} // namespace we::programs::editor
