#pragma once

#include "ContentBrowser/Models/ContentBrowserModel.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Core/Widgets/ToolbarIconButton.h"
#include "KindUI/Core/Widgets/ToolbarNavigationButton.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/WindIcon.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::editor::widgets { class SearchBox; }

namespace we::editor::contentbrowser {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;
using ::we::runtime::kindui::PrimaryButton;
using ::we::runtime::kindui::SecondaryButton;
using ::we::runtime::kindui::ToolbarNavigationButton;

class Breadcrumb;

// Square icon toggle for view modes (legacy, kept for compatibility).
class ToolbarIconToggle : public Widget {
public:
    ToolbarIconToggle(we::runtime::kindui::WindIconRef icon, const char* tooltip = nullptr);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

    void SetSelected(bool selected) { m_Selected = selected; }
    bool IsSelected() const { return m_Selected; }
    void SetFrameless(bool frameless) { m_Frameless = frameless; }
    bool IsFrameless() const { return m_Frameless; }
    void SetOnClicked(std::function<void()> callback) { m_OnClicked = callback; }

private:
    we::runtime::kindui::WindIconRef m_Icon = we::runtime::kindui::kWindIconNone;
    bool m_Selected = false;
    bool m_Frameless = true;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    std::function<void()> m_OnClicked;
};

// Icon + label + optional chevron (Filter, Sort, Import, Create) - legacy.
class ToolbarLabeledButton : public Widget {
public:
    enum class Variant { Standard, Primary, AddAction };

    ToolbarLabeledButton(const std::string& label, we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone,
        bool showChevron = false, Variant variant = Variant::Standard, float horizontalPadding = 8.0f);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

    void SetOnClicked(std::function<void()> callback) { m_OnClicked = callback; }

private:
    std::string m_Label;
    we::runtime::kindui::WindIconRef m_Icon = we::runtime::kindui::kWindIconNone;
    bool m_ShowChevron = false;
    Variant m_Variant = Variant::Standard;
    float m_HorizontalPadding = 8.0f;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    std::function<void()> m_OnClicked;
};

// Premium AAA toolbar with reusable components.
class ContentBrowserToolbarControls : public we::runtime::kindui::Row {
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

    std::shared_ptr<::we::editor::widgets::SearchBox> GetSearchBox() const { return m_SearchBox; }
    std::shared_ptr<Breadcrumb> GetBreadcrumb() const { return m_Breadcrumb; }

    void SetOnFilterClicked(std::function<void()> callback);
    void SetOnSortClicked(std::function<void()> callback);
    void SetOnImportClicked(std::function<void()> callback);
    void SetOnCreateClicked(std::function<void()> callback);
    void SetOnSaveClicked(std::function<void()> callback);
    void SetOnFabClicked(std::function<void()> callback);
    void SetOnPreviousClicked(std::function<void()> callback);
    void SetOnNextClicked(std::function<void()> callback);
    void SetOnFolderClicked(std::function<void()> callback);
    void SetOnViewModeChanged(std::function<void(ContentViewMode)> callback);
    void SetOnSettingsClicked(std::function<void()> callback);
    void SetOnMoreClicked(std::function<void()> callback);

private:
    ContentBrowserToolbarControls(ToolbarMode mode);
    void InitializeChildren();
    void ArrangeControlRow(const Rect& row, float contentLeft, float contentRight);

    ToolbarMode m_Mode;
    std::shared_ptr<Breadcrumb> m_Breadcrumb;
    std::shared_ptr<::we::editor::widgets::SearchBox> m_SearchBox;
    
    // Action buttons
    std::shared_ptr<ToolbarLabeledButton> m_CreateBtn;
    std::shared_ptr<ToolbarLabeledButton> m_ImportBtn;
    std::shared_ptr<ToolbarLabeledButton> m_SaveBtn;
    std::shared_ptr<ToolbarLabeledButton> m_FabBtn;
    std::shared_ptr<ToolbarNavigationButton> m_BackBtn;
    std::shared_ptr<ToolbarNavigationButton> m_ForwardBtn;
    std::shared_ptr<ToolbarNavigationButton> m_FolderBtn;
    
    // Legacy / secondary controls (for AssetPane mode)
    std::shared_ptr<ToolbarIconToggle> m_GridViewBtn;
    std::shared_ptr<ToolbarIconToggle> m_ListViewBtn;
    std::shared_ptr<ToolbarIconToggle> m_SettingsBtn;
    std::shared_ptr<ToolbarIconToggle> m_MoreBtn;
    std::shared_ptr<ToolbarIconToggle> m_FilterIconBtn;
    std::shared_ptr<ToolbarLabeledButton> m_SortBtn;
    std::shared_ptr<ToolbarLabeledButton> m_FilterBtn;

    std::function<void(ContentViewMode)> m_OnViewModeChanged;
    std::function<void()> m_OnSettingsClicked;
    std::function<void()> m_OnMoreClicked;
    std::function<void()> m_OnCreateClicked;
    std::function<void()> m_OnFilterClicked;
};

} // namespace we::editor::contentbrowser
