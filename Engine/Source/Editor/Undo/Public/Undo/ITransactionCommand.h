#pragma once

#include "Undo/Export.h"
#include "Undo/UndoTypes.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace we::editor::undo {

/// Atomic reversible unit. Implementations must not duplicate object state —
/// capture/restore via Serialization Reflection patches / snapshots.
class UNDO_API ITransactionCommand {
public:
    virtual ~ITransactionCommand() = default;

    [[nodiscard]] virtual std::string_view Label() const noexcept = 0;
    [[nodiscard]] virtual TransactionKind Kind() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t EstimatedBytes() const noexcept = 0;
    [[nodiscard]] virtual std::string_view MergeKey() const noexcept { return {}; }

    /// Apply forward change (first execution or redo).
    [[nodiscard]] virtual bool Redo() = 0;
    /// Reverse change.
    [[nodiscard]] virtual bool Undo() = 0;

    /// Attempt to absorb a newer command into this one (merge). Returns true if absorbed.
    [[nodiscard]] virtual bool TryMergeWith(const ITransactionCommand& /*newer*/) { return false; }

    /// Optional compression after commit (delta compaction). Default no-op.
    virtual void Compress() {}
};

using TransactionCommandPtr = std::shared_ptr<ITransactionCommand>;

} // namespace we::editor::undo
