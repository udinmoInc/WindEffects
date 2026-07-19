#pragma once

#include "Undo/Export.h"

#include <cstdint>
#include <string>

namespace we::editor::undo {

/// Stable transaction identity within a history stream.
struct UNDO_API TransactionId {
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const TransactionId& o) const noexcept {
        return value == o.value;
    }
    [[nodiscard]] constexpr bool operator!=(const TransactionId& o) const noexcept {
        return !(*this == o);
    }
};

/// Checkpoint / save-marker in history (branch-safe navigation).
struct UNDO_API CheckpointId {
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const CheckpointId& o) const noexcept {
        return value == o.value;
    }
};

/// Optional world association for multi-world editing sessions.
struct UNDO_API UndoWorldKey {
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const UndoWorldKey& o) const noexcept {
        return value == o.value;
    }
};

enum class TransactionKind : std::uint8_t {
    Generic = 0,
    PropertyChange,
    ObjectCreate,
    ObjectDelete,
    ActorSpawn,
    ActorDestroy,
    ComponentAdd,
    ComponentRemove,
    Reparent,
    Rename,
    AssetRename,
    AssetMove,
    AssetDelete,
    Prefab,
    Scene,
    Batch,
    Composite,
    Checkpoint,
};

enum class TransactionState : std::uint8_t {
    Open = 0,
    Committed,
    Cancelled,
    Undone,
    Redone,
};

enum class MergePolicyKind : std::uint8_t {
    Never = 0,
    ConsecutiveSameTarget,
    TimeWindowSameTarget,
};

struct UNDO_API TransactionDescriptor {
    std::string label;
    TransactionKind kind = TransactionKind::Generic;
    UndoWorldKey worldKey{};
    bool canMerge = true;
    bool isCheckpoint = false;
};

struct UNDO_API HistoryConfig {
    std::uint32_t maxTransactions = 10'000;
    std::uint64_t maxBytes = 256ull * 1024ull * 1024ull; // 256 MiB soft cap
    MergePolicyKind mergePolicy = MergePolicyKind::ConsecutiveSameTarget;
    std::uint32_t mergeWindowMs = 500;
    bool compressOnCommit = true;
    bool clearRedoOnNewTransaction = true;
};

} // namespace we::editor::undo
