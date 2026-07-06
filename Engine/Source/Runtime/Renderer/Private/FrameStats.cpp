#include "Renderer/FrameStats.hpp"
#include "Renderer/SceneEnvironmentUniform.hpp"

#include <chrono>
#include <sstream>
#include <algorithm>
#if WE_HAS_GLM
#include <glm/gtc/matrix_inverse.hpp>
#endif

namespace we::runtime::renderer {

namespace {
double NowMs() {
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

FramePassRecord* FindPassRecord(std::vector<FramePassRecord>& passes, const std::string& name) {
    for (auto& pass : passes) {
        if (pass.name == name) {
            return &pass;
        }
    }
    return nullptr;
}

#if WE_HAS_GLM
constexpr float kPi = 3.14159265358979323846f;

glm::vec3 UnprojectDirection(const glm::vec2& uv, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos) {
    const glm::vec2 ndc = uv * 2.0f - glm::vec2(1.0f);
    const glm::mat4 invView = glm::inverse(view);
    const glm::mat4 invProj = glm::inverse(proj);
    const glm::vec4 clip(ndc.x, ndc.y, 0.0f, 1.0f);
    const glm::vec4 world = invView * invProj * clip;
    const glm::vec3 farPoint = glm::vec3(world) / std::max(world.w, 1e-6f);
    return glm::normalize(farPoint - cameraPos);
}

glm::vec2 SkyViewLUTCoord(const glm::vec3& viewDir, const glm::vec3& sunDir, float& outZenithAngle) {
    const glm::vec3 vd = glm::normalize(viewDir);
    const glm::vec3 sd = glm::normalize(sunDir);
    outZenithAngle = std::acos(std::clamp(vd.y, -1.0f, 1.0f)) / kPi;

    const float sunAzimuth = std::atan2(sd.z, sd.x);
    const float viewAzimuth = std::atan2(vd.z, vd.x);
    float azimuthOffset = (viewAzimuth - sunAzimuth) / (2.0f * kPi);
    azimuthOffset -= std::floor(azimuthOffset);
    return glm::vec2(azimuthOffset, outZenithAngle);
}

glm::vec2 LutTransmittanceParamsToUv(float viewHeightKm, float viewZenithCosAngle, float planetR, float atmoR) {
    const float bottomR = planetR;
    const float topR = atmoR;
    const float H = std::sqrt(std::max(topR * topR - bottomR * bottomR, 0.0f));
    const float rho = std::sqrt(std::max(viewHeightKm * viewHeightKm - bottomR * bottomR, 0.0f));

    const float discriminant = viewHeightKm * viewHeightKm * (viewZenithCosAngle * viewZenithCosAngle - 1.0f)
        + topR * topR;
    const float d = std::max(0.0f, (-viewHeightKm * viewZenithCosAngle + std::sqrt(std::max(discriminant, 0.0f))));

    const float dMin = topR - viewHeightKm;
    const float dMax = rho + H;
    const float xMu = (d - dMin) / std::max(dMax - dMin, 1e-4f);
    const float xR = rho / std::max(H, 1e-4f);
    return glm::vec2(std::clamp(xMu, 0.0f, 1.0f), std::clamp(xR, 0.0f, 1.0f));
}

float SunDiskMask(const glm::vec3& viewDir, const glm::vec3& sunDir, float angularRadius) {
    const float cosAngle = glm::dot(glm::normalize(viewDir), glm::normalize(sunDir));
    const float cosRadius = std::cos(std::max(angularRadius, 0.004675f));
    const float t = std::clamp((cosAngle - (cosRadius - 0.00015f)) / 0.00015f, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
#endif
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
    m_Stats.passExecutions.clear();
    m_Stats.gpuAverageLuminance = 0.0f;
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

    if (FramePassRecord* pass = FindPassRecord(m_Stats.passExecutions, name)) {
        pass->cpuMs = ms;
    }
}

void FrameStatsCollector::RecordPassExecution(const char* passName, const char* status, double cpuMs) {
    if (!passName || !status) return;
    const std::string name(passName);
    const std::string value(status);
    if (FramePassRecord* pass = FindPassRecord(m_Stats.passExecutions, name)) {
        pass->status = value;
        if (cpuMs > 0.0) {
            pass->cpuMs = cpuMs;
        }
        return;
    }
    m_Stats.passExecutions.push_back(FramePassRecord{ name, value, cpuMs });
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

    RecordPassExecution(passName, status);
}

void FrameStatsCollector::SetGpuAverageLuminance(float luminance) {
    m_Stats.gpuAverageLuminance = luminance;
}

void FrameStatsCollector::SetExposureDiagnostics(
    bool autoExposureEnabled,
    float manualExposureEV,
    float sunDerivedExposureEV,
    float effectiveExposureEV) {
    m_Stats.autoExposureEnabled = autoExposureEnabled;
    m_Stats.manualExposureEV = manualExposureEV;
    m_Stats.sunDerivedExposureEV = sunDerivedExposureEV;
    m_Stats.effectiveExposureEV = effectiveExposureEV;
}

#if WE_HAS_GLM
void FrameStatsCollector::SetAtmosphereProbe(const AtmosphereFrameProbe& probe) {
    m_Stats.atmosphereProbe = probe;
}

AtmosphereFrameProbe FrameStatsCollector::ComputeAtmosphereProbe(
    const glm::mat4& view,
    const glm::mat4& proj,
    const glm::vec3& cameraPos,
    const glm::vec3& cameraForward,
    const SceneEnvironmentUniform& env) {

    AtmosphereFrameProbe probe{};
    probe.cameraForward = glm::normalize(cameraForward);
    probe.viewDirection = UnprojectDirection(glm::vec2(0.5f, 0.5f), view, proj, cameraPos);
    probe.viewDirectionLength = glm::length(probe.viewDirection);
    probe.sunDirection = glm::normalize(-glm::vec3(env.sunDirection));
    probe.sunDirectionLength = glm::length(glm::vec3(env.sunDirection));
    probe.viewSunDot = glm::dot(probe.viewDirection, probe.sunDirection);
    probe.skyViewUV = SkyViewLUTCoord(probe.viewDirection, probe.sunDirection, probe.viewZenithAngle);

    const float relAltKm = std::max((cameraPos.y - env.worldOrigin.y) * 0.001f, env.eyeAltitude);
    const float viewHeightKm = env.planetRadius + relAltKm;
    probe.transmittanceUV = LutTransmittanceParamsToUv(
        viewHeightKm, probe.viewDirection.y, env.planetRadius, env.planetRadius + env.atmosphereHeight);

    probe.sunAngularRadius = std::max(env.sunAngularRadius, 0.004675f);
    probe.sunDiskMask = SunDiskMask(probe.viewDirection, probe.sunDirection, probe.sunAngularRadius);
    probe.valid = true;
    return probe;
}
#endif

std::string FrameStatsCollector::GetOverlayText() const {
    std::ostringstream ss;
    ss << "CPU " << m_Stats.cpuFrameMs << " ms"
       << " | Draws " << m_Stats.drawCalls
       << " | Tris " << m_Stats.triangles
       << " | Passes " << m_Stats.passExecutions.size()
       << " | Sky " << m_Stats.skyPassMs << " ms (" << m_Stats.skyPassStatus << ")"
       << " | Fog " << m_Stats.fogPassMs << " ms (" << m_Stats.fogPassStatus << ")"
       << " | Post " << m_Stats.postPassMs << " ms (" << m_Stats.postPassStatus << ")"
       << " | LUT " << (m_Stats.atmosphereLutReady ? "OK" : "MISSING") << " (" << m_Stats.atmosphereLutStatus << ")";

#if WE_HAS_GLM
    if (m_Stats.atmosphereProbe.valid) {
        const auto& p = m_Stats.atmosphereProbe;
        ss << "\nViewDir len=" << p.viewDirectionLength
           << " | SunDir len=" << p.sunDirectionLength
           << " | dot=" << p.viewSunDot
           << " | Zenith=" << p.viewZenithAngle
           << " | SkyUV=(" << p.skyViewUV.x << "," << p.skyViewUV.y << ")"
           << " | TUV=(" << p.transmittanceUV.x << "," << p.transmittanceUV.y << ")"
           << " | SunDisk=" << p.sunDiskMask;
    }
#endif
    return ss.str();
}

const FrameStats& FrameStatsCollector::GetStats() const {
    return m_Stats;
}

} // namespace we::runtime::renderer
