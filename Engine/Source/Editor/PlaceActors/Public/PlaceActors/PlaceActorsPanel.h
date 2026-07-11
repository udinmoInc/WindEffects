#pragma once

#include "Core/Widget.h"
#include "PlaceActors/PlaceActorsTypes.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace WindEffects::Editor::UI {
class SearchBox;
class ToolButton;
}

namespace we::programs::editor {

class PlaceActorsPanel : public WindEffects::Editor::UI::Widget {
public:
    PlaceActorsPanel();
    ~PlaceActorsPanel() override;

    void SetExternalSearchFilter(const std::string& filter) {
        if (m_ExternalSearchFilter != filter) {
            m_ExternalSearchFilter = filter;
            m_NeedsLayout = true;
        }
    }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Tick(float deltaTime) override;

    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseWheel(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnKeyDown(const WindEffects::Editor::UI::KeyEvent& event) override;

private:
    struct LayoutEntry {
        enum class Type { CategoryHeader, Item } type;
        std::string categoryId;
        std::string toolId;
        WindEffects::Editor::UI::Rect geometry;
        float hoverAnim = 0.0f;
        float pressAnim = 0.0f;
        bool selected = false;
    };

    struct ContextMenuItem {
        std::string label;
        WindEffects::Editor::UI::Rect geometry;
        std::function<void()> action;
    };

    void RebuildData();
    void RebuildLayout();
    void RefreshFilteredContent();
    void SaveCategoryState() const;

    LayoutEntry* HitEntry(const WindEffects::Editor::UI::Point& position);
    void SpawnItem(const std::string& toolId);
    void ToggleFavorite(const std::string& toolId);
    void OpenContextMenu(const std::string& toolId, const WindEffects::Editor::UI::Point& position);
    void CloseContextMenu();
    void ShowTooltip(const PlaceActorsItemData& item, const WindEffects::Editor::UI::Rect& anchor);
    void HideTooltip();

    std::shared_ptr<WindEffects::Editor::UI::SearchBox> m_SearchBox;
    std::shared_ptr<WindEffects::Editor::UI::ToolButton> m_SortButton;
    std::shared_ptr<WindEffects::Editor::UI::ToolButton> m_ViewToggleButton;
    std::shared_ptr<WindEffects::Editor::UI::ToolButton> m_RecentButton;
    std::shared_ptr<WindEffects::Editor::UI::ToolButton> m_CategoryFilterButton;

    std::string m_SearchText;
    std::string m_ExternalSearchFilter;
    std::string m_CategoryFilter = "All";
    PlaceActorsViewMode m_ViewMode = PlaceActorsViewMode::List;
    PlaceActorsSortMode m_SortMode = PlaceActorsSortMode::Name;
    bool m_ShowRecentOnly = false;

    float m_ScrollOffset = 0.0f;
    float m_ContentHeight = 0.0f;
    WindEffects::Editor::UI::Rect m_SearchRowRect;
    WindEffects::Editor::UI::Rect m_ContentRect;
    WindEffects::Editor::UI::Rect m_TooltipRect;

    std::vector<PlaceActorsCategoryData> m_DisplayCategories;
    std::vector<LayoutEntry> m_Layout;
    std::vector<WindEffects::Editor::UI::Rect> m_SectionBackgrounds;
    std::unordered_map<std::string, bool> m_CategoryExpanded;

    bool m_ContextMenuOpen = false;
    WindEffects::Editor::UI::Rect m_ContextMenuRect;
    std::vector<ContextMenuItem> m_ContextMenuItems;
    int m_ContextMenuHovered = -1;

    std::string m_TooltipText;
    std::string m_TooltipToolId;
    int m_FocusedIndex = -1;

    const PlaceActorsItemData* m_PendingDragItem = nullptr;
    WindEffects::Editor::UI::Point m_DragStartPosition{};
    bool m_DragStarted = false;

    bool m_NeedsLayout = true;
};

} // namespace we::programs::editor
