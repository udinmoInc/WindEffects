// ==============================================================================
// WindEffects — Renderer — LightingSystem
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#pragma warning(push)
#pragma warning(disable : 4251)

#include "Camera/CameraUniform.h"
#include "Lighting/LightingScene.h"
#include "Lighting/LightingTypes.h"
#include "Lighting/IIndirectLightingProvider.h"
#include "Lighting/SceneEnvironmentUniform.h"
#include "Lighting/ShadowSystem.h"
#include "Renderer/Export.h"
#include "Renderer/Scalability/RenderingSettings.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <vector>

namespace we::runtime::ecs {
struct ExtractedFrameData;
}

namespace we::runtime::renderer {

class RenderGraph;

struct LightingCreateInfo {
    we::rhi::IRHIDevice* device = nullptr;
};

struct LightingFrameContext {
    const we::runtime::ecs::ExtractedFrameData* extract = nullptr;
    const CameraUniform* camera = nullptr;
    /// Optional: primary directional light is written back into environment for sky/PBR UBO.
    SceneEnvironmentUniform* environment = nullptr;
    uint32_t viewportWidth = 0;
    uint32_t viewportHeight = 0;
};

/// Modular lighting foundation: directional + local lights + environment + shadow prep.
/// Owns GPU light buffers via IRHI. Does not own RenderGraph scheduling.
class RENDERER_API LightingSystem {
public:
    LightingSystem();
    ~LightingSystem();

    LightingSystem(const LightingSystem&) = delete;
    LightingSystem& operator=(const LightingSystem&) = delete;

    bool Initialize(const LightingCreateInfo& info);
    void Configure(const LightingQualitySettings& lighting, const ShadowQualitySettings& shadows);
    void Configure(
        const LightingQualitySettings& lighting,
        const ShadowQualitySettings& shadows,
        uint32_t maxShadowMapResolution);

    void BeginFrame(const LightingFrameContext& context);
    void BuildRenderGraph(RenderGraph& graph);
    void EndFrame();
    void Shutdown();

    /// Uploads GPU light buffers (also invoked from LightingPreparePass).
    void UploadGpuData();

    [[nodiscard]] bool IsReady() const { return m_Initialized; }
    [[nodiscard]] bool Enabled() const { return m_LightingSettings.enabled; }
    [[nodiscard]] const LightingScene& GetScene() const { return m_Scene; }
    [[nodiscard]] ShadowSystem& GetShadows() { return m_Shadows; }
    [[nodiscard]] const ShadowSystem& GetShadows() const { return m_Shadows; }

    [[nodiscard]] we::rhi::RHIBufferHandle GetDirectionalLightBuffer() const {
        return m_DirectionalBuffer;
    }
    [[nodiscard]] we::rhi::RHIBufferHandle GetPointLightBuffer() const { return m_PointBuffer; }
    [[nodiscard]] we::rhi::RHIBufferHandle GetLightingConstantsBuffer() const {
        return m_ConstantsBuffer;
    }
    [[nodiscard]] uint32_t DirectionalLightCount() const {
        return static_cast<uint32_t>(m_GpuDirectional.size());
    }
    [[nodiscard]] uint32_t PointLightCount() const {
        return static_cast<uint32_t>(m_GpuPoints.size());
    }

    void SetIndirectLightingProvider(IIndirectLightingProvider* provider) {
        m_Indirect = provider;
    }
    [[nodiscard]] IIndirectLightingProvider* GetIndirectLightingProvider() const {
        return m_Indirect;
    }

private:
    void BuildScene(const LightingFrameContext& context);
    void SyncEnvironmentFromPrimary(SceneEnvironmentUniform& environment) const;
    bool EnsureBuffers();
    void DestroyBuffers();

    we::rhi::IRHIDevice* m_Device = nullptr;
    bool m_Initialized = false;
    LightingQualitySettings m_LightingSettings{};
    LightingScene m_Scene{};
    ShadowSystem m_Shadows{};
    IIndirectLightingProvider* m_Indirect = nullptr;
    NullIndirectLightingProvider m_NullIndirect{};

    std::vector<GPUDirectionalLight> m_GpuDirectional;
    std::vector<GPUPointLight> m_GpuPoints;
    std::vector<GPUSpotLight> m_GpuSpots;
    GPULightingConstants m_GpuConstants{};

    we::rhi::RHIBufferHandle m_DirectionalBuffer = we::rhi::RHIBufferHandle::Invalid;
    we::rhi::RHIBufferHandle m_PointBuffer = we::rhi::RHIBufferHandle::Invalid;
    we::rhi::RHIBufferHandle m_SpotBuffer = we::rhi::RHIBufferHandle::Invalid;
    we::rhi::RHIBufferHandle m_ConstantsBuffer = we::rhi::RHIBufferHandle::Invalid;
    uint64_t m_DirectionalCapacity = 0;
    uint64_t m_PointCapacity = 0;
    uint64_t m_SpotCapacity = 0;
};

} // namespace we::runtime::renderer

#pragma warning(pop)
