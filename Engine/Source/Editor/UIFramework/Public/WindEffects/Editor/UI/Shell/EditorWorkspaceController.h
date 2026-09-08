#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"
#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"
#include "KindUI/Panel/Panel.h"
#include "KindUI/Docking/DockContainer.h"
#include "KindUI/Docking/FloatingPanelFrame.h"
#include "KindUI/Layout/IPopupHost.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Layout/Splitter.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
        const std::shared_ptr<::we::runtime::kindui::panels::Panel>& panel,
        ::we::editor::docking::DockZone zone);

    void TogglePanelVisibility(const std::string& panelId);
    void SetPanelVisible(const std::string& panelId, bool visible);
    [[nodiscard]] bool IsPanelVisible(const std::string& panelId) const;
    void FloatPanel(const std::string& panelId);
    void FloatPanelAt(const std::string& panelId, const ::we::runtime::kindui::Point& position);
    void FloatPanelWidget(const std::shared_ptr<::we::runtime::kindui::panels::Panel>& panel);
    void FloatPanelWidget(
        const std::shared_ptr<::we::runtime::kindui::panels::Panel>& panel,
        const ::we::runtime::kindui::Point& position);
    void HidePanelWidget(const std::shared_ptr<::we::runtime::kindui::panels::Panel>& panel);
    void DockPanel(const std::string& panelId);
    void DockPanelTo(
        const std::string& panelId,
        const std::shared_ptr<::we::runtime::kindui::docking::DockContainer>& targetDock);
    /// Apply deferred float/dock after input + menu callbacks finish.
    void FlushPendingDockActions();
    /// Force core editor panels into their assigned docks (clears accidental floats).
    void EnsureDefaultDockPlacement();
    void FocusPanel(const std::string& panelId);

    void ApplyToolsPanelVisibility(bool visible);
    void SetBottomPanelIndex(int index);
    void FocusViewportNavigationPanel();

    [[nodiscard]] bool IsContentBrowserExpanded() const { return m_ContentBrowserExpanded; }
    void ToggleContentBrowserExpanded();

    void SetOnPanelVisibilityChanged(std::function<void()> callback);

    void LoadLayout();
    void SaveLayout() const;
    /// Drop dock/panel ownership before tearing down the UI tree / DLLs.
    void Reset();
    /// Release layout shared_ptrs after the live widget tree is destroyed.
    void ClearLayoutRefs();

private:
    EditorWorkspaceController() = default;

    struct FloatingHost {
        int id = 0;
        std::shared_ptr<::we::runtime::kindui::docking::FloatingPanelFrame> frame;
        std::shared_ptr<::we::runtime::kindui::docking::DockContainer> dock;
    };

    struct PanelEntry {
        std::shared_ptr<::we::runtime::kindui::panels::Panel> panel;
        int floatHostId = -1;
        ::we::editor::docking::DockZone zone = ::we::editor::docking::DockZone::Floating;
        ::we::editor::docking::DockZone homeZone = ::we::editor::docking::DockZone::Floating;
        bool visible = true;
        bool floating = false;
    };

    void ApplyToolsPaneWidth(float width);
    void BeginFloating(PanelEntry& entry, const std::string& panelId, const ::we::runtime::kindui::Point& position);
    void ApplyDockPanel(
        const std::string& panelId,
        const std::shared_ptr<::we::runtime::kindui::docking::DockContainer>& targetDock = nullptr);
    void ShowFloatingOptionsMenu(const std::string& panelId);
    void UpdateEmptyDockVisibility();
    void WireFloatingDock(FloatingHost& host);
    void DestroyFloatingHost(int hostId);
    void DetachPanelFromFloatHost(PanelEntry& entry);
    [[nodiscard]] FloatingHost* FindFloatingHost(int hostId);
    [[nodiscard]] const FloatingHost* FindFloatingHost(int hostId) const;
    [[nodiscard]] FloatingHost* FindFloatingHostByDock(
        const ::we::runtime::kindui::docking::DockContainer* dock);
    [[nodiscard]] FloatingHost* FindFloatingHostAtTabStrip(
        const ::we::runtime::kindui::Point& cursor,
        int excludeHostId = -1);
    [[nodiscard]] std::shared_ptr<::we::runtime::kindui::docking::DockContainer> FindZoneDockAtTabStrip(
        const ::we::runtime::kindui::Point& cursor) const;
    [[nodiscard]] FloatingHost& CreateFloatingHost(
        const ::we::runtime::kindui::Point& position,
        const ::we::runtime::kindui::Size& size);
    [[nodiscard]] ::we::editor::docking::DockZone ZoneForDock(
        const std::shared_ptr<::we::runtime::kindui::docking::DockContainer>& dock) const;
    [[nodiscard]] std::string FindPanelId(const ::we::runtime::kindui::panels::Panel* panel) const;

    std::shared_ptr<::we::runtime::kindui::docking::DockContainer> DockForPanel(const std::string& panelId) const;
    std::shared_ptr<::we::runtime::kindui::docking::DockContainer> DockForZone(
        ::we::editor::docking::DockZone zone) const;

    ::we::editor::shell::DockLayoutBuildResult m_Layout;
    std::unordered_map<std::string, PanelEntry> m_Panels;
    std::vector<FloatingHost> m_FloatHosts;
    int m_NextFloatHostId = 1;
    ::we::runtime::kindui::OverlayHost* m_PopupHost = nullptr;
    std::function<void()> m_OnPanelVisibilityChanged;

    std::string m_PendingFloatId;
    ::we::runtime::kindui::Point m_PendingFloatPos{};
    std::string m_PendingDockId;
    std::shared_ptr<::we::runtime::kindui::docking::DockContainer> m_PendingDockTarget;

    float m_ToolsPaneWidth = 300.0f;
    float m_RightSidebarWidth = 340.0f;
    bool m_ContentBrowserExpanded = true;
    float m_ContentBrowserBottomHeight = 240.0f;
};

UIFRAMEWORK_API ::we::runtime::kindui::IPopupHost* GetEditorPopupHost();

} // namespace we::programs::editor
