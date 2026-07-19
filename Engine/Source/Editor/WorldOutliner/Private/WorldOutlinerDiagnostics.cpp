#include "WorldOutliner/WorldOutlinerDiagnostics.h"

#include <sstream>

namespace we::editor::outliner {

WorldOutlinerDiagnostics& WorldOutlinerDiagnostics::Get() noexcept {
    static WorldOutlinerDiagnostics instance;
    return instance;
}

void WorldOutlinerDiagnostics::Reset() noexcept {
    m_Rebuilds.store(0);
    m_SelectionChanges.store(0);
    m_Commands.store(0);
    m_Transactions.store(0);
    m_FilterPasses.store(0);
    m_VisibleRows.store(0);
    m_NodeCount.store(0);
    m_RebuildMicros.store(0);
}

WorldOutlinerDiagnosticsSnapshot WorldOutlinerDiagnostics::Snapshot() const noexcept {
    WorldOutlinerDiagnosticsSnapshot s;
    s.rebuildCount = m_Rebuilds.load();
    s.selectionChanges = m_SelectionChanges.load();
    s.commandCount = m_Commands.load();
    s.transactionsRecorded = m_Transactions.load();
    s.filterPasses = m_FilterPasses.load();
    s.visibleRows = m_VisibleRows.load();
    s.nodeCount = m_NodeCount.load();
    s.rebuildMicros = m_RebuildMicros.load();
    return s;
}

std::string WorldOutlinerDiagnostics::FormatSummary() const {
    const auto s = Snapshot();
    std::ostringstream oss;
    oss << "WorldOutliner rebuilds=" << s.rebuildCount
        << " nodes=" << s.nodeCount
        << " visible=" << s.visibleRows
        << " sel=" << s.selectionChanges
        << " cmds=" << s.commandCount
        << " txns=" << s.transactionsRecorded
        << " rebuildUs=" << s.rebuildMicros;
    return oss.str();
}

void WorldOutlinerDiagnostics::OnRebuild(std::uint64_t micros, std::uint64_t nodes, std::uint64_t visible) noexcept {
    m_Rebuilds.fetch_add(1);
    m_RebuildMicros.fetch_add(micros);
    m_NodeCount.store(nodes);
    m_VisibleRows.store(visible);
}

void WorldOutlinerDiagnostics::OnSelectionChanged(std::uint64_t count) noexcept {
    m_SelectionChanges.fetch_add(1);
    (void)count;
}

void WorldOutlinerDiagnostics::OnCommand() noexcept { m_Commands.fetch_add(1); }
void WorldOutlinerDiagnostics::OnTransaction() noexcept { m_Transactions.fetch_add(1); }
void WorldOutlinerDiagnostics::OnFilterPass() noexcept { m_FilterPasses.fetch_add(1); }

} // namespace we::editor::outliner
