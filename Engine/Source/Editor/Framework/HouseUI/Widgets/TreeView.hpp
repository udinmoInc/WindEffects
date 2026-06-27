#pragma once

#include "../Core/Widget.hpp"
#include "../Core/Style.hpp"
#include "../Core/Icon.hpp"
#include <functional>
#include <string>
#include <vector>

namespace HouseEngine::UI {

// Tree node data structure
struct TreeNode {
    std::string id;
    std::string label;
    std::string iconName;
    std::vector<std::shared_ptr<TreeNode>> children;
    bool expanded = true;
    bool visible = true;
    bool locked = false;
    void* userData = nullptr;
};

// Tree view widget for hierarchical data
class TreeView : public Widget {
public:
    using OnSelectionChanged = std::function<void(const std::string& id)>;
    using OnItemDoubleClicked = std::function<void(const std::string& id)>;
    using OnVisibilityToggled = std::function<void(const std::string& id, bool visible)>;
    using OnLockToggled = std::function<void(const std::string& id, bool locked)>;

    TreeView();
    virtual ~TreeView() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    void OnKeyDown(const KeyEvent& event) override;

    // Tree management
    void SetRoot(const std::shared_ptr<TreeNode>& root);
    void AddItem(const std::shared_ptr<TreeNode>& item, const std::string& parentId = "");
    void RemoveItem(const std::string& id);
    void Clear();

    // Selection
    void SetSelectedId(const std::string& id);
    std::string GetSelectedId() const { return m_SelectedId; }

    // Callbacks
    void SetOnSelectionChanged(OnSelectionChanged callback) { m_OnSelectionChanged = callback; }
    void SetOnItemDoubleClicked(OnItemDoubleClicked callback) { m_OnItemDoubleClicked = callback; }
    void SetOnVisibilityToggled(OnVisibilityToggled callback) { m_OnVisibilityToggled = callback; }
    void SetOnLockToggled(OnLockToggled callback) { m_OnLockToggled = callback; }

    // Styling
    void SetItemHeight(float height) { m_ItemHeight = height; }
    void SetIndentWidth(float width) { m_IndentWidth = width; }

private:
    struct RenderItem {
        std::shared_ptr<TreeNode> node;
        int depth;
        Rect geometry;
    };

    void BuildRenderList();
    RenderItem* GetItemAtPosition(const Point& pos);
    void ToggleExpand(const std::string& id);

    std::shared_ptr<TreeNode> m_Root;
    std::vector<RenderItem> m_RenderList;
    std::string m_SelectedId;
    std::string m_HoveredId;

    float m_ScrollOffset = 0.0f;
    float m_ItemHeight = 24.0f;
    float m_IndentWidth = 20.0f;

    OnSelectionChanged m_OnSelectionChanged;
    OnItemDoubleClicked m_OnItemDoubleClicked;
    OnVisibilityToggled m_OnVisibilityToggled;
    OnLockToggled m_OnLockToggled;

    WidgetStyle m_Style;
    bool m_Dragging = false;
    Point m_DragStart;
};

} // namespace HouseEngine::UI
