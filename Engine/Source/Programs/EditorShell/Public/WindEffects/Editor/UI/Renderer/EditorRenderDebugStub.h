// ==============================================================================
// WindEffects — EditorShell — EditorRenderDebugStub
// Public API surface for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::renderer {

enum class ForensicHealth {
    Unknown,
    Pass,
    Warning,
    Error
};

struct GpuPassValidation {
    std::string name;
};

struct GpuRenderTargetInfo {
    std::string name;
};

struct RenderDebuggerFrameSnapshot {
    uint64_t frameNumber = 0;
    std::vector<GpuPassValidation> passes;
    std::vector<GpuRenderTargetInfo> targets;
};

struct ForensicFrameReport {
    uint64_t frameIndex = 0;
    bool frameFailed = false;
    ForensicHealth overallHealth = ForensicHealth::Unknown;
    std::string firstAnomalyReason;
};

} // namespace we::runtime::renderer
