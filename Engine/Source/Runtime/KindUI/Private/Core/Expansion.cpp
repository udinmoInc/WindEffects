// ==============================================================================
// WindEffects — KindUI — Expansion
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/Expansion.h"
#include "KindUI/Core/Widget.h"

namespace we::runtime::kindui {
namespace {

thread_local int t_ExpansionTransactionDepth = 0;

void CommitExpansionInvalidation(const char* reason) {
    UIRepaintGate::RequestLayoutReason(reason);
    UIRepaintGate::RequestPaintReason(reason);
}

} // namespace

Expansion::ScopedTransaction::ScopedTransaction(const char* reason)
    : m_Reason(reason && reason[0] ? reason : "Expansion")
{
    if (t_ExpansionTransactionDepth == 0) {
        UIRepaintGate::BeginBatch();
        m_Active = true;
    }
    ++t_ExpansionTransactionDepth;
}

Expansion::ScopedTransaction::~ScopedTransaction() {
    if (t_ExpansionTransactionDepth > 0) {
        --t_ExpansionTransactionDepth;
    }
    if (m_Active) {
        CommitExpansionInvalidation(m_Reason);
        UIRepaintGate::EndBatch();
        m_Active = false;
    }
}

void Expansion::SetExpanded(IExpansionNode& node, bool expanded) {
    if (node.IsExpanded() == expanded) {
        return;
    }
    ScopedTransaction transaction("Expansion");
    node.ApplyExpanded(expanded);
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
}

void Expansion::CollapseAllUnder(Widget& root) {
    ScopedTransaction transaction("CollapseAll");
    ApplyUnderWidget(root, false);
}

void Expansion::ExpandAll(IExpansionNode& root) {
    ScopedTransaction transaction("ExpandAll");
    if (auto* widget = dynamic_cast<Widget*>(&root)) {
        ApplyUnderWidget(*widget, true);
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
        return;
    }
    if (root.IsExpanded()) {
        root.ApplyExpanded(false);
    }
}

} // namespace we::runtime::kindui
