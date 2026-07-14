#pragma once

#include "Core/Widget.h"
#include "Core/Style.h"
#include "Core/Icon.h"
#include "Layout/ScrollViewport.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_set>
#include <volk.h>
#include "Models/ContentBrowserModel.h"
#include "Controllers/ContentBrowserController.h"

namespace WindEffects::Editor::UI {

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
    void Tick(float deltaTime) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    void OnKeyDown(const KeyEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override;

    void AddItem(const ContentItem& item);
    void RemoveItem(const std::string& id);
    void Clear();

    bool IsSelected(const std::string& id) const;
    const std::vector<std::string>& GetSelectedIds() const { return m_Model ? m_Model->selectedIds : m_EmptySelectedIds; }
    void ClearSelection() { if (m_Controller) m_Controller->ClearSelection(); }

    ContentViewMode GetViewMode() const { return m_Model ? m_Model->viewMode : ContentViewMode::LargeIcons; }
    void SetViewMode(ContentViewMode mode) { if (m_Controller) m_Controller->SetViewMode(mode); }

    void SetModel(std::shared_ptr<ContentBrowserModel> model);
    void SetController(std::shared_ptr<ContentBrowserController> controller) { m_Controller = controller; }

    void SetOnItemDoubleClicked(OnItemDoubleClicked callback) { m_OnItemDoubleClicked = callback; }
    void SetOnItemSelected(OnItemSelected callback) { m_OnItemSelected = callback; }
    void SetOnItemRightClicked(OnItemRightClicked callback) { m_OnItemRightClicked = callback; }
    void SetOnItemNeedsThumbnail(std::function<void(const std::string&)> callback) { m_OnItemNeedsThumbnail = callback; }
    void SetOnBackgroundRightClicked(std::function<void(const Point&)> callback) { m_OnBackgroundRightClicked = callback; }
    void SetOnVisibleItemsChanged(std::function<void(const std::unordered_set<std::string>&)> callback) {
        m_OnVisibleItemsChanged = callback;
    }

    void SetListRowHeight(float height) { m_ListRowHeight = height; }

    std::shared_ptr<ContentBrowserModel> GetModel() { return m_Model; }
    std::shared_ptr<ContentBrowserController> GetController() { return m_Controller; }

private:
    struct RenderItem {
        ContentItem item;
        Rect geometry;
        Rect thumbGeometry;
        int sourceIndex = -1;
    };

    struct GridMetrics {
        float cellWidth = 100.0f;
        float cellHeight = 120.0f;
        float thumbSize = 88.0f;
        float labelLineHeight = 14.0f;
        float labelLines = 2.0f;
        float labelGap = 6.0f;
        float hSpacing = 8.0f;
        float vSpacing = 8.0f;
        float padding = 16.0f;
    };

    void BuildRenderList();
    void MarkLayoutDirty() { m_LayoutDirty = true; }
    void CalculateGridLayout();
    void CalculateListLayout();
    void CalculateDetailsLayout();
    void CalculateTilesLayout();
    void UpdateVisibleRange();
    void RequestVisibleThumbnails();
    GridMetrics GetGridMetrics() const;
    ContentViewMode GetEffectiveViewMode() const;
    RenderItem* GetItemAtPosition(const Point& pos);
    void SyncScrollMetrics();
    void RecalculateLayout();
    void ScrollSelectionIntoView();
    float ComputeContentHeight() const;

    void PaintGridItem(PaintContext& context, const RenderItem& renderItem);
    void PaintListItem(PaintContext& context, const RenderItem& renderItem);
    void PaintTileChrome(PaintContext& context, const Rect& cell, bool selected, float hoverAlpha);
    void PaintAssetThumbnail(PaintContext& context, const Rect& thumbRect, const ContentItem& item, bool selected, bool hovered);
    void PaintItemLabel(PaintContext& context, const Rect& cell, const std::string& name, float maxWidth, int maxLines = 2);
    std::vector<std::string> WrapLabelText(PaintContext& context, const std::string& text, float maxWidth, float fontSize, int maxLines) const;

    std::shared_ptr<ContentBrowserModel> m_Model;
    std::shared_ptr<ContentBrowserController> m_Controller;
    std::vector<std::string> m_EmptySelectedIds;

    std::vector<RenderItem> m_RenderList;
    bool m_LayoutDirty = true;
    float m_LastLayoutWidth = -1.0f;
    float m_LastLayoutHeight = -1.0f;
    float m_LastScrollOffset = -1.0f;
    std::string m_LastSelectedId;
    std::string m_HoveredId;
    float m_ItemHoverAlpha = 0.0f;

    Point m_SelectStart{0,0};
    Point m_SelectEnd{0,0};
    Point m_DragStart{0,0};
    Point m_MousePos{0,0};
    Point m_LastClickPos{0,0};
    double m_LastClickTime = 0.0;
    bool m_IsSelecting = false;
    bool m_IsDragging = false;

    float m_ListRowHeight = 22.0f;
    ScrollViewport m_Scroll;
    ScrollViewportMetrics m_ScrollMetrics{};
    float m_ContentHeight = 0.0f;

    int m_FirstVisibleIndex = 0;
    int m_LastVisibleIndex = 0;

    OnItemDoubleClicked m_OnItemDoubleClicked;
    OnItemSelected m_OnItemSelected;
    OnItemRightClicked m_OnItemRightClicked;
    std::function<void(const std::string&)> m_OnItemNeedsThumbnail;
    std::function<void(const Point&)> m_OnBackgroundRightClicked;
    std::function<void(const std::unordered_set<std::string>&)> m_OnVisibleItemsChanged;

    WidgetStyle m_Style;
};

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
    const std::vector<std::string>& GetPath() const { return m_PathSegments; }
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
    std::vector<std::string> m_PathSegments;
};

class ContentBrowserStatusBar : public Widget {
public:
    ContentBrowserStatusBar();

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetSelectedCount(size_t count) { m_SelectedCount = count; }
    void SetAssetCount(size_t count) { m_AssetCount = count; }
    void SetFolderCount(size_t count) { m_FolderCount = count; }
    void SetMemoryUsage(size_t bytes) { m_MemoryUsage = bytes; }

private:
    size_t m_SelectedCount = 0;
    size_t m_AssetCount = 0;
    size_t m_FolderCount = 0;
    size_t m_MemoryUsage = 0;
};

} // namespace WindEffects::Editor::UI
