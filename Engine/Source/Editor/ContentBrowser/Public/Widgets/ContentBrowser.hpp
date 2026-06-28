#pragma once

#include "Core/Widget.hpp"
#include "Core/Style.hpp"
#include "Core/Icon.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <volk.h>
#include "Models/ContentBrowserModel.hpp"
#include "Controllers/ContentBrowserController.hpp"

namespace we::UI {


// Content browser widget with grid/list views
class ContentBrowser : public Widget {
public:
    using OnItemDoubleClicked = std::function<void(const ContentItem&)>;
    using OnItemSelected = std::function<void(const ContentItem&)>;
    using OnItemRightClicked = std::function<void(const ContentItem&, const Point&)>;

    ContentBrowser();
    virtual ~ContentBrowser() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    void OnKeyDown(const KeyEvent& event) override;

    // Content management
    void AddItem(const ContentItem& item);
    void RemoveItem(const std::string& id);
    void Clear();
    
    // Selection
    bool IsSelected(const std::string& id) const;
    const std::vector<std::string>& GetSelectedIds() const { return m_Model ? m_Model->selectedIds : m_EmptySelectedIds; }
    void ClearSelection() { if (m_Controller) m_Controller->ClearSelection(); }
    
    // View mode
    ContentViewMode GetViewMode() const { return m_Model ? m_Model->viewMode : ContentViewMode::Grid; }
    
    // Model and Controller
    void SetModel(std::shared_ptr<ContentBrowserModel> model) { 
        m_Model = model;
        if (m_Model) {
            m_Model->onModelChanged = [this]() {
                BuildRenderList();
            };
            BuildRenderList();
        }
    }
    void SetController(std::shared_ptr<ContentBrowserController> controller) { m_Controller = controller; }
    

    
    // Callbacks
    void SetOnItemDoubleClicked(OnItemDoubleClicked callback) { m_OnItemDoubleClicked = callback; }
    void SetOnItemSelected(OnItemSelected callback) { m_OnItemSelected = callback; }
    void SetOnItemRightClicked(OnItemRightClicked callback) { m_OnItemRightClicked = callback; }
    void SetOnItemNeedsThumbnail(std::function<void(const std::string&)> callback) { m_OnItemNeedsThumbnail = callback; }
    void SetOnBackgroundRightClicked(std::function<void(const Point&)> callback) { m_OnBackgroundRightClicked = callback; }

    // Styling
    void SetGridItemSize(float size) { m_GridItemSize = size; }
    void SetListRowHeight(float height) { m_ListRowHeight = height; }

private:
    struct RenderItem {
        ContentItem item;
        Rect geometry;
    };

    void BuildRenderList();
    void CalculateGridLayout();
    void CalculateListLayout();
    RenderItem* GetItemAtPosition(const Point& pos);

    std::shared_ptr<ContentBrowserModel> m_Model;
    std::shared_ptr<ContentBrowserController> m_Controller;
    std::vector<std::string> m_EmptySelectedIds;

    std::vector<RenderItem> m_RenderList;
    std::string m_LastSelectedId; // For shift-select
    std::string m_HoveredId;
    
    Point m_SelectStart{0,0};
    Point m_SelectEnd{0,0};
    Point m_DragStart{0,0};
    Point m_MousePos{0,0};
    bool m_IsSelecting = false;
    bool m_IsDragging = false;
    
    float m_ScrollOffset = 0.0f;
    float m_GridItemSize = 96.0f;
    float m_GridSpacing = 8.0f;
    float m_ListRowHeight = 32.0f;
    float m_ThumbnailSize = 64.0f;

    OnItemDoubleClicked m_OnItemDoubleClicked;
    OnItemSelected m_OnItemSelected;
    OnItemRightClicked m_OnItemRightClicked;
    std::function<void(const std::string&)> m_OnItemNeedsThumbnail;
    std::function<void(const Point&)> m_OnBackgroundRightClicked;

    WidgetStyle m_Style;
public:
    std::shared_ptr<ContentBrowserModel> GetModel() { return m_Model; }
    std::shared_ptr<ContentBrowserController> GetController() { return m_Controller; }
};

// Breadcrumb widget for navigation path
class Breadcrumb : public Widget {
public:
    Breadcrumb();
    virtual ~Breadcrumb() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;

    void SetPath(const std::vector<std::string>& path);
    void AddCrumb(const std::string& crumb);
    void Clear();

    using OnCrumbClicked = std::function<void(size_t index)>;
    void SetOnCrumbClicked(OnCrumbClicked callback) { m_OnCrumbClicked = callback; }

private:
    struct CrumbInfo {
        std::string text;
        Rect geometry;
        bool hovered = false;
    };

    void CalculateLayout();
    CrumbInfo* GetCrumbAtPosition(const Point& pos);

    std::vector<CrumbInfo> m_Crumbs;
    float m_SeparatorSpacing = 8.0f;
    float m_CrumbSpacing = 4.0f;
    int m_HoveredCrumb = -1;

    OnCrumbClicked m_OnCrumbClicked;
};

} // namespace we::editor::contentbrowser::UI
