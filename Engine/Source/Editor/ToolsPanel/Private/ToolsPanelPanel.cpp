#include "EditorRegistry.hpp"
#include "EditorModeController.hpp"
#include "EditorToolsRegistry.hpp"
#include "Widgets/Panel.hpp"
#include "Widgets/ToolsPanel.hpp"
#include "Core/DockTabIconRegistry.hpp"
#include "Core/Icon.hpp"

namespace we::programs::editor {

namespace {

void SyncPanelTitle(const std::shared_ptr<we::UI::Panel>& panel) {
    if (!panel) {
        return;
    }

    const auto* mode = EditorToolsRegistry::Get().FindMode(EditorModeController::Get().GetActiveModeId());
    if (!mode) {
        panel->SetTitle("Tools");
        return;
    }

    panel->SetTitle(mode->label);
    we::UI::DockTabIconRegistry::Get().RegisterIcon(mode->label, mode->iconName);
}

} // namespace

std::shared_ptr<we::UI::Panel> CreateToolsPanel() {
    auto panel = std::make_shared<we::UI::Panel>("Tools");
    panel->SetHeaderHeight(30.0f);

    panel->AddHeaderAction(we::UI::Icons::LockName, []() {
        auto& modeController = EditorModeController::Get();
        modeController.SetDrawerPinned(!modeController.IsDrawerPinned());
    });

    panel->AddHeaderAction(we::UI::Icons::XName, []() {
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

REGISTER_EDITOR_PANEL(Tools, CreateToolsPanel)

} // namespace we::programs::editor
