#pragma once

#include "Undo/Export.h"
#include "Undo/ITransaction.h"
#include "Undo/UndoTypes.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace we::editor::undo {

/// Undo/redo stacks + checkpoints. Branch-safe: new commits clear redo by default.
class UNDO_API ITransactionHistory {
public:
    virtual ~ITransactionHistory() = default;

    virtual void Configure(const HistoryConfig& config) = 0;
    [[nodiscard]] virtual const HistoryConfig& GetConfig() const noexcept = 0;

    [[nodiscard]] virtual std::size_t UndoCount() const noexcept = 0;
    [[nodiscard]] virtual std::size_t RedoCount() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t TrackedBytes() const noexcept = 0;
    [[nodiscard]] virtual bool CanUndo() const noexcept = 0;
    [[nodiscard]] virtual bool CanRedo() const noexcept = 0;

    [[nodiscard]] virtual std::span<const TransactionPtr> UndoStack() const noexcept = 0;
    [[nodiscard]] virtual std::span<const TransactionPtr> RedoStack() const noexcept = 0;

    [[nodiscard]] virtual TransactionPtr PeekUndo() const = 0;
    [[nodiscard]] virtual TransactionPtr PeekRedo() const = 0;

    /// Push a committed transaction onto the undo stack (clears redo if configured).
    virtual void PushCommitted(TransactionPtr transaction) = 0;

    [[nodiscard]] virtual bool UndoOne() = 0;
    [[nodiscard]] virtual bool RedoOne() = 0;
    [[nodiscard]] virtual std::size_t UndoMany(std::size_t count) = 0;
    [[nodiscard]] virtual std::size_t RedoMany(std::size_t count) = 0;

    /// Navigate to a checkpoint by undoing/redoing as needed.
    [[nodiscard]] virtual bool JumpToCheckpoint(CheckpointId id) = 0;
    [[nodiscard]] virtual CheckpointId MarkCheckpoint(std::string_view label) = 0;
    [[nodiscard]] virtual std::vector<CheckpointId> ListCheckpoints() const = 0;

    /// Dirty vs last checkpoint / save marker.
    [[nodiscard]] virtual bool IsDirty() const noexcept = 0;
    virtual void MarkSaved() = 0;

    virtual void Clear() = 0;
    virtual void ClearRedo() = 0;
    virtual void TrimToBudget() = 0;
};

} // namespace we::editor::undo
