// ==============================================================================
// WindEffects — Renderer — ViewportCloudRenderer
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
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <vector>

namespace we::runtime::renderer {

// Fullscreen Nubis volumetric clouds into the viewport HDR color target.
class ViewportCloudRenderer {
public:
    ViewportCloudRenderer() = default;
    ~ViewportCloudRenderer();

    ViewportCloudRenderer(const ViewportCloudRenderer&) = delete;
    ViewportCloudRenderer& operator=(const ViewportCloudRenderer&) = delete;

    bool Init(we::rhi::IRHIDevice* device);
    void Shutdown();

    void Draw(
        we::rhi::IRHICommandList& cmd,
        we::rhi::RHITextureHandle color,
        we::rhi::RHITextureHandle depth,
        we::rhi::Extent2D extent,
        const CameraUniform& camera,
        const SceneEnvironmentUniform& environment,
        const CloudUniform& clouds);

    [[nodiscard]] bool IsReady() const { return m_Ready; }

    /// Must be called before the viewport depth texture is destroyed/recreated.
    void InvalidateDepthBinding();

private:
    bool LoadShaders();
    bool LoadNoiseTextures();
    bool EnsurePipeline(we::rhi::Format colorFormat);
    bool EnsureDepthView(we::rhi::RHITextureHandle depth);
    [[nodiscard]] bool BindResourceDescriptors(we::rhi::RHITextureHandle depth);
    bool CreateDummyDepth();

    we::rhi::IRHIDevice* m_Device = nullptr;
    we::rhi::RHIShaderHandle m_Vs = we::rhi::RHIShaderHandle::Invalid;
    we::rhi::RHIShaderHandle m_Ps = we::rhi::RHIShaderHandle::Invalid;
    std::vector<uint8_t> m_VsBytes;
    std::vector<uint8_t> m_PsBytes;

    we::rhi::RHIDescriptorSetLayoutHandle m_CameraLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIDescriptorSetLayoutHandle m_EnvLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIDescriptorSetLayoutHandle m_CloudLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIDescriptorSetLayoutHandle m_ResourceLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIPipelineLayoutHandle m_PipelineLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
    we::rhi::RHIGraphicsPipelineHandle m_Pipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;

    we::rhi::RHIDescriptorPoolHandle m_Pool = we::rhi::RHIDescriptorPoolHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_CameraSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_EnvSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_CloudSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_ResourceSet = we::rhi::RHIDescriptorSetHandle::Invalid;

    we::rhi::RHIBufferHandle m_CameraBuffer = we::rhi::RHIBufferHandle::Invalid;
    we::rhi::RHIBufferHandle m_EnvBuffer = we::rhi::RHIBufferHandle::Invalid;
    we::rhi::RHIBufferHandle m_CloudBuffer = we::rhi::RHIBufferHandle::Invalid;

    LoadedDdsTexture m_BaseShape{};
    LoadedDdsTexture m_DetailShape{};
    LoadedDdsTexture m_CurlNoise{};
    we::rhi::RHISamplerHandle m_Sampler = we::rhi::RHISamplerHandle::Invalid;

    we::rhi::RHITextureHandle m_BoundDepth = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureViewHandle m_DepthView = we::rhi::RHITextureViewHandle::Invalid;
    we::rhi::RHITextureHandle m_DummyDepthTexture = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureViewHandle m_DummyDepthView = we::rhi::RHITextureViewHandle::Invalid;

    we::rhi::Format m_PipelineColorFormat = we::rhi::Format::Unknown;
    bool m_Ready = false;
    bool m_DepthBound = false;
};

} // namespace we::runtime::renderer
