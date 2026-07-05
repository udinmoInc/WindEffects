#include "EditorLayoutController.hpp"
#include "EditorPanelController.hpp"
#include "Layout/Splitter.hpp"
#include "Widgets/DockContainer.hpp"
#include "Core/Widget.hpp"
#include "Core/Logger.hpp"

namespace we::programs::editor {

EditorLayoutController& EditorLayoutController::Get() {
    static EditorLayoutController instance;
    return instance;
}

void EditorLayoutController::SetContentBrowserSplitter(const std::shared_ptr<we::UI::Splitter>& splitter) {
    m_ContentBrowserSplitter = splitter;
}

void EditorLayoutController::SetToolsPanelSplitter(const std::shared_ptr<we::UI::Splitter>& splitter) {
    m_ToolsPanelSplitter = splitter;
}

void EditorLayoutController::SetToolsPanelRoot(const std::shared_ptr<we::UI::Widget>& toolsPanelRoot) {
    m_ToolsPanelRoot = toolsPanelRoot;
}

void EditorLayoutController::ApplyToolsPanelVisibility(bool visible) {
    if (auto toolsRoot = m_ToolsPanelRoot.lock()) {
        toolsRoot->SetVisible(visible);
    }

    auto splitter = m_ToolsPanelSplitter.lock();
    if (!splitter) {
        return;
    }

    if (visible) {
        splitter->SetSplitRatio(m_SavedToolsPanelRatio);
    } else {
        m_SavedToolsPanelRatio = splitter->GetSplitRatio();
    }
}

void EditorLayoutController::SetRightBottomDock(const std::shared_ptr<we::UI::DockContainer>& dock) {
    m_RightBottomDock = dock;
}

void EditorLayoutController::FocusViewportNavigationPanel() {
    EditorPanelController::Get().SetPanelVisible(EditorPanelId::ViewportNavigation, true);
}

void EditorLayoutController::SetBottomPanels(
    const std::shared_ptr<we::UI::Widget>& contentBrowser,
    const std::shared_ptr<we::UI::Widget>& debugPanel) {
    m_ContentBrowserPanel = contentBrowser;
    m_DebugPanel = debugPanel;
}

void EditorLayoutController::SetBottomPanelIndex(int index) {
    if (index < 0 || index > 1 || index == m_BottomPanelIndex) {
        return;
    }

    auto splitter = m_ContentBrowserSplitter.lock();
    if (!splitter) {
        HE_ERROR("[EditorLayout] Content browser splitter not registered.");
        return;
    }

    std::shared_ptr<we::UI::Widget> panel = (index == 0)
        ? m_ContentBrowserPanel.lock()
        : m_DebugPanel.lock();
    if (!panel) {
        HE_ERROR("[EditorLayout] Bottom panel not registered.");
        return;
    }

    m_BottomPanelIndex = index;
    splitter->SetSecondChild(panel);
    HE_INFO(index == 0
        ? "[EditorLayout] Bottom panel switched to Assets."
        : "[EditorLayout] Bottom panel switched to Diagnostics.");
}

void EditorLayoutController::ToggleContentBrowserExpanded() {
    auto splitter = m_ContentBrowserSplitter.lock();
    if (!splitter) {
        HE_ERROR("[EditorLayout] Content browser splitter not registered.");
        return;
    }

    if (!m_ContentBrowserExpanded) {
        m_SavedContentBrowserRatio = splitter->GetSplitRatio();
        splitter->SetSplitRatio(0.1f);
        m_ContentBrowserExpanded = true;
        HE_INFO("[EditorLayout] Content Browser expanded to full view.");
    } else {
        splitter->SetSplitRatio(m_SavedContentBrowserRatio);
        m_ContentBrowserExpanded = false;
        HE_INFO("[EditorLayout] Content Browser restored to docked view.");
    }
}

} // namespace we::programs::editor
