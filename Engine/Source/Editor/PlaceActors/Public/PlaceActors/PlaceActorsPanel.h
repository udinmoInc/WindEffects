#pragma once

#include "Core/Widget.h"
#include "Layout/ScrollViewport.h"
#include "PlaceActors/PlaceActorsTypes.h"
#include "PlaceActors/PlaceActorsResponsiveGrid.h"
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
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;

private:
    struct LayoutEntry {
        enum class Type { CategoryHeader, Item, EmptyState } type;
        std::string categoryId;
        std::string toolId;
        WindEffects::Editor::UI::Rect geometry;
        WindEffects::Editor::UI::Rect previewGeometry;
        float hoverAnim = 0.0f;
        float pressAnim = 0.0f;
        float revealAnim = 1.0f;
        bool selected = false;
        int gridColumn = 0;
        int gridColumns = 1;
    };

    struct ContextMenuItem {
        std::string label;
        WindEffects::Editor::UI::Rect geometry;
        std::function<void()> action;
    };

    struct FilterMenuItem {
        std::string label;
        WindEffects::Editor::UI::Rect geometry;
        std::function<void()> action;
        bool checked = false;
    };

    void RebuildData();
    void RebuildLayout();
    void RefreshFilteredContent();
    void SyncScrollMetrics();
    void UpdateVisibleRange();
    void LoadPanelState();
    void SaveCategoryState() const;
    void SaveFavorites() const;
    void ScrollFocusedIntoView();

    LayoutEntry* HitEntry(const WindEffects::Editor::UI::Point& position);
    void SpawnItem(const std::string& toolId);
    void ToggleFavorite(const std::string& toolId);
    void OpenContextMenu(const std::string& toolId, const WindEffects::Editor::UI::Point& position);
    void CloseContextMenu();
    void ToggleFilterMenu(const WindEffects::Editor::UI::Point& anchor);
    void CloseFilterMenu();
    void ShowTooltip(const PlaceActorsItemData& item, const WindEffects::Editor::UI::Rect& anchor);
    void HideTooltip();
    float CategoryExpandAnim(const std::string& categoryId) const;
    bool IsFavoritesCategory(const std::string& categoryId) const;
    bool IsQuickAccessCategory(const std::string& categoryId) const;
    bool IsPinnedCategory(const std::string& categoryId) const;
    PlaceActorsGridMetrics MakeGridMetrics() const;
    void BuildQuickAccessCategory(const std::string& query);

    std::shared_ptr<WindEffects::Editor::UI::SearchBox> m_SearchBox;
    std::shared_ptr<WindEffects::Editor::UI::ToolButton> m_FilterButton;

    std::string m_SearchText;
    std::string m_ExternalSearchFilter;
    std::string m_CategoryFilter = "All";
    PlaceActorsViewMode m_ViewMode = PlaceActorsViewMode::Grid;
    PlaceActorsSortMode m_SortMode = PlaceActorsSortMode::Name;
    bool m_ShowRecentOnly = false;

    WindEffects::Editor::UI::ScrollViewport m_Scroll;
    WindEffects::Editor::UI::ScrollViewportMetrics m_ScrollMetrics{};
    float m_ContentHeight = 0.0f;
    float m_LastViewportWidth = -1.0f;

    WindEffects::Editor::UI::Rect m_SearchRowRect;
    WindEffects::Editor::UI::Rect m_ContentRect;
    WindEffects::Editor::UI::Rect m_TooltipRect;

    std::vector<PlaceActorsCategoryData> m_DisplayCategories;
    std::vector<LayoutEntry> m_Layout;
    std::vector<WindEffects::Editor::UI::Rect> m_SectionBackgrounds;
    std::unordered_map<std::string, bool> m_CategoryExpanded;
    std::unordered_map<std::string, float> m_CategoryExpandAnim;

    int m_FirstVisibleIndex = 0;
    int m_LastVisibleIndex = -1;

    bool m_ContextMenuOpen = false;
    WindEffects::Editor::UI::Rect m_ContextMenuRect;
    std::vector<ContextMenuItem> m_ContextMenuItems;
    int m_ContextMenuHovered = -1;

    bool m_FilterMenuOpen = false;
    WindEffects::Editor::UI::Rect m_FilterMenuRect;
    std::vector<FilterMenuItem> m_FilterMenuItems;
    int m_FilterMenuHovered = -1;

    std::string m_TooltipText;
    std::string m_TooltipToolId;
    int m_FocusedIndex = -1;

    const PlaceActorsItemData* m_PendingDragItem = nullptr;
    WindEffects::Editor::UI::Point m_DragStartPosition{};
    bool m_DragStarted = false;

    std::string m_LastClickToolId;
    double m_LastClickTime = 0.0;

    bool m_NeedsLayout = true;
    bool m_HasSearchResults = true;
};

} // namespace we::programs::editor
