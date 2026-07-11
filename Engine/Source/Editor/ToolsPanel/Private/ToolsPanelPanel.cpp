#include "WindEffects/Editor/EditorSDK.h"
#include "EditorModeController.h"
#include "Widgets/ToolsPanel.h"

namespace we::programs::editor {
using namespace WindEffects::Editor::UI;

namespace {

void SyncPanelTitle(const std::shared_ptr<Panel>& panel) {
    if (!panel) {
        return;
    }

    const auto* mode = EditorToolsRegistry::Get().FindMode(EditorModeController::Get().GetActiveModeId());
    if (!mode) {
        panel->SetTitle("Tools");
        panel->SetTabIcon(Icons::LayersName);
        return;
    }

    panel->SetTitle(mode->label);
    panel->SetTabIcon(mode->iconName);
}

} // namespace

std::shared_ptr<Panel> CreateToolsPanel() {
    auto toolsContent = std::make_shared<ToolsPanel>();
    toolsContent->InitializeFromRegistry();

    auto panel = PanelBuilder("Tools")
        .TabIcon(Icons::LayersName)
        .WithHeaderAction(Icons::LockName, []() {
            auto& modeController = EditorModeController::Get();
            modeController.SetDrawerPinned(!modeController.IsDrawerPinned());
        })
        .WithHeaderAction(Icons::XName, []() {
            EditorModeController::Get().SetDrawerVisible(false);
        })
        .Content(toolsContent);

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
    WE_PANEL(Tools).Title("Place Actors").Icon("tools-panel").Zone(DockZone::Left).SortOrder(0),
    CreateToolsPanel)

} // namespace we::programs::editor
