#include "WindEffects/Editor/EditorSDK.h"
#include "WindEffects/Editor/UI/Shell/EditorModeController.h"
#include "WindEffects/Editor/UI/Panel/PanelModeTabs.h"
#include "Widgets/ToolsPanel.h"

namespace we::programs::editor {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

using namespace ::we::runtime::kindui;
using ::we::editor::panels::Panel;
using ::we::editor::panels::PanelBuilder;
using ::we::editor::panels::PanelModeTabDescriptor;
using ::we::editor::panels::PanelModeTabs;
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

    auto modeTabs = std::make_shared<PanelModeTabs>();
  std::vector<PanelModeTabDescriptor> tabDescriptors;
    for (const auto* mode : EditorToolsRegistry::Get().GetModesSorted()) {
        if (!mode->customContent && !mode->opensToolDrawerByDefault) {
            continue;
        }
        tabDescriptors.push_back(PanelModeTabDescriptor{
            mode->id,
            mode->label,
            mode->icon
        });
    }
    modeTabs->SetTabs(std::move(tabDescriptors));
    modeTabs->SetActiveTabId(EditorModeController::Get().GetActiveModeId());
    modeTabs->SetOnTabChanged([toolsContent](const std::string& modeId) {
        EditorModeController::Get().SetActiveMode(modeId);
        toolsContent->OnModeChanged();
    });

    auto panel = PanelBuilder("Assets")
        .TabIcon(kWindIconNone)
        .WithHeaderAction(we::runtime::kindui::kWindIconNone, []() {
            auto& modeController = EditorModeController::Get();
            modeController.SetDrawerPinned(!modeController.IsDrawerPinned());
        })
        .WithHeaderAction(WindIcons::Close16, []() {
            EditorModeController::Get().SetDrawerVisible(false);
        })
        .ModeTabs(modeTabs)
        .Content(toolsContent);

    SyncPanelTitle(panel);
    std::weak_ptr<Panel> weakPanel = panel;
    std::weak_ptr<PanelModeTabs> weakModeTabs = modeTabs;
    std::weak_ptr<ToolsPanel> weakTools = toolsContent;
    EditorModeController::Get().AddModeChangedListener([weakPanel, weakModeTabs, weakTools](const std::string& modeId) {
        auto panel = weakPanel.lock();
        auto modeTabs = weakModeTabs.lock();
        auto tools = weakTools.lock();
        if (!panel || !modeTabs || !tools) {
            return;
        }
        SyncPanelTitle(panel);
        modeTabs->SetActiveTabId(modeId);
        tools->OnModeChanged();
    });

    panel->SetVisible(EditorModeController::Get().IsDrawerVisible());
    return panel;
}

REGISTER_UI_PANEL(Tools,
    WE_PANEL(Tools).Title("Actors").Icon("tools-panel").Zone(DockZone::Left).SortOrder(0),
    CreateToolsPanel)

} // namespace we::programs::editor
