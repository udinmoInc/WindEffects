#include "UndoInternal.h"

#include <sstream>

namespace we::editor::undo {

UndoDiagnostics& UndoDiagnostics::Get() noexcept {
    static UndoDiagnostics instance;
    return instance;
}

void UndoDiagnostics::Reset() noexcept {
    m_Committed.store(0);
    m_Undone.store(0);
    m_Redone.store(0);
    m_Merged.store(0);
    m_Cancelled.store(0);
    m_Commands.store(0);
    m_PropertyEdits.store(0);
    m_Bytes.store(0);
    m_UndoDepth.store(0);
    m_RedoDepth.store(0);
    m_Nesting.store(0);
}

UndoDiagnosticsSnapshot UndoDiagnostics::Snapshot() const noexcept {
    UndoDiagnosticsSnapshot snap;
    snap.transactionsCommitted = m_Committed.load();
    snap.transactionsUndone = m_Undone.load();
    snap.transactionsRedone = m_Redone.load();
    snap.transactionsMerged = m_Merged.load();
    snap.transactionsCancelled = m_Cancelled.load();
    snap.commandsExecuted = m_Commands.load();
    snap.propertyEditsRecorded = m_PropertyEdits.load();
    snap.bytesTracked = m_Bytes.load();
    snap.historyDepth = m_UndoDepth.load();
    snap.redoDepth = m_RedoDepth.load();
    snap.openNestingDepth = m_Nesting.load();
    return snap;
}

void UndoDiagnostics::OnCommit(std::uint64_t bytes) noexcept {
    m_Committed.fetch_add(1);
    m_Bytes.fetch_add(bytes);
}
void UndoDiagnostics::OnUndo() noexcept { m_Undone.fetch_add(1); }
void UndoDiagnostics::OnRedo() noexcept { m_Redone.fetch_add(1); }
void UndoDiagnostics::OnMerge() noexcept { m_Merged.fetch_add(1); }
void UndoDiagnostics::OnCancel() noexcept { m_Cancelled.fetch_add(1); }
void UndoDiagnostics::OnCommand() noexcept { m_Commands.fetch_add(1); }
void UndoDiagnostics::OnPropertyEdit() noexcept { m_PropertyEdits.fetch_add(1); }

void UndoDiagnostics::SetHistoryDepth(
    std::uint64_t undoDepth,
    std::uint64_t redoDepth,
    std::uint64_t nesting) noexcept
{
    m_UndoDepth.store(undoDepth);
    m_RedoDepth.store(redoDepth);
    m_Nesting.store(nesting);
}

std::string UndoDiagnostics::FormatSummary() const {
    const auto snap = Snapshot();
    std::ostringstream oss;
    oss << "UndoDiagnostics: committed=" << snap.transactionsCommitted
        << " undone=" << snap.transactionsUndone
        << " redone=" << snap.transactionsRedone
        << " merged=" << snap.transactionsMerged
        << " depth=" << snap.historyDepth
        << "/" << snap.redoDepth
        << " bytes=" << snap.bytesTracked;
    return oss.str();
}

} // namespace we::editor::undo
