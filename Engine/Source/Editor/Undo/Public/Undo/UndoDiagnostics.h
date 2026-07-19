#pragma once

#include "Undo/Export.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace we::editor::undo {

struct UNDO_API UndoDiagnosticsSnapshot {
    std::uint64_t transactionsCommitted = 0;
    std::uint64_t transactionsUndone = 0;
    std::uint64_t transactionsRedone = 0;
    std::uint64_t transactionsMerged = 0;
    std::uint64_t transactionsCancelled = 0;
    std::uint64_t commandsExecuted = 0;
    std::uint64_t propertyEditsRecorded = 0;
    std::uint64_t bytesTracked = 0;
    std::uint64_t historyDepth = 0;
    std::uint64_t redoDepth = 0;
    std::uint64_t openNestingDepth = 0;
};

/// Process-local counters for profiling / tests (not a service locator for behavior).
class UNDO_API UndoDiagnostics {
public:
    static UndoDiagnostics& Get() noexcept;

    void Reset() noexcept;
    [[nodiscard]] UndoDiagnosticsSnapshot Snapshot() const noexcept;

    void OnCommit(std::uint64_t bytes) noexcept;
    void OnUndo() noexcept;
    void OnRedo() noexcept;
    void OnMerge() noexcept;
    void OnCancel() noexcept;
    void OnCommand() noexcept;
    void OnPropertyEdit() noexcept;
    void SetHistoryDepth(std::uint64_t undoDepth, std::uint64_t redoDepth, std::uint64_t nesting) noexcept;

    [[nodiscard]] std::string FormatSummary() const;

private:
    std::atomic<std::uint64_t> m_Committed{0};
    std::atomic<std::uint64_t> m_Undone{0};
    std::atomic<std::uint64_t> m_Redone{0};
    std::atomic<std::uint64_t> m_Merged{0};
    std::atomic<std::uint64_t> m_Cancelled{0};
    std::atomic<std::uint64_t> m_Commands{0};
    std::atomic<std::uint64_t> m_PropertyEdits{0};
    std::atomic<std::uint64_t> m_Bytes{0};
    std::atomic<std::uint64_t> m_UndoDepth{0};
    std::atomic<std::uint64_t> m_RedoDepth{0};
    std::atomic<std::uint64_t> m_Nesting{0};
};

} // namespace we::editor::undo
