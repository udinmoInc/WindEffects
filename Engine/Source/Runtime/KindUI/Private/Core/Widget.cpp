#include "KindUI/Core/Widget.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/WidgetContext.h"
#include "KindUI/Layout/IPopupHost.h"
#include "KindUI/Profiling/UiPathDiagnostics.h"
#include "KindUI/Profiling/UiInputLatencyAudit.h"
#include "KindUI/Theming/StyleResolve.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/StyleClass.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "KindUI/Tokens/DesignToken.h"

namespace we::runtime::kindui {

Widget::Diagnostics* Widget::s_GlobalDiagnostics = nullptr;

void Widget::ResetDiagnostics() {
    if (s_GlobalDiagnostics) {
        s_GlobalDiagnostics->Reset();
    }
}

void Widget::Tick(float deltaTime) {
    if (!m_Visible || m_Children.empty()) return;

    const size_t count = m_Children.size();
    for (size_t i = 0; i < count && i < m_Children.size(); ++i) {
        if (const auto& child = m_Children[i]) {
            child->Tick(deltaTime);
        }
    }
}

Size Widget::ClampDesiredSize(const Size& desired) const {
    return Size{
        std::clamp(desired.width, m_MinSize.width, m_MaxSize.width),
        std::clamp(desired.height, m_MinSize.height, m_MaxSize.height)
    };
}

void Widget::InvalidateLayout() {
    if (m_NeedsLayout) {
        return;
    }
    m_NeedsLayout = true;
    UIRepaintGate::RequestLayout();
    UiPathDiagnostics::Get().OnLayoutInvalidation();
    if (s_GlobalDiagnostics) {
        ++s_GlobalDiagnostics->invalidateCount;
    }
}

void Widget::InvalidatePaint() {
    if (m_NeedsPaint) {
        return;
    }
    m_NeedsPaint = true;
    UIRepaintGate::RequestPaint();
    UiPathDiagnostics::Get().OnPaintInvalidation();
    UiInputLatencyAudit::Get().OnInvalidation();
    if (s_GlobalDiagnostics) {
        ++s_GlobalDiagnostics->invalidateCount;
    }
}

bool Widget::SubtreeNeedsPaint() const {
    if (m_NeedsPaint) {
        return true;
    }
    for (const auto& child : m_Children) {
        if (child && child->SubtreeNeedsPaint()) {
            return true;
        }
    }
    return false;
}

bool Widget::SubtreeNeedsLayout() const {
    if (m_NeedsLayout) {
        return true;
    }
    for (const auto& child : m_Children) {
        if (child && child->SubtreeNeedsLayout()) {
            return true;
        }
    }
    return false;
}

void Widget::ClearSubtreePaintDirty() {
    m_NeedsPaint = false;
    for (auto& child : m_Children) {
        if (child) {
            child->ClearSubtreePaintDirty();
        }
    }
}

void Widget::ClearSubtreeLayoutDirty() {
    m_NeedsLayout = false;
    for (auto& child : m_Children) {
        if (child) {
            child->ClearSubtreeLayoutDirty();
        }
    }
}

void Widget::InvalidateStyle() {
    m_NeedsStyle = true;
    InvalidatePaint();
}

void Widget::AddChild(const std::shared_ptr<Widget>& child) {
    if (!child) return;

    // Remove from old parent first
    if (auto oldParent = child->GetParent()) {
        oldParent->RemoveChild(child);
    }

    child->m_Parent = shared_from_this();
    if (m_Context) {
        child->SetContext(m_Context);
    }
    m_Children.push_back(child);
    InvalidateLayout();
}

void Widget::AttachOverlayChild(const std::shared_ptr<Widget>& child) {
    if (!child) {
        return;
    }
    if (auto oldParent = child->GetParent()) {
        oldParent->DetachOverlayChild(child);
    }
    child->m_Parent = shared_from_this();
    if (m_Context) {
        child->SetContext(m_Context);
    }
    m_Children.push_back(child);
    UIRepaintGate::RequestPaint();
    UiPathDiagnostics::Get().OnPaintInvalidation();
}

void Widget::RemoveChild(const std::shared_ptr<Widget>& child) {
    if (!child) return;

    auto it = std::find(m_Children.begin(), m_Children.end(), child);
    if (it != m_Children.end()) {
        child->m_Parent.reset();
        m_Children.erase(it);
        InvalidateLayout();
    }
}

void Widget::DetachOverlayChild(const std::shared_ptr<Widget>& child) {
    if (!child) {
        return;
    }
    auto it = std::find(m_Children.begin(), m_Children.end(), child);
    if (it != m_Children.end()) {
        child->m_Parent.reset();
        m_Children.erase(it);
        UIRepaintGate::RequestPaint();
        UiPathDiagnostics::Get().OnPaintInvalidation();
    }
}

void Widget::ClearChildren() {
    for (auto& child : m_Children) {
        child->m_Parent.reset();
    }
    m_Children.clear();
    InvalidateLayout();
}

void Widget::SetContext(std::shared_ptr<IWidgetContext> context) {
    m_Context = std::move(context);
    for (auto& child : m_Children) {
        if (child) {
            child->SetContext(m_Context);
        }
    }
}

IStyleResolver& Widget::Styles() const {
    if (!m_Context) {
        throw std::runtime_error("Widget accessed style system without context");
    }
    return m_Context->GetStyleResolver();
}

Color Widget::ThemeColor(ColorToken token) const {
    if (!m_Context) {
        return ResolveColor(token);
    }
    return m_Context->GetTheme().ResolveColor(token);
}

float Widget::ThemeMetric(MetricToken token) const {
    if (!m_Context) {
        return ResolveMetric(token);
    }
    return m_Context->GetTheme().ResolveMetric(token);
}

Margin Widget::ThemePadding(PaddingToken token) const {
    if (!m_Context) {
        return ResolvePadding(token);
    }
    return m_Context->GetTheme().ResolvePadding(token);
}

ResolvedStyle Widget::ResolveStyle(StyleRole role) const {
    return Styles().Resolve(role);
}

ResolvedStyle Widget::ResolveStyleClass() const {
    if (m_StyleClass.empty()) {
        return {};
    }
    const StyleClass cls = StyleClassRegistry::Get().Resolve(m_StyleClass);
    const ResolvedStyle base = Styles().ResolveClass(m_StyleClass);
    return StyleResolve::ApplyState(
        base,
        cls,
        Theme(),
        Styles().GetDpiScale(),
        m_Hovered,
        m_Pressed,
        !IsEnabled(),
        m_Selected);
}

ResolvedStyle Widget::ResolveEffectiveStyle(StyleRole fallbackRole) const {
    if (!m_StyleClass.empty()) {
        return ResolveStyleClass();
    }
    return ResolveStyle(fallbackRole);
}

void Widget::SetEnabled(bool enabled) {
    if (m_Enabled == enabled) return;
    m_Enabled = enabled;
    InvalidateStyle();
    InvalidatePaint();
}

void Widget::SetSelected(bool selected) {
    if (m_Selected == selected) return;
    m_Selected = selected;
    InvalidateStyle();
    InvalidatePaint();
}

void Widget::SetLoading(bool loading) {
    if (m_Loading == loading) return;
    m_Loading = loading;
    InvalidatePaint();
}

void Widget::SetCollapsed(bool collapsed) {
    if (m_Collapsed == collapsed) return;
    m_Collapsed = collapsed;
    SetVisible(!collapsed);
    InvalidateLayout();
}

float Widget::Scaled(float logicalValue) const {
    return Styles().Scaled(logicalValue);
}

IKindUITheme& Widget::Theme() const {
    if (!m_Context) {
        return ResolveDefaultTheme();
    }
    return m_Context->GetTheme();
}

Color Widget::ThemeInteractiveBackground(float hoverAnim, float pressAnim, bool selected) const {
    if (!m_Context) {
        return ResolveInteractiveBackground(hoverAnim, pressAnim, selected);
    }
    return m_Context->GetTheme().InteractiveBackground(hoverAnim, pressAnim, selected);
}

Color Widget::ThemeTextForState(bool hovered, bool active) const {
    if (!m_Context) {
        return ResolveTextForState(hovered, active);
    }
    return m_Context->GetTheme().TextForState(hovered, active);
}

Color Widget::ThemeIconForState(bool hovered, bool active) const {
    if (!m_Context) {
        return ResolveIconForState(hovered, active);
    }
    return m_Context->GetTheme().IconForState(hovered, active);
}

IPopupHost* Widget::GetPopupHost() const {
    return m_Context ? m_Context->GetPopupHost() : nullptr;
}

namespace {

bool PointInClip(const Point& pos, const Rect* clip) {
    return clip == nullptr || clip->Contains(pos);
}

Rect EffectiveChildClip(const Rect* parentClip, const std::optional<Rect>& localClip) {
    if (!localClip.has_value()) {
        return parentClip ? *parentClip : Rect{};
    }
    if (!parentClip) {
        return *localClip;
    }
    return localClip->Intersect(*parentClip);
}

} // namespace

std::shared_ptr<Widget> Widget::HitTestChildren(const Point& pos, const Rect* clip) const {
    for (auto it = m_Children.rbegin(); it != m_Children.rend(); ++it) {
        if (!*it) {
            continue;
        }
        if (auto hit = (*it)->HitTestPoint(pos, clip)) {
            return hit;
        }
    }
    return nullptr;
}

std::shared_ptr<Widget> Widget::HitTestPoint(const Point& pos, const Rect* clip) {
    if (!IsVisible() || IsPointerTransparent() || !IsEnabled()) {
        return nullptr;
    }
    if (!PointInClip(pos, clip) || !m_Geometry.Contains(pos)) {
        return nullptr;
    }

  const std::optional<Rect> localClip = GetHitTestClipRect();
    const Rect effectiveClip = EffectiveChildClip(clip, localClip);
    const Rect* childClip = clip;
    if (localClip.has_value() || clip != nullptr) {
        if (effectiveClip.IsEmpty() || !effectiveClip.Contains(pos)) {
            return nullptr;
        }
        childClip = &effectiveClip;
    }

    if (auto hit = HitTestChildren(pos, childClip)) {
        return hit;
    }

    if (IsInteractiveContainer()) {
        return shared_from_this();
    }

    // Leaf widgets still participate in hit testing so clicks route to their handlers.
    if (m_Children.empty()) {
        return shared_from_this();
    }

    return nullptr;
}

} // namespace we::runtime::kindui

