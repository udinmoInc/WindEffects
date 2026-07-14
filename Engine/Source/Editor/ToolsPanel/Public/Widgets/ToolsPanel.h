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

namespace WindEffects::Editor::UI {
class SearchBox;
}

namespace we::programs::editor {

/// Tool list content for the active editor mode. Header/close/tab chrome is owned by Panel.
class TOOLSPANEL_API ToolsPanel : public WindEffects::Editor::UI::Widget {
public:
    ToolsPanel();
    ~ToolsPanel() override;

    void InitializeFromRegistry();
    void OnModeChanged();

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Tick(float deltaTime) override;

    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnKeyDown(const WindEffects::Editor::UI::KeyEvent& event) override;

    bool HitTest(const WindEffects::Editor::UI::Point& position) const;

private:
    struct SectionHit {
        std::string categoryId;
        WindEffects::Editor::UI::Rect headerRect;
        bool expanded = true;
    };

    struct ToolHit {
        const EditorToolAction* tool = nullptr;
        WindEffects::Editor::UI::Rect geometry;
        bool hovered = false;
        bool favorite = false;
    };

    struct ContextMenuItem {
        std::string label;
        std::function<void()> action;
        WindEffects::Editor::UI::Rect geometry;
    };

    void RebuildLayout();
    void RebuildModeContent();
    void ExecuteTool(const EditorToolAction* tool);
    bool IsCategoryExpanded(const std::string& categoryId, bool defaultExpanded) const;
    void SetCategoryExpanded(const std::string& categoryId, bool expanded);
    void CloseContextMenu();
    void ShowToolContextMenu(const EditorToolAction* tool, const WindEffects::Editor::UI::Point& position);
    bool HandleShortcut(const WindEffects::Editor::UI::KeyEvent& event);

    void SaveState() const;
    [[nodiscard]] std::string GetActiveModeId() const;
    // Compact modes (Select) keep transform tools, but the pinned drawer shows Place Actors.
    [[nodiscard]] std::string GetDrawerContentModeId() const;

    SectionHit* HitSectionHeader(const WindEffects::Editor::UI::Point& p);
    ToolHit* HitTool(const WindEffects::Editor::UI::Point& p);

    ToolsPanelState m_State;

    std::string m_SearchText;
    std::shared_ptr<WindEffects::Editor::UI::SearchBox> m_SearchBox;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_ModeContentWidget;
    std::string m_ModeContentModeId;
    std::string m_ModeContentSearchText;

    WindEffects::Editor::UI::Rect m_PanelRect;
    WindEffects::Editor::UI::Rect m_SearchRect;
    WindEffects::Editor::UI::Rect m_ContentRect;

    std::vector<SectionHit> m_Sections;
    std::vector<ToolHit> m_ToolHits;

    const EditorToolAction* m_PendingDragTool = nullptr;
    WindEffects::Editor::UI::Point m_DragStartPosition{};
    bool m_DragStarted = false;

    bool m_ContextMenuOpen = false;
    WindEffects::Editor::UI::Rect m_ContextMenuRect;
    std::vector<ContextMenuItem> m_ContextMenuItems;
    int m_ContextMenuHovered = -1;
};

} // namespace we::programs::editor
