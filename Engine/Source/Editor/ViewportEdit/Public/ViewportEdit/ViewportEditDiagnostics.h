#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace we::editor::viewportedit {

struct VIEWPORTEDIT_API ViewportEditDiagnosticsSnapshot {
    std::uint64_t pickCount = 0;
    std::uint64_t selectionChanges = 0;
    std::uint64_t transformOps = 0;
    std::uint64_t transactionsRecorded = 0;
    std::uint64_t toolActivations = 0;
    std::uint64_t modeActivations = 0;
    std::uint64_t modesLoaded = 0;
    std::uint64_t modesUnloaded = 0;
    std::uint64_t commandExecutions = 0;
    std::uint64_t hitTestMicros = 0;
    std::uint64_t selectionCount = 0;
};

class VIEWPORTEDIT_API ViewportEditDiagnostics {
public:
    static ViewportEditDiagnostics& Get() noexcept;
    void Reset() noexcept;
    [[nodiscard]] ViewportEditDiagnosticsSnapshot Snapshot() const noexcept;
    [[nodiscard]] std::string FormatSummary() const;

    void OnPick(std::uint64_t micros) noexcept;
    void OnSelectionChanged(std::uint64_t count) noexcept;
    void OnTransform() noexcept;
    void OnTransaction() noexcept;
    void OnToolActivated() noexcept;
    void OnModeActivated() noexcept;
    void OnModeLoaded() noexcept;
    void OnModeUnloaded() noexcept;
    void OnCommandExecuted() noexcept;

private:
    std::atomic<std::uint64_t> m_Picks{0};
    std::atomic<std::uint64_t> m_SelectionChanges{0};
    std::atomic<std::uint64_t> m_Transforms{0};
    std::atomic<std::uint64_t> m_Transactions{0};
    std::atomic<std::uint64_t> m_ToolActivations{0};
    std::atomic<std::uint64_t> m_ModeActivations{0};
    std::atomic<std::uint64_t> m_ModesLoaded{0};
    std::atomic<std::uint64_t> m_ModesUnloaded{0};
    std::atomic<std::uint64_t> m_Commands{0};
    std::atomic<std::uint64_t> m_HitMicros{0};
    std::atomic<std::uint64_t> m_SelectionCount{0};
};

} // namespace we::editor::viewportedit
