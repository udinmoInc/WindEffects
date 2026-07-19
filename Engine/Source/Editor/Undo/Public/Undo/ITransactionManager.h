#pragma once

#include "Undo/Export.h"
#include "Undo/IMergePolicy.h"
#include "Undo/ITransaction.h"
#include "Undo/ITransactionCommand.h"
#include "Undo/ITransactionHistory.h"
#include "Undo/ITransactionListener.h"
#include "Undo/UndoTypes.h"
#include "Reflection/TypeId.h"
#include "Serialization/Delta.h"

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::editor::undo {

/// Main editing façade — nested transactions, command execution, undo/redo.
class UNDO_API ITransactionManager {
public:
    virtual ~ITransactionManager() = default;

    [[nodiscard]] virtual ITransactionHistory& History() noexcept = 0;
    [[nodiscard]] virtual const ITransactionHistory& History() const noexcept = 0;

    virtual void SetMergePolicy(std::shared_ptr<IMergePolicy> policy) = 0;
    virtual void AddListener(std::shared_ptr<ITransactionListener> listener) = 0;
    virtual void RemoveListener(const ITransactionListener* listener) = 0;

    /// Begin a (possibly nested) transaction. Outer commit flushes as one history entry.
    [[nodiscard]] virtual TransactionId BeginTransaction(const TransactionDescriptor& desc) = 0;
    [[nodiscard]] virtual bool EndTransaction() = 0;
    [[nodiscard]] virtual bool CancelTransaction() = 0;
    [[nodiscard]] virtual bool HasOpenTransaction() const noexcept = 0;
    [[nodiscard]] virtual std::uint32_t NestingDepth() const noexcept = 0;
    [[nodiscard]] virtual ITransaction* ActiveTransaction() noexcept = 0;

    /// Execute a command inside the active transaction (or auto-wrap as a single txn).
    [[nodiscard]] virtual bool Execute(TransactionCommandPtr command) = 0;

    /// Convenience: record a property change that already happened (or apply + record).
    [[nodiscard]] virtual bool RecordPropertyChange(
        reflection::TypeId typeId,
        std::span<void* const> instances,
        std::string_view path,
        std::vector<std::uint8_t> beforeBytes,
        std::vector<std::uint8_t> afterBytes,
        std::string_view label = {}) = 0;

    /// Full-object snapshot pair (create/delete/prefab) via Serialization.
    [[nodiscard]] virtual bool RecordSnapshotSwap(
        reflection::TypeId typeId,
        void* instance,
        serialization::Snapshot before,
        serialization::Snapshot after,
        std::string_view label,
        TransactionKind kind = TransactionKind::Generic) = 0;

    /// Custom reversible ops (asset rename, actor spawn, etc.) via callbacks.
    [[nodiscard]] virtual bool RecordCustom(
        std::string_view label,
        TransactionKind kind,
        std::string mergeKey,
        std::function<bool()> undoFn,
        std::function<bool()> redoFn,
        std::uint64_t estimatedBytes = 0) = 0;

    [[nodiscard]] virtual bool Undo() = 0;
    [[nodiscard]] virtual bool Redo() = 0;
    [[nodiscard]] virtual bool CanUndo() const noexcept = 0;
    [[nodiscard]] virtual bool CanRedo() const noexcept = 0;

    /// Suppress recording while applying undo/redo (re-entrancy guard).
    [[nodiscard]] virtual bool IsApplying() const noexcept = 0;
    virtual void SuspendRecording(bool suspend) = 0;
    [[nodiscard]] virtual bool IsRecordingSuspended() const noexcept = 0;

    virtual void SetActiveWorldKey(UndoWorldKey key) = 0;
    [[nodiscard]] virtual UndoWorldKey ActiveWorldKey() const noexcept = 0;
};

} // namespace we::editor::undo
