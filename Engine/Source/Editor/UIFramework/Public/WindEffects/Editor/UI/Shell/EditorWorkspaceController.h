#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"
#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"
#include "KindUI/Layout/IPopupHost.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace we::runtime::kindui {
class Splitter;
class OverlayHost;
}
namespace we::editor::ui {
using we::runtime::kindui::Splitter;
using we::runtime::kindui::OverlayHost;
class Panel;
class DockContainer;
}

namespace we::programs::editor {

class UIFRAMEWORK_API EditorWorkspaceController {
public:
    static EditorWorkspaceController& Get();

    void BindLayout(const we::editor::ui::DockLayoutBuildResult& layout);
    void SetPopupHost(we::runtime::kindui::OverlayHost* host);
    [[nodiscard]] we::runtime::kindui::IPopupHost* GetPopupHost() const;

    void RegisterPanel(
        const std::string& panelId,
        const std::shared_ptr<we::editor::ui::Panel>& panel,
        we::editor::ui::DockZone zone);

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

    std::shared_ptr<we::editor::ui::DockContainer> DockForZone(
        we::editor::ui::DockZone zone) const;

    struct PanelEntry {
        std::shared_ptr<we::editor::ui::Panel> panel;
        we::editor::ui::DockZone zone = we::editor::ui::DockZone::Floating;
        bool visible = true;
    };

    we::editor::ui::DockLayoutBuildResult m_Layout;
    std::unordered_map<std::string, PanelEntry> m_Panels;
    we::runtime::kindui::OverlayHost* m_PopupHost = nullptr;
    std::function<void()> m_OnPanelVisibilityChanged;

    float m_ToolsSplitRatio = 0.18f;
    bool m_ContentBrowserExpanded = true;
    float m_ContentBrowserSplitRatio = 0.70f;
};

UIFRAMEWORK_API we::runtime::kindui::IPopupHost* GetEditorPopupHost();

} // namespace we::programs::editor
