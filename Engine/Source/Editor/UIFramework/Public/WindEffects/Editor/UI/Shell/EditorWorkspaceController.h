#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"
#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"
#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "WindEffects/Editor/UI/Widgets/DockContainer.h"
#include "KindUI/Layout/IPopupHost.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Layout/Splitter.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace we::programs::editor {
using ::we::runtime::kindui::Splitter;

class UIFRAMEWORK_API EditorWorkspaceController {
public:
    static EditorWorkspaceController& Get();

    void BindLayout(const ::we::editor::shell::DockLayoutBuildResult& layout);
    void SetPopupHost(::we::runtime::kindui::OverlayHost* host);
    [[nodiscard]] ::we::runtime::kindui::IPopupHost* GetPopupHost() const;

    void RegisterPanel(
        const std::string& panelId,
        const std::shared_ptr<::we::editor::panels::Panel>& panel,
        ::we::editor::docking::DockZone zone);

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

    std::shared_ptr<::we::editor::docking::DockContainer> DockForZone(
        ::we::editor::docking::DockZone zone) const;

    struct PanelEntry {
        std::shared_ptr<::we::editor::panels::Panel> panel;
        ::we::editor::docking::DockZone zone = ::we::editor::docking::DockZone::Floating;
        bool visible = true;
    };

    ::we::editor::shell::DockLayoutBuildResult m_Layout;
    std::unordered_map<std::string, PanelEntry> m_Panels;
    ::we::runtime::kindui::OverlayHost* m_PopupHost = nullptr;
    std::function<void()> m_OnPanelVisibilityChanged;

    float m_ToolsSplitRatio = 0.18f;
    bool m_ContentBrowserExpanded = true;
    float m_ContentBrowserSplitRatio = 0.70f;
};

UIFRAMEWORK_API ::we::runtime::kindui::IPopupHost* GetEditorPopupHost();

} // namespace we::programs::editor
