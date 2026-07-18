#pragma once

#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/ScrollViewport.h"
#include "PlaceActors/PlaceActorsTypes.h"
#include "PlaceActors/PlaceActorsResponsiveGrid.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "KindUI/Input/InputEvents.h"

namespace we::editor::placeactors {
using ::we::runtime::kindui::KeyEvent;
using ::we::runtime::kindui::ScrollViewport;
using ::we::runtime::kindui::ScrollViewportMetrics;
}

#include "ContentBrowser/Widgets/SearchBox.h"
#include "Widgets/ToolButton.h"

namespace we::programs::editor {

class PlaceActorsPanel : public we::runtime::kindui::Widget {
public:
    PlaceActorsPanel();
    ~PlaceActorsPanel() override;

    void SetExternalSearchFilter(const std::string& filter) {
        if (m_ExternalSearchFilter != filter) {
            m_ExternalSearchFilter = filter;
            m_NeedsLayout = true;
        }
    }

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void Tick(float deltaTime) override;

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseWheel(const we::runtime::kindui::MouseEvent& event) override;
    void OnKeyDown(const we::runtime::kindui::KeyEvent& event) override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;

private:
    struct LayoutEntry {
        enum class Type { CategoryHeader, Item, EmptyState } type;
        std::string categoryId;
        std::string toolId;
        we::runtime::kindui::Rect geometry;
        we::runtime::kindui::Rect previewGeometry;
        float hoverAnim = 0.0f;
        float pressAnim = 0.0f;
        float revealAnim = 1.0f;
        bool selected = false;
        int gridColumn = 0;
        int gridColumns = 1;
    };

    struct ContextMenuItem {
        std::string label;
        we::runtime::kindui::Rect geometry;
        std::function<void()> action;
    };

    struct FilterMenuItem {
        std::string label;
        we::runtime::kindui::Rect geometry;
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

    LayoutEntry* HitEntry(const we::runtime::kindui::Point& position);
    void SpawnItem(const std::string& toolId);
    void ToggleFavorite(const std::string& toolId);
    void OpenContextMenu(const std::string& toolId, const we::runtime::kindui::Point& position);
    void CloseContextMenu();
    void ToggleFilterMenu(const we::runtime::kindui::Point& anchor);
    void CloseFilterMenu();
    void ShowTooltip(const PlaceActorsItemData& item, const we::runtime::kindui::Rect& anchor);
    void HideTooltip();
    float CategoryExpandAnim(const std::string& categoryId) const;
    bool IsFavoritesCategory(const std::string& categoryId) const;
    bool IsQuickAccessCategory(const std::string& categoryId) const;
    bool IsPinnedCategory(const std::string& categoryId) const;
    PlaceActorsGridMetrics MakeGridMetrics() const;
    void BuildQuickAccessCategory(const std::string& query);

    std::shared_ptr<::we::editor::widgets::SearchBox> m_SearchBox;
    std::shared_ptr<::we::editor::toolbar::ToolButton> m_FilterButton;

    std::string m_SearchText;
    std::string m_ExternalSearchFilter;
    std::string m_CategoryFilter = "All";
    PlaceActorsViewMode m_ViewMode = PlaceActorsViewMode::Grid;
    PlaceActorsSortMode m_SortMode = PlaceActorsSortMode::Name;
    bool m_ShowRecentOnly = false;

    we::runtime::kindui::ScrollViewport m_Scroll;
    we::runtime::kindui::ScrollViewportMetrics m_ScrollMetrics{};
    float m_ContentHeight = 0.0f;
    float m_LastViewportWidth = -1.0f;

    we::runtime::kindui::Rect m_SearchRowRect;
    we::runtime::kindui::Rect m_ContentRect;
    we::runtime::kindui::Rect m_TooltipRect;

    std::vector<PlaceActorsCategoryData> m_DisplayCategories;
    std::vector<LayoutEntry> m_Layout;
    std::vector<we::runtime::kindui::Rect> m_SectionBackgrounds;
    std::unordered_map<std::string, bool> m_CategoryExpanded;
    std::unordered_map<std::string, float> m_CategoryExpandAnim;

    int m_FirstVisibleIndex = 0;
    int m_LastVisibleIndex = -1;

    bool m_ContextMenuOpen = false;
    we::runtime::kindui::Rect m_ContextMenuRect;
    std::vector<ContextMenuItem> m_ContextMenuItems;
    int m_ContextMenuHovered = -1;

    bool m_FilterMenuOpen = false;
    we::runtime::kindui::Rect m_FilterMenuRect;
    std::vector<FilterMenuItem> m_FilterMenuItems;
    int m_FilterMenuHovered = -1;

    std::string m_TooltipText;
    std::string m_TooltipToolId;
    int m_FocusedIndex = -1;

    const PlaceActorsItemData* m_PendingDragItem = nullptr;
    we::runtime::kindui::Point m_DragStartPosition{};
    bool m_DragStarted = false;

    std::string m_LastClickToolId;
    double m_LastClickTime = 0.0;

    bool m_NeedsLayout = true;
    bool m_HasSearchResults = true;
};

} // namespace we::programs::editor
