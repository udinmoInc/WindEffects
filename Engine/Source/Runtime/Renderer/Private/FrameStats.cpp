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
    m_Stats.atmosphereLutReady = false;
    m_Stats.skyPipelineValid = false;
    m_Stats.postPipelineValid = false;
    m_Stats.atmosphereLutStatus = "pending";
    m_Stats.skyPassStatus = "pending";
    m_Stats.cloudsPassStatus = "pending";
    m_Stats.fogPassStatus = "pending";
    m_Stats.postPassStatus = "pending";
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

void FrameStatsCollector::SetAtmosphereLutReady(bool ready) {
    m_Stats.atmosphereLutReady = ready;
    m_Stats.atmosphereLutStatus = ready ? "ok" : "missing";
}

void FrameStatsCollector::SetSkyPipelineValid(bool valid) {
    m_Stats.skyPipelineValid = valid;
}

void FrameStatsCollector::SetPostPipelineValid(bool valid) {
    m_Stats.postPipelineValid = valid;
}

void FrameStatsCollector::SetPassStatus(const char* passName, const char* status) {
    if (!passName || !status) return;
    const std::string name(passName);
    const std::string value(status);
    if (name == "AtmosphereLUT") m_Stats.atmosphereLutStatus = value;
    else if (name == "SkyAtmosphere") m_Stats.skyPassStatus = value;
    else if (name == "VolumetricClouds") m_Stats.cloudsPassStatus = value;
    else if (name == "FogComposite") m_Stats.fogPassStatus = value;
    else if (name == "PostExposure" || name == "ToneMapping") m_Stats.postPassStatus = value;
}

std::string FrameStatsCollector::GetOverlayText() const {
    std::ostringstream ss;
    ss << "CPU " << m_Stats.cpuFrameMs << " ms"
       << " | Draws " << m_Stats.drawCalls
       << " | Tris " << m_Stats.triangles
       << " | Sky " << m_Stats.skyPassMs << " ms (" << m_Stats.skyPassStatus << ")"
       << " | Fog " << m_Stats.fogPassMs << " ms (" << m_Stats.fogPassStatus << ")"
       << " | Post " << m_Stats.postPassMs << " ms (" << m_Stats.postPassStatus << ")"
       << " | LUT " << (m_Stats.atmosphereLutReady ? "OK" : "MISSING") << " (" << m_Stats.atmosphereLutStatus << ")";
    return ss.str();
}

const FrameStats& FrameStatsCollector::GetStats() const {
    return m_Stats;
}

} // namespace we::runtime::renderer
