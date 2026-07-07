#pragma once

#include "Core/Widget.h"
#include "Widgets/TreeView.h"
#include <functional>
#include <string>
#include <vector>

namespace we::UI {

class ExplorerPanelHeader : public Widget {
public:
    static constexpr float kDefaultHeight = 32.0f;
    static constexpr float kLogoLogicalSize = 16.0f;

    using FilterOptions = TreeView::FilterOptions;

    ExplorerPanelHeader();

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnKeyDown(const KeyEvent& event) override;
    void OnTextInput(const std::string& text);
    bool ShowsPointerCursor(const Point& position) const override;

    // Geometry getters for menu positioning
    Rect GetFilterButtonGeometry() const { return m_FilterButtonGeometry; }

    // Search functionality
    void SetSearchQuery(const std::string& query);
    std::string GetSearchQuery() const { return m_SearchQuery; }
    void SetOnSearchChanged(std::function<void(const std::string&)> callback) { m_OnSearchChanged = std::move(callback); }

    // Filter functionality
    void SetOnFilterClicked(std::function<void()> callback) { m_OnFilterClicked = std::move(callback); }
    void SetOnNewFolder(std::function<void()> callback) { m_OnNewFolder = std::move(callback); }
    void SetOnRefresh(std::function<void()> callback) { m_OnRefresh = std::move(callback); }

    // Filter state
    FilterOptions GetFilterOptions() const { return m_FilterOptions; }
    void SetFilterOptions(const FilterOptions& options) { m_FilterOptions = options; }

private:
    struct HeaderButton {
        std::string iconName;
        Rect geometry;
    };

    void PaintToolbarButton(PaintContext& context, const Rect& geometry, 
                           const std::string& iconName, bool hovered, const Theme& theme);
    int HitButtonIndex(const Point& position) const;
    bool IsSearchFocused() const { return m_SearchFocused; }

    std::string m_SearchQuery;
    bool m_SearchFocused = false;
    float m_CursorBlink = 0.0f;
    
    Rect m_SearchBoxGeometry;
    Rect m_FilterButtonGeometry;
    Rect m_NewFolderButtonGeometry;
    Rect m_RefreshButtonGeometry;

    int m_HoveredButton = -1;
    int m_PressedButton = -1;

    FilterOptions m_FilterOptions;

    std::function<void(const std::string&)> m_OnSearchChanged;
    std::function<void()> m_OnFilterClicked;
    std::function<void()> m_OnNewFolder;
    std::function<void()> m_OnRefresh;
};

} // namespace we::UI
