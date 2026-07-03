#pragma once

#include "Core/Widget.hpp"
#include "Core/Widgets/PrimaryToolbarButton.hpp"
#include "Core/Widgets/SecondaryToolbarButton.hpp"
#include "Core/Widgets/ToolbarIconButton.hpp"
#include "Core/Widgets/ToolbarNavigationButton.hpp"
#include "Core/ToolbarDesignTokens.hpp"
#include "Models/ContentBrowserModel.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::UI {

class SearchBox;
class Breadcrumb;

// Square icon toggle for view modes (legacy, kept for compatibility).
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

// Icon + label + optional chevron (Filter, Sort, Import, Create) - legacy.
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

// Premium AAA toolbar with reusable components.
class ContentBrowserToolbarControls : public Widget {
public:
    enum class ToolbarMode {
        Full,           // Panel toolbar: create, import, back, forward, folder
        AssetPane       // Asset pane toolbar: search, save all, filter icon
    };

    static std::shared_ptr<ContentBrowserToolbarControls> Create(ToolbarMode mode = ToolbarMode::Full);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;

    std::shared_ptr<SearchBox> GetSearchBox() const { return m_SearchBox; }
    std::shared_ptr<Breadcrumb> GetBreadcrumb() const { return m_Breadcrumb; }

    void SetOnFilterClicked(std::function<void()> callback);
    void SetOnSortClicked(std::function<void()> callback);
    void SetOnImportClicked(std::function<void()> callback);
    void SetOnCreateClicked(std::function<void()> callback);
    void SetOnSaveClicked(std::function<void()> callback);
    void SetOnPreviousClicked(std::function<void()> callback);
    void SetOnNextClicked(std::function<void()> callback);
    void SetOnFolderClicked(std::function<void()> callback);
    void SetOnViewModeChanged(std::function<void(ContentViewMode)> callback);
    void SetOnSettingsClicked(std::function<void()> callback);

private:
    ContentBrowserToolbarControls(ToolbarMode mode);
    void InitializeChildren();
    void ArrangeControlRow(const Rect& row, float contentLeft, float contentRight);

    ToolbarMode m_Mode;
    std::shared_ptr<Breadcrumb> m_Breadcrumb;
    std::shared_ptr<SearchBox> m_SearchBox;
    
    // New reusable components
    std::shared_ptr<PrimaryToolbarButton> m_CreateBtn;
    std::shared_ptr<SecondaryToolbarButton> m_ImportBtn;
    std::shared_ptr<ToolbarNavigationButton> m_BackBtn;
    std::shared_ptr<ToolbarNavigationButton> m_ForwardBtn;
    std::shared_ptr<ToolbarNavigationButton> m_FolderBtn;
    
    // Legacy components (for AssetPane mode)
    std::shared_ptr<ToolbarIconToggle> m_GridViewBtn;
    std::shared_ptr<ToolbarIconToggle> m_ListViewBtn;
    std::shared_ptr<ToolbarIconToggle> m_SettingsBtn;
    std::shared_ptr<ToolbarIconToggle> m_FilterIconBtn;
    std::shared_ptr<ToolbarLabeledButton> m_SortBtn;
    std::shared_ptr<ToolbarLabeledButton> m_FilterBtn;
    std::shared_ptr<ToolbarLabeledButton> m_SaveBtn;
};

} // namespace we::UI
