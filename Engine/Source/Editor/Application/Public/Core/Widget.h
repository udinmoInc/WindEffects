#pragma once

#include "Application/Export.h"

#include "Core/Geometry.h"
#include "Core/EventSystem.h"
#include <memory>
#include <vector>
#include <string>

namespace we::UI {

class PaintContext;

enum class HorizontalAlignment {
    Left,
    Center,
    Right,
    Fill
};

enum class VerticalAlignment {
    Top,
    Center,
    Bottom,
    Fill
};

class APPLICATION_API Widget : public std::enable_shared_from_this<Widget> {
public:
    Widget() = default;
    virtual ~Widget() = default;

    virtual void Construct() {}
    virtual void Tick(float deltaTime);

    // Layout engine methods
    virtual Size Measure(const Size& availableSize) = 0;
    virtual void Arrange(const Rect& allottedRect) = 0;

    // Paint pass
    virtual void Paint(PaintContext& context) = 0;

    // Event handlers
    virtual void OnMouseDown(const MouseEvent&) {}
    virtual void OnMouseMove(const MouseEvent&) {}
    virtual void OnMouseUp(const MouseEvent&) {}
    virtual void OnMouseWheel(const MouseEvent&) {}
    virtual void OnKeyDown(const KeyEvent&) {}
    virtual bool ShowsPointerCursor(const Point&) const { return false; }
    virtual void OnFocus() { m_Focused = true; }
    virtual void OnBlur() { m_Focused = false; }
    
    // Deferred callback execution (called after event processing to avoid use-after-free)
    virtual void ExecutePendingCallback() {}
    
    // State management
    virtual void SetActive(bool active) { m_IsActive = active; }
    virtual bool IsActive() const { return m_IsActive; }

    static uint32_t s_TotalWidgetCount;
    static uint32_t s_VisibleWidgetCount;
    static uint32_t s_HiddenWidgetCount;
    static uint32_t s_PaintCalls;
    static uint32_t s_ArrangeChildrenCalls;
    static uint32_t s_LayoutPassCount;
    static uint32_t s_InvalidateCount;
    static uint32_t s_WidgetsPainted;
    static void ResetDiagnostics();

    // Child management
    void AddChild(const std::shared_ptr<Widget>& child);
    void RemoveChild(const std::shared_ptr<Widget>& child);
    void ClearChildren();

    // Getters and setters
    const std::vector<std::shared_ptr<Widget>>& GetChildren() const { return m_Children; }
    std::shared_ptr<Widget> GetParent() const { return m_Parent.lock(); }

    const Rect& GetGeometry() const { return m_Geometry; }
    const Size& GetDesiredSize() const { return m_DesiredSize; }

    bool IsFocused() const { return m_Focused; }
    bool IsHovered() const { return m_Hovered; }
    void SetHovered(bool hovered) { m_Hovered = hovered; }

    // Visibility
    bool IsVisible() const { return m_Visible; }
    void SetVisible(bool visible) { m_Visible = visible; }

    // Alignment
    HorizontalAlignment GetHorizontalAlignment() const { return m_HAlign; }
    void SetHorizontalAlignment(HorizontalAlignment align) { m_HAlign = align; }

    VerticalAlignment GetVerticalAlignment() const { return m_VAlign; }
    void SetVerticalAlignment(VerticalAlignment align) { m_VAlign = align; }

protected:
    std::weak_ptr<Widget> m_Parent;
    std::vector<std::shared_ptr<Widget>> m_Children;

    Rect m_Geometry;
    Size m_DesiredSize;

    bool m_Focused = false;
    bool m_Hovered = false;
    bool m_Visible = true;
    bool m_IsActive = false;

    HorizontalAlignment m_HAlign = HorizontalAlignment::Fill;
    VerticalAlignment m_VAlign = VerticalAlignment::Center;
};

} // namespace we::editor::application::UI
