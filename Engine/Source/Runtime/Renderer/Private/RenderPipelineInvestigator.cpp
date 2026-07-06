#include "Renderer/RenderPipelineInvestigator.hpp"
#include "Renderer/RendererDebug.hpp"
#include "Renderer/RendererConfig.hpp"
#include "Core/LogCategory.hpp"
#include "Core/DiagnosticMacros.hpp"
#include "Core/EditorConfigPaths.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

#if WE_HAS_GLM
#include <glm/gtc/matrix_transform.hpp>
#endif

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

namespace {
VkDevice s_CachedDevice = VK_NULL_HANDLE;

float HalfToFloat(std::uint16_t value) {
    const std::uint32_t sign = (value >> 15) & 0x1;
    const std::uint32_t exponent = (value >> 10) & 0x1F;
    const std::uint32_t mantissa = value & 0x3FF;
    if (exponent == 0) {
        if (mantissa == 0) return sign ? -0.0f : 0.0f;
        const float result = std::ldexp(static_cast<float>(mantissa), -24);
        return sign ? -result : result;
    }
    if (exponent == 31) return mantissa == 0 ? (sign ? -INFINITY : INFINITY) : NAN;
    const std::uint32_t floatBits = (sign << 31) | ((exponent + (127 - 15)) << 23) | (mantissa << 13);
    float result = 0.0f;
    std::memcpy(&result, &floatBits, sizeof(result));
    return result;
}

bool StartsWith(const char* value, const char* prefix) {
    return value != nullptr && prefix != nullptr && std::strncmp(value, prefix, std::strlen(prefix)) == 0;
}

const char* EffectStepName(int step) {
    switch (step) {
        case 0: return "ClearOnly";
        case 1: return "Clear+Sky";
        case 2: return "Clear+Sky+Geometry";
        case 3: return "Clear+Sky+Geometry+Clouds";
        case 4: return "Clear+Sky+Geometry+Clouds+Fog";
        case 5: return "Clear+Sky+Geometry+Clouds+Fog+Post";
        default: return "Custom";
    }
}

bool EffectStepAllowsPass(int step, RenderPassId pass) {
    switch (pass) {
        case RenderPassId::Clear:
            return true;
        case RenderPassId::SkyAtmosphere:
            return step >= 1;
        case RenderPassId::Geometry:
        case RenderPassId::Lighting:
            return step >= 2;
        case RenderPassId::VolumetricClouds:
            return step >= 3;
        case RenderPassId::Fog:
        case RenderPassId::AerialPerspective:
            return step >= 4;
        case RenderPassId::PostProcessing:
        case RenderPassId::Exposure:
        case RenderPassId::ToneMapping:
            return step >= 5;
        default:
            return true;
    }
}

std::string TrimConfigValue(std::string value) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

bool ParseConfigBool(const std::string& value, bool fallback) {
    const std::string lower = TrimConfigValue(value);
    if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") return true;
    if (lower == "0" || lower == "false" || lower == "no" || lower == "off") return false;
    return fallback;
}

int ParseConfigInt(const std::string& value, int fallback) {
    try {
        return std::stoi(TrimConfigValue(value));
    } catch (...) {
        return fallback;
    }
}

float ParseConfigFloat(const std::string& value, float fallback) {
    try {
        return std::stof(TrimConfigValue(value));
    } catch (...) {
        return fallback;
    }
}

#if WE_HAS_GLM
glm::vec3 HorizontalForwardFromCamera(const glm::mat4& view) {
    const glm::vec3 forward = -glm::vec3(view[0][2], view[1][2], view[2][2]);
    glm::vec3 horizontal(forward.x, 0.0f, forward.z);
    const float len = glm::length(horizontal);
    if (len < 1e-4f) {
        horizontal = glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
        horizontal /= len;
    }
    return horizontal;
}

glm::vec3 WorldDirectionAtElevation(const glm::vec3& horizontalForward, float elevationDeg) {
    const float elevation = glm::radians(elevationDeg);
    const float cosEl = std::cos(elevation);
    const float sinEl = std::sin(elevation);
    return glm::normalize(horizontalForward * cosEl + glm::vec3(0.0f, 1.0f, 0.0f) * sinEl);
}

bool ProjectWorldDirectionToPixel(
    const glm::vec3& worldDir,
    const glm::mat4& view,
    const glm::mat4& proj,
    const glm::vec3& cameraPos,
    uint32_t width,
    uint32_t height,
    int& outX,
    int& outY) {

    const glm::vec4 clip = proj * view * glm::vec4(cameraPos + worldDir * 100000.0f, 1.0f);
    if (clip.w <= 0.0f) {
        return false;
    }
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f) {
        return false;
    }
    const float u = ndc.x * 0.5f + 0.5f;
    const float v = ndc.y * 0.5f + 0.5f;
    outX = static_cast<int>(std::lround(u * static_cast<float>(width - 1)));
    outY = static_cast<int>(std::lround(v * static_cast<float>(height - 1)));
    outX = std::clamp(outX, 0, static_cast<int>(width > 0 ? width - 1 : 0));
    outY = std::clamp(outY, 0, static_cast<int>(height > 0 ? height - 1 : 0));
    return true;
}

void SampleRgbAt(
    const std::vector<float>& rgba,
    uint32_t width,
    uint32_t height,
    int x,
    int y,
    float& outR,
    float& outG,
    float& outB) {

    const size_t idx = (static_cast<size_t>(y) * width + static_cast<size_t>(x)) * 4;
    if (idx + 2 >= rgba.size()) {
        outR = outG = outB = 0.0f;
        return;
    }
    outR = rgba[idx];
    outG = rgba[idx + 1];
    outB = rgba[idx + 2];
}
#endif

} // namespace

struct RenderPipelineInvestigator::ResolvedStageGates {
    bool skyAtmosphere = true;
    bool tonemapping = true;
    bool autoExposure = true;
    bool fog = true;
    bool clouds = true;
    bool bloom = true;
    bool sunDisk = true;
    bool postProcess = true;
    bool geometry = true;
    bool grid = true;
};

const char* RenderPipelineInvestigator::SkyIsolationStepName(int step) {
    switch (step) {
        case 0: return "Baseline (clear only)";
        case 1: return "SkyAtmosphere only";
        case 2: return "SkyAtmosphere + Tonemapping";
        case 3: return "SkyAtmosphere + Tonemapping + AutoExposure";
        case 4: return "+ Fog composite";
        case 5: return "+ VolumetricClouds";
        case 6: return "+ Bloom";
        case 7: return "+ Sun disk";
        default: return "Custom";
    }
}

RenderPipelineInvestigator::ResolvedStageGates RenderPipelineInvestigator::ResolveStageGates() const {
    ResolvedStageGates gates{};
    if (!m_Settings.enabled) {
        return gates;
    }

    if (m_Settings.skyIsolationStep >= 0) {
        const int step = m_Settings.skyIsolationStep;
        gates.skyAtmosphere = step >= 1;
        gates.postProcess = step >= 2;
        gates.tonemapping = step >= 2;
        gates.autoExposure = step >= 3;
        gates.fog = step >= 4;
        gates.clouds = step >= 5;
        gates.bloom = step >= 6;
        gates.sunDisk = step >= 7;
        gates.geometry = false;
        gates.grid = false;
        return gates;
    }

    gates.skyAtmosphere = !m_Settings.disableSky;
    gates.tonemapping = !m_Settings.disableToneMapping && !m_Settings.bypassToneMapping;
    gates.autoExposure = !m_Settings.disableAutoExposure;
    gates.fog = !m_Settings.disableFog && !m_Settings.disableAerialPerspective;
    gates.clouds = !m_Settings.disableClouds;
    gates.bloom = !m_Settings.disableBloom;
    gates.sunDisk = !m_Settings.disableSunDisk;
    gates.geometry = !m_Settings.disableGeometry;
    gates.grid = !m_Settings.disableGrid;
    gates.postProcess = !m_Settings.disablePostProcessing;
    return gates;
}

RenderPipelineInvestigator& RenderPipelineInvestigator::Get() {
    static RenderPipelineInvestigator instance;
    return instance;
}

const char* RenderPipelineInvestigator::PassName(RenderPassId pass) {
    switch (pass) {
        case RenderPassId::Clear: return "Clear";
        case RenderPassId::SkyAtmosphere: return "SkyAtmosphere";
        case RenderPassId::Geometry: return "Geometry";
        case RenderPassId::Lighting: return "Lighting";
        case RenderPassId::VolumetricClouds: return "VolumetricClouds";
        case RenderPassId::Fog: return "Fog";
        case RenderPassId::AerialPerspective: return "AerialPerspective";
        case RenderPassId::PostProcessing: return "PostProcessing";
        case RenderPassId::Exposure: return "Exposure";
        case RenderPassId::ToneMapping: return "ToneMapping";
        case RenderPassId::Present: return "Present";
        default: return "Unknown";
    }
}

RenderPipelineInvestigatorSettings RenderPipelineInvestigator::ParseCommandLine(int argc, char* argv[]) {
    RenderPipelineInvestigatorSettings settings{};
    if (const char* env = std::getenv("WE_PIPELINE_INVESTIGATION")) {
        if (env[0] == '1' || std::strcmp(env, "true") == 0) {
            settings.enabled = true;
            settings.enableGpuReadback = true;
        }
    }
    if (const char* envStep = std::getenv("WE_PIPELINE_SKY_ISOLATION_STEP")) {
        settings.skyIsolationStepFromCommandLine = true;
        settings.enabled = true;
        settings.skyIsolationStep = std::atoi(envStep);
        settings.haltOnInvalid = false;
        settings.logEveryFrame = true;
        settings.enableGpuReadback = true;
    }
    if (const char* envStep = std::getenv("WE_PIPELINE_EFFECT_STEP")) {
        settings.enabled = true;
        settings.effectStep = std::atoi(envStep);
        settings.haltOnInvalid = false;
    }
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-PipelineInvestigation") == 0 || std::strcmp(argv[i], "--pipeline-investigation") == 0) {
            settings.enabled = true;
            settings.enableGpuReadback = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineStripAll") == 0 || std::strcmp(argv[i], "--pipeline-strip-all") == 0) {
            settings.enabled = true;
            settings.effectStep = 0;
            settings.haltOnInvalid = false;
            settings.logEveryFrame = false;
            settings.writeReportFile = false;
            continue;
        }
        if (StartsWith(argv[i], "-PipelineSkyIsolationStep=")) {
            settings.skyIsolationStepFromCommandLine = true;
            const int step = std::atoi(argv[i] + std::strlen("-PipelineSkyIsolationStep="));
            settings.skyIsolationStep = step;
            settings.enabled = step >= 0;
            if (step >= 0) {
                settings.haltOnInvalid = false;
                settings.logEveryFrame = true;
                settings.enableGpuReadback = true;
            }
            continue;
        }
        if (StartsWith(argv[i], "-PipelineEffectStep=")) {
            settings.enabled = true;
            settings.effectStep = std::atoi(argv[i] + std::strlen("-PipelineEffectStep="));
            settings.haltOnInvalid = false;
            settings.logEveryFrame = false;
            settings.writeReportFile = false;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoHalt") == 0) {
            settings.haltOnInvalid = false;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoSky") == 0) {
            settings.enabled = true;
            settings.disableSky = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoPost") == 0) {
            settings.enabled = true;
            settings.disablePostProcessing = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoGrid") == 0) {
            settings.enabled = true;
            settings.disableGrid = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoClouds") == 0) {
            settings.enabled = true;
            settings.disableClouds = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoFog") == 0) {
            settings.enabled = true;
            settings.disableFog = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoGeometry") == 0) {
            settings.enabled = true;
            settings.disableGeometry = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoSunDisk") == 0) {
            settings.enabled = true;
            settings.disableSunDisk = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoToneMap") == 0) {
            settings.enabled = true;
            settings.disableToneMapping = true;
            settings.bypassToneMapping = true;
            continue;
        }
        if (StartsWith(argv[i], "-PipelineOnlyPass=")) {
            settings.enabled = true;
            settings.enableGpuReadback = true;
            settings.onlyPass = std::atoi(argv[i] + std::strlen("-PipelineOnlyPass="));
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineConstantBlueSky") == 0) {
            settings.enabled = true;
            settings.forceConstantBlueSky = true;
            settings.disableGeometry = true;
            settings.disableClouds = true;
            settings.disableFog = true;
            settings.disableBloom = true;
            settings.forceExposureOne = true;
            settings.disableAutoExposure = true;
            settings.bypassToneMapping = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineForceExposureOne") == 0) {
            settings.enabled = true;
            settings.forceExposureOne = true;
            settings.disableAutoExposure = true;
        }
        if (std::strcmp(argv[i], "-PipelineBypassToneMap") == 0) {
            settings.enabled = true;
            settings.bypassToneMapping = true;
        }
    }
    return settings;
}

namespace {

std::filesystem::path PipelineSkyIsolationPersistPath() {
    return std::filesystem::path("Saved") / "Config" / "pipeline_sky_isolation.ini";
}

void ApplyPipelineSkyIsolationSection(
    const std::string& key,
    const std::string& value,
    RenderPipelineInvestigatorSettings& settings) {

    if (key == "Enabled") {
        if (ParseConfigBool(value, false)) settings.enabled = true;
    } else if (key == "SkyIsolationStep") {
        const int step = ParseConfigInt(value, -1);
        if (step >= 0) {
            settings.enabled = true;
            settings.skyIsolationStep = step;
        }
    } else if (key == "DisableSky") settings.disableSky = ParseConfigBool(value, settings.disableSky);
    else if (key == "DisableToneMapping") {
        settings.disableToneMapping = ParseConfigBool(value, settings.disableToneMapping);
        if (settings.disableToneMapping) settings.bypassToneMapping = true;
    } else if (key == "DisableAutoExposure") settings.disableAutoExposure = ParseConfigBool(value, settings.disableAutoExposure);
    else if (key == "DisableFog") settings.disableFog = ParseConfigBool(value, settings.disableFog);
    else if (key == "DisableClouds") settings.disableClouds = ParseConfigBool(value, settings.disableClouds);
    else if (key == "DisableBloom") settings.disableBloom = ParseConfigBool(value, settings.disableBloom);
    else if (key == "DisableSunDisk") settings.disableSunDisk = ParseConfigBool(value, settings.disableSunDisk);
    else if (key == "DisableGrid") settings.disableGrid = ParseConfigBool(value, settings.disableGrid);
    else if (key == "DisableGeometry") settings.disableGeometry = ParseConfigBool(value, settings.disableGeometry);
    else if (key == "DisablePostProcessing") settings.disablePostProcessing = ParseConfigBool(value, settings.disablePostProcessing);
    else if (key == "FixedExposureEV") settings.fixedExposureEV = ParseConfigFloat(value, settings.fixedExposureEV);
    else if (key == "FixedExposureMultiplier") settings.fixedExposureMultiplier = ParseConfigFloat(value, settings.fixedExposureMultiplier);
    else if (key == "BypassToneMapping") settings.bypassToneMapping = ParseConfigBool(value, settings.bypassToneMapping);
    else if (key == "ForceExposureOne") {
        settings.forceExposureOne = ParseConfigBool(value, settings.forceExposureOne);
        if (settings.forceExposureOne) settings.disableAutoExposure = true;
    }
}

} // namespace

void RenderPipelineInvestigator::ApplyPersistedOverrides(RenderPipelineInvestigatorSettings& settings) {
    const std::filesystem::path configPath = PipelineSkyIsolationPersistPath();
    std::ifstream file(configPath);
    if (!file) return;

    bool inSection = false;
    std::string line;
    while (std::getline(file, line)) {
        line = TrimConfigValue(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line.front() == '[' && line.back() == ']') {
            const std::string section = line.substr(1, line.size() - 2);
            inSection = section == "PipelineSkyIsolation";
            continue;
        }
        if (!inSection) continue;

        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = TrimConfigValue(line.substr(0, eq));
        const std::string value = line.substr(eq + 1);
        ApplyPipelineSkyIsolationSection(key, value, settings);
    }
}

void RenderPipelineInvestigator::ApplyConfigOverrides(RenderPipelineInvestigatorSettings& settings) {
    const std::filesystem::path configPath = we::core::ResolveEditorConfigPath("function.ini");
    std::ifstream file(configPath);
    if (!file) return;

    bool inSection = false;
    std::string line;
    while (std::getline(file, line)) {
        line = TrimConfigValue(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line.front() == '[' && line.back() == ']') {
            const std::string section = line.substr(1, line.size() - 2);
            inSection = section == "PipelineSkyIsolation";
            continue;
        }
        if (!inSection) continue;

        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = TrimConfigValue(line.substr(0, eq));
        const std::string value = line.substr(eq + 1);

        ApplyPipelineSkyIsolationSection(key, value, settings);
    }
}

void RenderPipelineInvestigator::PersistSettings(const RenderPipelineInvestigatorSettings& settings) const {
    const std::filesystem::path configPath = PipelineSkyIsolationPersistPath();
    std::error_code ec;

    if (!settings.enabled || settings.skyIsolationStep < 0) {
        std::filesystem::remove(configPath, ec);
        return;
    }

    std::filesystem::create_directories(configPath.parent_path(), ec);
    std::ofstream file(configPath);
    if (!file) return;

    file << "[PipelineSkyIsolation]\n";
    file << "Enabled=1\n";
    file << "SkyIsolationStep=" << settings.skyIsolationStep << "\n";
}

void RenderPipelineInvestigator::Configure(const RenderPipelineInvestigatorSettings& settings) {
    m_Settings = settings;
    if (m_Settings.skyIsolationStep >= 0) {
        m_Settings.enableGpuReadback = true;
        m_Settings.haltOnInvalid = false;
        m_Settings.maxHdrComponent = 1.0e12f;
        m_Settings.writeReportFile = true;
    }
    m_WarmupRemaining = m_Settings.warmupFrames;
    m_ShouldHalt = false;
    PersistSettings(settings);
    if (!settings.enabled) return;
    if (settings.skyIsolationStep >= 0) {
        WE_LOG_INFO(LogCategory::Renderer.data(),
            std::string("Sky isolation step ") + std::to_string(settings.skyIsolationStep)
                + " (" + SkyIsolationStepName(settings.skyIsolationStep)
                + "). Advance with -PipelineSkyIsolationStep=N or function.ini [PipelineSkyIsolation].");
    } else if (settings.effectStep >= 0) {
        WE_LOG_INFO(LogCategory::Renderer.data(),
            std::string("Render pipeline effect step ") + std::to_string(settings.effectStep)
                + " (" + EffectStepName(settings.effectStep) + "). Add effects one at a time with -PipelineEffectStep=N.");
    } else {
        WE_LOG_INFO(LogCategory::Renderer.data(), "Render pipeline debug overrides enabled.");
    }
    if (settings.enableGpuReadback) {
        WE_LOG_INFO(LogCategory::Renderer.data(), "Render pipeline GPU readback investigation enabled.");
    }
}

void RenderPipelineInvestigator::BeginFrame(uint64_t frameIndex) {
    if (s_CachedDevice != VK_NULL_HANDLE) {
        for (auto& cp : m_Pending) {
            if (cp.stagingBuffer != VK_NULL_HANDLE) vkDestroyBuffer(s_CachedDevice, cp.stagingBuffer, nullptr);
            if (cp.stagingMemory != VK_NULL_HANDLE) vkFreeMemory(s_CachedDevice, cp.stagingMemory, nullptr);
        }
    }
    m_Pending.clear();
    m_Audit = {};
    m_LastReport = {};
    m_LastReport.frameIndex = frameIndex;
    if (m_WarmupRemaining > 0) --m_WarmupRemaining;
}

void RenderPipelineInvestigator::DestroyPending(const VulkanContext& context) {
    const VkDevice device = context.GetDevice();
    s_CachedDevice = device;
    for (auto& cp : m_Pending) {
        if (cp.stagingBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, cp.stagingBuffer, nullptr);
        if (cp.stagingMemory != VK_NULL_HANDLE) vkFreeMemory(device, cp.stagingMemory, nullptr);
        cp.stagingBuffer = VK_NULL_HANDLE;
        cp.stagingMemory = VK_NULL_HANDLE;
    }
}

void RenderPipelineInvestigator::ApplyEnvironmentOverrides(SceneEnvironmentUniform& uniform) const {
    if (!m_Settings.enabled) return;

    const ResolvedStageGates gates = ResolveStageGates();

    if (m_Settings.forceConstantBlueSky) uniform.atmosphereDebugMode = 104;
    if (!gates.clouds) uniform.enableClouds = 0.0f;
    if (!gates.fog) uniform.enableVolumetricFog = 0.0f;
    if (!gates.bloom) uniform.bloomIntensity = 0.0f;
    if (!gates.sunDisk) uniform.enableSunDisk = 0.0f;

    const bool useFixedExposure = !gates.autoExposure || m_Settings.forceExposureOne || m_Settings.disableAutoExposure;
    if (useFixedExposure) {
        uniform.enableAutoExposure = 0.0f;
        uniform.exposureEV = m_Settings.forceExposureOne ? 0.0f : m_Settings.fixedExposureEV;
        uniform.exposureCompensation = 0.0f;
        uniform.pipelineFixedExposureMultiplier = m_Settings.fixedExposureMultiplier;
    } else {
        uniform.pipelineFixedExposureMultiplier = 0.0f;
    }

    const bool bypassToneMap = !gates.tonemapping || m_Settings.bypassToneMapping || m_Settings.disableToneMapping;
    uniform.pipelineBypassToneMapping = bypassToneMap ? 1 : 0;
}

bool RenderPipelineInvestigator::ShouldRunPostProcess() const {
    if (RendererDebug::Get().IsMinimalRendererActive()) {
        return RendererDebug::Get().ShouldRunPostProcess();
    }
    if (!m_Settings.enabled) return true;
    if (m_Settings.onlyPass >= 0) {
        const auto pass = static_cast<RenderPassId>(m_Settings.onlyPass);
        return pass == RenderPassId::PostProcessing
            || pass == RenderPassId::Exposure
            || pass == RenderPassId::ToneMapping;
    }
    if (m_Settings.effectStep >= 0) {
        return EffectStepAllowsPass(m_Settings.effectStep, RenderPassId::PostProcessing);
    }
    return ResolveStageGates().postProcess;
}

bool RenderPipelineInvestigator::ShouldApplyBloom() const {
    if (RendererDebug::Get().IsMinimalRendererActive()) {
        return RendererDebug::Get().ShouldApplyBloom();
    }
    if (!m_Settings.enabled) return true;
    return ResolveStageGates().bloom;
}

bool RenderPipelineInvestigator::ShouldApplyTonemapping() const {
    if (RendererDebug::Get().IsMinimalRendererActive()) {
        return RendererDebug::Get().ShouldApplyTonemapping();
    }
    if (!m_Settings.enabled) return true;
    return ResolveStageGates().tonemapping && !m_Settings.bypassToneMapping && !m_Settings.disableToneMapping;
}

bool RenderPipelineInvestigator::ShouldApplyAutoExposure() const {
    if (RendererDebug::Get().IsMinimalRendererActive()) {
        return RendererDebug::Get().ShouldApplyAutoExposure();
    }
    if (!m_Settings.enabled) return true;
    return ResolveStageGates().autoExposure && !m_Settings.disableAutoExposure && !m_Settings.forceExposureOne;
}

bool RenderPipelineInvestigator::ShouldRenderSunDisk() const {
    if (!m_Settings.enabled) return true;
    return ResolveStageGates().sunDisk;
}

bool RenderPipelineInvestigator::ShouldRunPass(RenderPassId pass) const {
    if (RendererDebug::Get().IsMinimalRendererActive()) {
        return RendererDebug::Get().ShouldRunPass(pass);
    }
    if (!m_Settings.enabled) return true;
    if (m_Settings.onlyPass >= 0) {
        const int only = m_Settings.onlyPass;
        if (pass == RenderPassId::Clear) return true;
        return static_cast<int>(pass) == only;
    }
    if (m_Settings.effectStep >= 0) {
        return EffectStepAllowsPass(m_Settings.effectStep, pass);
    }

    const ResolvedStageGates gates = ResolveStageGates();
    switch (pass) {
        case RenderPassId::SkyAtmosphere:
            return gates.skyAtmosphere;
        case RenderPassId::Geometry:
        case RenderPassId::Lighting:
            return gates.geometry;
        case RenderPassId::VolumetricClouds:
            return gates.clouds;
        case RenderPassId::Fog:
        case RenderPassId::AerialPerspective:
            return gates.fog;
        case RenderPassId::PostProcessing:
        case RenderPassId::Exposure:
        case RenderPassId::ToneMapping:
            return gates.postProcess;
        default:
            return true;
    }
}

bool RenderPipelineInvestigator::ShouldRenderGrid() const {
    if (RendererDebug::Get().IsMinimalRendererActive()) {
        return RendererDebug::Get().ShouldRenderGrid();
    }
    if (!m_Settings.enabled) return true;
    if (m_Settings.effectStep >= 0 || m_Settings.skyIsolationStep >= 0) return false;
    return !m_Settings.disableGrid;
}

void RenderPipelineInvestigator::RecordCameraAndEnvironment(const glm::vec3& cameraPos, const SceneEnvironmentUniform& env, float gpuAvgLuminance) {
    m_CameraLog.cameraX = cameraPos.x;
    m_CameraLog.cameraY = cameraPos.y;
    m_CameraLog.cameraZ = cameraPos.z;
    m_CameraLog.cameraHeight = cameraPos.y - env.worldOrigin.y;
    m_CameraLog.planetRadius = env.planetRadius;
    m_CameraLog.atmosphereRadius = env.planetRadius + env.atmosphereHeight;
    m_CameraLog.sunDirX = env.sunDirection.x;
    m_CameraLog.sunDirY = env.sunDirection.y;
    m_CameraLog.sunDirZ = env.sunDirection.z;
    m_CameraLog.sunIntensity = env.sunIntensity;
    m_CameraLog.exposureEV = env.exposureEV;
    m_CameraLog.hdrSkyLuminance = env.hdrSkyLuminance;
    m_CameraLog.avgLuminanceGpu = gpuAvgLuminance;
}

void RenderPipelineInvestigator::RecordCameraMatrices(const glm::mat4& view, const glm::mat4& proj) {
    m_CameraView = view;
    m_CameraProj = proj;
    m_HasCameraMatrices = true;
}

void RenderPipelineInvestigator::AuditPassBegin(RenderPassId pass) {
    if (!m_Settings.enabled) return;
    switch (pass) {
        case RenderPassId::Clear: ++m_Audit.clearPassCount; break;
        case RenderPassId::SkyAtmosphere: ++m_Audit.skyPassCount; break;
        case RenderPassId::Geometry:
        case RenderPassId::Lighting: ++m_Audit.geometryPassCount; break;
        case RenderPassId::VolumetricClouds: ++m_Audit.cloudsPassCount; break;
        case RenderPassId::Fog:
        case RenderPassId::AerialPerspective: ++m_Audit.fogPassCount; break;
        case RenderPassId::Exposure: ++m_Audit.exposurePassCount; break;
        case RenderPassId::ToneMapping: ++m_Audit.toneMapPassCount; break;
        case RenderPassId::Present: ++m_Audit.presentPassCount; break;
        default: break;
    }
}

void RenderPipelineInvestigator::AuditPassEnd(RenderPassId) {}

void RenderPipelineInvestigator::EnqueueCheckpoint(
    VkCommandBuffer cmd,
    const VulkanContext& context,
    VkImage colorImage,
    uint32_t width,
    uint32_t height,
    RenderPassId pass) {

    if (!m_Settings.enabled || m_WarmupRemaining > 0 || colorImage == VK_NULL_HANDLE || width == 0 || height == 0
        || !m_Settings.enableGpuReadback) return;

    const VkDeviceSize bytesPerPixel = 8;
    const VkDeviceSize rowPitch = ((width * bytesPerPixel + 255) / 256) * 256;
    const VkDeviceSize bufferSize = rowPitch * height;

    PendingCheckpoint cp{};
    cp.pass = pass;
    cp.width = width;
    cp.height = height;
    context.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        cp.stagingBuffer, cp.stagingMemory);

    const VkImageLayout srcLayout = (pass == RenderPassId::Clear || pass == RenderPassId::SkyAtmosphere || pass == RenderPassId::Geometry || pass == RenderPassId::VolumetricClouds || pass == RenderPassId::Fog)
        ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    context.CmdTransitionImageLayout(cmd, colorImage, srcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = { width, height, 1 };
    vkCmdCopyImageToBuffer(cmd, colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cp.stagingBuffer, 1, &region);

    context.CmdTransitionImageLayout(cmd, colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayout);

    m_Pending.push_back(cp);
}

HdrBufferStats RenderPipelineInvestigator::AnalyzePixels(const std::vector<float>& rgba, float maxComponent) const {
    HdrBufferStats stats{};
    if (rgba.empty()) return stats;
    const int stride = std::max(1, m_Settings.sampleStride);
    stats.minR = stats.minG = stats.minB = 1e30f;
    stats.maxR = stats.maxG = stats.maxB = -1e30f;
    double lumSum = 0.0;
    for (size_t i = 0; i + 2 < rgba.size(); i += static_cast<size_t>(stride) * 4) {
        const float r = rgba[i];
        const float g = rgba[i + 1];
        const float b = rgba[i + 2];
        ++stats.samples;
        if (std::isnan(r) || std::isnan(g) || std::isnan(b)) ++stats.nanCount;
        if (std::isinf(r) || std::isinf(g) || std::isinf(b)) ++stats.infCount;
        if (r < 0.0f || g < 0.0f || b < 0.0f) ++stats.negativeCount;
        if (r > maxComponent || g > maxComponent || b > maxComponent) ++stats.overLimitCount;
        stats.minR = std::min(stats.minR, r); stats.minG = std::min(stats.minG, g); stats.minB = std::min(stats.minB, b);
        stats.maxR = std::max(stats.maxR, r); stats.maxG = std::max(stats.maxG, g); stats.maxB = std::max(stats.maxB, b);
        lumSum += 0.2126 * r + 0.7152 * g + 0.0722 * b;
    }
    stats.avgLuminance = stats.samples > 0 ? static_cast<float>(lumSum / stats.samples) : 0.0f;
    stats.valid = stats.samples > 0;
    return stats;
}

bool RenderPipelineInvestigator::ValidateStats(const HdrBufferStats& stats, bool isLdr, std::string& outReason) const {
    if (!stats.valid) { outReason = "no samples"; return false; }
    if (stats.nanCount > 0) { outReason = "NaN in RGB (" + std::to_string(stats.nanCount) + " samples)"; return false; }
    if (stats.infCount > 0) { outReason = "Inf in RGB (" + std::to_string(stats.infCount) + " samples)"; return false; }
    if (stats.negativeCount > 0) { outReason = "negative RGB (" + std::to_string(stats.negativeCount) + " samples)"; return false; }
    const float limit = isLdr ? 1.05f : m_Settings.maxHdrComponent;
    if (stats.overLimitCount > 0) {
        outReason = "component exceeds limit " + std::to_string(limit)
            + " max=(" + std::to_string(stats.maxR) + "," + std::to_string(stats.maxG) + "," + std::to_string(stats.maxB) + ")";
        return false;
    }
    return true;
}

HdrBufferStats RenderPipelineInvestigator::ReadbackStaging(const VulkanContext& context, const PendingCheckpoint& cp) const {
    uint32_t width = 0;
    uint32_t height = 0;
    const std::vector<float> rgba = ReadbackStagingPixels(context, cp, width, height);
    const bool isLdr = (cp.pass == RenderPassId::ToneMapping || cp.pass == RenderPassId::Present);
    return AnalyzePixels(rgba, isLdr ? 1.05f : m_Settings.maxHdrComponent);
}

std::vector<float> RenderPipelineInvestigator::ReadbackStagingPixels(
    const VulkanContext& context,
    const PendingCheckpoint& cp,
    uint32_t& outWidth,
    uint32_t& outHeight) const {

    outWidth = cp.width;
    outHeight = cp.height;
    std::vector<float> rgba;
    if (cp.stagingBuffer == VK_NULL_HANDLE || cp.width == 0 || cp.height == 0) {
        return rgba;
    }

    const VkDeviceSize bytesPerPixel = 8;
    const VkDeviceSize rowPitch = ((cp.width * bytesPerPixel + 255) / 256) * 256;
    const VkDeviceSize bufferSize = rowPitch * cp.height;
    void* mapped = nullptr;
    vkMapMemory(context.GetDevice(), cp.stagingMemory, 0, bufferSize, 0, &mapped);

    rgba.resize(static_cast<size_t>(cp.width) * cp.height * 4);
    for (uint32_t y = 0; y < cp.height; ++y) {
        const auto* row = reinterpret_cast<const std::uint8_t*>(mapped) + y * rowPitch;
        for (uint32_t x = 0; x < cp.width; ++x) {
            const auto* pixel = row + x * bytesPerPixel;
            const auto r16 = static_cast<std::uint16_t>(pixel[0] | (pixel[1] << 8));
            const auto g16 = static_cast<std::uint16_t>(pixel[2] | (pixel[3] << 8));
            const auto b16 = static_cast<std::uint16_t>(pixel[4] | (pixel[5] << 8));
            const size_t idx = (static_cast<size_t>(y) * cp.width + x) * 4;
            rgba[idx] = HalfToFloat(r16);
            rgba[idx + 1] = HalfToFloat(g16);
            rgba[idx + 2] = HalfToFloat(b16);
            rgba[idx + 3] = 1.0f;
        }
    }
    vkUnmapMemory(context.GetDevice(), cp.stagingMemory);
    return rgba;
}

std::array<SkyElevationProbe, 3> RenderPipelineInvestigator::SampleSkyElevationProbes(
    const std::vector<float>& rgba,
    uint32_t width,
    uint32_t height,
    bool isLdr) const {

    std::array<SkyElevationProbe, 3> probes{};
    static constexpr std::array<const char*, 3> kLabels = { "zenith", "mid", "horizon" };
    static constexpr std::array<float, 3> kElevations = { 85.0f, 45.0f, 3.0f };

#if WE_HAS_GLM
    if (!m_HasCameraMatrices || rgba.empty() || width == 0 || height == 0) {
        return probes;
    }

    const glm::vec3 cameraPos(m_CameraLog.cameraX, m_CameraLog.cameraY, m_CameraLog.cameraZ);
    const glm::vec3 horizontalForward = HorizontalForwardFromCamera(m_CameraView);

    for (size_t i = 0; i < probes.size(); ++i) {
        probes[i].label = kLabels[i];
        probes[i].elevationDeg = kElevations[i];
        const glm::vec3 worldDir = WorldDirectionAtElevation(horizontalForward, kElevations[i]);
        probes[i].onScreen = ProjectWorldDirectionToPixel(
            worldDir,
            m_CameraView,
            m_CameraProj,
            cameraPos,
            width,
            height,
            probes[i].pixelX,
            probes[i].pixelY);
        if (!probes[i].onScreen) {
            continue;
        }
        SampleRgbAt(rgba, width, height, probes[i].pixelX, probes[i].pixelY, probes[i].r, probes[i].g, probes[i].b);
        probes[i].valid = true;
        if (isLdr) {
            probes[i].r = std::clamp(probes[i].r, 0.0f, 1.0f);
            probes[i].g = std::clamp(probes[i].g, 0.0f, 1.0f);
            probes[i].b = std::clamp(probes[i].b, 0.0f, 1.0f);
        }
    }
#else
    (void)rgba;
    (void)width;
    (void)height;
    (void)isLdr;
    for (size_t i = 0; i < probes.size(); ++i) {
        probes[i].label = kLabels[i];
        probes[i].elevationDeg = kElevations[i];
    }
#endif
    return probes;
}

void RenderPipelineInvestigator::LogSkyProbes(const SkyProbeFrameReport& probes) const {
    auto logProbe = [](const char* stage, const SkyElevationProbe& probe) {
        if (!probe.valid) {
            WE_LOG_INFO(LogCategory::Renderer.data(),
                std::string("SKY_PROBE_") + stage + " " + probe.label + "(" + std::to_string(probe.elevationDeg)
                    + "deg) OFF_SCREEN — elevation not visible in current view frustum.");
            return;
        }
        std::ostringstream ss;
        ss << "SKY_PROBE_" << stage << " " << probe.label << "(" << probe.elevationDeg << "deg)"
           << " @(" << probe.pixelX << "," << probe.pixelY << ")"
           << " linear_RGB=(" << probe.r << "," << probe.g << "," << probe.b << ")"
           << " lum=" << (0.2126f * probe.r + 0.7152f * probe.g + 0.0722f * probe.b);
        WE_LOG_INFO(LogCategory::Renderer.data(), ss.str());
    };

    if (probes.hasHdrProbes) {
        for (const auto& probe : probes.hdrProbes) {
            logProbe("HDR", probe);
        }
    }
    if (probes.hasLdrProbes) {
        for (const auto& probe : probes.ldrProbes) {
            logProbe("LDR", probe);
        }
    }
}

void RenderPipelineInvestigator::WriteSkyProbeReport(const PipelineInvestigationReport& report) const {
    if (m_Settings.skyIsolationStep < 0) {
        return;
    }
    if (!report.skyProbes.hasHdrProbes && !report.skyProbes.hasLdrProbes) {
        return;
    }

    std::error_code ec;
    const std::filesystem::path dir = std::filesystem::path("Saved") / "SkyIsolation";
    std::filesystem::create_directories(dir, ec);
    const std::filesystem::path path = dir / "probe_latest.txt";
    std::ofstream file(path);
    if (!file) {
        return;
    }

    file << "frame=" << report.frameIndex << "\n";
    file << "sky_isolation_step=" << m_Settings.skyIsolationStep << " ("
         << SkyIsolationStepName(m_Settings.skyIsolationStep) << ")\n";
    file << "camera=(" << report.cameraLog.cameraX << "," << report.cameraLog.cameraY << ","
         << report.cameraLog.cameraZ << ")\n";

    auto writeProbe = [&](const char* stage, const SkyElevationProbe& probe) {
        file << stage << "_" << probe.label << "_elevation_deg=" << probe.elevationDeg << "\n";
        if (!probe.valid) {
            file << stage << "_" << probe.label << "_status=OFF_SCREEN\n";
            return;
        }
        file << stage << "_" << probe.label << "_pixel=(" << probe.pixelX << "," << probe.pixelY << ")\n";
        file << stage << "_" << probe.label << "_linear_rgb=(" << probe.r << "," << probe.g << "," << probe.b << ")\n";
    };

    if (report.skyProbes.hasHdrProbes) {
        for (const auto& probe : report.skyProbes.hdrProbes) {
            writeProbe("hdr", probe);
        }
    }
    if (report.skyProbes.hasLdrProbes) {
        for (const auto& probe : report.skyProbes.ldrProbes) {
            writeProbe("ldr", probe);
        }
    }

    if (report.skyProbes.hasHdrProbes) {
        const auto& z = report.skyProbes.hdrProbes[0];
        const auto& m = report.skyProbes.hdrProbes[1];
        const auto& h = report.skyProbes.hdrProbes[2];
        if (z.valid && m.valid && h.valid) {
            const float zLum = 0.2126f * z.r + 0.7152f * z.g + 0.0722f * z.b;
            const float mLum = 0.2126f * m.r + 0.7152f * m.g + 0.0722f * m.b;
            const float hLum = 0.2126f * h.r + 0.7152f * h.g + 0.0722f * h.b;
            const float maxDelta = std::max({ std::fabs(zLum - mLum), std::fabs(mLum - hLum), std::fabs(z.r - m.r), std::fabs(m.r - h.r) });
            file << "hdr_variation_lum_delta=" << maxDelta << "\n";
            file << "hdr_flat_suspect=" << (maxDelta < 0.01f ? "yes" : "no") << "\n";
        }
    }
    if (report.skyProbes.hasLdrProbes) {
        const auto& z = report.skyProbes.ldrProbes[0];
        const auto& m = report.skyProbes.ldrProbes[1];
        const auto& h = report.skyProbes.ldrProbes[2];
        if (z.valid && m.valid && h.valid) {
            const float bZenith = z.b / std::max(z.r, 1e-4f);
            const float bHorizon = h.b / std::max(h.r, 1e-4f);
            file << "ldr_zenith_b_over_r=" << bZenith << "\n";
            file << "ldr_horizon_b_over_r=" << bHorizon << "\n";
            file << "ldr_gradient_visible=" << (bZenith > bHorizon + 0.05f ? "yes" : "no") << "\n";
        }
    }
}

void RenderPipelineInvestigator::StoreIntermediateStats(RenderPassId pass, const HdrBufferStats& stats) {
    switch (pass) {
        case RenderPassId::PostProcessing: m_CameraLog.hdrBeforeExposure = stats; break;
        case RenderPassId::Exposure: m_CameraLog.hdrAfterExposure = stats; break;
        case RenderPassId::ToneMapping: m_CameraLog.hdrBeforeToneMap = stats; m_CameraLog.finalLdr = stats; break;
        default: break;
    }
}

PipelineInvestigationReport RenderPipelineInvestigator::FinalizeFrame(const VulkanContext& context, VkFence fence) {
    PipelineInvestigationReport report = m_LastReport;
    report.cameraLog = m_CameraLog;
    report.audit = m_Audit;

    if (!m_Settings.enabled || m_WarmupRemaining > 0) {
        DestroyPending(context);
        m_Pending.clear();
        return report;
    }

    if (!m_Settings.enableGpuReadback) {
        DestroyPending(context);
        m_Pending.clear();
        return report;
    }

    if (fence != VK_NULL_HANDLE) vkWaitForFences(context.GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);

    for (const auto& cp : m_Pending) {
        PassCheckpointResult result{};
        result.pass = cp.pass;
        result.stats = ReadbackStaging(context, cp);
        const bool isLdr = (cp.pass == RenderPassId::ToneMapping || cp.pass == RenderPassId::Present);
        result.failed = !ValidateStats(result.stats, isLdr, result.failureReason);
        report.checkpoints.push_back(result);
        StoreIntermediateStats(cp.pass, result.stats);

        if (result.failed && !report.frameFailed) {
            report.frameFailed = true;
            report.firstFailingPass = cp.pass;
            report.firstFailureReason = result.failureReason;
        }
    }

    if (m_Audit.skyPassCount > 1) {
        report.frameFailed = true;
        if (report.firstFailingPass == RenderPassId::Count) {
            report.firstFailingPass = RenderPassId::SkyAtmosphere;
            report.firstFailureReason = "SkyAtmosphere executed " + std::to_string(m_Audit.skyPassCount) + " times";
            report.minimalFixHint = "Ensure DrawSkyAtmospherePass is called exactly once per frame.";
        }
    }
    if (m_Audit.exposurePassCount > 1 || m_Audit.toneMapPassCount > 1) {
        report.frameFailed = true;
        if (report.firstFailingPass == RenderPassId::Count) {
            report.firstFailingPass = RenderPassId::PostProcessing;
            report.firstFailureReason = "Post exposure/tone map executed more than once";
            report.minimalFixHint = "PostProcessStack::Apply must run exactly once per frame.";
        }
    }

    m_LastReport = report;
    if (m_Settings.logEveryFrame) LogFrameReport(report);
    if (m_Settings.writeReportFile) WriteReportFile(report);
    if (report.frameFailed && m_Settings.haltOnInvalid) m_ShouldHalt = true;

    DestroyPending(context);
    m_Pending.clear();
    return report;
}

void RenderPipelineInvestigator::LogFrameReport(const PipelineInvestigationReport& report) const {
    std::ostringstream ss;
    ss << "PIPELINE frame=" << report.frameIndex
       << " cam=(" << report.cameraLog.cameraX << "," << report.cameraLog.cameraY << "," << report.cameraLog.cameraZ << ")"
       << " height=" << report.cameraLog.cameraHeight
       << " EV=" << report.cameraLog.exposureEV
       << " hdrSkyLum=" << report.cameraLog.hdrSkyLuminance
       << " gpuAvgLum=" << report.cameraLog.avgLuminanceGpu;
    WE_LOG_INFO(LogCategory::Renderer.data(), ss.str());
    for (const auto& cp : report.checkpoints) {
        std::ostringstream line;
        line << "  [" << PassName(cp.pass) << "]"
             << " min=(" << cp.stats.minR << "," << cp.stats.minG << "," << cp.stats.minB << ")"
             << " max=(" << cp.stats.maxR << "," << cp.stats.maxG << "," << cp.stats.maxB << ")"
             << " avgLum=" << cp.stats.avgLuminance
             << " nan=" << cp.stats.nanCount << " inf=" << cp.stats.infCount
             << " neg=" << cp.stats.negativeCount << " over=" << cp.stats.overLimitCount;
        if (cp.failed) line << " FAIL: " << cp.failureReason;
        WE_LOG_INFO(LogCategory::Renderer.data(), line.str());
    }
    if (report.frameFailed) {
        WE_LOG_CRITICAL(LogCategory::Renderer.data(),
            std::string("PIPELINE FIRST FAIL [") + PassName(report.firstFailingPass) + "]: "
                + report.firstFailureReason + (report.minimalFixHint.empty() ? "" : " | fix: " + report.minimalFixHint));
    }
}

void RenderPipelineInvestigator::WriteReportFile(const PipelineInvestigationReport& report) const {
    std::error_code ec;
    const std::filesystem::path dir = std::filesystem::path("Saved") / "PipelineInvestigation";
    std::filesystem::create_directories(dir, ec);
    const std::filesystem::path path = dir / ("frame_" + std::to_string(report.frameIndex) + ".txt");
    std::ofstream file(path);
    if (!file) return;
    file << "frame=" << report.frameIndex << "\n";
    file << "failed=" << (report.frameFailed ? "yes" : "no") << "\n";
    if (report.frameFailed) {
        file << "first_fail_pass=" << PassName(report.firstFailingPass) << "\n";
        file << "reason=" << report.firstFailureReason << "\n";
        file << "fix=" << report.minimalFixHint << "\n";
    }
    for (const auto& cp : report.checkpoints) {
        file << PassName(cp.pass) << " max=(" << cp.stats.maxR << "," << cp.stats.maxG << "," << cp.stats.maxB << ")"
             << " avgLum=" << cp.stats.avgLuminance << " fail=" << (cp.failed ? cp.failureReason : "no") << "\n";
    }
}

#endif
} // namespace we::runtime::renderer
