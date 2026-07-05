#pragma once

#include "Application/Export.hpp"

#include <memory>

namespace we::UI {
class Splitter;
class DockContainer;
}

namespace we::programs::editor {

/// Persists editor shell layout between sessions (split ratios, active tabs, panel visibility).
class APPLICATION_API EditorLayoutPersistence {
public:
    static EditorLayoutPersistence& Get();

    void BindLayout(
        const std::shared_ptr<we::UI::Splitter>& mainHorizontal,
        const std::shared_ptr<we::UI::Splitter>& leftCenterVertical,
        const std::shared_ptr<we::UI::Splitter>& editorTopRow,
        const std::shared_ptr<we::UI::Splitter>& rightSideVertical,
        const std::shared_ptr<we::UI::DockContainer>& explorerDock,
        const std::shared_ptr<we::UI::DockContainer>& centerDock);

    void Load();
    void Save() const;

private:
    EditorLayoutPersistence() = default;

    std::weak_ptr<we::UI::Splitter> m_MainHorizontal;
    std::weak_ptr<we::UI::Splitter> m_LeftCenterVertical;
    std::weak_ptr<we::UI::Splitter> m_EditorTopRow;
    std::weak_ptr<we::UI::Splitter> m_RightSideVertical;
    std::weak_ptr<we::UI::DockContainer> m_ExplorerDock;
    std::weak_ptr<we::UI::DockContainer> m_CenterDock;
};

} // namespace we::programs::editor
