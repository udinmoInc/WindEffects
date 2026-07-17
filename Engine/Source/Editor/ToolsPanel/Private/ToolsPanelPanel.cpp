#include "WindEffects/Editor/EditorSDK.h"
#include "WindEffects/Editor/UI/Shell/EditorModeController.h"
#include "Widgets/ToolsPanel.h"

namespace we::programs::editor {
using namespace we::runtime::kindui;
using namespace we::editor::ui;

namespace {

void SyncPanelTitle(const std::shared_ptr<Panel>& panel) {
    if (!panel) {
        return;
    }

    const std::string activeModeId = EditorModeController::Get().GetActiveModeId();
    const auto* mode = EditorToolsRegistry::Get().FindMode(activeModeId);
    // Compact modes (Select) keep the Place Actors drawer UI while transform tools stay active.
    if (mode && !mode->opensToolDrawerByDefault) {
        if (const auto* actors = EditorToolsRegistry::Get().FindMode("Actors")) {
            if (actors->customContent) {
                panel->SetTitle(actors->label);
                panel->SetTabIcon(actors->iconName);
                return;
            }
        }
    }

    if (!mode) {
        panel->SetTitle("Actors");
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

    auto panel = PanelBuilder("Assets")
        .TabIcon(Icons::CubeName)
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
    WE_PANEL(Tools).Title("Actors").Icon("tools-panel").Zone(DockZone::Left).SortOrder(0),
    CreateToolsPanel)

} // namespace we::programs::editor
