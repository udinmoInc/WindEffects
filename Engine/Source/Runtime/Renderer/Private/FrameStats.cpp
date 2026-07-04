#include "Renderer/FrameStats.hpp"

#include <chrono>
#include <sstream>

namespace we::runtime::renderer {

namespace {
double NowMs() {
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
}

FrameStatsCollector& FrameStatsCollector::Get() {
    static FrameStatsCollector instance;
    return instance;
}

void FrameStatsCollector::BeginFrame() {
    m_FrameStartMs = NowMs();
    m_Stats.drawCalls = 0;
    m_Stats.triangles = 0;
    m_Stats.descriptorUpdates = 0;
}

void FrameStatsCollector::EndFrame() {
    m_Stats.cpuFrameMs = NowMs() - m_FrameStartMs;
}

void FrameStatsCollector::AddDrawCall(uint32_t triangles) {
    ++m_Stats.drawCalls;
    m_Stats.triangles += triangles;
}

void FrameStatsCollector::AddDescriptorUpdate(uint32_t count) {
    m_Stats.descriptorUpdates += count;
}

void FrameStatsCollector::RecordPassMs(const char* passName, double ms) {
    if (!passName) return;
    const std::string name(passName);
    if (name == "SkyAtmosphere") m_Stats.skyPassMs = ms;
    else if (name == "VolumetricClouds") m_Stats.cloudsPassMs = ms;
    else if (name == "Scene") m_Stats.scenePassMs = ms;
    else if (name == "FogComposite") m_Stats.fogPassMs = ms;
    else if (name == "PostExposure") m_Stats.postPassMs = ms;
    else if (name == "AtmosphereLUT") m_Stats.lutGenerationMs = ms;
}

std::string FrameStatsCollector::GetOverlayText() const {
    std::ostringstream ss;
    ss << "CPU " << m_Stats.cpuFrameMs << " ms"
       << " | Draws " << m_Stats.drawCalls
       << " | Tris " << m_Stats.triangles
       << " | Sky " << m_Stats.skyPassMs << " ms"
       << " | Fog " << m_Stats.fogPassMs << " ms"
       << " | Post " << m_Stats.postPassMs << " ms"
       << " | LUT " << (m_Stats.atmosphereLutReady ? "OK" : "MISSING");
    return ss.str();
}

} // namespace we::runtime::renderer
