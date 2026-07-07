#pragma once

#include "ToolsPanel/Export.h"

#include "Core/Widget.h"
#include "Core/PaintContext.h"
#include "EditorToolsRegistry.h"
#include "ToolsPanelState.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::UI {
class SearchBox;
}

namespace we::programs::editor {

/// Tool list content for the active editor mode. Header/close/tab chrome is owned by Panel.
class TOOLSPANEL_API ToolsPanel : public we::UI::Widget {
public:
    ToolsPanel();
    ~ToolsPanel() override;

    void InitializeFromRegistry();
    void OnModeChanged();

    we::UI::Size Measure(const we::UI::Size& availableSize) override;
    void Arrange(const we::UI::Rect& allottedRect) override;
    void Paint(we::UI::PaintContext& context) override;
    void Tick(float deltaTime) override;

    void OnMouseDown(const we::UI::MouseEvent& event) override;
    void OnMouseMove(const we::UI::MouseEvent& event) override;
    void OnMouseUp(const we::UI::MouseEvent& event) override;
    void OnKeyDown(const we::UI::KeyEvent& event) override;

    bool HitTest(const we::UI::Point& position) const;

private:
    struct SectionHit {
        std::string categoryId;
        we::UI::Rect headerRect;
        bool expanded = true;
    };

    struct ToolHit {
        const EditorToolAction* tool = nullptr;
        we::UI::Rect geometry;
        bool hovered = false;
        bool favorite = false;
    };

    struct ContextMenuItem {
        std::string label;
        std::function<void()> action;
        we::UI::Rect geometry;
    };

    void RebuildLayout();
    void RebuildModeContent();
    void ExecuteTool(const EditorToolAction* tool);
    bool IsCategoryExpanded(const std::string& categoryId, bool defaultExpanded) const;
    void SetCategoryExpanded(const std::string& categoryId, bool expanded);
    void CloseContextMenu();
    void ShowToolContextMenu(const EditorToolAction* tool, const we::UI::Point& position);
    bool HandleShortcut(const we::UI::KeyEvent& event);

    void SaveState() const;
    [[nodiscard]] std::string GetActiveModeId() const;

    SectionHit* HitSectionHeader(const we::UI::Point& p);
    ToolHit* HitTool(const we::UI::Point& p);

    ToolsPanelState m_State;

    std::string m_SearchText;
    std::shared_ptr<we::UI::SearchBox> m_SearchBox;
    std::shared_ptr<we::UI::Widget> m_ModeContentWidget;
    std::string m_ModeContentModeId;
    std::string m_ModeContentSearchText;

    we::UI::Rect m_PanelRect;
    we::UI::Rect m_SearchRect;
    we::UI::Rect m_ContentRect;

    std::vector<SectionHit> m_Sections;
    std::vector<ToolHit> m_ToolHits;

    const EditorToolAction* m_PendingDragTool = nullptr;
    we::UI::Point m_DragStartPosition{};
    bool m_DragStarted = false;

    bool m_ContextMenuOpen = false;
    we::UI::Rect m_ContextMenuRect;
    std::vector<ContextMenuItem> m_ContextMenuItems;
    int m_ContextMenuHovered = -1;
};

} // namespace we::programs::editor
