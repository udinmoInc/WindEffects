#pragma once

#include "WorldOutliner/Export.h"
#include "WorldOutliner/OutlinerTypes.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace we::editor::outliner {

struct WORLDOUTLINER_API WorldOutlinerDiagnosticsSnapshot {
    std::uint64_t rebuildCount = 0;
    std::uint64_t selectionChanges = 0;
    std::uint64_t commandCount = 0;
    std::uint64_t transactionsRecorded = 0;
    std::uint64_t filterPasses = 0;
    std::uint64_t visibleRows = 0;
    std::uint64_t nodeCount = 0;
    std::uint64_t rebuildMicros = 0;
};

class WORLDOUTLINER_API WorldOutlinerDiagnostics {
public:
    static WorldOutlinerDiagnostics& Get() noexcept;
    void Reset() noexcept;
    [[nodiscard]] WorldOutlinerDiagnosticsSnapshot Snapshot() const noexcept;
    [[nodiscard]] std::string FormatSummary() const;

    void OnRebuild(std::uint64_t micros, std::uint64_t nodes, std::uint64_t visible) noexcept;
    void OnSelectionChanged(std::uint64_t count) noexcept;
    void OnCommand() noexcept;
    void OnTransaction() noexcept;
    void OnFilterPass() noexcept;

private:
    std::atomic<std::uint64_t> m_Rebuilds{0};
    std::atomic<std::uint64_t> m_SelectionChanges{0};
    std::atomic<std::uint64_t> m_Commands{0};
    std::atomic<std::uint64_t> m_Transactions{0};
    std::atomic<std::uint64_t> m_FilterPasses{0};
    std::atomic<std::uint64_t> m_VisibleRows{0};
    std::atomic<std::uint64_t> m_NodeCount{0};
    std::atomic<std::uint64_t> m_RebuildMicros{0};
};

} // namespace we::editor::outliner
