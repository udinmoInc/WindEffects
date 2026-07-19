#include "ViewportEdit/ViewportEditDiagnostics.h"

#include <sstream>

namespace we::editor::viewportedit {

ViewportEditDiagnostics& ViewportEditDiagnostics::Get() noexcept {
    static ViewportEditDiagnostics instance;
    return instance;
}

void ViewportEditDiagnostics::Reset() noexcept {
    m_Picks.store(0);
    m_SelectionChanges.store(0);
    m_Transforms.store(0);
    m_Transactions.store(0);
    m_ToolActivations.store(0);
    m_HitMicros.store(0);
    m_SelectionCount.store(0);
}

ViewportEditDiagnosticsSnapshot ViewportEditDiagnostics::Snapshot() const noexcept {
    ViewportEditDiagnosticsSnapshot snap;
    snap.pickCount = m_Picks.load();
    snap.selectionChanges = m_SelectionChanges.load();
    snap.transformOps = m_Transforms.load();
    snap.transactionsRecorded = m_Transactions.load();
    snap.toolActivations = m_ToolActivations.load();
    snap.hitTestMicros = m_HitMicros.load();
    snap.selectionCount = m_SelectionCount.load();
    return snap;
}

std::string ViewportEditDiagnostics::FormatSummary() const {
    const auto s = Snapshot();
    std::ostringstream oss;
    oss << "ViewportEdit picks=" << s.pickCount
        << " selectionChanges=" << s.selectionChanges
        << " transforms=" << s.transformOps
        << " txns=" << s.transactionsRecorded
        << " tools=" << s.toolActivations
        << " hitUs=" << s.hitTestMicros
        << " selCount=" << s.selectionCount;
    return oss.str();
}

void ViewportEditDiagnostics::OnPick(std::uint64_t micros) noexcept {
    m_Picks.fetch_add(1);
    m_HitMicros.fetch_add(micros);
}

void ViewportEditDiagnostics::OnSelectionChanged(std::uint64_t count) noexcept {
    m_SelectionChanges.fetch_add(1);
    m_SelectionCount.store(count);
}

void ViewportEditDiagnostics::OnTransform() noexcept { m_Transforms.fetch_add(1); }
void ViewportEditDiagnostics::OnTransaction() noexcept { m_Transactions.fetch_add(1); }
void ViewportEditDiagnostics::OnToolActivated() noexcept { m_ToolActivations.fetch_add(1); }

} // namespace we::editor::viewportedit
