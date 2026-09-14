// ==============================================================================
// WindEffects — KindUI — UIStateChange
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/UIStateChange.h"

#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/UIDirtyRegionTracker.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Diagnostics/PaintCauseLog.h"
#include "KindUI/Diagnostics/UiInputLatencyAudit.h"
#include "KindUI/Diagnostics/UiPathDiagnostics.h"

namespace we::runtime::kindui {
namespace {

thread_local int t_TransactionDepth = 0;
thread_local bool t_DeferredLayout = false;
thread_local bool t_DeferredPaint = false;

UIStateChangeGate::FrameStats s_FrameStats{};

[[nodiscard]] StateInvalidation ClassifyInvalidation(StateChangeKind kind) {
    switch (kind) {
    case StateChangeKind::Expansion:
    case StateChangeKind::Visibility:
        return StateInvalidation::LayoutAndPaint;
    case StateChangeKind::Hover:
    case StateChangeKind::Pressed:
    case StateChangeKind::Focus:
    case StateChangeKind::Selection:
    case StateChangeKind::Property:
    case StateChangeKind::Value:
    case StateChangeKind::Style:
    case StateChangeKind::Enabled:
    case StateChangeKind::Animation:
        return StateInvalidation::Paint;
    }
    return StateInvalidation::Paint;
}

[[nodiscard]] const char* ReasonFor(StateChangeKind kind) {
    switch (kind) {
    case StateChangeKind::Hover: return "Hover";
    case StateChangeKind::Pressed: return "Pressed";
    case StateChangeKind::Focus: return "Focus";
    case StateChangeKind::Selection: return "Selection";
    case StateChangeKind::Property: return "Property";
    case StateChangeKind::Value: return "Value";
    case StateChangeKind::Style: return "Style";
    case StateChangeKind::Enabled: return "Enabled";
    case StateChangeKind::Expansion: return "Expansion";
    case StateChangeKind::Visibility: return "Visibility";
    case StateChangeKind::Animation: return "Animation";
    }
    return "State";
}

} // namespace

bool UIStateChangeGate::ApplyPaintChange(Widget& widget, StateChangeKind kind) {
    const bool visible = widget.IsEffectivelyVisible();
    if (visible) {
        widget.InvalidateRetainedPaintForRepaint();
    }
    if (widget.NeedsPaint()) {
        return false;
    }

    const bool inTx = InTransaction();
    const bool armGate = visible && !inTx;
    widget.InvalidatePaintImpl(armGate);
    if (visible) {
        if (inTx) {
            t_DeferredPaint = true;
        } else {
            ++s_FrameStats.gatePaintArms;
        }
        if (UiInputLatencyAudit::IsEnabled()) {
            UiInputLatencyAudit::Get().OnInvalidation();
        }
    }
    (void)kind;
    return true;
}

void UIStateChangeGate::ApplyVisibilityChange(Widget& widget) {
    const bool inTx = InTransaction();
    const bool armGate = !inTx;

    if (!widget.NeedsLayout()) {
        widget.InvalidateLayoutImpl(armGate);
        if (inTx) {
            t_DeferredLayout = true;
        } else {
            ++s_FrameStats.gateLayoutArms;
        }
        ++s_FrameStats.widgetsLayout;
    } else {
        ++s_FrameStats.deduped;
    }

    if (!widget.NeedsPaint()) {
        widget.InvalidatePaintImpl(armGate);
        if (inTx) {
            t_DeferredPaint = true;
        } else {
            ++s_FrameStats.gatePaintArms;
        }
        ++s_FrameStats.widgetsPaint;
        if (UiInputLatencyAudit::IsEnabled()) {
            UiInputLatencyAudit::Get().OnInvalidation();
        }
    } else {
        ++s_FrameStats.deduped;
    }
}

bool UIStateChangeGate::ApplyLayoutChange(Widget& widget, StateChangeKind kind) {
    const bool visible = widget.IsEffectivelyVisible();
    if (widget.NeedsLayout()) {
        return false;
    }

    const bool inTx = InTransaction();
    const bool armGate = visible && !inTx;
    widget.InvalidateLayoutImpl(armGate);
    if (visible) {
        if (inTx) {
            t_DeferredLayout = true;
        } else {
            ++s_FrameStats.gateLayoutArms;
        }
    }
    (void)kind;
    return true;
}

void UIStateChangeGate::Post(Widget& widget, StateChangeKind kind) {
    (void)Notify(widget, kind);
}

StateInvalidation UIStateChangeGate::Notify(Widget& widget, StateChangeKind kind) {
    ++s_FrameStats.notifications;
    const StateInvalidation inv = ClassifyInvalidation(kind);
    switch (inv) {
    case StateInvalidation::None:
        return StateInvalidation::None;
    case StateInvalidation::Paint:
        if (ApplyPaintChange(widget, kind)) {
            ++s_FrameStats.widgetsPaint;
        } else {
            ++s_FrameStats.deduped;
        }
        return inv;
    case StateInvalidation::LayoutAndPaint:
        if (kind == StateChangeKind::Visibility) {
            ApplyVisibilityChange(widget);
        } else {
            if (ApplyLayoutChange(widget, kind)) {
                ++s_FrameStats.widgetsLayout;
            } else {
                ++s_FrameStats.deduped;
            }
            if (ApplyPaintChange(widget, kind)) {
                ++s_FrameStats.widgetsPaint;
            } else {
                ++s_FrameStats.deduped;
            }
        }
        return inv;
    }
    return StateInvalidation::None;
}

UIStateChangeGate::ScopedTransaction::ScopedTransaction(const char* reason)
    : m_Reason(reason && reason[0] ? reason : "StateBatch")
{
    if (t_TransactionDepth == 0) {
        UIRepaintGate::BeginBatch();
        t_DeferredLayout = false;
        t_DeferredPaint = false;
        m_Outer = true;
        ++s_FrameStats.transactions;
    }
    ++t_TransactionDepth;
}

UIStateChangeGate::ScopedTransaction::~ScopedTransaction() {
    if (t_TransactionDepth > 0) {
        --t_TransactionDepth;
    }
    if (!m_Outer) {
        return;
    }
    CommitTransaction(m_Reason, t_DeferredLayout, t_DeferredPaint);
    t_DeferredLayout = false;
    t_DeferredPaint = false;
    UIRepaintGate::EndBatch();
}

void UIStateChangeGate::CommitTransaction(const char* reason, bool deferredLayout, bool deferredPaint) {
    if (deferredLayout) {
        UIRepaintGate::RequestLayoutReason(reason);
        ++s_FrameStats.gateLayoutArms;
    }
    if (deferredPaint) {
        UIRepaintGate::RequestPaintReason(reason);
        ++s_FrameStats.gatePaintArms;
    }
}

bool UIStateChangeGate::InTransaction() {
    return t_TransactionDepth > 0 || UIRepaintGate::InBatch();
}

void UIStateChangeGate::ResetFrameStats() {
    s_FrameStats = {};
}

const UIStateChangeGate::FrameStats& UIStateChangeGate::CurrentFrameStats() {
    return s_FrameStats;
}

const char* UIStateChangeGate::KindName(StateChangeKind kind) {
    return ReasonFor(kind);
}

const char* UIStateChangeGate::InvalidationName(StateInvalidation inv) {
    switch (inv) {
    case StateInvalidation::None: return "None";
    case StateInvalidation::Paint: return "Paint";
    case StateInvalidation::LayoutAndPaint: return "LayoutAndPaint";
    }
    return "Unknown";
}

} // namespace we::runtime::kindui
