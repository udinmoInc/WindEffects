#pragma once

#include <memory>

namespace we::UI {
class Splitter;
}

namespace we::programs::editor {

/// Manages editor-wide layout state (e.g. content browser maximize).
class EditorLayoutController {
public:
    static EditorLayoutController& Get();

    void SetContentBrowserSplitter(const std::shared_ptr<we::UI::Splitter>& splitter);
    void ToggleContentBrowserExpanded();
    bool IsContentBrowserExpanded() const { return m_ContentBrowserExpanded; }

private:
    EditorLayoutController() = default;

    std::weak_ptr<we::UI::Splitter> m_ContentBrowserSplitter;
    float m_SavedContentBrowserRatio = 0.7f;
    bool m_ContentBrowserExpanded = false;
};

} // namespace we::programs::editor
