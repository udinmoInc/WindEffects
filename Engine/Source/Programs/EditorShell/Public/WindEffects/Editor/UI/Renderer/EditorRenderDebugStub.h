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
