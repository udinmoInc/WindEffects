#include "WindEffects/Editor/EditorSDK.h"
#include "WindEffects/Editor/UI/Shell/EditorModeController.h"
#include "Widgets/ToolsPanel.h"

namespace we::programs::editor {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

using namespace ::we::runtime::kindui;
using ::we::editor::panels::Panel;
using ::we::editor::panels::PanelBuilder;
using ::we::editor::docking::DockZone;
using ::we::editor::shell::EditorModeController;
using ::we::editor::toolspanel::EditorToolsRegistry;

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
                panel->SetTabIcon(actors->icon);
                return;
            }
        }
    }

    if (!mode) {
        panel->SetTitle("Actors");
        panel->SetTabIcon(kWindIconNone);
        return;
    }

    panel->SetTitle(mode->label);
    panel->SetTabIcon(mode->icon);
}

} // namespace

std::shared_ptr<Panel> CreateToolsPanel() {
    auto toolsContent = std::make_shared<ToolsPanel>();
    toolsContent->InitializeFromRegistry(toolsContent);

    auto panel = PanelBuilder("Assets")
        .TabIcon(kWindIconNone)
        .WithHeaderAction(we::runtime::kindui::kWindIconNone, []() {
            auto& modeController = EditorModeController::Get();
            modeController.SetDrawerPinned(!modeController.IsDrawerPinned());
        })
        .WithHeaderAction(WindIcons::Close16, []() {
            EditorModeController::Get().SetDrawerVisible(false);
        })
        .Content(toolsContent);

    SyncPanelTitle(panel);
    std::weak_ptr<Panel> weakPanel = panel;
    std::weak_ptr<ToolsPanel> weakTools = toolsContent;
    EditorModeController::Get().AddModeChangedListener([weakPanel, weakTools](const std::string&) {
        auto panel = weakPanel.lock();
        auto tools = weakTools.lock();
        if (!panel || !tools) {
            return;
        }
        SyncPanelTitle(panel);
        tools->OnModeChanged();
    });

    panel->SetVisible(EditorModeController::Get().IsDrawerVisible());
    return panel;
}

REGISTER_UI_PANEL(Tools,
    WE_PANEL(Tools).Title("Actors").Icon("tools-panel").Zone(DockZone::Left).SortOrder(0),
    CreateToolsPanel)

} // namespace we::programs::editor
