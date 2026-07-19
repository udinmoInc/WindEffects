#pragma once

#include "Undo/Export.h"
#include "Undo/ITransactionCommand.h"
#include "Undo/UndoTypes.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace we::editor::undo {

/// Committed or open group of commands with a shared label / world key.
class UNDO_API ITransaction {
public:
    virtual ~ITransaction() = default;

    [[nodiscard]] virtual TransactionId Id() const noexcept = 0;
    [[nodiscard]] virtual std::string_view Label() const noexcept = 0;
    [[nodiscard]] virtual TransactionKind Kind() const noexcept = 0;
    [[nodiscard]] virtual TransactionState State() const noexcept = 0;
    [[nodiscard]] virtual UndoWorldKey WorldKey() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t TimestampMs() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t EstimatedBytes() const noexcept = 0;
    [[nodiscard]] virtual std::size_t CommandCount() const noexcept = 0;
    [[nodiscard]] virtual std::span<const TransactionCommandPtr> Commands() const noexcept = 0;
    [[nodiscard]] virtual bool CanMerge() const noexcept = 0;
    [[nodiscard]] virtual bool IsCheckpoint() const noexcept = 0;

    virtual void AddCommand(TransactionCommandPtr command) = 0;
    [[nodiscard]] virtual bool Undo() = 0;
    [[nodiscard]] virtual bool Redo() = 0;
    virtual void Compress() = 0;
};

using TransactionPtr = std::shared_ptr<ITransaction>;

} // namespace we::editor::undo
