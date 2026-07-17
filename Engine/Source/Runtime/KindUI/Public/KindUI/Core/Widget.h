#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Geometry.h"
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Theming/IThemeProvider.h"

#include <memory>
#include <string>
#include <vector>

namespace we::runtime::kindui {

class PaintContext;
class IWidgetContext;
class IPopupHost;

#pragma warning(push)
#pragma warning(disable: 4251 4275)

class KINDUI_API Widget : public std::enable_shared_from_this<Widget> {
public:
    Widget() = default;
    virtual ~Widget() = default;

    virtual void Construct() {}
    virtual void Tick(float deltaTime);

    virtual Size Measure(const Size& availableSize) = 0;
    virtual void Arrange(const Rect& allottedRect) = 0;
    virtual void Paint(PaintContext& context) = 0;

    virtual void OnMouseDown(const MouseEvent&) {}
    virtual void OnMouseMove(const MouseEvent&) {}
    virtual void OnMouseUp(const MouseEvent&) {}
    virtual void OnMouseWheel(const MouseEvent&) {}
    virtual void OnKeyDown(const KeyEvent&) {}
    virtual void OnKeyUp(const KeyEvent&) {}
    virtual bool ShowsPointerCursor(const Point&) const { return false; }
    virtual void OnFocus() { m_Focused = true; }
    virtual void OnBlur() { m_Focused = false; }

    virtual void ExecutePendingCallback() {}

    /// Tab-order / keyboard focus eligibility. Layout containers return false by default.
    [[nodiscard]] virtual bool IsFocusable() const { return m_Focusable; }
    virtual void SetFocusable(bool focusable) { m_Focusable = focusable; }

    /// Whether this widget accepts interaction (distinct from tab-active state).
    [[nodiscard]] virtual bool IsEnabled() const { return m_Enabled && m_IsActive; }
    virtual void SetEnabled(bool enabled);

    virtual void SetActive(bool active) { m_IsActive = active; }
    virtual bool IsActive() const { return m_IsActive; }

    [[nodiscard]] bool IsFocused() const { return m_Focused; }
    [[nodiscard]] bool IsHovered() const { return m_Hovered; }
    void SetHovered(bool hovered) { m_Hovered = hovered; }

    [[nodiscard]] bool IsPressed() const { return m_Pressed; }
    void SetPressed(bool pressed) { m_Pressed = pressed; InvalidatePaint(); }

    [[nodiscard]] bool IsSelected() const { return m_Selected; }
    void SetSelected(bool selected);

    [[nodiscard]] bool IsLoading() const { return m_Loading; }
    void SetLoading(bool loading);

    [[nodiscard]] bool IsReadOnly() const { return m_ReadOnly; }
    void SetReadOnly(bool readOnly) { m_ReadOnly = readOnly; InvalidatePaint(); }

    [[nodiscard]] bool IsCollapsed() const { return m_Collapsed; }
    void SetCollapsed(bool collapsed);

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

    static Diagnostics* s_GlobalDiagnostics;
    static void ResetDiagnostics();

    void AddChild(const std::shared_ptr<Widget>& child);
    void RemoveChild(const std::shared_ptr<Widget>& child);
    void ClearChildren();

    const std::vector<std::shared_ptr<Widget>>& GetChildren() const { return m_Children; }
    std::shared_ptr<Widget> GetParent() const { return m_Parent.lock(); }

    const Rect& GetGeometry() const { return m_Geometry; }
    const Size& GetDesiredSize() const { return m_DesiredSize; }

    bool IsVisible() const { return m_Visible; }
    void SetVisible(bool visible) {
        if (m_Visible == visible) return;
        m_Visible = visible;
        InvalidateLayout();
        InvalidatePaint();
    }

    HorizontalAlignment GetHorizontalAlignment() const { return m_HAlign; }
    void SetHorizontalAlignment(HorizontalAlignment align) { m_HAlign = align; InvalidateLayout(); }

    VerticalAlignment GetVerticalAlignment() const { return m_VAlign; }
    void SetVerticalAlignment(VerticalAlignment align) { m_VAlign = align; InvalidateLayout(); }

    void SetMargin(const Margin& margin) { m_Margin = margin; InvalidateLayout(); }
    [[nodiscard]] const Margin& GetMargin() const { return m_Margin; }

    void SetMinSize(const Size& size) { m_MinSize = size; InvalidateLayout(); }
    void SetMaxSize(const Size& size) { m_MaxSize = size; InvalidateLayout(); }
    [[nodiscard]] const Size& GetMinSize() const { return m_MinSize; }
    [[nodiscard]] const Size& GetMaxSize() const { return m_MaxSize; }
    [[nodiscard]] Size ClampDesiredSize(const Size& desired) const;

    void SetFlexGrow(float grow) { m_FlexGrow = grow; InvalidateLayout(); }
    void SetFlexShrink(float shrink) { m_FlexShrink = shrink; InvalidateLayout(); }
    void SetFlexBasis(float basis) { m_FlexBasis = basis; InvalidateLayout(); }
    [[nodiscard]] float GetFlexGrow() const { return m_FlexGrow; }
    [[nodiscard]] float GetFlexShrink() const { return m_FlexShrink; }
    [[nodiscard]] float GetFlexBasis() const { return m_FlexBasis; }

    void SetStyleClass(std::string className) { m_StyleClass = std::move(className); InvalidateStyle(); }
    [[nodiscard]] const std::string& GetStyleClass() const { return m_StyleClass; }

    void InvalidateLayout();
    void InvalidatePaint();
    void InvalidateStyle();
    [[nodiscard]] bool NeedsLayout() const { return m_NeedsLayout; }
    [[nodiscard]] bool NeedsPaint() const { return m_NeedsPaint; }
    [[nodiscard]] bool NeedsStyle() const { return m_NeedsStyle; }
    void ClearLayoutDirty() { m_NeedsLayout = false; }
    void ClearPaintDirty() { m_NeedsPaint = false; }
    void ClearStyleDirty() { m_NeedsStyle = false; }

    void SetContext(std::shared_ptr<IWidgetContext> context);
    [[nodiscard]] IWidgetContext* GetContext() const { return m_Context.get(); }
    [[nodiscard]] IStyleResolver& Styles() const;
    [[nodiscard]] Color ThemeColor(ThemeToken token) const;
    [[nodiscard]] float ThemeMetric(ThemeToken token) const;
    [[nodiscard]] Margin ThemePadding(ThemeToken token) const;
    [[nodiscard]] ResolvedStyle ResolveStyle(StyleRole role) const;
    [[nodiscard]] ResolvedStyle ResolveStyleClass() const;
    [[nodiscard]] ResolvedStyle ResolveEffectiveStyle(StyleRole fallbackRole) const;
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
    bool m_Pressed = false;
    bool m_Selected = false;
    bool m_Loading = false;
    bool m_ReadOnly = false;
    bool m_Collapsed = false;
    bool m_Focusable = false;
    bool m_Enabled = true;
    bool m_Visible = true;
    bool m_IsActive = true;
    bool m_NeedsLayout = true;
    bool m_NeedsPaint = true;
    bool m_NeedsStyle = true;

    HorizontalAlignment m_HAlign = HorizontalAlignment::Fill;
    VerticalAlignment m_VAlign = VerticalAlignment::Center;

    Margin m_Margin{};
    Size m_MinSize{ 0.0f, 0.0f };
    Size m_MaxSize{ 1.0e9f, 1.0e9f };
    float m_FlexGrow = 0.0f;
    float m_FlexShrink = 1.0f;
    float m_FlexBasis = -1.0f;
#pragma warning(push)
#pragma warning(disable: 4251)
    std::string m_StyleClass;
#pragma warning(pop)
};

#pragma warning(pop)

} // namespace we::runtime::kindui
