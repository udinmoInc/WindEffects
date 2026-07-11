#include "EditorModeController.h"
#include "EditorToolsRegistry.h"
#include "WindEffects/Editor/UI/Extensions/ExtensionBootstrap.h"
#include "Widgets/Panel.h"
#include "Widgets/ToolsPanel.h"
#include "Core/Icon.h"

namespace we::programs::editor {

namespace {

void SyncPanelTitle(const std::shared_ptr<WindEffects::Editor::UI::Panel>& panel) {
    if (!panel) {
        return;
    }

    const auto* mode = EditorToolsRegistry::Get().FindMode(EditorModeController::Get().GetActiveModeId());
    if (!mode) {
        panel->SetTitle("Tools");
        panel->SetTabIcon(WindEffects::Editor::UI::Icons::LayersName);
        return;
    }

    panel->SetTitle(mode->label);
    panel->SetTabIcon(mode->iconName);
}

} // namespace

std::shared_ptr<WindEffects::Editor::UI::Panel> CreateToolsPanel() {
    auto panel = std::make_shared<WindEffects::Editor::UI::Panel>("Tools");
    panel->SetHeaderHeight(30.0f);
    panel->SetTabIcon(WindEffects::Editor::UI::Icons::LayersName);

    panel->AddHeaderAction(WindEffects::Editor::UI::Icons::LockName, []() {
        auto& modeController = EditorModeController::Get();
        modeController.SetDrawerPinned(!modeController.IsDrawerPinned());
    });

    panel->AddHeaderAction(WindEffects::Editor::UI::Icons::XName, []() {
        EditorModeController::Get().SetDrawerVisible(false);
    });

    auto toolsContent = std::make_shared<ToolsPanel>();
    toolsContent->InitializeFromRegistry();
    panel->SetContent(toolsContent);

    SyncPanelTitle(panel);
    EditorModeController::Get().AddModeChangedListener([panel](const std::string&) {
        SyncPanelTitle(panel);
        if (auto* tools = dynamic_cast<ToolsPanel*>(panel->GetContent().get())) {
            tools->OnModeChanged();
        }
    });

    panel->SetVisible(EditorModeController::Get().IsDrawerVisible());
    return panel;
}

REGISTER_UI_PANEL(Tools,
    (WindEffects::Editor::UI::DockPanelDescriptor{.title = "Place Actors", .iconResource = "tools-panel", .defaultZone = WindEffects::Editor::UI::DockZone::Left, .defaultVisible = true}),
    CreateToolsPanel)

} // namespace we::programs::editor
