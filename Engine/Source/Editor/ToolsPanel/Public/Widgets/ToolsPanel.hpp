#pragma once

#include "Core/Widget.hpp"
#include "Core/PaintContext.hpp"
#include "EditorToolsRegistry.hpp"
#include "ToolsPanelState.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::UI {
class SearchBox;
}

namespace we::programs::editor {

/// Narrow left tool drawer for the active editor mode (hidden until a mode needs it).
class ToolsPanel : public we::UI::Widget {
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

    float GetOccupiedWidth() const { return m_AnimatedWidth; }
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

    float TargetWidth() const;
    void SaveState() const;
    [[nodiscard]] std::string GetActiveModeId() const;

    SectionHit* HitSectionHeader(const we::UI::Point& p);
    ToolHit* HitTool(const we::UI::Point& p);

    ToolsPanelState m_State;
    float m_AnimatedWidth = 0.0f;

    std::string m_SearchText;
    std::shared_ptr<we::UI::SearchBox> m_SearchBox;
    std::shared_ptr<we::UI::Widget> m_ModeContentWidget;
    std::string m_ModeContentModeId;
    std::string m_ModeContentSearchText;

    we::UI::Rect m_PanelRect;
    we::UI::Rect m_HeaderRect;
    we::UI::Rect m_SearchRect;
    we::UI::Rect m_ContentRect;
    we::UI::Rect m_ResizeHandleRect;

    std::vector<SectionHit> m_Sections;
    std::vector<ToolHit> m_ToolHits;

    bool m_DraggingResize = false;
    float m_ResizeStartX = 0.0f;
    float m_ResizeStartWidth = 0.0f;

    const EditorToolAction* m_PendingDragTool = nullptr;
    we::UI::Point m_DragStartPosition{};
    bool m_DragStarted = false;

    bool m_ContextMenuOpen = false;
    we::UI::Rect m_ContextMenuRect;
    std::vector<ContextMenuItem> m_ContextMenuItems;
    int m_ContextMenuHovered = -1;

    we::UI::Rect m_PinButtonRect;
    we::UI::Rect m_CloseButtonRect;
    bool m_PinHovered = false;
    bool m_CloseHovered = false;
};

} // namespace we::programs::editor
