#pragma once

#include "Undo/Export.h"
#include "Undo/ITransaction.h"
#include "Undo/ITransactionCommand.h"
#include "Undo/UndoTypes.h"

#include <cstdint>
#include <memory>

namespace we::editor::undo {

/// Policy for merging consecutive transactions / commands.
class UNDO_API IMergePolicy {
public:
    virtual ~IMergePolicy() = default;

    [[nodiscard]] virtual bool CanMergeTransactions(
        const ITransaction& older,
        const ITransaction& newer) const = 0;

    [[nodiscard]] virtual bool CanMergeCommands(
        const ITransactionCommand& older,
        const ITransactionCommand& newer) const = 0;
};

[[nodiscard]] UNDO_API std::shared_ptr<IMergePolicy> CreateMergePolicy(MergePolicyKind kind, std::uint32_t windowMs = 500);

} // namespace we::editor::undo
