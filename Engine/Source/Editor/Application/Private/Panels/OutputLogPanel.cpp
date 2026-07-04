#include "EditorRegistry.hpp"
#include "Widgets/Panel.hpp"
#include "Widgets/OutputLogWidget.hpp"
#include "Widgets/SearchBox.hpp"
#include "Widgets/Button.hpp"
#include "Layout/Box.hpp"
#include "Core/Logger.hpp"

namespace we::programs::editor {

std::shared_ptr<we::UI::Panel> CreateOutputLogPanel() {
    auto panel = std::make_shared<we::UI::Panel>("Output Log");
    panel->SetHeaderHeight(30.0f);

    auto toolbar = std::make_shared<we::UI::HorizontalBox>();
    toolbar->SetPadding(we::UI::Margin{ 6.0f, 4.0f, 6.0f, 4.0f });

    auto outputWidget = std::make_shared<we::UI::OutputLogWidget>();

    auto searchBox = std::make_shared<we::UI::SearchBox>();
    searchBox->SetPlaceholder("Search logs...");
    searchBox->SetFillWidth(true);
    searchBox->SetOnTextChanged([outputWidget](const std::string& text) {
        outputWidget->SetSearchQuery(text);
    });

    auto clearButton = std::make_shared<we::UI::Button>("Clear");
    clearButton->SetOnClick([outputWidget]() { outputWidget->Clear(); });

    auto pauseButton = std::make_shared<we::UI::Button>("Pause");
    pauseButton->SetOnClick([outputWidget, pauseButton]() {
        const bool paused = !outputWidget->IsPaused();
        outputWidget->SetPaused(paused);
        pauseButton->SetLabel(paused ? "Resume" : "Pause");
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
