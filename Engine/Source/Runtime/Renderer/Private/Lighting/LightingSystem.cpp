// ==============================================================================
// WindEffects — Renderer — LightingSystem
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Lighting/LightingSystem.h"
#include "Lighting/LightingPasses.h"
#include "Lighting/EnvironmentLightingEvaluator.h"
#include "ECS/RenderExtract.h"
#include "RHI/Desc.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <span>

namespace we::runtime::renderer {
namespace {

we::math::Vec3 NormalizeOrFallback(we::math::Vec3 v, we::math::Vec3 fallback) {
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lenSq < 1.0e-12f) {
        return fallback;
    }
    const float inv = 1.0f / std::sqrt(lenSq);
    return {v.x * inv, v.y * inv, v.z * inv};
}

float CosDegrees(float degrees) {
    return std::cos(degrees * 0.017453292519943295f);
}

} // namespace

LightingSystem::LightingSystem() = default;

LightingSystem::~LightingSystem() {
    Shutdown();
}

bool LightingSystem::Initialize(const LightingCreateInfo& info) {
    Shutdown();
    m_Device = info.device;
    m_Indirect = &m_NullIndirect;
    if (m_Device) {
        (void)m_Shadows.Initialize(m_Device);
    }
    m_Initialized = true;
    return true;
}

void LightingSystem::Configure(
    const LightingQualitySettings& lighting,
    const ShadowQualitySettings& shadows) {
    Configure(lighting, shadows, 2048);
}

void LightingSystem::Configure(
    const LightingQualitySettings& lighting,
    const ShadowQualitySettings& shadows,
    uint32_t maxShadowMapResolution) {
    m_LightingSettings = lighting;
    m_Shadows.Configure(shadows, maxShadowMapResolution);
}

void LightingSystem::BeginFrame(const LightingFrameContext& context) {
    if (!m_Initialized) {
        return;
    }

    BuildScene(context);

    if (context.environment) {
        SyncEnvironmentFromPrimary(*context.environment);
        // Refresh sky irradiance from the shared Sun+Atmosphere model after sun sync.
        const auto envCtx =
            EnvironmentLightingEvaluator::FromEnvironmentUniform(*context.environment);
        EnvironmentLightingEvaluator::ApplyToEnvironmentUniform(*context.environment, envCtx);
        m_Scene.environment.ambientUpper = context.environment->skyAmbientColor;
        m_Scene.environment.ambientLower = context.environment->skyLightLowerColor;
        m_Scene.environment.skyLightIntensity = context.environment->skyLightIntensity;
    }

    CameraUniform camera{};
    if (context.camera) {
        camera = *context.camera;
    }
    m_Shadows.BeginFrame(camera, m_Scene.PrimaryDirectional());
}

void LightingSystem::BuildRenderGraph(RenderGraph& graph) {
    if (!m_Initialized || !m_LightingSettings.enabled) {
        return;
    }
    graph.AddPass(std::make_unique<LightingPreparePass>(this));
    m_Shadows.BuildRenderGraph(graph);
}

void LightingSystem::EndFrame() {}

void LightingSystem::Shutdown() {
    DestroyBuffers();
    m_Shadows.Shutdown();
    m_Scene.Clear();
    m_GpuDirectional.clear();
    m_GpuPoints.clear();
    m_GpuSpots.clear();
    m_GpuConstants = {};
    m_Device = nullptr;
    m_Initialized = false;
}

void LightingSystem::BuildScene(const LightingFrameContext& context) {
    m_Scene.Clear();
    m_GpuDirectional.clear();
    m_GpuPoints.clear();
    m_GpuSpots.clear();
    m_GpuConstants = {};

    if (!m_LightingSettings.enabled) {
        return;
    }

    if (context.environment) {
        m_Scene.environment.ambientUpper = context.environment->skyAmbientColor;
        m_Scene.environment.ambientLower = context.environment->skyLightLowerColor;
        m_Scene.environment.skyLightIntensity = context.environment->skyLightIntensity;
    }

    const auto* extract = context.extract;
    if (extract && !extract->directionalLights.empty()) {
        const uint32_t maxDir = std::min(
            kMaxDirectionalLights,
            static_cast<uint32_t>(extract->directionalLights.size()));
        for (uint32_t i = 0; i < maxDir; ++i) {
            const auto& src = extract->directionalLights[i];
            DirectionalLight light{};
            light.direction = NormalizeOrFallback(
                {src.direction[0], src.direction[1], src.direction[2]},
                {0.3f, -0.8f, 0.2f});
            light.color = {src.color[0], src.color[1], src.color[2]};
            light.intensity = src.intensity;
            light.castsShadows = src.castShadows;
            light.enabled = true;
            m_Scene.directionalLights.push_back(light);
        }
    } else if (context.environment) {
        DirectionalLight sun{};
        sun.direction = NormalizeOrFallback(
            context.environment->sunDirection, {0.3f, -0.8f, 0.2f});
        sun.color = context.environment->sunColor;
        sun.intensity = context.environment->sunIntensity;
        sun.castsShadows = context.environment->sunCastShadows != 0;
        sun.enabled = true;
        m_Scene.directionalLights.push_back(sun);
    }

    const uint32_t maxLocal = std::max(1u, m_LightingSettings.maxLocalLights);
    if (extract) {
        uint32_t localBudget = maxLocal;
        for (const auto& src : extract->pointLights) {
            if (localBudget == 0) {
                break;
            }
            PointLight light{};
            light.position = {src.position[0], src.position[1], src.position[2]};
            light.color = {src.color[0], src.color[1], src.color[2]};
            light.intensity = src.intensity;
            light.range = src.range;
            light.enabled = true;
            m_Scene.pointLights.push_back(light);
            --localBudget;
        }
        for (const auto& src : extract->spotLights) {
            if (localBudget == 0 || m_Scene.spotLights.size() >= kMaxSpotLights) {
                break;
            }
            SpotLight light{};
            light.position = {src.position[0], src.position[1], src.position[2]};
            light.direction = NormalizeOrFallback(
                {src.direction[0], src.direction[1], src.direction[2]},
                {0.0f, -1.0f, 0.0f});
            light.color = {src.color[0], src.color[1], src.color[2]};
            light.intensity = src.intensity;
            light.range = src.range;
            light.innerConeDegrees = src.innerConeDegrees;
            light.outerConeDegrees = src.outerConeDegrees;
            light.enabled = true;
            m_Scene.spotLights.push_back(light);
            --localBudget;
        }
    }

    m_GpuDirectional.reserve(m_Scene.directionalLights.size());
    for (const auto& light : m_Scene.directionalLights) {
        if (!light.enabled) {
            continue;
        }
        GPUDirectionalLight gpu{};
        gpu.direction[0] = light.direction.x;
        gpu.direction[1] = light.direction.y;
        gpu.direction[2] = light.direction.z;
        gpu.direction[3] = light.castsShadows ? 1.0f : 0.0f;
        gpu.colorIntensity[0] = light.color.x;
        gpu.colorIntensity[1] = light.color.y;
        gpu.colorIntensity[2] = light.color.z;
        gpu.colorIntensity[3] = light.intensity;
        m_GpuDirectional.push_back(gpu);
    }

    m_GpuPoints.reserve(m_Scene.pointLights.size());
    for (const auto& light : m_Scene.pointLights) {
        if (!light.enabled) {
            continue;
        }
        GPUPointLight gpu{};
        gpu.positionRange[0] = light.position.x;
        gpu.positionRange[1] = light.position.y;
        gpu.positionRange[2] = light.position.z;
        gpu.positionRange[3] = light.range;
        gpu.colorIntensity[0] = light.color.x;
        gpu.colorIntensity[1] = light.color.y;
        gpu.colorIntensity[2] = light.color.z;
        gpu.colorIntensity[3] = light.intensity;
        m_GpuPoints.push_back(gpu);
    }

    m_GpuSpots.reserve(m_Scene.spotLights.size());
    for (const auto& light : m_Scene.spotLights) {
        if (!light.enabled) {
            continue;
        }
        GPUSpotLight gpu{};
        gpu.positionRange[0] = light.position.x;
        gpu.positionRange[1] = light.position.y;
        gpu.positionRange[2] = light.position.z;
        gpu.positionRange[3] = light.range;
        gpu.directionCone[0] = light.direction.x;
        gpu.directionCone[1] = light.direction.y;
        gpu.directionCone[2] = light.direction.z;
        gpu.directionCone[3] = CosDegrees(light.outerConeDegrees);
        gpu.colorIntensity[0] = light.color.x;
        gpu.colorIntensity[1] = light.color.y;
        gpu.colorIntensity[2] = light.color.z;
        gpu.colorIntensity[3] = light.intensity;
        gpu.innerConePad[0] = CosDegrees(light.innerConeDegrees);
        m_GpuSpots.push_back(gpu);
    }

    m_GpuConstants.directionalCount = static_cast<uint32_t>(m_GpuDirectional.size());
    m_GpuConstants.pointCount = static_cast<uint32_t>(m_GpuPoints.size());
    m_GpuConstants.spotCount = static_cast<uint32_t>(m_GpuSpots.size());
    m_GpuConstants.maxLocalLights = maxLocal;
    m_GpuConstants.ambientUpper[0] = m_Scene.environment.ambientUpper.x;
    m_GpuConstants.ambientUpper[1] = m_Scene.environment.ambientUpper.y;
    m_GpuConstants.ambientUpper[2] = m_Scene.environment.ambientUpper.z;
    m_GpuConstants.ambientUpper[3] = 0.0f;
    m_GpuConstants.ambientLower[0] = m_Scene.environment.ambientLower.x;
    m_GpuConstants.ambientLower[1] = m_Scene.environment.ambientLower.y;
    m_GpuConstants.ambientLower[2] = m_Scene.environment.ambientLower.z;
    m_GpuConstants.ambientLower[3] = 0.0f;
    m_GpuConstants.skyLightIntensity = m_Scene.environment.skyLightIntensity;
}

void LightingSystem::SyncEnvironmentFromPrimary(SceneEnvironmentUniform& environment) const {
    const DirectionalLight* primary = m_Scene.PrimaryDirectional();
    if (!primary) {
        return;
    }
    environment.sunDirection = primary->direction;
    environment.sunColor = primary->color;
    environment.sunIntensity = primary->intensity;
    environment.sunCastShadows = primary->castsShadows ? 1 : 0;
}

bool LightingSystem::EnsureBuffers() {
    if (!m_Device || !m_Device->IsValid()) {
        return false;
    }

    auto ensure = [this](we::rhi::RHIBufferHandle& handle, uint64_t& capacity, uint64_t needed,
                         const char* name) -> bool {
        if (needed == 0) {
            needed = 16; // keep a valid small buffer for binding
        }
        if (handle != we::rhi::RHIBufferHandle::Invalid && capacity >= needed) {
            return true;
        }
        if (handle != we::rhi::RHIBufferHandle::Invalid) {
            (void)m_Device->DestroyBuffer(handle);
            handle = we::rhi::RHIBufferHandle::Invalid;
            capacity = 0;
        }
        we::rhi::BufferDesc desc{};
        desc.size = needed;
        desc.usage = we::rhi::BufferUsage::Storage | we::rhi::BufferUsage::TransferDst;
        desc.memory = we::rhi::MemoryUsage::HostVisible;
        desc.debugName = name;
        auto created = m_Device->CreateBuffer(desc);
        if (!created) {
            return false;
        }
        handle = *created;
        capacity = needed;
        return true;
    };

    const uint64_t dirBytes = std::max<uint64_t>(
        sizeof(GPUDirectionalLight),
        m_GpuDirectional.size() * sizeof(GPUDirectionalLight));
    const uint64_t pointBytes = std::max<uint64_t>(
        sizeof(GPUPointLight),
        m_GpuPoints.size() * sizeof(GPUPointLight));
    const uint64_t spotBytes = std::max<uint64_t>(
        sizeof(GPUSpotLight),
        m_GpuSpots.size() * sizeof(GPUSpotLight));

    if (!ensure(m_DirectionalBuffer, m_DirectionalCapacity, dirBytes, "Lighting.Directional")) {
        return false;
    }
    if (!ensure(m_PointBuffer, m_PointCapacity, pointBytes, "Lighting.Points")) {
        return false;
    }
    if (!ensure(m_SpotBuffer, m_SpotCapacity, spotBytes, "Lighting.Spots")) {
        return false;
    }

    if (m_ConstantsBuffer == we::rhi::RHIBufferHandle::Invalid) {
        we::rhi::BufferDesc desc{};
        desc.size = sizeof(GPULightingConstants);
        desc.usage = we::rhi::BufferUsage::Uniform | we::rhi::BufferUsage::TransferDst;
        desc.memory = we::rhi::MemoryUsage::HostVisible;
        desc.debugName = "Lighting.Constants";
        auto created = m_Device->CreateBuffer(desc);
        if (!created) {
            return false;
        }
        m_ConstantsBuffer = *created;
    }
    return true;
}

void LightingSystem::DestroyBuffers() {
    if (!m_Device) {
        m_DirectionalBuffer = we::rhi::RHIBufferHandle::Invalid;
        m_PointBuffer = we::rhi::RHIBufferHandle::Invalid;
        m_SpotBuffer = we::rhi::RHIBufferHandle::Invalid;
        m_ConstantsBuffer = we::rhi::RHIBufferHandle::Invalid;
        m_DirectionalCapacity = 0;
        m_PointCapacity = 0;
        m_SpotCapacity = 0;
        return;
    }
    if (m_DirectionalBuffer != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->DestroyBuffer(m_DirectionalBuffer);
        m_DirectionalBuffer = we::rhi::RHIBufferHandle::Invalid;
    }
    if (m_PointBuffer != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->DestroyBuffer(m_PointBuffer);
        m_PointBuffer = we::rhi::RHIBufferHandle::Invalid;
    }
    if (m_SpotBuffer != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->DestroyBuffer(m_SpotBuffer);
        m_SpotBuffer = we::rhi::RHIBufferHandle::Invalid;
    }
    if (m_ConstantsBuffer != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->DestroyBuffer(m_ConstantsBuffer);
        m_ConstantsBuffer = we::rhi::RHIBufferHandle::Invalid;
    }
    m_DirectionalCapacity = 0;
    m_PointCapacity = 0;
    m_SpotCapacity = 0;
}

void LightingSystem::UploadGpuData() {
    if (!m_Initialized || !m_LightingSettings.enabled) {
        return;
    }
    if (!EnsureBuffers()) {
        return;
    }

    auto upload = [this](we::rhi::RHIBufferHandle handle, const void* data, size_t bytes) {
        if (handle == we::rhi::RHIBufferHandle::Invalid || bytes == 0 || !data) {
            return;
        }
        const auto span = std::span<const uint8_t>(
            static_cast<const uint8_t*>(data), bytes);
        (void)m_Device->UpdateBuffer(handle, span);
    };

    if (!m_GpuDirectional.empty()) {
        upload(m_DirectionalBuffer, m_GpuDirectional.data(),
            m_GpuDirectional.size() * sizeof(GPUDirectionalLight));
    }
    if (!m_GpuPoints.empty()) {
        upload(m_PointBuffer, m_GpuPoints.data(),
            m_GpuPoints.size() * sizeof(GPUPointLight));
    }
    if (!m_GpuSpots.empty()) {
        upload(m_SpotBuffer, m_GpuSpots.data(),
            m_GpuSpots.size() * sizeof(GPUSpotLight));
    }
    upload(m_ConstantsBuffer, &m_GpuConstants, sizeof(m_GpuConstants));
    m_Shadows.UploadGpuData();
}

} // namespace we::runtime::renderer
