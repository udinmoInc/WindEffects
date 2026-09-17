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
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace we::runtime::renderer {
namespace {

QualityLevel QualityFromInt(int value) {
    value = std::clamp(value, 0, 5);
    return static_cast<QualityLevel>(value);
}

#if WE_HAS_NLOHMANN_JSON
void ReadQualityObject(
    const nlohmann::json& root,
    const char* key,
    bool& enabled,
    QualityLevel& quality) {
    if (!root.contains(key) || !root[key].is_object()) {
        return;
    }
    const auto& obj = root[key];
    if (obj.contains("Enabled")) {
        enabled = obj["Enabled"].get<bool>();
    }
    if (obj.contains("Quality")) {
        quality = QualityFromInt(obj["Quality"].get<int>());
        if (!enabled) {
            quality = QualityLevel::Disabled;
        }
    }
}

bool LoadProfileJson(const std::filesystem::path& path, RenderingProfileDesc& desc) {
    std::ifstream input(path);
    if (!input) {
        return false;
    }
    nlohmann::json root;
    try {
        input >> root;
    } catch (const std::exception& ex) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            std::string("Scalability profile parse failed: ") + path.string() + " — " + ex.what());
        return false;
    }

    if (root.contains("Name") && root["Name"].is_string()) {
        desc.name = root["Name"].get<std::string>();
        desc.settings.profileName = desc.name;
    }

    auto& s = desc.settings;
    if (root.contains("Resolution") && root["Resolution"].is_object()) {
        const auto& r = root["Resolution"];
        s.resolution.screenPercentage = r.value("ScreenPercentage", s.resolution.screenPercentage);
        s.resolution.maxShadowMapResolution = r.value("MaxShadowMapResolution", s.resolution.maxShadowMapResolution);
        s.resolution.msaaSamples = r.value("MSAASamples", s.resolution.msaaSamples);
        s.resolution.dynamicResolution = r.value("DynamicResolution", s.resolution.dynamicResolution);
    }

    ReadQualityObject(root, "Shadows", s.shadows.enabled, s.shadows.quality);
    if (root.contains("Shadows") && root["Shadows"].is_object()) {
        const auto& sh = root["Shadows"];
        s.shadows.cascadeCount = sh.value("CascadeCount", s.shadows.cascadeCount);
        s.shadows.resolutionScale = sh.value("ResolutionScale", s.shadows.resolutionScale);
        s.shadows.softShadows = sh.value("SoftShadows", s.shadows.softShadows);
        s.shadows.contactShadows = sh.value("ContactShadows", s.shadows.contactShadows);
    }

    ReadQualityObject(root, "Lighting", s.lighting.enabled, s.lighting.quality);
    if (root.contains("Lighting") && root["Lighting"].is_object()) {
        s.lighting.maxLocalLights = root["Lighting"].value("MaxLocalLights", s.lighting.maxLocalLights);
    }

    if (root.contains("Geometry") && root["Geometry"].is_object()) {
        const auto& g = root["Geometry"];
        s.geometry.quality = QualityFromInt(g.value("Quality", static_cast<int>(s.geometry.quality)));
        s.geometry.lodBias = g.value("LodBias", s.geometry.lodBias);
        s.geometry.meshShaders = g.value("MeshShaders", s.geometry.meshShaders);
        if (g.contains("MeshShaderRequirement")) {
            const int req = g["MeshShaderRequirement"].get<int>();
            s.geometry.meshShaderRequirement = static_cast<FeatureRequirement>(std::clamp(req, 0, 3));
        }
    }

    if (root.contains("Textures") && root["Textures"].is_object()) {
        const auto& t = root["Textures"];
        s.textures.quality = QualityFromInt(t.value("Quality", static_cast<int>(s.textures.quality)));
        s.textures.anisotropy = t.value("Anisotropy", s.textures.anisotropy);
        s.textures.streamingPoolMb = t.value("StreamingPoolMb", s.textures.streamingPoolMb);
    }

    ReadQualityObject(root, "Reflections", s.reflections.enabled, s.reflections.quality);
    if (root.contains("Reflections") && root["Reflections"].is_object()) {
        const auto& r = root["Reflections"];
        s.reflections.screenSpace = r.value("ScreenSpace", s.reflections.screenSpace);
        s.reflections.rayTraced = r.value("RayTraced", s.reflections.rayTraced);
    }

    ReadQualityObject(root, "GI", s.globalIllumination.enabled, s.globalIllumination.quality);
    if (root.contains("GI") && root["GI"].is_object()) {
        s.globalIllumination.rayTraced = root["GI"].value("RayTraced", s.globalIllumination.rayTraced);
    }

    ReadQualityObject(root, "Terrain", s.terrain.enabled, s.terrain.quality);
    if (root.contains("Terrain") && root["Terrain"].is_object()) {
        const auto& t = root["Terrain"];
        s.terrain.lodBias = t.value("LodBias", s.terrain.lodBias);
        s.terrain.clipmapLevels = t.value("ClipmapLevels", s.terrain.clipmapLevels);
    }

    ReadQualityObject(root, "Foliage", s.foliage.enabled, s.foliage.quality);
    if (root.contains("Foliage") && root["Foliage"].is_object()) {
        const auto& f = root["Foliage"];
        s.foliage.densityScale = f.value("DensityScale", s.foliage.densityScale);
        s.foliage.lodBias = f.value("LodBias", s.foliage.lodBias);
    }

    ReadQualityObject(root, "Atmosphere", s.atmosphere.enabled, s.atmosphere.quality);
    ReadQualityObject(root, "Volumetrics", s.volumetrics.enabled, s.volumetrics.quality);
    if (root.contains("Volumetrics") && root["Volumetrics"].is_object()) {
        const auto& v = root["Volumetrics"];
        s.volumetrics.resolutionScale = v.value("ResolutionScale", s.volumetrics.resolutionScale);
        s.volumetrics.maxSteps = v.value("MaxSteps", s.volumetrics.maxSteps);
    }

    ReadQualityObject(root, "Clouds", s.clouds.enabled, s.clouds.quality);
    if (root.contains("Clouds") && root["Clouds"].is_object()) {
        const auto& c = root["Clouds"];
        s.clouds.maxSteps = c.value("MaxSteps", s.clouds.maxSteps);
        s.clouds.resolutionScale = c.value("ResolutionScale", s.clouds.resolutionScale);
        s.clouds.temporalReprojection = c.value("TemporalReprojection", s.clouds.temporalReprojection);
    }

    ReadQualityObject(root, "Water", s.water.enabled, s.water.quality);
    if (root.contains("Water") && root["Water"].is_object()) {
        const auto& w = root["Water"];
        s.water.reflections = w.value("Reflections", s.water.reflections);
        s.water.caustics = w.value("Caustics", s.water.caustics);
    }

    ReadQualityObject(root, "Particles", s.particles.enabled, s.particles.quality);
    if (root.contains("Particles") && root["Particles"].is_object()) {
        s.particles.budgetScale = root["Particles"].value("BudgetScale", s.particles.budgetScale);
    }

    ReadQualityObject(root, "PostProcess", s.postProcess.enabled, s.postProcess.quality);
    if (root.contains("PostProcess") && root["PostProcess"].is_object()) {
        const auto& p = root["PostProcess"];
        s.postProcess.bloom = p.value("Bloom", s.postProcess.bloom);
        s.postProcess.motionBlur = p.value("MotionBlur", s.postProcess.motionBlur);
        s.postProcess.ambientOcclusion = p.value("AmbientOcclusion", s.postProcess.ambientOcclusion);
        s.postProcess.temporalAA = p.value("TemporalAA", s.postProcess.temporalAA);
    }

    if (root.contains("LOD") && root["LOD"].is_object()) {
        const auto& l = root["LOD"];
        s.lod.globalBias = l.value("GlobalBias", s.lod.globalBias);
        s.lod.cullDistanceScale = l.value("CullDistanceScale", s.lod.cullDistanceScale);
    }

    if (root.contains("Streaming") && root["Streaming"].is_object()) {
        const auto& st = root["Streaming"];
        s.streaming.texturePoolMb = st.value("TexturePoolMb", s.streaming.texturePoolMb);
        s.streaming.meshPoolMb = st.value("MeshPoolMb", s.streaming.meshPoolMb);
        s.streaming.ioBudgetScale = st.value("IoBudgetScale", s.streaming.ioBudgetScale);
    }

    return true;
}
#endif

std::filesystem::path ResolveProfileDirectory() {
    auto& paths = we::core::PathService::Get();
    std::vector<std::filesystem::path> candidates = {
        paths.EngineConfigRoot() / "Runtime" / "Scalability" / "Profiles",
        paths.ConfigRoot() / "Runtime" / "Scalability" / "Profiles",
        paths.EngineConfigRoot() / "Runtime" / "Scalability",
        paths.ConfigRoot() / "Runtime" / "Scalability",
    };
    if (const auto repo = we::core::PathService::FindRepositoryRoot(paths.ExecutableDirectory())) {
        candidates.push_back(*repo / "Engine" / "Config" / "Runtime" / "Scalability" / "Profiles");
        candidates.push_back(*repo / "Engine" / "Config" / "Runtime" / "Scalability");
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
        desc.settings.volumetrics = {true, QualityLevel::High, 0.75f, 64};
        desc.settings.clouds = {true, QualityLevel::High, 64, 1.0f, true};
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
        desc.settings.clouds = {true, QualityLevel::Low, 16, 0.5f, true};
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
        desc.settings.clouds = {false, QualityLevel::Disabled, 0, 0.25f, false};
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

    mark(previous.clouds.enabled != next.clouds.enabled
            || previous.clouds.maxSteps != next.clouds.maxSteps
            || previous.clouds.resolutionScale != next.clouds.resolutionScale,
        ScalabilityUpdateFlags::RecreateResources | ScalabilityUpdateFlags::RebuildRenderGraph);

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
    m_HasCloudOverride = false;
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

#if WE_HAS_NLOHMANN_JSON
    const auto dir = ResolveProfileDirectory();
    if (dir.empty()) {
        WE_LOG_INFO(we::LogCategory::Renderer.data(),
            "Scalability: no profile directory — using built-in High-End defaults");
        return true;
    }

    // Authoritative path: JSON overlays builtins. Only HighEnd is expected on disk
    // during the AAA-first phase; Balanced/Low stay as dormant builtins.
    for (const auto id : ids) {
        const auto path = dir / (std::string(ProfileFileStem(id)) + ".json");
        if (!std::filesystem::exists(path)) {
            continue;
        }
        RenderingProfileDesc desc = MakeBuiltinProfile(id);
        if (LoadProfileJson(path, desc)) {
            m_Profiles[id] = std::move(desc);
            WE_LOG_INFO(we::LogCategory::Renderer.data(),
                std::string("Scalability loaded profile: ") + path.string());
        }
    }
#else
    WE_LOG_WARN(we::LogCategory::Renderer.data(),
        "Scalability: nlohmann_json unavailable — built-in presets only");
#endif
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
    if (m_HasCloudOverride) {
        resolved.clouds = m_CloudOverride;
        if (!resolved.clouds.enabled) {
            resolved.clouds.quality = QualityLevel::Disabled;
        }
    }
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
    if (id == RenderingProfileId::Custom && !m_HasCloudOverride) {
        // Custom without overrides behaves as High-End until override API is used.
        id = RenderingProfileId::HighEnd;
    }

    const ResolvedRenderingSettings previous = m_Pending;
    m_ActiveProfileId = id;
    if (id != RenderingProfileId::Custom) {
        m_HasCloudOverride = false;
    }
    ResolveActiveProfile();
    const ScalabilityUpdateFlags flags = DiffSettings(previous, m_Pending);

    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        std::string("Scalability profile set to: ") + m_Pending.profileName);
    for (const auto& note : m_Pending.fallbackNotes) {
        WE_LOG_INFO(we::LogCategory::Renderer.data(), std::string("  fallback: ") + note);
    }
    return flags;
}

ScalabilityUpdateFlags ScalabilityManager::SetCloudQualityOverride(
    const CloudQualitySettings& clouds) {
    const ResolvedRenderingSettings previous = m_Pending;
    m_HasCloudOverride = true;
    m_CloudOverride = clouds;
    m_ActiveProfileId = RenderingProfileId::Custom;
    ResolveActiveProfile();
    m_Pending.profileId = RenderingProfileId::Custom;
    m_Pending.profileName = "Custom";
    return DiffSettings(previous, m_Pending);
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
