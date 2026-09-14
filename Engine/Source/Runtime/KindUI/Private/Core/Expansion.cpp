// ==============================================================================
// WindEffects — KindUI — Expansion
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/Expansion.h"
#include "KindUI/Core/UIStateChange.h"
#include "KindUI/Core/Widget.h"

namespace we::runtime::kindui {
namespace {

void MarkExpansionWidgetDirty(Widget& widget) {
    UIStateChangeGate::Post(widget, StateChangeKind::Expansion);
}

} // namespace

Expansion::ScopedTransaction::ScopedTransaction(const char* reason)
    : m_Gate(reason && reason[0] ? reason : "Expansion")
{
}

Expansion::ScopedTransaction::~ScopedTransaction() = default;

void Expansion::SetExpanded(IExpansionNode& node, bool expanded) {
    if (node.IsExpanded() == expanded) {
        return;
    }
    ScopedTransaction transaction("Expansion");
    node.ApplyExpanded(expanded);
    if (auto* widget = dynamic_cast<Widget*>(&node)) {
        MarkExpansionWidgetDirty(*widget);
    }
}

void Expansion::ApplyUnderWidget(Widget& root, bool expanded) {
    if (auto* expansionNode = dynamic_cast<IExpansionNode*>(&root)) {
        if (expansionNode->IsExpanded() != expanded) {
            expansionNode->ApplyExpanded(expanded);
        }
    }
    for (const auto& child : root.GetChildren()) {
        if (child) {
            ApplyUnderWidget(*child, expanded);
        }
    }
}

void Expansion::ExpandAllUnder(Widget& root) {
    ScopedTransaction transaction("ExpandAll");
    ApplyUnderWidget(root, true);
    MarkExpansionWidgetDirty(root);
}

void Expansion::CollapseAllUnder(Widget& root) {
    ScopedTransaction transaction("CollapseAll");
    ApplyUnderWidget(root, false);
    MarkExpansionWidgetDirty(root);
}

void Expansion::ExpandAll(IExpansionNode& root) {
    ScopedTransaction transaction("ExpandAll");
    if (auto* widget = dynamic_cast<Widget*>(&root)) {
        ApplyUnderWidget(*widget, true);
        MarkExpansionWidgetDirty(*widget);
        return;
    }
    if (!root.IsExpanded()) {
        root.ApplyExpanded(true);
    }
}

void Expansion::CollapseAll(IExpansionNode& root) {
    ScopedTransaction transaction("CollapseAll");
    if (auto* widget = dynamic_cast<Widget*>(&root)) {
        ApplyUnderWidget(*widget, false);
        MarkExpansionWidgetDirty(*widget);
        return;
    }
    if (root.IsExpanded()) {
        root.ApplyExpanded(false);
    }
}

} // namespace we::runtime::kindui
