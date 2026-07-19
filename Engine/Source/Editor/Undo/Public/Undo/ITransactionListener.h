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
