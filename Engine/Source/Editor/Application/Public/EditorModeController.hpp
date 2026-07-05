#pragma once

#include "Application/Export.hpp"

#include <functional>
#include <string>
#include <vector>

namespace we::programs::editor {

/// Central editor mode workflow: active mode, tool-drawer visibility, and persisted layout.
class APPLICATION_API EditorModeController {
public:
    static EditorModeController& Get();

    void InitializeFromRegistry();

    [[nodiscard]] const std::string& GetActiveModeId() const { return m_ActiveModeId; }
    [[nodiscard]] bool IsDrawerVisible() const { return m_DrawerVisible; }
    [[nodiscard]] bool IsDrawerCollapsed() const { return m_DrawerCollapsed; }
    [[nodiscard]] bool IsDrawerPinned() const { return m_DrawerPinned; }
    [[nodiscard]] float GetDrawerWidth() const { return m_DrawerWidth; }

    void SetActiveMode(const std::string& modeId);
    void SetDrawerVisible(bool visible);
    void SetDrawerCollapsed(bool collapsed);
    void SetDrawerPinned(bool pinned);
    void SetDrawerWidth(float width);
    void ToggleDrawer();

    using ModeChangedCallback = std::function<void(const std::string& modeId)>;
    void AddModeChangedListener(ModeChangedCallback callback);

    void LoadState();
    void SaveState() const;

private:
    EditorModeController() = default;

    void ApplyModeDrawerPolicy();
    void NotifyModeChanged();

    std::string m_ActiveModeId = "Actors";
    bool m_DrawerVisible = true;
    bool m_DrawerCollapsed = false;
    bool m_DrawerPinned = true;
    float m_DrawerWidth = 280.0f;

    std::vector<ModeChangedCallback> m_ModeListeners;
};

} // namespace we::programs::editor
