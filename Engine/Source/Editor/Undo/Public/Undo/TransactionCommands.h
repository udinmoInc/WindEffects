#pragma once

#include "Undo/Export.h"
#include "Undo/ITransactionCommand.h"
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

/// Factory helpers — all restore paths go through Reflection/Serialization.

[[nodiscard]] UNDO_API TransactionCommandPtr MakePropertyChangeCommand(
    reflection::ITypeRegistry& registry,
    reflection::TypeId typeId,
    std::vector<void*> instances,
    std::string path,
    std::vector<std::uint8_t> beforeBytes,
    std::vector<std::uint8_t> afterBytes,
    std::string label = {});

[[nodiscard]] UNDO_API TransactionCommandPtr MakeSnapshotSwapCommand(
    serialization::ISerializer* serializer,
    reflection::ITypeRegistry& registry,
    reflection::TypeId typeId,
    void* instance,
    serialization::Snapshot before,
    serialization::Snapshot after,
    std::string label,
    TransactionKind kind = TransactionKind::Generic);

[[nodiscard]] UNDO_API TransactionCommandPtr MakeCustomCommand(
    std::string label,
    TransactionKind kind,
    std::string mergeKey,
    std::function<bool()> undoFn,
    std::function<bool()> redoFn,
    std::uint64_t estimatedBytes = 0);

[[nodiscard]] UNDO_API TransactionCommandPtr MakeCompositeCommand(
    std::string label,
    TransactionKind kind,
    std::vector<TransactionCommandPtr> children);

} // namespace we::editor::undo
