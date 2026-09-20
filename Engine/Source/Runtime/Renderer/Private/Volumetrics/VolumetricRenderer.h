// ==============================================================================
// WindEffects — Renderer — VolumetricRenderer
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Camera/CameraUniform.h"
#include "Graph/DdsTextureLoader.h"
#include "Lighting/CloudUniform.h"
#include "Lighting/SceneEnvironmentUniform.h"
#include "Renderer/Scalability/RenderingSettings.h"
#include "Renderer/Volumetrics/IVolumetricProvider.h"
#include "Renderer/Volumetrics/LocalFogUniform.h"
#include "Renderer/Volumetrics/VolumetricFrameUniform.h"
#include "Renderer/Volumetrics/VolumetricTypes.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace we::runtime::renderer {

class LightingSystem;
class CloudVolumeProvider;
class HeightFogVolumeProvider;
class LocalFogVolumeProvider;
class AtmosphereVolumeProvider;

/// Unified volumetric pipeline (clouds + height/local fog foundation).
/// Descriptor sets: Camera=0, Env=1, VolumetricFrame=2, Cloud=3, Resources=4.
class VolumetricRenderer {
public:
    VolumetricRenderer();
    ~VolumetricRenderer();

    VolumetricRenderer(const VolumetricRenderer&) = delete;
    VolumetricRenderer& operator=(const VolumetricRenderer&) = delete;

    bool Init(we::rhi::IRHIDevice* device);
    void Shutdown();

    void Draw(
        we::rhi::IRHICommandList& cmd,
        we::rhi::RHITextureHandle color,
        we::rhi::RHITextureHandle depth,
        we::rhi::Extent2D extent,
        const CameraUniform& camera,
        const SceneEnvironmentUniform& environment,
        const CloudUniform& clouds,
        const VolumetricQualitySettings& quality,
        uint32_t frameIndex,
        float resolutionScale = 1.0f);

    [[nodiscard]] bool IsReady() const { return m_Ready; }

    /// Must be called before the viewport depth texture is destroyed/recreated.
    void InvalidateDepthBinding();

    void SetLightingSystem(LightingSystem* lighting) { m_Lighting = lighting; }
    [[nodiscard]] LightingSystem* GetLightingSystem() const { return m_Lighting; }

    [[nodiscard]] IVolumetricProvider* GetProvider(VolumetricProviderType type);
    [[nodiscard]] const IVolumetricProvider* GetProvider(VolumetricProviderType type) const;

    void SetLocalFog(const LocalFogUniform& fog);
    [[nodiscard]] LocalFogUniform& GetLocalFog();
    [[nodiscard]] const LocalFogUniform& GetLocalFog() const;

    void SetHeightFogEnabled(bool enabled);

private:
    void RegisterDefaultProviders();
    VolumetricFrameUniform BuildFrameUniform(
        const VolumetricQualitySettings& quality,
        uint32_t frameIndex,
        float resolutionScale) const;
    bool LoadShaders();
    bool LoadNoiseTextures();
    bool EnsurePipeline(we::rhi::Format colorFormat);
    bool EnsureUpsamplePipeline(we::rhi::Format colorFormat);
    bool EnsureOffscreenTarget(we::rhi::Extent2D fullExtent, float resolutionScale);
    bool EnsureHistoryTargets(we::rhi::Extent2D extent);
    void DestroyHistoryTargets();
    bool EnsureDepthView(we::rhi::RHITextureHandle depth);
    [[nodiscard]] bool BindResourceDescriptors(we::rhi::RHITextureHandle depth);
    bool CreateDummyDepth();
    void DestroyOffscreenTarget();

    we::rhi::IRHIDevice* m_Device = nullptr;
    LightingSystem* m_Lighting = nullptr;

    std::unique_ptr<CloudVolumeProvider> m_CloudProvider;
    std::unique_ptr<HeightFogVolumeProvider> m_HeightFogProvider;
    std::unique_ptr<LocalFogVolumeProvider> m_LocalFogProvider;
    std::unique_ptr<AtmosphereVolumeProvider> m_AtmosphereProvider;
    std::vector<IVolumetricProvider*> m_Providers;

    we::rhi::RHIShaderHandle m_Vs = we::rhi::RHIShaderHandle::Invalid;
    we::rhi::RHIShaderHandle m_Ps = we::rhi::RHIShaderHandle::Invalid;
    we::rhi::RHIShaderHandle m_UpsampleVs = we::rhi::RHIShaderHandle::Invalid;
    we::rhi::RHIShaderHandle m_UpsamplePs = we::rhi::RHIShaderHandle::Invalid;
    std::vector<uint8_t> m_VsBytes;
    std::vector<uint8_t> m_PsBytes;
    std::vector<uint8_t> m_UpsampleVsBytes;
    std::vector<uint8_t> m_UpsamplePsBytes;

    we::rhi::RHIDescriptorSetLayoutHandle m_CameraLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIDescriptorSetLayoutHandle m_EnvLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIDescriptorSetLayoutHandle m_FrameLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIDescriptorSetLayoutHandle m_CloudLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIDescriptorSetLayoutHandle m_ResourceLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIDescriptorSetLayoutHandle m_UpsampleLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIPipelineLayoutHandle m_PipelineLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
    we::rhi::RHIPipelineLayoutHandle m_UpsamplePipelineLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
    we::rhi::RHIGraphicsPipelineHandle m_Pipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
    we::rhi::RHIGraphicsPipelineHandle m_UpsamplePipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;

    we::rhi::RHIDescriptorPoolHandle m_Pool = we::rhi::RHIDescriptorPoolHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_CameraSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_EnvSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_FrameSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_CloudSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_ResourceSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_UpsampleSet = we::rhi::RHIDescriptorSetHandle::Invalid;

    we::rhi::RHIBufferHandle m_CameraBuffer = we::rhi::RHIBufferHandle::Invalid;
    we::rhi::RHIBufferHandle m_EnvBuffer = we::rhi::RHIBufferHandle::Invalid;
    we::rhi::RHIBufferHandle m_FrameBuffer = we::rhi::RHIBufferHandle::Invalid;
    we::rhi::RHIBufferHandle m_CloudBuffer = we::rhi::RHIBufferHandle::Invalid;

    LoadedDdsTexture m_BaseShape{};
    LoadedDdsTexture m_DetailShape{};
    LoadedDdsTexture m_WeatherMap{};
    LoadedDdsTexture m_CurlNoise{};
    we::rhi::RHISamplerHandle m_Sampler = we::rhi::RHISamplerHandle::Invalid;
    we::rhi::RHISamplerHandle m_UpsampleSampler = we::rhi::RHISamplerHandle::Invalid;

    we::rhi::RHITextureHandle m_BoundDepth = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureViewHandle m_DepthView = we::rhi::RHITextureViewHandle::Invalid;
    we::rhi::RHITextureHandle m_DummyDepthTexture = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureViewHandle m_DummyDepthView = we::rhi::RHITextureViewHandle::Invalid;

    we::rhi::RHITextureHandle m_OffscreenColor = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureViewHandle m_OffscreenView = we::rhi::RHITextureViewHandle::Invalid;
    we::rhi::Extent2D m_OffscreenExtent{};
    we::rhi::Extent2D m_OffscreenFullExtent{};
    float m_OffscreenScale = 1.0f;
    bool m_OffscreenInSrv = false;

    // Temporal history ping-pong (RGBA16F); bound when shaders are ready.
    we::rhi::RHITextureHandle m_History[2] = {
        we::rhi::RHITextureHandle::Invalid,
        we::rhi::RHITextureHandle::Invalid};
    we::rhi::RHITextureViewHandle m_HistoryView[2] = {
        we::rhi::RHITextureViewHandle::Invalid,
        we::rhi::RHITextureViewHandle::Invalid};
    we::rhi::Extent2D m_HistoryExtent{};
    uint32_t m_HistoryWriteIndex = 0;

    we::rhi::Format m_PipelineColorFormat = we::rhi::Format::Unknown;
    we::rhi::Format m_UpsampleColorFormat = we::rhi::Format::Unknown;
    bool m_Ready = false;
    bool m_DepthBound = false;
};

} // namespace we::runtime::renderer
