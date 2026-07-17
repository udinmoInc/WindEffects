#pragma once

#include "ToolsPanel/Export.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PaintContext.h"
#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"
#include "ToolsPanelState.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "KindUI/Input/InputEvents.h"

namespace we::editor::widgets { class SearchBox; }

namespace we::programs::editor {
using ::we::editor::toolspanel::EditorToolAction;

/// Tool list content for the active editor mode.
class TOOLSPANEL_API ToolsPanel : public we::runtime::kindui::Widget {
public:
    ToolsPanel();
    ~ToolsPanel() override;

    void InitializeFromRegistry();
    void OnModeChanged();

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void Tick(float deltaTime) override;

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    void OnKeyDown(const we::runtime::kindui::KeyEvent& event) override;

    bool HitTest(const we::runtime::kindui::Point& position) const;

private:
    struct SectionHit {
        std::string categoryId;
        we::runtime::kindui::Rect headerRect;
        bool expanded = true;
    };

    struct ToolHit {
        const EditorToolAction* tool = nullptr;
        we::runtime::kindui::Rect geometry;
        bool hovered = false;
        bool favorite = false;
    };

    struct ContextMenuItem {
        std::string label;
        std::function<void()> action;
        we::runtime::kindui::Rect geometry;
    };

    void RebuildLayout();
    void RebuildModeContent();
    void ExecuteTool(const EditorToolAction* tool);
    bool IsCategoryExpanded(const std::string& categoryId, bool defaultExpanded) const;
    void SetCategoryExpanded(const std::string& categoryId, bool expanded);
    void CloseContextMenu();
    void ShowToolContextMenu(const EditorToolAction* tool, const we::runtime::kindui::Point& position);
    bool HandleShortcut(const we::runtime::kindui::KeyEvent& event);

    void SaveState() const;
    [[nodiscard]] std::string GetActiveModeId() const;
    // Compact modes (Select) keep transform tools, but the pinned drawer shows Place Actors.
    [[nodiscard]] std::string GetDrawerContentModeId() const;

    SectionHit* HitSectionHeader(const we::runtime::kindui::Point& p);
    ToolHit* HitTool(const we::runtime::kindui::Point& p);

    ToolsPanelState m_State;

    std::string m_SearchText;
    std::shared_ptr<::we::editor::widgets::SearchBox> m_SearchBox;
    std::shared_ptr<we::runtime::kindui::Widget> m_ModeContentWidget;
    std::string m_ModeContentModeId;
    std::string m_ModeContentSearchText;

    we::runtime::kindui::Rect m_PanelRect;
    we::runtime::kindui::Rect m_SearchRect;
    we::runtime::kindui::Rect m_ContentRect;

    std::vector<SectionHit> m_Sections;
    std::vector<ToolHit> m_ToolHits;

    const EditorToolAction* m_PendingDragTool = nullptr;
    we::runtime::kindui::Point m_DragStartPosition{};
    bool m_DragStarted = false;

    bool m_ContextMenuOpen = false;
    we::runtime::kindui::Rect m_ContextMenuRect;
    std::vector<ContextMenuItem> m_ContextMenuItems;
    int m_ContextMenuHovered = -1;
};

} // namespace we::programs::editor
