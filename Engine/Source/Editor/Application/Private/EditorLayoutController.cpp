#include "EditorLayoutController.hpp"
#include "Layout/Splitter.hpp"
#include "Core/Logger.hpp"

namespace we::programs::editor {

EditorLayoutController& EditorLayoutController::Get() {
    static EditorLayoutController instance;
    return instance;
}

void EditorLayoutController::SetContentBrowserSplitter(const std::shared_ptr<we::UI::Splitter>& splitter) {
    m_ContentBrowserSplitter = splitter;
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
