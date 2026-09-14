// ==============================================================================
// WindEffects — KindUI — UIStateChange
// Canonical state-change notification and invalidation routing.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

class Widget;

/// Semantic interaction / presentation state change (not structural layout).
enum class StateChangeKind : uint8_t {
    Hover,
    Pressed,
    Focus,
    Selection,
    Property,
    Value,
    Style,
    Enabled,
    Expansion,
    Visibility,
    Animation,
};

/// What invalidation work a state change requires.
enum class StateInvalidation : uint8_t {
    None,
    Paint,
    LayoutAndPaint,
};

/// Central router for widget state changes. Separates state invalidation from
/// ad-hoc InvalidatePaint/InvalidateLayout calls, coalesces gate arms inside
/// transactions, and deduplicates per-widget paint/layout dirty marking.
class KINDUI_API UIStateChangeGate {
public:
    struct FrameStats {
        uint32_t notifications = 0;
        uint32_t deduped = 0;
        uint32_t widgetsPaint = 0;
        uint32_t widgetsLayout = 0;
        uint32_t gatePaintArms = 0;
        uint32_t gateLayoutArms = 0;
        uint32_t transactions = 0;
    };

    /// Route a state change to the correct invalidation path for `widget`.
    [[nodiscard]] static StateInvalidation Notify(Widget& widget, StateChangeKind kind);

    /// Fire-and-forget variant for widget setters and bindings.
    static void Post(Widget& widget, StateChangeKind kind);

    /// Coalesce multiple state notifications into one layout/paint gate arm.
    class KINDUI_API ScopedTransaction {
    public:
        explicit ScopedTransaction(const char* reason = "StateBatch");
        ~ScopedTransaction();

        ScopedTransaction(const ScopedTransaction&) = delete;
        ScopedTransaction& operator=(const ScopedTransaction&) = delete;

    private:
        const char* m_Reason = "StateBatch";
        bool m_Outer = false;
    };

    [[nodiscard]] static bool InTransaction();
    static void ResetFrameStats();
    [[nodiscard]] static const FrameStats& CurrentFrameStats();
    [[nodiscard]] static const char* KindName(StateChangeKind kind);
    [[nodiscard]] static const char* InvalidationName(StateInvalidation inv);

private:
    static void CommitTransaction(const char* reason, bool deferredLayout, bool deferredPaint);
    static bool ApplyPaintChange(Widget& widget, StateChangeKind kind);
    static void ApplyVisibilityChange(Widget& widget);
    static bool ApplyLayoutChange(Widget& widget, StateChangeKind kind);
};

} // namespace we::runtime::kindui
