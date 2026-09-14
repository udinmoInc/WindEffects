// ==============================================================================
// WindEffects — KindUI — Widget
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/UIStateChange.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/UIDirtyRegionTracker.h"
#include "KindUI/Core/LayoutIncremental.h"
#include "KindUI/Diagnostics/PaintCauseLog.h"
#include "KindUI/Core/WidgetContext.h"
#include "KindUI/UI/IPopupHost.h"
#include "KindUI/Diagnostics/UiPathDiagnostics.h"
#include "KindUI/Diagnostics/UiInputLatencyAudit.h"
#include "Theming/StyleClass.h"
#include "KindUI/Theme/ThemeAccess.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include "KindUI/Theme/DesignToken.h"

namespace we::runtime::kindui {

Widget::Diagnostics* Widget::s_GlobalDiagnostics = nullptr;
Widget::PaintRetentionStats Widget::s_PaintRetentionStats{};

uint64_t Widget::EstimateRetainedPaintCapacityBytes() const {
    if (!m_RetainedPaintStore) {
        return 0;
    }
    const auto& store = *std::static_pointer_cast<const std::vector<DrawCommand>>(m_RetainedPaintStore);
    return static_cast<uint64_t>(store.capacity()) * static_cast<uint64_t>(sizeof(DrawCommand));
}

void Widget::ResetDiagnostics() {
    if (s_GlobalDiagnostics) {
        s_GlobalDiagnostics->Reset();
    }
}

namespace {

using RetainedPaintStore = std::vector<DrawCommand>;

struct PaintRetentionFrame {
    struct Range {
        Widget* widget = nullptr;
        uint32_t begin = 0;
        uint32_t end = 0;
    };
    int depth = 0;
    std::vector<Range> ranges;
};

PaintRetentionFrame& RetentionFrame() {
    thread_local PaintRetentionFrame frame;
    return frame;
}

} // namespace

void Widget::ClearStaleRetainedStoresUnder(Widget* root, const std::shared_ptr<void>& keepStore) {
    if (!root) {
        return;
    }
    if (root->m_RetainedPaintStore && root->m_RetainedPaintStore.get() != keepStore.get()) {
        root->m_RetainedPaintValid = false;
        root->m_RetainedPaintStore.reset();
        root->m_RetainedPaintBegin = 0;
        root->m_RetainedPaintEnd = 0;
    }
    for (const auto& child : root->m_Children) {
        ClearStaleRetainedStoresUnder(child.get(), keepStore);
    }
}

void Widget::PaintSubtree(PaintContext& context) {
    if (!m_Visible) {
        if (m_SubtreeNeedsPaint || m_NeedsPaint) {
            ClearSubtreePaintDirty();
        }
        return;
    }

    const bool canReplay = context.IsPaintRetentionEnabled()
        && AllowsPaintRetention()
        && m_RetainedPaintValid
        && static_cast<bool>(m_RetainedPaintStore)
        && !SubtreeNeedsPaint();

    if (canReplay) {
        const auto& store = *std::static_pointer_cast<const RetainedPaintStore>(m_RetainedPaintStore);
        const uint32_t replayBegin = static_cast<uint32_t>(context.CommandCount());
        context.AppendCommands(
            store,
            static_cast<size_t>(m_RetainedPaintBegin),
            static_cast<size_t>(m_RetainedPaintEnd));
        const uint32_t replayEnd = static_cast<uint32_t>(context.CommandCount());
        ++s_PaintRetentionStats.subtreesReplayed;
        s_PaintRetentionStats.commandsReplayed += (m_RetainedPaintEnd - m_RetainedPaintBegin);
        // Only rebase while an outermost paint is collecting ranges (depth > 0).
        // Root-level replay must not append to the thread-local range list.
        auto& frame = RetentionFrame();
        if (frame.depth > 0) {
            frame.ranges.push_back(PaintRetentionFrame::Range{ this, replayBegin, replayEnd });
        }
        return;
    }

    auto& frame = RetentionFrame();
    const size_t commandStart = context.CommandCount();
    ++frame.depth;
    Paint(context);
    --frame.depth;
    ++s_PaintRetentionStats.subtreesPainted;

    ClearSubtreePaintDirty();

    const uint32_t commandEnd = static_cast<uint32_t>(context.CommandCount());
    const uint32_t commandBegin = static_cast<uint32_t>(commandStart);
    frame.ranges.push_back(PaintRetentionFrame::Range{ this, commandBegin, commandEnd });

    // One shared command store per outermost PaintSubtree; widgets keep index slices.
    if (frame.depth == 0) {
        const auto& cmds = context.GetCommands();
        auto store = std::make_shared<RetainedPaintStore>();
        if (commandBegin <= commandEnd && commandBegin <= cmds.size()) {
            const size_t end = (std::min)(static_cast<size_t>(commandEnd), cmds.size());
            store->assign(
                cmds.begin() + static_cast<std::ptrdiff_t>(commandBegin),
                cmds.begin() + static_cast<std::ptrdiff_t>(end));
            // assign() retains capacity — collapse/expand cycles must not keep expand peak forever.
            if (store->capacity() > store->size() * 2u + 64u) {
                store->shrink_to_fit();
            }
        }
        for (const auto& range : frame.ranges) {
            if (!range.widget) {
                continue;
            }
            if (!range.widget->AllowsPaintRetention()) {
                range.widget->m_RetainedPaintValid = false;
                range.widget->m_RetainedPaintStore.reset();
                range.widget->m_RetainedPaintBegin = 0;
                range.widget->m_RetainedPaintEnd = 0;
                continue;
            }
            if (range.begin < commandBegin || range.end > commandEnd || range.begin > range.end) {
                continue;
            }
            range.widget->m_RetainedPaintStore = store;
            range.widget->m_RetainedPaintBegin = range.begin - commandBegin;
            range.widget->m_RetainedPaintEnd = range.end - commandBegin;
            range.widget->m_RetainedPaintValid = true;
        }
        // Drop older shared stores held by widgets not visited this pass (collapsed/hidden).
        ClearStaleRetainedStoresUnder(this, store);
        s_PaintRetentionStats.commandsRecorded += static_cast<uint32_t>(store->size());
        frame.ranges.clear();
    }
}

void Widget::Tick(float deltaTime) {
    if ((!m_Visible && !m_TicksWhenHidden) || m_Children.empty()) {
        return;
    }

    const size_t count = m_Children.size();
    for (size_t i = 0; i < count && i < m_Children.size(); ++i) {
        const auto& child = m_Children[i];
        // Skip invisible children unless they opted into hidden sync (selection chrome, etc.).
        // Keeps hover-damp / MarkAnimating off the hot path for collapsed subtrees.
        if (!child || (!child->IsVisible() && !child->TicksWhenHidden())) {
            continue;
        }
        child->Tick(deltaTime);
    }
}

Size Widget::ClampDesiredSize(const Size& desired) const {
    return Size{
        std::clamp(desired.width, m_MinSize.width, m_MaxSize.width),
        std::clamp(desired.height, m_MinSize.height, m_MaxSize.height)
    };
}

bool Widget::ShouldFireClickOnLeftUp(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return false;
    }
    const bool wasPressed = m_Pressed;
    SetPressed(false);
    return IsEnabled() && (wasPressed || m_Geometry.Contains(event.position));
}

bool Widget::IsEffectivelyVisible() const {
    for (const Widget* w = this; w; ) {
        if (!w->m_Visible) {
            return false;
        }
        const auto parent = w->m_Parent.lock();
        w = parent.get();
    }
    return true;
}

void Widget::InvalidateMeasureCache() {
    m_HasValidMeasureCache = false;
}

void Widget::NoteMeasureCache(const Size& availableSize) {
    m_LastMeasureAvailable = availableSize;
    m_HasValidMeasureCache = true;
}

bool Widget::CanSkipMeasure(const Size& availableSize) const {
    if (m_SubtreeNeedsLayout || !m_HasValidMeasureCache) {
        return false;
    }
    return SizeApproxEqual(availableSize.width, m_LastMeasureAvailable.width)
        && SizeApproxEqual(availableSize.height, m_LastMeasureAvailable.height);
}

bool Widget::CanSkipArrange(const Rect& allottedRect) const {
    if (m_SubtreeNeedsLayout) {
        return false;
    }
    // Containers must run Arrange so they can reposition children (StatusBar flat
    // layout, PanelBodyLayout regions, Flex distribution, etc.). Skipping when
    // only the outer rect matches leaves descendants at stale viewport geometry
    // while ArrangeChild also skips them — black/empty UI and chips at origin.
    if (!m_Children.empty()) {
        return false;
    }
    const bool needsPlacement =
        (allottedRect.width > 0.5f || allottedRect.height > 0.5f)
        && m_Geometry.width <= 0.5f
        && m_Geometry.height <= 0.5f;
    if (needsPlacement) {
        return false;
    }
    return SizeApproxEqual(allottedRect.x, m_Geometry.x)
        && SizeApproxEqual(allottedRect.y, m_Geometry.y)
        && SizeApproxEqual(allottedRect.width, m_Geometry.width)
        && SizeApproxEqual(allottedRect.height, m_Geometry.height);
}

Size Widget::MeasureChild(const std::shared_ptr<Widget>& child, const Size& availableSize) {
    auto& stats = LayoutIncrementalStats::Current();
    ++stats.measureAttempts;
    if (!child) {
        return {};
    }
    if (!child->IsVisible()) {
        ++stats.measureSkipped;
        return {};
    }
    if (child->CanSkipMeasure(availableSize)) {
        ++stats.measureSkipped;
        return child->GetDesiredSize();
    }
    Size desired = child->Measure(availableSize);
    child->NoteMeasureCache(availableSize);
    ++stats.measureRan;
    return desired;
}

void Widget::ArrangeChild(const std::shared_ptr<Widget>& child, const Rect& allottedRect) {
    auto& stats = LayoutIncrementalStats::Current();
    ++stats.arrangeAttempts;
    if (!child) {
        return;
    }
    if (child->CanSkipArrange(allottedRect)) {
        ++stats.arrangeSkipped;
        return;
    }
    child->Arrange(allottedRect);
    ++stats.arrangeRan;
}


void Widget::ReleaseRetainedPaintSubtree() {
    m_RetainedPaintValid = false;
    m_RetainedPaintStore.reset();
    m_RetainedPaintBegin = 0;
    m_RetainedPaintEnd = 0;
    for (auto& child : m_Children) {
        if (child) {
            child->ReleaseRetainedPaintSubtree();
        }
    }
}

void Widget::CommitGeometry(const Rect& rect) {
    const bool moved =
        m_Geometry.x != rect.x
        || m_Geometry.y != rect.y
        || m_Geometry.width != rect.width
        || m_Geometry.height != rect.height;
    if (moved) {
        // Always record the move; do not walk parents here (Arrange can run while the
        // tree is mid-reparent and IsEffectivelyVisible is not safe on every node).
        UIDirtyRegionTracker::Get().AddMove(m_Geometry, rect);
        // Retained draws bake absolute coordinates — moving without a drop leaves
        // chips/icons painted at the old location (status bar over Inspector, etc.).
        ReleaseRetainedPaintSubtree();
        // Ancestors keep a single retained slice that still embeds our old absolute draws.
        // Clearing only this subtree leaves WindowShell/root free to PaintSubtree-replay
        // stale chip commands mid-panel while the real footer geometry is correct.
        auto parent = m_Parent.lock();
        while (parent) {
            parent->m_RetainedPaintValid = false;
            parent->m_RetainedPaintStore.reset();
            parent->m_RetainedPaintBegin = 0;
            parent->m_RetainedPaintEnd = 0;
            parent = parent->m_Parent.lock();
        }
    }
    m_Geometry = rect;
}

void Widget::PaintVisibleChildren(PaintContext& context) {
    for (auto& child : m_Children) {
        if (child && child->IsVisible()) {
            child->PaintSubtree(context);
        }
    }
}

void Widget::SetVisible(bool visible) {
    if (m_Visible == visible) {
        return;
    }
    m_Visible = visible;
    if (!visible) {
        ReleaseRetainedPaintSubtree();
    }
    UIStateChangeGate::Post(*this, StateChangeKind::Visibility);
}

void Widget::SetVisibleSilent(bool visible) {
    if (m_Visible == visible) {
        return;
    }
    m_Visible = visible;
    if (!visible) {
        // Collapse/hide must not keep retained command buffers for hidden subtrees.
        ReleaseRetainedPaintSubtree();
    }
}

void Widget::InvalidateLayout() {
    // Hidden/collapsed subtrees mark dirty bits only — no gate spam, no retained wipe on
    // visible ancestors. Becoming visible (SetVisible / Expansion batch) flushes work.
    InvalidateLayoutImpl(IsEffectivelyVisible());
}

void Widget::InvalidatePaint() {
    InvalidatePaintImpl(IsEffectivelyVisible());
}

void Widget::InvalidateRetainedPaintUpward() {
    for (Widget* w = this; w; ) {
        w->m_RetainedPaintValid = false;
        w->m_RetainedPaintStore.reset();
        w->m_RetainedPaintBegin = 0;
        w->m_RetainedPaintEnd = 0;
        auto parent = w->m_Parent.lock();
        w = parent.get();
    }
}

void Widget::InvalidateRetainedPaintForRepaint() {
    // Descendants own their own retained slices. Clearing only ancestors leaves scrolled
    // or newly-shown children free to PaintSubtree-replay absolute draws at old positions
    // (stuck scroll, one-frame-late expand, black holes).
    ReleaseRetainedPaintSubtree();
    auto parent = m_Parent.lock();
    while (parent) {
        parent->m_RetainedPaintValid = false;
        parent->m_RetainedPaintStore.reset();
        parent->m_RetainedPaintBegin = 0;
        parent->m_RetainedPaintEnd = 0;
        parent = parent->m_Parent.lock();
    }
}

void Widget::InvalidateLayoutImpl(bool armGate) {
    if (armGate) {
        InvalidateRetainedPaintForRepaint();
    }
    InvalidateMeasureCache();

    // Grow / fixed-basis children fill leftover space; their desired-size changes do not
    // rewrite parent intrinsic size. Still mark the immediate parent NeedsLayout so it
    // re-Arranges, but stop NeedsLayout further up (SubtreeNeedsLayout still propagates).
    const bool desiredSizeMayAffectParent = !(m_FlexGrow > 0.0f || m_FlexBasis >= 0.0f);

    // Overlay content (popups/modals/tooltips): isolate from the base editor tree.
    bool underOverlay = m_IsOverlayContent;
    if (!underOverlay) {
        for (auto p = m_Parent.lock(); p; p = p->m_Parent.lock()) {
            if (p->m_IsOverlayContent) {
                underOverlay = true;
                break;
            }
        }
    }

    if (m_NeedsLayout) {
        // Still ensure ancestors carry subtree dirty for incremental walks.
        auto p = m_Parent.lock();
        while (p && !p->m_SubtreeNeedsLayout) {
            p->m_SubtreeNeedsLayout = true;
            p->InvalidateMeasureCache();
            p = p->m_Parent.lock();
        }
        return;
    }
    m_NeedsLayout = true;
    m_SubtreeNeedsLayout = true;
    auto p = m_Parent.lock();
    bool firstParent = true;
    while (p) {
        p->m_SubtreeNeedsLayout = true;
        p->InvalidateMeasureCache();
        // Overlay content must not force NeedsLayout on OverlayHost/base ancestors —
        // that would re-Measure/Arrange the entire editor shell.
        if (armGate && !underOverlay && (firstParent || desiredSizeMayAffectParent)) {
            p->m_NeedsLayout = true;
        }
        firstParent = false;
        p = p->m_Parent.lock();
    }
    if (armGate) {
        if (m_Geometry.IsEmpty()) {
            UIDirtyRegionTracker::Get().MarkFullDirty();
        } else {
            UIDirtyRegionTracker::Get().Add(m_Geometry, 4.0f);
        }
        if (underOverlay) {
            UIRepaintGate::RequestOverlayLayoutReason("OverlayContent");
            UIRepaintGate::RequestPaintReason("OverlayContent");
        } else {
            UIRepaintGate::RequestLayout();
        }
        UiPathDiagnostics::Get().OnLayoutInvalidation();
    }
    if (s_GlobalDiagnostics) {
        ++s_GlobalDiagnostics->invalidateCount;
    }
}

void Widget::InvalidatePaintImpl(bool armGate) {
    if (armGate) {
        InvalidateRetainedPaintForRepaint();
    } else {
        // Offline updates: drop this subtree's slices; keep visible ancestors' retention.
        ReleaseRetainedPaintSubtree();
    }
    if (m_NeedsPaint) {
        return;
    }
    m_NeedsPaint = true;
    m_SubtreeNeedsPaint = true;
    auto p = m_Parent.lock();
    while (p) {
        if (armGate) {
            p->m_NeedsPaint = true;
        }
        p->m_SubtreeNeedsPaint = true;
        p = p->m_Parent.lock();
    }
    if (armGate) {
        if (m_Geometry.IsEmpty()) {
            UIDirtyRegionTracker::Get().MarkFullDirty();
        } else {
            UIDirtyRegionTracker::Get().Add(m_Geometry);
        }
        PaintCauseLog::Get().Push("invalidate", WE_PAINT_CALLER);
        UIRepaintGate::RequestPaint();
        UiPathDiagnostics::Get().OnPaintInvalidation();
        UiInputLatencyAudit::Get().OnInvalidation();
    }
    if (s_GlobalDiagnostics) {
        ++s_GlobalDiagnostics->invalidateCount;
    }
}

bool Widget::SubtreeNeedsPaint() const {
    // O(1) check via aggregate dirty bit propagated upward in InvalidatePaint().
    return m_SubtreeNeedsPaint;
}

bool Widget::SubtreeNeedsLayout() const {
    // O(1) check via aggregate dirty bit propagated upward in InvalidateLayout().
    return m_SubtreeNeedsLayout;
}

void Widget::ClearSubtreePaintDirty() {
    if (!m_NeedsPaint && !m_SubtreeNeedsPaint) {
        return;
    }
    m_NeedsPaint = false;
    m_SubtreeNeedsPaint = false;
    for (auto& child : m_Children) {
        if (child) {
            child->ClearSubtreePaintDirty();
        }
    }
}

void Widget::ClearSubtreeLayoutDirty() {
    m_NeedsLayout = false;
    m_SubtreeNeedsLayout = false;
    for (auto& child : m_Children) {
        if (child) {
            child->ClearSubtreeLayoutDirty();
        }
    }
}

void Widget::InvalidateStyle() {
    m_NeedsStyle = true;
    UIStateChangeGate::Post(*this, StateChangeKind::Style);
}

void Widget::AddChild(const std::shared_ptr<Widget>& child) {
    if (!child) return;

    // Remove from old parent first
    if (auto oldParent = child->GetParent()) {
        oldParent->RemoveChild(child);
    }

    try {
        child->m_Parent = shared_from_this();
    } catch (const std::bad_weak_ptr&) {
        child->m_Parent.reset();
    }
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
        // Prefer RemoveChild so docked AddChild parents are cleared correctly.
        oldParent->RemoveChild(child);
    }
    try {
        child->m_Parent = shared_from_this();
    } catch (const std::bad_weak_ptr&) {
        child->m_Parent.reset();
    }
    if (m_Context) {
        child->SetContext(m_Context);
    }
    m_Children.push_back(child);
    // Overlay attach changes the painted tree without layout — invalidate retention.
    InvalidatePaint();
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
        // Overlay detach changes the painted tree without layout — invalidate retention.
        InvalidatePaint();
    }
}

void Widget::ClearChildren() {
    for (auto& child : m_Children) {
        child->m_Parent.reset();
    }
    m_Children.clear();
    InvalidateLayout();
}

void Widget::ClearChildrenSilent() {
    for (auto& child : m_Children) {
        if (child) {
            child->m_Parent.reset();
        }
    }
    m_Children.clear();
}

void Widget::AddChildSilent(const std::shared_ptr<Widget>& child) {
    if (!child) {
        return;
    }
    if (auto oldParent = child->GetParent()) {
        // Always detach silently — RemoveChild would re-arm UIRepaintGate mid-Arrange.
        auto it = std::find(oldParent->m_Children.begin(), oldParent->m_Children.end(), child);
        if (it != oldParent->m_Children.end()) {
            oldParent->m_Children.erase(it);
        }
        child->m_Parent.reset();
    }
    try {
        child->m_Parent = shared_from_this();
    } catch (const std::bad_weak_ptr&) {
        child->m_Parent.reset();
    }
    if (m_Context) {
        child->SetContext(m_Context);
    }
    m_Children.push_back(child);
}

void Widget::SetContext(std::shared_ptr<IWidgetContext> context) {
    m_Context = std::move(context);
    try {
        auto self = shared_from_this();
        for (auto& child : m_Children) {
            if (child) {
                child->m_Parent = self;
            }
        }
    } catch (const std::bad_weak_ptr&) {
    }
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
    return StyleResolve::ResolveWithState(
        m_StyleClass,
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
    m_NeedsStyle = true;
    UIStateChangeGate::Post(*this, StateChangeKind::Enabled);
}

void Widget::SetSelected(bool selected) {
    if (m_Selected == selected) return;
    m_Selected = selected;
    m_NeedsStyle = true;
    UIStateChangeGate::Post(*this, StateChangeKind::Selection);
}

void Widget::SetLoading(bool loading) {
    if (m_Loading == loading) return;
    m_Loading = loading;
    UIStateChangeGate::Post(*this, StateChangeKind::Animation);
}

void Widget::SetCollapsed(bool collapsed) {
    if (m_Collapsed == collapsed) return;
    m_Collapsed = collapsed;
    SetVisible(!collapsed); // SetVisible calls InvalidateLayout() + InvalidatePaint() internally.
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

