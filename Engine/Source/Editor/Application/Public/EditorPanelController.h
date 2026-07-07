#pragma once

#include "Application/Export.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::UI {
class DockContainer;
class Panel;
class Widget;
}

namespace we::programs::editor {

enum class EditorPanelId {
    Viewport,
    Game,
    Tools,
    ContentBrowser,
    Explorer,
    Details,
    ViewportNavigation,
    Debug,
    OutputLog
};

enum class EditorDockZone {
    Left,
    Center,
    Right,
    RightInspector,
    Bottom,
    Floating
};

/// Central registry for editor panel visibility, focus, and dock zone placement.
class APPLICATION_API EditorPanelController {
public:
    static EditorPanelController& Get();

    void RegisterDockZone(EditorDockZone zone, const std::shared_ptr<we::UI::DockContainer>& dock);
    void RegisterPanel(EditorPanelId id,
                     const std::string& menuLabel,
                     const std::shared_ptr<we::UI::Panel>& panel,
                     EditorDockZone defaultZone);

    void SetPanelVisible(EditorPanelId id, bool visible);
    void TogglePanelVisibility(EditorPanelId id);
    bool IsPanelVisible(EditorPanelId id) const;
    void FocusPanel(EditorPanelId id);
    void FloatPanel(EditorPanelId id);
    void DockPanel(EditorPanelId id, EditorDockZone zone);

    std::string GetMenuLabel(EditorPanelId id) const;
    std::shared_ptr<we::UI::Panel> GetPanel(EditorPanelId id) const;

    void SetOnPanelVisibilityChanged(std::function<void()> callback);

private:
    EditorPanelController() = default;

    struct PanelEntry {
        std::string menuLabel;
        std::shared_ptr<we::UI::Panel> panel;
        EditorDockZone zone = EditorDockZone::Right;
        bool visible = true;
        bool floating = false;
    };

    void EnsurePanelInDock(const PanelEntry& entry);
    std::shared_ptr<we::UI::DockContainer> GetDock(EditorDockZone zone) const;

    std::unordered_map<EditorDockZone, std::weak_ptr<we::UI::DockContainer>> m_DockZones;
    std::unordered_map<EditorPanelId, PanelEntry> m_Panels;
    std::function<void()> m_OnVisibilityChanged;
};

} // namespace we::programs::editor
