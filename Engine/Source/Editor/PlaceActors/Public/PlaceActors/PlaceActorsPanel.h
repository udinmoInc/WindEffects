#pragma once

#include "Core/Widget.h"
#include "PlaceActors/PlaceActorsTypes.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::UI {
class SearchBox;
class ToolButton;
}

namespace we::programs::editor {

class PlaceActorsPanel : public we::UI::Widget {
public:
    PlaceActorsPanel();
    ~PlaceActorsPanel() override;

    void SetExternalSearchFilter(const std::string& filter) {
        if (m_ExternalSearchFilter != filter) {
            m_ExternalSearchFilter = filter;
            m_NeedsLayout = true;
        }
    }

    we::UI::Size Measure(const we::UI::Size& availableSize) override;
    void Arrange(const we::UI::Rect& allottedRect) override;
    void Paint(we::UI::PaintContext& context) override;
    void Tick(float deltaTime) override;

    void OnMouseDown(const we::UI::MouseEvent& event) override;
    void OnMouseMove(const we::UI::MouseEvent& event) override;
    void OnMouseUp(const we::UI::MouseEvent& event) override;
    void OnMouseWheel(const we::UI::MouseEvent& event) override;
    void OnKeyDown(const we::UI::KeyEvent& event) override;

private:
    struct LayoutEntry {
        enum class Type { CategoryHeader, Item } type;
        std::string categoryId;
        std::string toolId;
        we::UI::Rect geometry;
        float hoverAnim = 0.0f;
        float pressAnim = 0.0f;
        bool selected = false;
    };

    struct ContextMenuItem {
        std::string label;
        we::UI::Rect geometry;
        std::function<void()> action;
    };

    void RebuildData();
    void RebuildLayout();
    void SaveCategoryState() const;

    LayoutEntry* HitEntry(const we::UI::Point& position);
    void SpawnItem(const std::string& toolId);
    void ToggleFavorite(const std::string& toolId);
    void OpenContextMenu(const std::string& toolId, const we::UI::Point& position);
    void CloseContextMenu();
    void ShowTooltip(const PlaceActorsItemData& item, const we::UI::Rect& anchor);
    void HideTooltip();

    std::shared_ptr<we::UI::SearchBox> m_SearchBox;
    std::shared_ptr<we::UI::ToolButton> m_SortButton;
    std::shared_ptr<we::UI::ToolButton> m_ViewToggleButton;
    std::shared_ptr<we::UI::ToolButton> m_RecentButton;
    std::shared_ptr<we::UI::ToolButton> m_CategoryFilterButton;

    std::string m_SearchText;
    std::string m_ExternalSearchFilter;
    std::string m_CategoryFilter = "All";
    PlaceActorsViewMode m_ViewMode = PlaceActorsViewMode::Grid;
    PlaceActorsSortMode m_SortMode = PlaceActorsSortMode::Name;
    bool m_ShowRecentOnly = false;

    float m_ScrollOffset = 0.0f;
    float m_ContentHeight = 0.0f;
    we::UI::Rect m_ToolbarRect;
    we::UI::Rect m_ContentRect;
    we::UI::Rect m_TooltipRect;

    std::vector<PlaceActorsCategoryData> m_DisplayCategories;
    std::vector<LayoutEntry> m_Layout;
    std::unordered_map<std::string, bool> m_CategoryExpanded;

    bool m_ContextMenuOpen = false;
    we::UI::Rect m_ContextMenuRect;
    std::vector<ContextMenuItem> m_ContextMenuItems;
    int m_ContextMenuHovered = -1;

    std::string m_TooltipText;
    std::string m_TooltipToolId;
    int m_FocusedIndex = -1;

    const PlaceActorsItemData* m_PendingDragItem = nullptr;
    we::UI::Point m_DragStartPosition{};
    bool m_DragStarted = false;

    bool m_NeedsLayout = true;
};

} // namespace we::programs::editor
