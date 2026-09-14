// ==============================================================================
// WindEffects — KindUI — KindUIInteractionBenchmark
// Stub implementations so Editor env-gated benches link. Prefer the dedicated
// virtualization benchmark (WE_UI_VIRT_BENCH) for list/tree scale metrics.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Diagnostics/KindUIInteractionBenchmark.h"

namespace we::runtime::kindui {

KindUIInteractionReport RunKindUIInteractionBenchmark(uint32_t /*dragSteps*/) {
    KindUIInteractionReport report{};
    report.summary = "KindUI interaction bench stub (no scenarios)";
    return report;
}

UiLatencyBenchmarkReport RunUiInputLatencyBenchmark(uint32_t /*steps*/) {
    UiLatencyBenchmarkReport report{};
    report.summary = "KindUI latency bench stub (no scenarios)";
    return report;
}

HitTestAuditReport RunHitTestAudit() {
    HitTestAuditReport report{};
    report.summary = "KindUI hit-test audit stub (no cases)";
    return report;
}

} // namespace we::runtime::kindui
