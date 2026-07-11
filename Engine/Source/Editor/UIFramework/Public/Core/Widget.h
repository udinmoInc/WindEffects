#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "Core/Geometry.h"
#include "Core/EventSystem.h"
#include "WindEffects/Editor/UI/Theming/IThemeProvider.h"

#include <memory>
#include <vector>
#include <string>

namespace WindEffects::Editor::UI {

class PaintContext;
class IWidgetContext;
class IPopupHost;

#pragma warning(push)
#pragma warning(disable: 4251 4275)

class UIFRAMEWORK_API Widget : public std::enable_shared_from_this<Widget> {
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

    // Diagnostics (moved from static to instance-level to avoid global state)
    struct Diagnostics {
        uint32_t totalWidgetCount = 0;
        uint32_t visibleWidgetCount = 0;
        uint32_t hiddenWidgetCount = 0;
        uint32_t paintCalls = 0;
        uint32_t arrangeChildrenCalls = 0;
        uint32_t layoutPassCount = 0;
        uint32_t invalidateCount = 0;
        uint32_t widgetsPainted = 0;
        
        void Reset() {
            totalWidgetCount = 0;
            visibleWidgetCount = 0;
            hiddenWidgetCount = 0;
            paintCalls = 0;
            arrangeChildrenCalls = 0;
            layoutPassCount = 0;
            invalidateCount = 0;
            widgetsPainted = 0;
        }
    };
    
    // Optional diagnostics pointer (set by owner for tracking)
    static Diagnostics* s_GlobalDiagnostics;
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

    void SetContext(std::shared_ptr<IWidgetContext> context);
    [[nodiscard]] IWidgetContext* GetContext() const { return m_Context.get(); }
    [[nodiscard]] IStyleResolver& Styles() const;
    [[nodiscard]] Color ThemeColor(ThemeToken token) const;
    [[nodiscard]] float ThemeMetric(ThemeToken token) const;
    [[nodiscard]] Margin ThemePadding(ThemeToken token) const;
    [[nodiscard]] ResolvedStyle ResolveStyle(StyleRole role) const;
    [[nodiscard]] float Scaled(float logicalValue) const;
    [[nodiscard]] IThemeProvider& Theme() const;
    [[nodiscard]] Color ThemeInteractiveBackground(float hoverAnim, float pressAnim, bool selected = false) const;
    [[nodiscard]] Color ThemeTextForState(bool hovered, bool active = false) const;
    [[nodiscard]] Color ThemeIconForState(bool hovered, bool active = false) const;

    [[nodiscard]] IPopupHost* GetPopupHost() const;

protected:
#pragma warning(push)
#pragma warning(disable: 4251)
    std::weak_ptr<Widget> m_Parent;
    std::shared_ptr<IWidgetContext> m_Context;
    std::vector<std::shared_ptr<Widget>> m_Children;
#pragma warning(pop)

    Rect m_Geometry;
    Size m_DesiredSize;

    bool m_Focused = false;
    bool m_Hovered = false;
    bool m_Visible = true;
    bool m_IsActive = true;

    HorizontalAlignment m_HAlign = HorizontalAlignment::Fill;
    VerticalAlignment m_VAlign = VerticalAlignment::Center;
};

#pragma warning(pop)

} // namespace WindEffects::Editor::UI
