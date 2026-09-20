// ==============================================================================
// WindEffects — Renderer — ScalabilityManager
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Renderer/Scalability/ScalabilityManager.h"
#include "Renderer/Scalability/CapabilityResolver.h"
#include "Renderer/Scalability/ScalabilityDiagnostics.h"

#include "Core/Logger.h"
#include "Core/LogCategory.h"
#include "Core/Paths.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::renderer {
namespace {
constexpr std::size_t kResolvedVolumetricQualityBytes = sizeof(VolumetricQualitySettings);
static_assert(kResolvedVolumetricQualityBytes == 28, "ScalabilityManager VolumetricQualitySettings size mismatch");

using IniSectionMap = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

std::string Trim(std::string value) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool ParseBool(const std::string& value, bool fallback) {
    const std::string lower = ToLower(Trim(value));
    if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") {
        return true;
    }
    if (lower == "0" || lower == "false" || lower == "no" || lower == "off") {
        return false;
    }
    WE_LOG_WARN(we::LogCategory::Renderer.data(),
        std::string("Scalability INI: invalid bool '") + value + "' — using default");
    return fallback;
}

float ParseFloat(const std::string& value, float fallback) {
    try {
        return std::stof(Trim(value));
    } catch (...) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            std::string("Scalability INI: invalid float '") + value + "' — using default");
        return fallback;
    }
}

uint32_t ParseUInt(const std::string& value, uint32_t fallback) {
    try {
        const long long parsed = std::stoll(Trim(value));
        if (parsed < 0) {
            WE_LOG_WARN(we::LogCategory::Renderer.data(),
                std::string("Scalability INI: negative uint '") + value + "' — using default");
            return fallback;
        }
        return static_cast<uint32_t>(parsed);
    } catch (...) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            std::string("Scalability INI: invalid uint '") + value + "' — using default");
        return fallback;
    }
}

QualityLevel ParseQuality(const std::string& value, QualityLevel fallback) {
    const std::string lower = ToLower(Trim(value));
    if (lower == "disabled" || lower == "off" || lower == "0") {
        return QualityLevel::Disabled;
    }
    if (lower == "low" || lower == "1") {
        return QualityLevel::Low;
    }
    if (lower == "medium" || lower == "med" || lower == "2") {
        return QualityLevel::Medium;
    }
    if (lower == "high" || lower == "3") {
        return QualityLevel::High;
    }
    if (lower == "ultra" || lower == "veryhigh" || lower == "4") {
        return QualityLevel::Ultra;
    }
    if (lower == "epic" || lower == "5") {
        return QualityLevel::Epic;
    }
    WE_LOG_WARN(we::LogCategory::Renderer.data(),
        std::string("Scalability INI: invalid Quality '") + value + "' — using default");
    return fallback;
}

FeatureRequirement ParseRequirement(const std::string& value, FeatureRequirement fallback) {
    const std::string lower = ToLower(Trim(value));
    if (lower == "off" || lower == "0") {
        return FeatureRequirement::Off;
    }
    if (lower == "optional" || lower == "1") {
        return FeatureRequirement::Optional;
    }
    if (lower == "preferred" || lower == "2") {
        return FeatureRequirement::Preferred;
    }
    if (lower == "required" || lower == "3") {
        return FeatureRequirement::Required;
    }
    WE_LOG_WARN(we::LogCategory::Renderer.data(),
        std::string("Scalability INI: invalid FeatureRequirement '") + value + "' — using default");
    return fallback;
}

IniSectionMap LoadIniSections(const std::filesystem::path& path) {
    IniSectionMap sections;
    std::ifstream file(path);
    if (!file.is_open()) {
        return sections;
    }

    std::string currentSection;
    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            currentSection = Trim(line.substr(1, line.size() - 2));
            continue;
        }
        if (currentSection.empty()) {
            continue;
        }
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string key = Trim(line.substr(0, separator));
        const std::string value = Trim(line.substr(separator + 1));
        if (!key.empty()) {
            sections[currentSection][key] = value;
        }
    }
    return sections;
}

const std::unordered_map<std::string, std::string>* FindSection(
    const IniSectionMap& sections,
    const char* name) {
    const auto it = sections.find(name);
    return it != sections.end() ? &it->second : nullptr;
}

std::string SectionGet(
    const std::unordered_map<std::string, std::string>* section,
    const char* key) {
    if (!section) {
        return {};
    }
    const auto it = section->find(key);
    return it != section->end() ? it->second : std::string{};
}

void ApplyEnabledQuality(
    const std::unordered_map<std::string, std::string>* section,
    bool& enabled,
    QualityLevel& quality) {
    if (!section) {
        return;
    }
    if (const auto enabledValue = SectionGet(section, "Enabled"); !enabledValue.empty()) {
        enabled = ParseBool(enabledValue, enabled);
    }
    if (const auto qualityValue = SectionGet(section, "Quality"); !qualityValue.empty()) {
        quality = ParseQuality(qualityValue, quality);
    }
    if (!enabled) {
        quality = QualityLevel::Disabled;
    }
}

bool LoadProfileIni(const std::filesystem::path& path, RenderingProfileDesc& desc) {
    const IniSectionMap sections = LoadIniSections(path);
    if (sections.empty()) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            std::string("Scalability profile empty or unreadable: ") + path.string());
        return false;
    }

    auto& s = desc.settings;

    if (const auto* profile = FindSection(sections, "Profile")) {
        if (const auto name = SectionGet(profile, "Name"); !name.empty()) {
            desc.name = name;
            s.profileName = name;
        }
    }

    if (const auto* resolution = FindSection(sections, "Resolution")) {
        if (const auto v = SectionGet(resolution, "ScreenPercentage"); !v.empty()) {
            s.resolution.screenPercentage = ParseFloat(v, s.resolution.screenPercentage);
        }
        if (const auto v = SectionGet(resolution, "MaxShadowMapResolution"); !v.empty()) {
            s.resolution.maxShadowMapResolution = ParseUInt(v, s.resolution.maxShadowMapResolution);
        }
        if (const auto v = SectionGet(resolution, "MSAASamples"); !v.empty()) {
            s.resolution.msaaSamples = ParseUInt(v, s.resolution.msaaSamples);
        }
        if (const auto v = SectionGet(resolution, "DynamicResolution"); !v.empty()) {
            s.resolution.dynamicResolution = ParseBool(v, s.resolution.dynamicResolution);
        }
    }

    if (const auto* geometry = FindSection(sections, "Geometry")) {
        if (const auto v = SectionGet(geometry, "Quality"); !v.empty()) {
            s.geometry.quality = ParseQuality(v, s.geometry.quality);
        }
        if (const auto v = SectionGet(geometry, "LodBias"); !v.empty()) {
            s.geometry.lodBias = ParseFloat(v, s.geometry.lodBias);
        }
        if (const auto v = SectionGet(geometry, "MeshShaders"); !v.empty()) {
            s.geometry.meshShaders = ParseBool(v, s.geometry.meshShaders);
        }
        if (const auto v = SectionGet(geometry, "MeshShaderRequirement"); !v.empty()) {
            s.geometry.meshShaderRequirement = ParseRequirement(v, s.geometry.meshShaderRequirement);
        }
    }

    if (const auto* textures = FindSection(sections, "Textures")) {
        if (const auto v = SectionGet(textures, "Quality"); !v.empty()) {
            s.textures.quality = ParseQuality(v, s.textures.quality);
        }
        if (const auto v = SectionGet(textures, "Anisotropy"); !v.empty()) {
            s.textures.anisotropy = ParseFloat(v, s.textures.anisotropy);
        }
        if (const auto v = SectionGet(textures, "StreamingPoolMb"); !v.empty()) {
            s.textures.streamingPoolMb = ParseUInt(v, s.textures.streamingPoolMb);
        }
    }

    if (const auto* lighting = FindSection(sections, "Lighting")) {
        ApplyEnabledQuality(lighting, s.lighting.enabled, s.lighting.quality);
        if (const auto v = SectionGet(lighting, "MaxLocalLights"); !v.empty()) {
            s.lighting.maxLocalLights = ParseUInt(v, s.lighting.maxLocalLights);
        }
    }

    if (const auto* shadows = FindSection(sections, "Shadows")) {
        ApplyEnabledQuality(shadows, s.shadows.enabled, s.shadows.quality);
        if (const auto v = SectionGet(shadows, "CascadeCount"); !v.empty()) {
            s.shadows.cascadeCount = ParseUInt(v, s.shadows.cascadeCount);
        }
        if (const auto v = SectionGet(shadows, "ResolutionScale"); !v.empty()) {
            s.shadows.resolutionScale = ParseFloat(v, s.shadows.resolutionScale);
        }
        if (const auto v = SectionGet(shadows, "SoftShadows"); !v.empty()) {
            s.shadows.softShadows = ParseBool(v, s.shadows.softShadows);
        }
        if (const auto v = SectionGet(shadows, "ContactShadows"); !v.empty()) {
            s.shadows.contactShadows = ParseBool(v, s.shadows.contactShadows);
        }
        if (const auto v = SectionGet(shadows, "ShadowDistance"); !v.empty()) {
            s.shadows.shadowDistance = ParseFloat(v, s.shadows.shadowDistance);
        }
        if (const auto v = SectionGet(shadows, "ShadowNear"); !v.empty()) {
            s.shadows.shadowNear = ParseFloat(v, s.shadows.shadowNear);
        }
        if (const auto v = SectionGet(shadows, "CascadeSplitLambda"); !v.empty()) {
            s.shadows.cascadeSplitLambda = ParseFloat(v, s.shadows.cascadeSplitLambda);
        }
        if (const auto v = SectionGet(shadows, "CascadeBlend"); !v.empty()) {
            s.shadows.cascadeBlend = ParseFloat(v, s.shadows.cascadeBlend);
        }
        if (const auto v = SectionGet(shadows, "DepthBias"); !v.empty()) {
            s.shadows.depthBias = ParseFloat(v, s.shadows.depthBias);
        }
        if (const auto v = SectionGet(shadows, "NormalBias"); !v.empty()) {
            s.shadows.normalBias = ParseFloat(v, s.shadows.normalBias);
        }
        if (const auto v = SectionGet(shadows, "FilterRadius"); !v.empty()) {
            s.shadows.filterRadius = ParseFloat(v, s.shadows.filterRadius);
        }
        if (const auto v = SectionGet(shadows, "ContactShadowLength"); !v.empty()) {
            s.shadows.contactShadowLength = ParseFloat(v, s.shadows.contactShadowLength);
        }
    }

    if (const auto* reflections = FindSection(sections, "Reflections")) {
        ApplyEnabledQuality(reflections, s.reflections.enabled, s.reflections.quality);
        if (const auto v = SectionGet(reflections, "ScreenSpace"); !v.empty()) {
            s.reflections.screenSpace = ParseBool(v, s.reflections.screenSpace);
        }
        if (const auto v = SectionGet(reflections, "RayTraced"); !v.empty()) {
            s.reflections.rayTraced = ParseBool(v, s.reflections.rayTraced);
        }
        if (const auto v = SectionGet(reflections, "RayTracingRequirement"); !v.empty()) {
            s.reflections.rayTracingRequirement = ParseRequirement(v, s.reflections.rayTracingRequirement);
        }
    }

    // Accept either [GI] or [GlobalIllumination]
    const auto* gi = FindSection(sections, "GI");
    if (!gi) {
        gi = FindSection(sections, "GlobalIllumination");
    }
    if (gi) {
        ApplyEnabledQuality(gi, s.globalIllumination.enabled, s.globalIllumination.quality);
        if (const auto v = SectionGet(gi, "RayTraced"); !v.empty()) {
            s.globalIllumination.rayTraced = ParseBool(v, s.globalIllumination.rayTraced);
        }
        if (const auto v = SectionGet(gi, "RayTracingRequirement"); !v.empty()) {
            s.globalIllumination.rayTracingRequirement =
                ParseRequirement(v, s.globalIllumination.rayTracingRequirement);
        }
    }

    if (const auto* terrain = FindSection(sections, "Terrain")) {
        ApplyEnabledQuality(terrain, s.terrain.enabled, s.terrain.quality);
        if (const auto v = SectionGet(terrain, "LodBias"); !v.empty()) {
            s.terrain.lodBias = ParseFloat(v, s.terrain.lodBias);
        }
        if (const auto v = SectionGet(terrain, "ClipmapLevels"); !v.empty()) {
            s.terrain.clipmapLevels = ParseUInt(v, s.terrain.clipmapLevels);
        }
    }

    if (const auto* foliage = FindSection(sections, "Foliage")) {
        ApplyEnabledQuality(foliage, s.foliage.enabled, s.foliage.quality);
        if (const auto v = SectionGet(foliage, "DensityScale"); !v.empty()) {
            s.foliage.densityScale = ParseFloat(v, s.foliage.densityScale);
        }
        if (const auto v = SectionGet(foliage, "LodBias"); !v.empty()) {
            s.foliage.lodBias = ParseFloat(v, s.foliage.lodBias);
        }
    }

    if (const auto* atmosphere = FindSection(sections, "Atmosphere")) {
        ApplyEnabledQuality(atmosphere, s.atmosphere.enabled, s.atmosphere.quality);
    }

    if (const auto* volumetrics = FindSection(sections, "Volumetrics")) {
        ApplyEnabledQuality(volumetrics, s.volumetrics.enabled, s.volumetrics.quality);
        if (const auto v = SectionGet(volumetrics, "ResolutionScale"); !v.empty()) {
            s.volumetrics.resolutionScale = ParseFloat(v, s.volumetrics.resolutionScale);
        }
        if (const auto v = SectionGet(volumetrics, "MaxSteps"); !v.empty()) {
            s.volumetrics.maxSteps = ParseUInt(v, s.volumetrics.maxSteps);
        }
        if (const auto v = SectionGet(volumetrics, "LightSteps"); !v.empty()) {
            s.volumetrics.lightSteps = ParseUInt(v, s.volumetrics.lightSteps);
        }
        if (const auto v = SectionGet(volumetrics, "ShadowSteps"); !v.empty()) {
            s.volumetrics.shadowSteps = ParseUInt(v, s.volumetrics.shadowSteps);
        }
        if (const auto v = SectionGet(volumetrics, "TemporalQuality"); !v.empty()) {
            s.volumetrics.temporalQuality = ParseUInt(v, s.volumetrics.temporalQuality);
        }
        if (const auto v = SectionGet(volumetrics, "TemporalBlend"); !v.empty()) {
            s.volumetrics.temporalBlend = ParseFloat(v, s.volumetrics.temporalBlend);
        }
    }

    if (const auto* water = FindSection(sections, "Water")) {
        ApplyEnabledQuality(water, s.water.enabled, s.water.quality);
        if (const auto v = SectionGet(water, "Reflections"); !v.empty()) {
            s.water.reflections = ParseBool(v, s.water.reflections);
        }
        if (const auto v = SectionGet(water, "Caustics"); !v.empty()) {
            s.water.caustics = ParseBool(v, s.water.caustics);
        }
    }

    if (const auto* particles = FindSection(sections, "Particles")) {
        ApplyEnabledQuality(particles, s.particles.enabled, s.particles.quality);
        if (const auto v = SectionGet(particles, "BudgetScale"); !v.empty()) {
            s.particles.budgetScale = ParseFloat(v, s.particles.budgetScale);
        }
    }

    if (const auto* post = FindSection(sections, "PostProcess")) {
        ApplyEnabledQuality(post, s.postProcess.enabled, s.postProcess.quality);
        if (const auto v = SectionGet(post, "Bloom"); !v.empty()) {
            s.postProcess.bloom = ParseBool(v, s.postProcess.bloom);
        }
        if (const auto v = SectionGet(post, "MotionBlur"); !v.empty()) {
            s.postProcess.motionBlur = ParseBool(v, s.postProcess.motionBlur);
        }
        if (const auto v = SectionGet(post, "AmbientOcclusion"); !v.empty()) {
            s.postProcess.ambientOcclusion = ParseBool(v, s.postProcess.ambientOcclusion);
        }
        if (const auto v = SectionGet(post, "TemporalAA"); !v.empty()) {
            s.postProcess.temporalAA = ParseBool(v, s.postProcess.temporalAA);
        }
    }

    if (const auto* lod = FindSection(sections, "LOD")) {
        if (const auto v = SectionGet(lod, "GlobalBias"); !v.empty()) {
            s.lod.globalBias = ParseFloat(v, s.lod.globalBias);
        }
        if (const auto v = SectionGet(lod, "CullDistanceScale"); !v.empty()) {
            s.lod.cullDistanceScale = ParseFloat(v, s.lod.cullDistanceScale);
        }
    }

    if (const auto* streaming = FindSection(sections, "Streaming")) {
        if (const auto v = SectionGet(streaming, "TexturePoolMb"); !v.empty()) {
            s.streaming.texturePoolMb = ParseUInt(v, s.streaming.texturePoolMb);
        }
        if (const auto v = SectionGet(streaming, "MeshPoolMb"); !v.empty()) {
            s.streaming.meshPoolMb = ParseUInt(v, s.streaming.meshPoolMb);
        }
        if (const auto v = SectionGet(streaming, "IoBudgetScale"); !v.empty()) {
            s.streaming.ioBudgetScale = ParseFloat(v, s.streaming.ioBudgetScale);
        }
    }

    return true;
}

std::filesystem::path ResolveProfileDirectory() {
    auto& paths = we::core::PathService::Get();
    std::vector<std::filesystem::path> candidates = {
        paths.EngineConfigRoot() / "Runtime" / "Scalability" / "Profiles",
        paths.ConfigRoot() / "Runtime" / "Scalability" / "Profiles",
    };
    if (const auto repo = we::core::PathService::FindRepositoryRoot(paths.ExecutableDirectory())) {
        candidates.push_back(*repo / "Engine" / "Config" / "Runtime" / "Scalability" / "Profiles");
    }
    if (const auto found = we::core::PathService::FindExisting(candidates)) {
        return *found;
    }
    return {};
}

} // namespace

RenderingProfileDesc ScalabilityManager::MakeBuiltinProfile(RenderingProfileId id) {
    RenderingProfileDesc desc;
    desc.id = id;
    desc.settings.profileId = id;

    switch (id) {
    case RenderingProfileId::HighEnd:
        // Production target — hosts future AAA feature systems via settings slices.
        desc.name = "High-End";
        desc.settings.profileName = desc.name;
        desc.settings.resolution = {100.0f, 4096, 1, false};
        desc.settings.geometry = {QualityLevel::Ultra, 0.0f, true, FeatureRequirement::Preferred};
        desc.settings.textures = {QualityLevel::Ultra, 16.0f, 1024};
        desc.settings.lighting = {true, QualityLevel::High, 128};
        desc.settings.shadows = {true, QualityLevel::Ultra, 4, 1.0f, true, true};
        desc.settings.reflections = {true, QualityLevel::High, true, true, FeatureRequirement::Preferred};
        desc.settings.globalIllumination = {true, QualityLevel::High, true, FeatureRequirement::Preferred};
        desc.settings.terrain = {true, QualityLevel::Ultra, 0.75f, 10};
        desc.settings.foliage = {true, QualityLevel::Ultra, 1.0f, 0.0f};
        desc.settings.atmosphere = {true, QualityLevel::High};
        desc.settings.volumetrics = {true, QualityLevel::High, 0.75f, 192};
        desc.settings.water = {true, QualityLevel::High, true, true};
        desc.settings.particles = {true, QualityLevel::High, 1.0f};
        desc.settings.postProcess = {true, QualityLevel::High, true, true, true, true};
        desc.settings.lod = {0.0f, 1.0f};
        desc.settings.streaming = {1024, 512, 1.0f};
        break;

    case RenderingProfileId::Balanced:
        // Dormant scalability preset — not actively tuned this phase.
        desc.name = "Balanced";
        desc.settings.profileName = desc.name;
        desc.settings.resolution = {85.0f, 2048, 1, true};
        desc.settings.geometry = {QualityLevel::Medium, 0.5f, false, FeatureRequirement::Optional};
        desc.settings.textures = {QualityLevel::Medium, 4.0f, 256};
        desc.settings.lighting = {true, QualityLevel::Medium, 32};
        desc.settings.shadows = {true, QualityLevel::Medium, 2, 0.75f, true, false};
        desc.settings.reflections = {true, QualityLevel::Low, true, false, FeatureRequirement::Off};
        desc.settings.globalIllumination = {true, QualityLevel::Low, false, FeatureRequirement::Off};
        desc.settings.terrain = {true, QualityLevel::Medium, 1.25f, 6};
        desc.settings.foliage = {true, QualityLevel::Medium, 0.65f, 0.5f};
        desc.settings.atmosphere = {true, QualityLevel::Medium};
        desc.settings.volumetrics = {true, QualityLevel::Low, 0.35f, 16};
        desc.settings.water = {true, QualityLevel::Medium, true, false};
        desc.settings.particles = {true, QualityLevel::Medium, 0.6f};
        desc.settings.postProcess = {true, QualityLevel::Medium, true, false, true, true};
        desc.settings.lod = {0.5f, 0.75f};
        desc.settings.streaming = {256, 128, 0.75f};
        break;

    case RenderingProfileId::Low:
        // Dormant scalability preset — not actively tuned this phase.
        desc.name = "Low";
        desc.settings.profileName = desc.name;
        desc.settings.resolution = {70.0f, 1024, 1, true};
        desc.settings.geometry = {QualityLevel::Low, 1.0f, false, FeatureRequirement::Off};
        desc.settings.textures = {QualityLevel::Low, 1.0f, 128};
        desc.settings.lighting = {true, QualityLevel::Low, 8};
        desc.settings.shadows = {true, QualityLevel::Low, 1, 0.5f, false, false};
        desc.settings.reflections = {false, QualityLevel::Disabled, false, false, FeatureRequirement::Off};
        desc.settings.globalIllumination = {false, QualityLevel::Disabled, false, FeatureRequirement::Off};
        desc.settings.terrain = {true, QualityLevel::Low, 1.75f, 4};
        desc.settings.foliage = {true, QualityLevel::Low, 0.35f, 1.0f};
        desc.settings.atmosphere = {true, QualityLevel::Low};
        desc.settings.volumetrics = {false, QualityLevel::Disabled, 0.0f, 0};
        desc.settings.water = {true, QualityLevel::Low, false, false};
        desc.settings.particles = {true, QualityLevel::Low, 0.35f};
        desc.settings.postProcess = {true, QualityLevel::Low, false, false, false, false};
        desc.settings.lod = {1.0f, 0.5f};
        desc.settings.streaming = {128, 64, 0.5f};
        break;

    case RenderingProfileId::Custom:
        desc = MakeBuiltinProfile(RenderingProfileId::HighEnd);
        desc.id = RenderingProfileId::Custom;
        desc.name = "Custom";
        desc.settings.profileId = RenderingProfileId::Custom;
        desc.settings.profileName = desc.name;
        break;
    }

    return desc;
}

ScalabilityUpdateFlags ScalabilityManager::DiffSettings(
    const ResolvedRenderingSettings& previous,
    const ResolvedRenderingSettings& next) {
    ScalabilityUpdateFlags flags = ScalabilityUpdateFlags::None;

    if (previous.profileId != next.profileId) {
        flags = flags | ScalabilityUpdateFlags::RebuildRenderGraph;
    }

    auto mark = [&](bool changed, ScalabilityUpdateFlags bit) {
        if (changed) {
            flags = flags | bit;
        }
    };

    mark(previous.shadows.enabled != next.shadows.enabled
            || previous.shadows.cascadeCount != next.shadows.cascadeCount
            || previous.shadows.resolutionScale != next.shadows.resolutionScale,
        ScalabilityUpdateFlags::RecreateResources | ScalabilityUpdateFlags::RebuildRenderGraph
            | ScalabilityUpdateFlags::RecreatePipelines);

    mark(previous.volumetrics.enabled != next.volumetrics.enabled,
        ScalabilityUpdateFlags::RebuildRenderGraph | ScalabilityUpdateFlags::RecreateResources);

    mark(previous.geometry.meshShaders != next.geometry.meshShaders
            || previous.reflections.rayTraced != next.reflections.rayTraced
            || previous.globalIllumination.rayTraced != next.globalIllumination.rayTraced,
        ScalabilityUpdateFlags::RecreatePipelines);

    mark(previous.resolution.msaaSamples != next.resolution.msaaSamples,
        ScalabilityUpdateFlags::RecreateResources | ScalabilityUpdateFlags::RestartRequired);

    if (flags == ScalabilityUpdateFlags::None) {
        flags = ScalabilityUpdateFlags::UpdateSettings;
    } else if (!HasFlag(flags, ScalabilityUpdateFlags::UpdateSettings)) {
        flags = flags | ScalabilityUpdateFlags::UpdateSettings;
    }
    return flags;
}

ScalabilityManager::ScalabilityManager() = default;
ScalabilityManager::~ScalabilityManager() {
    Shutdown();
}

void ScalabilityManager::Initialize() {
    if (m_Initialized) {
        return;
    }
    ReloadProfiles();
    m_ActiveProfileId = RenderingProfileId::HighEnd;
    ResolveActiveProfile();
    m_Published = m_Pending;
    m_PendingDirty = false;
    m_Initialized = true;
    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        std::string("Scalability initialized — profile: ") + m_Published.profileName);
}

void ScalabilityManager::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    m_Profiles.clear();
    m_PendingDirty = false;
    m_Initialized = false;
}

bool ScalabilityManager::ReloadProfiles() {
    m_Profiles.clear();
    const RenderingProfileId ids[] = {
        RenderingProfileId::HighEnd,
        RenderingProfileId::Balanced,
        RenderingProfileId::Low,
    };
    for (const auto id : ids) {
        m_Profiles[id] = MakeBuiltinProfile(id);
    }

    const auto dir = ResolveProfileDirectory();
    if (dir.empty()) {
        WE_LOG_INFO(we::LogCategory::Renderer.data(),
            "Scalability: no profile directory — using built-in High-End defaults");
        return true;
    }

    // Authoritative path: INI overlays builtins. Only HighEnd is expected on disk
    // during the AAA-first phase; Balanced/Low stay as dormant builtins.
    for (const auto id : ids) {
        const auto path = dir / (std::string(ProfileFileStem(id)) + ".ini");
        if (!std::filesystem::exists(path)) {
            continue;
        }
        RenderingProfileDesc desc = MakeBuiltinProfile(id);
        if (LoadProfileIni(path, desc)) {
            m_Profiles[id] = std::move(desc);
            WE_LOG_INFO(we::LogCategory::Renderer.data(),
                std::string("Scalability loaded profile: ") + path.string());
        }
    }
    return true;
}

void ScalabilityManager::SetRHICapabilities(const we::rhi::RHICapabilities& caps) {
    m_Capabilities = caps;
    if (m_Initialized) {
        ResolveActiveProfile();
    }
}

void ScalabilityManager::SetRHIBackend(we::rhi::RHIBackend backend) {
    m_Backend = backend;
}

void ScalabilityManager::QueueResolved(ResolvedRenderingSettings resolved) {
    m_Pending = std::move(resolved);
    m_PendingDirty = true;
}

void ScalabilityManager::ResolveActiveProfile() {
    auto it = m_Profiles.find(m_ActiveProfileId);
    if (it == m_Profiles.end()) {
        m_Profiles[m_ActiveProfileId] = MakeBuiltinProfile(m_ActiveProfileId);
        it = m_Profiles.find(m_ActiveProfileId);
    }
    QueueResolved(CapabilityResolver::Resolve(it->second, m_Capabilities));
}

ScalabilityUpdateFlags ScalabilityManager::SetProfile(RenderingProfileId id) {
    if (id == RenderingProfileId::Custom) {
        id = RenderingProfileId::HighEnd;
    }

    const ResolvedRenderingSettings previous = m_Pending;
    m_ActiveProfileId = id;
    ResolveActiveProfile();
    const ScalabilityUpdateFlags flags = DiffSettings(previous, m_Pending);

    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        std::string("Scalability profile set to: ") + m_Pending.profileName);
    for (const auto& note : m_Pending.fallbackNotes) {
        WE_LOG_INFO(we::LogCategory::Renderer.data(), std::string("  fallback: ") + note);
    }
    return flags;
}

void ScalabilityManager::PublishFrameSettings() {
    if (!m_PendingDirty) {
        return;
    }
    m_Published = m_Pending;
    m_PendingDirty = false;
}

ScalabilityDiagnosticsSnapshot ScalabilityManager::CaptureDiagnostics() const {
    ScalabilityDiagnosticsSnapshot snap;
    snap.profileId = m_ActiveProfileId;
    snap.profileName = m_Pending.profileName;
    snap.backend = m_Backend;
    snap.capabilities = m_Capabilities;
    snap.settings = m_Published;
    return snap;
}

} // namespace we::runtime::renderer
