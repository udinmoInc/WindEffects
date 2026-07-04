#pragma once

#include "Renderer/Export.hpp"
#include <cstdint>
#include <string>

namespace we::runtime::renderer {

struct FrameStats {
    double cpuFrameMs = 0.0;
    double gpuFrameMs = 0.0;
    uint32_t drawCalls = 0;
    uint32_t triangles = 0;
    uint32_t descriptorUpdates = 0;
    double skyPassMs = 0.0;
    double cloudsPassMs = 0.0;
    double scenePassMs = 0.0;
    double fogPassMs = 0.0;
    double postPassMs = 0.0;
    double lutGenerationMs = 0.0;
    bool atmosphereLutReady = false;
    bool skyPipelineValid = false;
    bool postPipelineValid = false;
    std::string lastDiagnosticSummary;
};

class FrameStatsCollector {
public:
    RENDERER_API static FrameStatsCollector& Get();

    RENDERER_API void BeginFrame();
    RENDERER_API void EndFrame();
    RENDERER_API void AddDrawCall(uint32_t triangles = 0);
    RENDERER_API void AddDescriptorUpdate(uint32_t count = 1);
    RENDERER_API void RecordPassMs(const char* passName, double ms);

    RENDERER_API const FrameStats& GetStats() const { return m_Stats; }
    RENDERER_API std::string GetOverlayText() const;

private:
    FrameStats m_Stats{};
    double m_FrameStartMs = 0.0;
};

} // namespace we::runtime::renderer
