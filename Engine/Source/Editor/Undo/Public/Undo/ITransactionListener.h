// ==============================================================================
// WindEffects — Undo — ITransactionListener
// Public API surface for the Undo module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Undo/Export.h"
#include "Undo/ITransaction.h"

namespace we::editor::undo {

/// Observer for transaction lifecycle (tools, dirty indicators, diagnostics).
class UNDO_API ITransactionListener {
public:
    virtual ~ITransactionListener() = default;

    virtual void OnTransactionBegun(const ITransaction& transaction) { (void)transaction; }
    virtual void OnTransactionCommitted(const ITransaction& transaction) { (void)transaction; }
    virtual void OnTransactionCancelled(const ITransaction& transaction) { (void)transaction; }
    virtual void OnTransactionUndone(const ITransaction& transaction) { (void)transaction; }
    virtual void OnTransactionRedone(const ITransaction& transaction) { (void)transaction; }
    virtual void OnHistoryChanged() {}
};

} // namespace we::editor::undo
