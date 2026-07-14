#pragma once

#include "ContentBrowser/Export.h"

#include "Core/Widget.h"
#include "Core/Style.h"
#include "Core/Icon.h"
#include "Layout/ScrollViewport.h"
#include <functional>
#include <string>
#include <vector>
#include <volk.h>

namespace WindEffects::Editor::UI {

// Tree node data structure
struct TreeNode {
    std::string id;
    std::string label;
    std::string iconName;
    VkDescriptorSet iconTexture = VK_NULL_HANDLE;
    std::vector<std::shared_ptr<TreeNode>> children;
    bool expanded = true;
    bool visible = true;
    bool locked = false;
    void* userData = nullptr;
};

// Tree view widget for hierarchical data
class CONTENTBROWSER_API TreeView : public Widget {
public:
    using OnSelectionChanged = std::function<void(const std::vector<std::string>& ids)>;
    using OnItemDoubleClicked = std::function<void(const std::string& id)>;
    using OnVisibilityToggled = std::function<void(const std::string& id, bool visible)>;
    using OnLockToggled = std::function<void(const std::string& id, bool locked)>;
    using OnRenameCommitted = std::function<void(const std::string& id, const std::string& newLabel)>;
    using OnReparentRequested = std::function<void(const std::string& childId, const std::string& newParentId)>;

    TreeView();
    virtual ~TreeView() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    void OnKeyDown(const KeyEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override;

    // Tree management
    void SetRoot(const std::shared_ptr<TreeNode>& root);
    void AddItem(const std::shared_ptr<TreeNode>& item, const std::string& parentId = "");
    void RemoveItem(const std::string& id);
    void Clear();

    // Selection
    void SetSelectedId(const std::string& id);
    void SetSelectedIds(const std::vector<std::string>& ids);
    std::string GetSelectedId() const;
    const std::vector<std::string>& GetSelectedIds() const { return m_SelectedIds; }

    // Callbacks
    void SetOnSelectionChanged(OnSelectionChanged callback) { m_OnSelectionChanged = callback; }
    void SetOnItemDoubleClicked(OnItemDoubleClicked callback) { m_OnItemDoubleClicked = callback; }
    void SetOnVisibilityToggled(OnVisibilityToggled callback) { m_OnVisibilityToggled = callback; }
    void SetOnLockToggled(OnLockToggled callback) { m_OnLockToggled = callback; }
    void SetOnRenameCommitted(OnRenameCommitted callback) { m_OnRenameCommitted = callback; }
    void SetOnReparentRequested(OnReparentRequested callback) { m_OnReparentRequested = callback; }

    // Styling
    void SetItemHeight(float height);
    void SetIndentWidth(float width);
    void SetExplorerStyle(bool enabled) { m_ExplorerStyle = enabled; if (enabled) m_Style.text.size = 13.0f; }
    void SetShowRowControls(bool show) { m_ShowRowControls = show; }

    // Search and Filter
    void SetSearchQuery(const std::string& query);
    std::string GetSearchQuery() const { return m_SearchQuery; }
    struct FilterOptions {
        bool showFolders = true;
        bool showAssets = true;
        bool showActors = true;
        bool showComponents = true;
        bool showHidden = false;
        bool showLocked = true;
        bool showEmptyFolders = true;
        bool showFavorites = false;
        int sortOrder = 0; // 0: A-Z, 1: Z-A, 2: Modified Recently
    };
    void SetFilterOptions(const FilterOptions& options) { m_FilterOptions = options; MarkRenderListDirty(); BuildRenderList(); }
    FilterOptions GetFilterOptions() const { return m_FilterOptions; }

private:
    struct RenderItem {
        std::shared_ptr<TreeNode> node;
        int depth;
        int flatIndex;
        Rect geometry;
    };

    void BuildRenderList();
    void MarkRenderListDirty() { m_RenderListDirty = true; }
    void UpdateVisibleRange();
    RenderItem* GetItemAtPosition(const Point& pos);
    std::shared_ptr<TreeNode> FindNode(const std::string& id);
    void ToggleExpand(const std::string& id);
    void BeginRename(const std::string& id);
    void CommitRename();
    void CancelRename();
    void ShowContextMenu(const std::string& id, const Point& position);
    void HandleSelection(const std::string& id, bool shift, bool ctrl);
    void SetZoomLevel(float zoomLevel);
    bool IsSelected(const std::string& id) const;
    int GetVisibleRowCount() const;
    void SyncScrollMetrics();
    void ScrollSelectionIntoView();

    std::shared_ptr<TreeNode> m_Root;
    std::vector<RenderItem> m_RenderList;
    bool m_RenderListDirty = true;
    std::vector<std::string> m_SelectedIds;
    std::string m_HoveredId;
    std::string m_DropTargetId;

    float m_ContentHeight = 0.0f;
    ScrollViewport m_Scroll;
    ScrollViewportMetrics m_ScrollMetrics{};
    int m_FirstVisibleIndex = 0;
    int m_LastVisibleIndex = 0;
    float m_BaseItemHeight = 24.0f;
    float m_BaseIndentWidth = 20.0f;
    float m_ItemHeight = 24.0f;
    float m_IndentWidth = 20.0f;
    float m_ZoomLevel = 1.0f;
    bool m_ExplorerStyle = false;
    bool m_ShowRowControls = true;

    std::string m_RenamingId;
    std::string m_RenameBuffer;
    float m_RenameCursorBlink = 0.0f;

    std::string m_SearchQuery;
    FilterOptions m_FilterOptions;

    bool m_Dragging = false;
    Point m_DragStart{};
    std::string m_DragSourceId;

    OnSelectionChanged m_OnSelectionChanged;
    OnItemDoubleClicked m_OnItemDoubleClicked;
    OnVisibilityToggled m_OnVisibilityToggled;
    OnLockToggled m_OnLockToggled;
    OnRenameCommitted m_OnRenameCommitted;
    OnReparentRequested m_OnReparentRequested;

    WidgetStyle m_Style;
};

} // namespace WindEffects::Editor::UI
