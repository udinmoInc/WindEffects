#pragma once

#include "Application/Export.hpp"

#include "Core/Widget.hpp"
#include <memory>
#include <vector>
#include <string>

namespace we::UI {

enum class DockDirection {
    Left,
    Right,
    Top,
    Bottom,
    Center // Tabbed
};

struct DockNode {
    std::string Id;
    bool IsSplit = false;
    DockDirection SplitDirection = DockDirection::Left;
    float SplitRatio = 0.5f;
    
    std::shared_ptr<DockNode> ChildA;
    std::shared_ptr<DockNode> ChildB;
    
    std::vector<std::shared_ptr<Widget>> Tabs;
    int ActiveTab = 0;
};

class APPLICATION_API DockSpace : public Widget {
public:
    DockSpace();
    virtual ~DockSpace() = default;

    virtual Size Measure(const Size& availableSize) override;
    virtual void Arrange(const Rect& allottedRect) override;
    virtual void Paint(PaintContext& context) override;

    // Events
    virtual void OnMouseDown(const MouseEvent& event) override;
    virtual void OnMouseMove(const MouseEvent& event) override;
    virtual void OnMouseUp(const MouseEvent& event) override;

    void DockWidget(std::shared_ptr<Widget> widget, DockDirection direction, const std::string& targetNodeId = "");

private:
    void ArrangeNode(std::shared_ptr<DockNode> node, const Rect& rect);
    void PaintNode(std::shared_ptr<DockNode> node, PaintContext& context, const Rect& rect);

    std::shared_ptr<DockNode> m_RootNode;
    
    // Drag and Drop state
    bool m_IsDragging = false;
    std::shared_ptr<Widget> m_DraggedWidget;
    DockDirection m_PreviewDirection = DockDirection::Center;
    Rect m_PreviewRect;
};

} // namespace we::editor::application::UI
