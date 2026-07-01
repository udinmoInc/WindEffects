#pragma once

#include <memory>

namespace we::UI {
class Splitter;
class Widget;
class DockContainer;
}

namespace we::programs::editor {

/// Manages editor-wide layout state (e.g. content browser maximize).
class EditorLayoutController {
public:
    static EditorLayoutController& Get();

    void SetContentBrowserSplitter(const std::shared_ptr<we::UI::Splitter>& splitter);
    void ToggleContentBrowserExpanded();
    bool IsContentBrowserExpanded() const { return m_ContentBrowserExpanded; }

    void SetToolsPanelSplitter(const std::shared_ptr<we::UI::Splitter>& splitter);
    void SetToolsPanelRoot(const std::shared_ptr<we::UI::Widget>& toolsPanelRoot);
    void ApplyToolsPanelVisibility(bool visible);

    void SetRightBottomDock(const std::shared_ptr<we::UI::DockContainer>& dock);
    void SetViewportNavigationTabIndex(int index) { m_ViewportNavigationTabIndex = index; }
    void FocusViewportNavigationPanel();

    void SetBottomPanels(const std::shared_ptr<we::UI::Widget>& contentBrowser,
                         const std::shared_ptr<we::UI::Widget>& debugPanel);
    void SetBottomPanelIndex(int index);
    int GetBottomPanelIndex() const { return m_BottomPanelIndex; }

private:
    EditorLayoutController() = default;

    std::weak_ptr<we::UI::Splitter> m_ContentBrowserSplitter;
    float m_SavedContentBrowserRatio = 0.7f;
    bool m_ContentBrowserExpanded = false;

    std::weak_ptr<we::UI::Splitter> m_ToolsPanelSplitter;
    std::weak_ptr<we::UI::Widget> m_ToolsPanelRoot;
    float m_SavedToolsPanelRatio = 0.18f;

    std::weak_ptr<we::UI::DockContainer> m_RightBottomDock;
    int m_ViewportNavigationTabIndex = 1;

    std::weak_ptr<we::UI::Widget> m_ContentBrowserPanel;
    std::weak_ptr<we::UI::Widget> m_DebugPanel;
    int m_BottomPanelIndex = 0;
};

} // namespace we::programs::editor
