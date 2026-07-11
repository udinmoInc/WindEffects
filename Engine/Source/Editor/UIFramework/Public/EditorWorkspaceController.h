#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"
#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"
#include "WindEffects/Editor/UI/Layout/IPopupHost.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace WindEffects::Editor::UI {
class Panel;
class DockContainer;
class Splitter;
class OverlayHost;
}

namespace we::programs::editor {

class UIFRAMEWORK_API EditorWorkspaceController {
public:
    static EditorWorkspaceController& Get();

    void BindLayout(const WindEffects::Editor::UI::DockLayoutBuildResult& layout);
    void SetPopupHost(WindEffects::Editor::UI::OverlayHost* host);
    [[nodiscard]] WindEffects::Editor::UI::IPopupHost* GetPopupHost() const;

    void RegisterPanel(
        const std::string& panelId,
        const std::shared_ptr<WindEffects::Editor::UI::Panel>& panel,
        WindEffects::Editor::UI::DockZone zone);

    void TogglePanelVisibility(const std::string& panelId);
    void SetPanelVisible(const std::string& panelId, bool visible);
    [[nodiscard]] bool IsPanelVisible(const std::string& panelId) const;
    void FloatPanel(const std::string& panelId);
    void FocusPanel(const std::string& panelId);

    void ApplyToolsPanelVisibility(bool visible);
    void SetBottomPanelIndex(int index);
    void FocusViewportNavigationPanel();

    [[nodiscard]] bool IsContentBrowserExpanded() const { return m_ContentBrowserExpanded; }
    void ToggleContentBrowserExpanded();

    void SetOnPanelVisibilityChanged(std::function<void()> callback);

    void LoadLayout();
    void SaveLayout() const;

private:
    EditorWorkspaceController() = default;

    std::shared_ptr<WindEffects::Editor::UI::DockContainer> DockForZone(
        WindEffects::Editor::UI::DockZone zone) const;

    struct PanelEntry {
        std::shared_ptr<WindEffects::Editor::UI::Panel> panel;
        WindEffects::Editor::UI::DockZone zone = WindEffects::Editor::UI::DockZone::Floating;
        bool visible = true;
    };

    WindEffects::Editor::UI::DockLayoutBuildResult m_Layout;
    std::unordered_map<std::string, PanelEntry> m_Panels;
    WindEffects::Editor::UI::OverlayHost* m_PopupHost = nullptr;
    std::function<void()> m_OnPanelVisibilityChanged;

    float m_ToolsSplitRatio = 0.18f;
    bool m_ContentBrowserExpanded = true;
    float m_ContentBrowserSplitRatio = 0.70f;
};

UIFRAMEWORK_API WindEffects::Editor::UI::IPopupHost* GetEditorPopupHost();

} // namespace we::programs::editor
