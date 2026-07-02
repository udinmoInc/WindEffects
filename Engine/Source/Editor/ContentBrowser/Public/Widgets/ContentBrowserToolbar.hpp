#pragma once

#include "Core/Widget.hpp"
#include "Models/ContentBrowserModel.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::UI {

class SearchBox;

// Vertical divider between toolbar control groups.
class ToolbarSeparator : public Widget {
public:
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
};

// Square icon toggle for view modes (28–32 px).
class ToolbarIconToggle : public Widget {
public:
    ToolbarIconToggle(const std::string& iconName, const char* tooltip = nullptr);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

    void SetSelected(bool selected) { m_Selected = selected; }
    bool IsSelected() const { return m_Selected; }
    void SetOnClicked(std::function<void()> callback) { m_OnClicked = callback; }

private:
    std::string m_IconName;
    bool m_Selected = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    std::function<void()> m_OnClicked;
};

// Icon + label + optional chevron (Filter, Sort, Import).
class ToolbarLabeledButton : public Widget {
public:
    enum class Variant { Standard, Primary };

    ToolbarLabeledButton(const std::string& label, const std::string& iconName = {},
        bool showChevron = false, Variant variant = Variant::Standard, float horizontalPadding = 12.0f);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

    void SetOnClicked(std::function<void()> callback) { m_OnClicked = callback; }

private:
    std::string m_Label;
    std::string m_IconName;
    bool m_ShowChevron = false;
    Variant m_Variant = Variant::Standard;
    float m_HorizontalPadding = 12.0f;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    std::function<void()> m_OnClicked;
};

// Second toolbar row: search, view modes, sort, filter, import, create.
class ContentBrowserToolbarControls : public Widget {
public:
    using ViewModeCallback = std::function<void(ContentViewMode)>;

    static std::shared_ptr<ContentBrowserToolbarControls> Create();

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;

    std::shared_ptr<SearchBox> GetSearchBox() const { return m_SearchBox; }
    void SetViewMode(ContentViewMode mode);
    void SetOnViewModeChanged(ViewModeCallback callback) { m_OnViewModeChanged = callback; }

    void SetOnFilterClicked(std::function<void()> callback);
    void SetOnSortClicked(std::function<void()> callback);
    void SetOnImportClicked(std::function<void()> callback);
    void SetOnCreateClicked(std::function<void()> callback);

private:
    ContentBrowserToolbarControls() = default;
    void InitializeChildren();
    void SyncViewToggles(ContentViewMode mode);

    std::shared_ptr<SearchBox> m_SearchBox;
    std::shared_ptr<ToolbarIconToggle> m_ViewLarge;
    std::shared_ptr<ToolbarIconToggle> m_ViewMedium;
    std::shared_ptr<ToolbarIconToggle> m_ViewSmall;
    std::shared_ptr<ToolbarIconToggle> m_ViewList;
    std::shared_ptr<ToolbarIconToggle> m_ViewDetails;
    std::shared_ptr<ToolbarLabeledButton> m_SortBtn;
    std::shared_ptr<ToolbarLabeledButton> m_FilterBtn;
    std::shared_ptr<ToolbarLabeledButton> m_ImportBtn;
    std::shared_ptr<ToolbarLabeledButton> m_CreateBtn;
    std::shared_ptr<ToolbarSeparator> m_Sep1;
    std::shared_ptr<ToolbarSeparator> m_Sep2;
    std::shared_ptr<ToolbarSeparator> m_Sep3;
    std::shared_ptr<ToolbarSeparator> m_Sep4;
    std::shared_ptr<ToolbarSeparator> m_Sep5;

    ViewModeCallback m_OnViewModeChanged;
};

} // namespace we::UI
