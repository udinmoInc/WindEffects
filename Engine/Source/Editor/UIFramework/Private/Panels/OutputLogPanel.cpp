#include "WindEffects/Editor/UI/Extensions/ExtensionBootstrap.h"
#include "Widgets/Panel.h"
#include "Widgets/OutputLogWidget.h"
#include "Widgets/TextBox.h"
#include "Widgets/Button.h"
#include "Layout/Box.h"

namespace we::programs::editor {

std::shared_ptr<WindEffects::Editor::UI::Panel> CreateOutputLogPanel() {
    auto panel = std::make_shared<WindEffects::Editor::UI::Panel>("Output Log");
    panel->SetHeaderHeight(30.0f);
    panel->SetTabIcon(WindEffects::Editor::UI::Icons::ConsoleName);

    auto toolbar = std::make_shared<WindEffects::Editor::UI::HorizontalBox>();
    toolbar->SetPadding(WindEffects::Editor::UI::Margin{ 6.0f, 4.0f, 6.0f, 4.0f });

    auto outputWidget = std::make_shared<WindEffects::Editor::UI::OutputLogWidget>();

    auto searchBox = std::make_shared<WindEffects::Editor::UI::TextBox>("", [outputWidget](const std::string& text) {
        outputWidget->SetSearchQuery(text);
    });

    auto clearButton = std::make_shared<WindEffects::Editor::UI::Button>("Clear", [outputWidget]() {
        outputWidget->Clear();
    });

    auto pauseButton = std::make_shared<WindEffects::Editor::UI::Button>("Pause");
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

REGISTER_UI_PANEL(OutputLog,
    (WindEffects::Editor::UI::DockPanelDescriptor{.title = "Output Log", .iconResource = "output-log", .defaultZone = WindEffects::Editor::UI::DockZone::Floating, .defaultVisible = false}),
    CreateOutputLogPanel)

} // namespace we::programs::editor
