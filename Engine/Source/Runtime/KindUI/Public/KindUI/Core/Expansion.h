// ==============================================================================
// WindEffects — KindUI — Expansion
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/UIRepaintGate.h"

#include <cstddef>
#include <functional>
#include <vector>

namespace we::runtime::kindui {

class Widget;

/// Expandable unit for the shared KindUI expansion system (CollapsibleGroup, etc.).
class KINDUI_API IExpansionNode {
public:
    virtual ~IExpansionNode() = default;

    [[nodiscard]] virtual bool IsExpanded() const = 0;
    /// Apply expanded/collapsed state without requesting layout/paint.
    /// Visibility/geometry side effects are allowed; invalidation is owned by Expansion.
    virtual void ApplyExpanded(bool expanded) = 0;
};

/// Canonical Expand All / Collapse All / SetExpanded entry points.
/// All multi-node mutations run inside one UIRepaintGate batch so layout/paint
/// coalesce to a single rebuild after the transaction commits.
class KINDUI_API Expansion {
public:
    /// Batches gate invalidation for a multi-step expansion transaction.
    class KINDUI_API ScopedTransaction {
    public:
        explicit ScopedTransaction(const char* reason = "Expansion");
        ~ScopedTransaction();

        ScopedTransaction(const ScopedTransaction&) = delete;
        ScopedTransaction& operator=(const ScopedTransaction&) = delete;

    private:
        const char* m_Reason = "Expansion";
        bool m_Active = false;
    };

    /// Single-node expand/collapse with batched invalidation.
    static void SetExpanded(IExpansionNode& node, bool expanded);

    /// Expand/collapse every IExpansionNode under a widget subtree (one transaction).
    static void ExpandAllUnder(Widget& root);
    static void CollapseAllUnder(Widget& root);

    /// Expand/collapse an explicit list of expansion roots (and their IExpansionNode children
    /// discovered via the widget tree when roots are Widgets). Prefer ExpandAllUnder for UI trees.
    static void ExpandAll(IExpansionNode& root);
    static void CollapseAll(IExpansionNode& root);

    /// Generic model-tree Expand/Collapse All (property trees, outliner models, …).
    /// `setExpanded` must only mutate state — no layout/paint. One gate transaction wraps the walk.
    template <typename NodePtr, typename GetChildren, typename SetExpandedFn>
    static void ExpandAll(
        const std::vector<NodePtr>& roots,
        GetChildren&& getChildren,
        SetExpandedFn&& setExpanded)
    {
        ApplyAll(roots, true, std::forward<GetChildren>(getChildren), std::forward<SetExpandedFn>(setExpanded));
    }

    template <typename NodePtr, typename GetChildren, typename SetExpandedFn>
    static void CollapseAll(
        const std::vector<NodePtr>& roots,
        GetChildren&& getChildren,
        SetExpandedFn&& setExpanded)
    {
        ApplyAll(roots, false, std::forward<GetChildren>(getChildren), std::forward<SetExpandedFn>(setExpanded));
    }

private:
    template <typename NodePtr, typename GetChildren, typename SetExpandedFn>
    static void ApplyAll(
        const std::vector<NodePtr>& roots,
        bool expanded,
        GetChildren&& getChildren,
        SetExpandedFn&& setExpanded)
    {
        ScopedTransaction transaction(expanded ? "ExpandAll" : "CollapseAll");
        std::function<void(const NodePtr&)> walk = [&](const NodePtr& node) {
            if (!node) {
                return;
            }
            setExpanded(node, expanded);
            for (const auto& child : getChildren(node)) {
                walk(child);
            }
        };
        for (const auto& root : roots) {
            walk(root);
        }
    }

    static void ApplyUnderWidget(Widget& root, bool expanded);
};

} // namespace we::runtime::kindui
