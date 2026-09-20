// ==============================================================================
// WindEffects — Renderer — VolumetricRenderer
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Volumetrics/VolumetricRenderer.h"

#include "Volumetrics/CloudVolumeProvider.h"
#include "Volumetrics/HeightFogVolumeProvider.h"
#include "Volumetrics/LocalFogVolumeProvider.h"
#include "Volumetrics/AtmosphereVolumeProvider.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Core/Paths.h"
#include "RHI/Desc.h"
#include "RHI/ShaderBytecode.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <span>
#include <string>

namespace we::runtime::renderer {
namespace {

std::filesystem::path ResolveCloudAsset(const char* fileName) {
    auto& paths = we::core::PathService::Get();
    const std::array candidates = {
        // Staged Shipping layout: Engine/Content/Cloud
        paths.EngineContentRoot() / "Cloud" / fileName,
        paths.EngineRoot() / "Engine" / "Content" / "Cloud" / fileName,
        // Source-tree bake location (dev checkout)
        paths.EngineRoot() / "Engine" / "EngineContent" / "Cloud" / fileName,
        paths.EngineRoot() / "Projects" / "AAAHighEnd" / "Content" / "Textures" / fileName,
    };
    for (const auto& c : candidates) {
        if (std::filesystem::exists(c)) {
            return c;
        }
    }
    return candidates.front();
}

} // namespace


VolumetricRenderer::VolumetricRenderer() {
    RegisterDefaultProviders();
}

void VolumetricRenderer::RegisterDefaultProviders() {
    m_CloudProvider = std::make_unique<CloudVolumeProvider>();
    m_HeightFogProvider = std::make_unique<HeightFogVolumeProvider>();
    m_LocalFogProvider = std::make_unique<LocalFogVolumeProvider>();
    m_AtmosphereProvider = std::make_unique<AtmosphereVolumeProvider>();
    m_Providers = {
        m_AtmosphereProvider.get(),
        m_CloudProvider.get(),
        m_HeightFogProvider.get(),
        m_LocalFogProvider.get(),
    };
}

IVolumetricProvider* VolumetricRenderer::GetProvider(VolumetricProviderType type) {
    for (IVolumetricProvider* p : m_Providers) {
        if (p && p->GetType() == type) {
            return p;
        }
    }
    return nullptr;
}

const IVolumetricProvider* VolumetricRenderer::GetProvider(VolumetricProviderType type) const {
    for (const IVolumetricProvider* p : m_Providers) {
        if (p && p->GetType() == type) {
            return p;
        }
    }
    return nullptr;
}

void VolumetricRenderer::SetLocalFog(const LocalFogUniform& fog) {
    if (!m_LocalFogProvider) {
        RegisterDefaultProviders();
    }
    m_LocalFogProvider->SetLocalFog(fog);
}

LocalFogUniform& VolumetricRenderer::GetLocalFog() {
    if (!m_LocalFogProvider) {
        RegisterDefaultProviders();
    }
    return m_LocalFogProvider->GetLocalFog();
}

const LocalFogUniform& VolumetricRenderer::GetLocalFog() const {
    // Const path: providers are created in the constructor.
    return m_LocalFogProvider->GetLocalFog();
}

void VolumetricRenderer::SetHeightFogEnabled(bool enabled) {
    if (m_HeightFogProvider) {
        m_HeightFogProvider->SetEnabled(enabled);
    }
}

VolumetricFrameUniform VolumetricRenderer::BuildFrameUniform(
    const VolumetricQualitySettings& quality,
    uint32_t frameIndex,
    float resolutionScale) const
{
    VolumetricFrameUniform frame{};
    frame.maxSteps = quality.maxSteps;
    frame.lightSteps = quality.lightSteps;
    frame.shadowSteps = quality.shadowSteps;
    frame.frameIndex = frameIndex;
    frame.resolutionScale = resolutionScale;
    frame.temporalBlend = quality.temporalBlend;
    // Golden-ratio temporal jitter in [0,1)
    const float phi = 0.61803398875f;
    frame.jitterX = std::fmod(static_cast<float>(frameIndex) * phi, 1.0f);
    frame.jitterY = std::fmod(static_cast<float>(frameIndex) * phi * phi, 1.0f);
    frame.temporalQuality = quality.temporalQuality;
    frame.shadowQuality = static_cast<uint32_t>(quality.quality);
    frame.lightingQuality = static_cast<uint32_t>(quality.quality);

    VolumetricProviderMask mask{};
    for (const IVolumetricProvider* p : m_Providers) {
        if (p && p->IsEnabled()) {
            mask.Set(p->GetType());
        }
    }
    frame.providerMask = mask.bits;

    if (m_HeightFogProvider) {
        m_HeightFogProvider->FillFrameUniform(frame);
    }
    if (m_LocalFogProvider) {
        m_LocalFogProvider->FillFrameUniform(frame);
    }
    if (m_AtmosphereProvider) {
        m_AtmosphereProvider->FillFrameUniform(frame);
    }
    return frame;
}

VolumetricRenderer::~VolumetricRenderer() {
    Shutdown();
}

bool VolumetricRenderer::Init(we::rhi::IRHIDevice* device) {
    Shutdown();
    m_Device = device;
    if (!m_Device) {
        return false;
    }
    if (!LoadShaders()) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "VolumetricRenderer: failed to load VolumetricClouds shaders.");
        Shutdown();
        return false;
    }
    if (!LoadNoiseTextures()) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "VolumetricRenderer: failed to load cloud noise DDS assets.");
        Shutdown();
        return false;
    }

    we::rhi::DescriptorSetLayoutDesc cameraLayoutDesc{};
    cameraLayoutDesc.debugName = "Volumetric.CameraLayout";
    cameraLayoutDesc.bindings.push_back({
        0, we::rhi::DescriptorType::UniformBuffer, 1,
        we::rhi::ShaderStageFlags::Vertex | we::rhi::ShaderStageFlags::Fragment});
    auto cameraLayout = m_Device->CreateDescriptorSetLayout(cameraLayoutDesc);
    if (!cameraLayout) {
        Shutdown();
        return false;
    }
    m_CameraLayout = *cameraLayout;

    we::rhi::DescriptorSetLayoutDesc envLayoutDesc{};
    envLayoutDesc.debugName = "Volumetric.EnvLayout";
    envLayoutDesc.bindings.push_back({
        0, we::rhi::DescriptorType::UniformBuffer, 1,
        we::rhi::ShaderStageFlags::Vertex | we::rhi::ShaderStageFlags::Fragment});
    auto envLayout = m_Device->CreateDescriptorSetLayout(envLayoutDesc);
    if (!envLayout) {
        Shutdown();
        return false;
    }
    m_EnvLayout = *envLayout;

    we::rhi::DescriptorSetLayoutDesc frameLayoutDesc{};
    frameLayoutDesc.debugName = "Volumetric.FrameLayout";
    frameLayoutDesc.bindings.push_back({
        0, we::rhi::DescriptorType::UniformBuffer, 1,
        we::rhi::ShaderStageFlags::Fragment});
    auto frameLayout = m_Device->CreateDescriptorSetLayout(frameLayoutDesc);
    if (!frameLayout) {
        Shutdown();
        return false;
    }
    m_FrameLayout = *frameLayout;

    we::rhi::DescriptorSetLayoutDesc cloudLayoutDesc{};
    cloudLayoutDesc.debugName = "Volumetric.CloudLayout";
    cloudLayoutDesc.bindings.push_back({
        0, we::rhi::DescriptorType::UniformBuffer, 1,
        we::rhi::ShaderStageFlags::Fragment});
    auto cloudLayout = m_Device->CreateDescriptorSetLayout(cloudLayoutDesc);
    if (!cloudLayout) {
        Shutdown();
        return false;
    }
    m_CloudLayout = *cloudLayout;

    we::rhi::DescriptorSetLayoutDesc resourceLayoutDesc{};
    resourceLayoutDesc.debugName = "Volumetric.ResourceLayout";
    // t0: depth (Load). t1/s1: base shape. t2: detail. t3: weather map. t4: curl.
    resourceLayoutDesc.bindings.push_back({
        0, we::rhi::DescriptorType::SampledImage, 1,
        we::rhi::ShaderStageFlags::Fragment});
    resourceLayoutDesc.bindings.push_back({
        1, we::rhi::DescriptorType::CombinedImageSampler, 1,
        we::rhi::ShaderStageFlags::Fragment});
    resourceLayoutDesc.bindings.push_back({
        2, we::rhi::DescriptorType::SampledImage, 1,
        we::rhi::ShaderStageFlags::Fragment});
    resourceLayoutDesc.bindings.push_back({
        3, we::rhi::DescriptorType::SampledImage, 1,
        we::rhi::ShaderStageFlags::Fragment});
    resourceLayoutDesc.bindings.push_back({
        4, we::rhi::DescriptorType::SampledImage, 1,
        we::rhi::ShaderStageFlags::Fragment});
    auto resourceLayout = m_Device->CreateDescriptorSetLayout(resourceLayoutDesc);
    if (!resourceLayout) {
        Shutdown();
        return false;
    }
    m_ResourceLayout = *resourceLayout;

    we::rhi::PipelineLayoutDesc pipelineLayoutDesc{};
    pipelineLayoutDesc.debugName = "Volumetric.PipelineLayout";
    pipelineLayoutDesc.setLayouts = {
        m_CameraLayout, m_EnvLayout, m_FrameLayout, m_CloudLayout, m_ResourceLayout};
    auto pipelineLayout = m_Device->CreatePipelineLayout(pipelineLayoutDesc);
    if (!pipelineLayout) {
        Shutdown();
        return false;
    }
    m_PipelineLayout = *pipelineLayout;

    we::rhi::DescriptorSetLayoutDesc upsampleLayoutDesc{};
    upsampleLayoutDesc.debugName = "Volumetric.UpsampleLayout";
    upsampleLayoutDesc.bindings.push_back({
        0, we::rhi::DescriptorType::CombinedImageSampler, 1,
        we::rhi::ShaderStageFlags::Fragment});
    auto upsampleLayout = m_Device->CreateDescriptorSetLayout(upsampleLayoutDesc);
    if (!upsampleLayout) {
        Shutdown();
        return false;
    }
    m_UpsampleLayout = *upsampleLayout;

    we::rhi::PipelineLayoutDesc upsamplePipelineLayoutDesc{};
    upsamplePipelineLayoutDesc.debugName = "Volumetric.UpsamplePipelineLayout";
    upsamplePipelineLayoutDesc.setLayouts = {m_UpsampleLayout};
    auto upsamplePipelineLayout = m_Device->CreatePipelineLayout(upsamplePipelineLayoutDesc);
    if (!upsamplePipelineLayout) {
        Shutdown();
        return false;
    }
    m_UpsamplePipelineLayout = *upsamplePipelineLayout;

    auto makeUbo = [&](uint64_t size, const char* name) -> we::rhi::RHIBufferHandle {
        // DX12 CBVs require SizeInBytes multiple of 256; keep the underlying
        // buffer at least that large so CreateConstantBufferView stays valid.
        we::rhi::BufferDesc buf{};
        buf.size = (std::max)(size, static_cast<uint64_t>(256));
        buf.usage = we::rhi::BufferUsage::Uniform;
        buf.memory = we::rhi::MemoryUsage::HostVisible;
        buf.debugName = name;
        auto created = m_Device->CreateBuffer(buf);
        return created ? *created : we::rhi::RHIBufferHandle::Invalid;
    };
    m_CameraBuffer = makeUbo(sizeof(CameraUniform), "Volumetric.CameraUBO");
    m_EnvBuffer = makeUbo(sizeof(SceneEnvironmentUniform), "Volumetric.EnvUBO");
    m_FrameBuffer = makeUbo(sizeof(VolumetricFrameUniform), "Volumetric.FrameUBO");
    m_CloudBuffer = makeUbo(sizeof(CloudUniform), "Volumetric.CloudUBO");
    if (m_CameraBuffer == we::rhi::RHIBufferHandle::Invalid
        || m_EnvBuffer == we::rhi::RHIBufferHandle::Invalid
        || m_FrameBuffer == we::rhi::RHIBufferHandle::Invalid
        || m_CloudBuffer == we::rhi::RHIBufferHandle::Invalid) {
        Shutdown();
        return false;
    }

    we::rhi::SamplerDesc sampDesc{};
    sampDesc.magFilter = we::rhi::Filter::Linear;
    sampDesc.minFilter = we::rhi::Filter::Linear;
    sampDesc.mipFilter = we::rhi::Filter::Linear;
    sampDesc.addressU = we::rhi::AddressMode::Repeat;
    sampDesc.addressV = we::rhi::AddressMode::Repeat;
    sampDesc.addressW = we::rhi::AddressMode::Repeat;
    sampDesc.anisotropy = false;
    sampDesc.minLod = 0.0f;
    sampDesc.maxLod = 16.0f;
    sampDesc.debugName = "Volumetric.WrapSampler";
    auto sampler = m_Device->CreateSampler(sampDesc);
    if (!sampler) {
        Shutdown();
        return false;
    }
    m_Sampler = *sampler;
    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        "Volumetric sampler: filter=Linear/Linear/Linear "
        "address=Repeat/Repeat/Repeat (DX12 WRAP / Vulkan REPEAT on U/V/W)");

    we::rhi::SamplerDesc upSampDesc{};
    upSampDesc.magFilter = we::rhi::Filter::Linear;
    upSampDesc.minFilter = we::rhi::Filter::Linear;
    upSampDesc.mipFilter = we::rhi::Filter::Linear;
    upSampDesc.addressU = we::rhi::AddressMode::ClampToEdge;
    upSampDesc.addressV = we::rhi::AddressMode::ClampToEdge;
    upSampDesc.addressW = we::rhi::AddressMode::ClampToEdge;
    upSampDesc.debugName = "Volumetric.UpsampleSampler";
    auto upSampler = m_Device->CreateSampler(upSampDesc);
    if (!upSampler) {
        Shutdown();
        return false;
    }
    m_UpsampleSampler = *upSampler;

    we::rhi::DescriptorPoolDesc poolDesc{};
    poolDesc.maxSets = 16;
    poolDesc.debugName = "Volumetric.Pool";
    poolDesc.poolSizes.push_back({we::rhi::DescriptorType::UniformBuffer, 8});
    poolDesc.poolSizes.push_back({we::rhi::DescriptorType::SampledImage, 16});
    poolDesc.poolSizes.push_back({we::rhi::DescriptorType::CombinedImageSampler, 16});
    poolDesc.poolSizes.push_back({we::rhi::DescriptorType::Sampler, 8});
    auto pool = m_Device->CreateDescriptorPool(poolDesc);
    if (!pool) {
        Shutdown();
        return false;
    }
    m_Pool = *pool;

    auto allocSet = [&](we::rhi::RHIDescriptorSetLayoutHandle layout)
        -> we::rhi::RHIDescriptorSetHandle {
        we::rhi::DescriptorSetAllocateDesc desc{};
        desc.pool = m_Pool;
        desc.layout = layout;
        auto set = m_Device->AllocateDescriptorSet(desc);
        return set ? *set : we::rhi::RHIDescriptorSetHandle::Invalid;
    };
    m_CameraSet = allocSet(m_CameraLayout);
    m_EnvSet = allocSet(m_EnvLayout);
    m_FrameSet = allocSet(m_FrameLayout);
    m_CloudSet = allocSet(m_CloudLayout);
    m_ResourceSet = allocSet(m_ResourceLayout);
    m_UpsampleSet = allocSet(m_UpsampleLayout);
    if (m_CameraSet == we::rhi::RHIDescriptorSetHandle::Invalid
        || m_EnvSet == we::rhi::RHIDescriptorSetHandle::Invalid
        || m_FrameSet == we::rhi::RHIDescriptorSetHandle::Invalid
        || m_CloudSet == we::rhi::RHIDescriptorSetHandle::Invalid
        || m_ResourceSet == we::rhi::RHIDescriptorSetHandle::Invalid
        || m_UpsampleSet == we::rhi::RHIDescriptorSetHandle::Invalid) {
        Shutdown();
        return false;
    }

    we::rhi::DescriptorBufferInfo camInfo{m_CameraBuffer, 0, sizeof(CameraUniform)};
    we::rhi::DescriptorBufferInfo envInfo{m_EnvBuffer, 0, sizeof(SceneEnvironmentUniform)};
    we::rhi::DescriptorBufferInfo frameInfo{m_FrameBuffer, 0, sizeof(VolumetricFrameUniform)};
    we::rhi::DescriptorBufferInfo cloudInfo{m_CloudBuffer, 0, sizeof(CloudUniform)};
    we::rhi::WriteDescriptorSet writes[4]{};
    writes[0].set = m_CameraSet;
    writes[0].binding = 0;
    writes[0].type = we::rhi::DescriptorType::UniformBuffer;
    writes[0].count = 1;
    writes[0].bufferInfos = &camInfo;
    writes[1].set = m_EnvSet;
    writes[1].binding = 0;
    writes[1].type = we::rhi::DescriptorType::UniformBuffer;
    writes[1].count = 1;
    writes[1].bufferInfos = &envInfo;
    writes[2].set = m_FrameSet;
    writes[2].binding = 0;
    writes[2].type = we::rhi::DescriptorType::UniformBuffer;
    writes[2].count = 1;
    writes[2].bufferInfos = &frameInfo;
    writes[3].set = m_CloudSet;
    writes[3].binding = 0;
    writes[3].type = we::rhi::DescriptorType::UniformBuffer;
    writes[3].count = 1;
    writes[3].bufferInfos = &cloudInfo;
    m_Device->UpdateDescriptorSets(writes);

    if (!CreateDummyDepth()) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "VolumetricRenderer: failed to create dummy depth SRV.");
        Shutdown();
        return false;
    }

    // Bind immutable noise + weather textures.
    // t0 depth (dummy), t1/s1 base, t2 detail, t3 weather map, t4 curl — WRAP/LINEAR.
    we::rhi::DescriptorImageInfo depthInfo{};
    depthInfo.view = m_DummyDepthView;
    depthInfo.imageLayout = we::rhi::ResourceState::ShaderResource;

    we::rhi::DescriptorImageInfo baseInfo{};
    baseInfo.sampler = m_Sampler;
    baseInfo.view = m_BaseShape.view;
    baseInfo.imageLayout = we::rhi::ResourceState::ShaderResource;
    we::rhi::DescriptorImageInfo detailInfo{};
    detailInfo.view = m_DetailShape.view;
    detailInfo.imageLayout = we::rhi::ResourceState::ShaderResource;
    we::rhi::DescriptorImageInfo weatherInfo{};
    weatherInfo.view = m_WeatherMap.view;
    weatherInfo.imageLayout = we::rhi::ResourceState::ShaderResource;
    we::rhi::DescriptorImageInfo curlInfo{};
    curlInfo.view = m_CurlNoise.view;
    curlInfo.imageLayout = we::rhi::ResourceState::ShaderResource;

    we::rhi::WriteDescriptorSet texWrites[5]{};
    texWrites[0].set = m_ResourceSet;
    texWrites[0].binding = 0;
    texWrites[0].type = we::rhi::DescriptorType::SampledImage;
    texWrites[0].count = 1;
    texWrites[0].imageInfos = &depthInfo;
    texWrites[1].set = m_ResourceSet;
    texWrites[1].binding = 1;
    texWrites[1].type = we::rhi::DescriptorType::CombinedImageSampler;
    texWrites[1].count = 1;
    texWrites[1].imageInfos = &baseInfo;
    texWrites[2].set = m_ResourceSet;
    texWrites[2].binding = 2;
    texWrites[2].type = we::rhi::DescriptorType::SampledImage;
    texWrites[2].count = 1;
    texWrites[2].imageInfos = &detailInfo;
    texWrites[3].set = m_ResourceSet;
    texWrites[3].binding = 3;
    texWrites[3].type = we::rhi::DescriptorType::SampledImage;
    texWrites[3].count = 1;
    texWrites[3].imageInfos = &weatherInfo;
    texWrites[4].set = m_ResourceSet;
    texWrites[4].binding = 4;
    texWrites[4].type = we::rhi::DescriptorType::SampledImage;
    texWrites[4].count = 1;
    texWrites[4].imageInfos = &curlInfo;
    m_Device->UpdateDescriptorSets(texWrites);
    m_DepthBound = true;

    m_Ready = true;
    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        "VolumetricRenderer ready (VolumetricClouds / Nubis).");
    return true;
}

void VolumetricRenderer::DestroyOffscreenTarget() {
    if (!m_Device) {
        m_OffscreenColor = we::rhi::RHITextureHandle::Invalid;
        m_OffscreenView = we::rhi::RHITextureViewHandle::Invalid;
    m_OffscreenExtent = {};
    m_OffscreenFullExtent = {};
    m_OffscreenScale = 1.0f;
    m_OffscreenInSrv = false;
    return;
    }
    if (m_OffscreenView != we::rhi::RHITextureViewHandle::Invalid) {
        (void)m_Device->DestroyTextureView(m_OffscreenView);
        m_OffscreenView = we::rhi::RHITextureViewHandle::Invalid;
    }
    if (m_OffscreenColor != we::rhi::RHITextureHandle::Invalid) {
        (void)m_Device->DestroyTexture(m_OffscreenColor);
        m_OffscreenColor = we::rhi::RHITextureHandle::Invalid;
    }
    m_OffscreenExtent = {};
    m_OffscreenFullExtent = {};
    m_OffscreenScale = 1.0f;
    m_OffscreenInSrv = false;
}

void VolumetricRenderer::Shutdown() {
    if (!m_Device) {
        m_Ready = false;
        return;
    }

    DestroyOffscreenTarget();
    DestroyHistoryTargets();

    if (m_Pipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid) {
        (void)m_Device->DestroyGraphicsPipeline(m_Pipeline);
        m_Pipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
    }
    if (m_UpsamplePipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid) {
        (void)m_Device->DestroyGraphicsPipeline(m_UpsamplePipeline);
        m_UpsamplePipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
    }
    if (m_PipelineLayout != we::rhi::RHIPipelineLayoutHandle::Invalid) {
        (void)m_Device->DestroyPipelineLayout(m_PipelineLayout);
        m_PipelineLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
    }
    if (m_UpsamplePipelineLayout != we::rhi::RHIPipelineLayoutHandle::Invalid) {
        (void)m_Device->DestroyPipelineLayout(m_UpsamplePipelineLayout);
        m_UpsamplePipelineLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
    }
    if (m_DepthView != we::rhi::RHITextureViewHandle::Invalid) {
        (void)m_Device->DestroyTextureView(m_DepthView);
        m_DepthView = we::rhi::RHITextureViewHandle::Invalid;
    }
    m_BoundDepth = we::rhi::RHITextureHandle::Invalid;
    m_DepthBound = false;

    if (m_DummyDepthView != we::rhi::RHITextureViewHandle::Invalid) {
        (void)m_Device->DestroyTextureView(m_DummyDepthView);
        m_DummyDepthView = we::rhi::RHITextureViewHandle::Invalid;
    }
    if (m_DummyDepthTexture != we::rhi::RHITextureHandle::Invalid) {
        (void)m_Device->DestroyTexture(m_DummyDepthTexture);
        m_DummyDepthTexture = we::rhi::RHITextureHandle::Invalid;
    }

    if (m_Pool != we::rhi::RHIDescriptorPoolHandle::Invalid) {
        (void)m_Device->DestroyDescriptorPool(m_Pool);
        m_Pool = we::rhi::RHIDescriptorPoolHandle::Invalid;
        m_CameraSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        m_EnvSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        m_FrameSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        m_CloudSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        m_ResourceSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        m_UpsampleSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    auto destroyLayout = [&](we::rhi::RHIDescriptorSetLayoutHandle& h) {
        if (h != we::rhi::RHIDescriptorSetLayoutHandle::Invalid) {
            (void)m_Device->DestroyDescriptorSetLayout(h);
            h = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
        }
    };
    destroyLayout(m_CameraLayout);
    destroyLayout(m_EnvLayout);
    destroyLayout(m_FrameLayout);
    destroyLayout(m_CloudLayout);
    destroyLayout(m_ResourceLayout);
    destroyLayout(m_UpsampleLayout);

    auto destroyBuf = [&](we::rhi::RHIBufferHandle& h) {
        if (h != we::rhi::RHIBufferHandle::Invalid) {
            (void)m_Device->DestroyBuffer(h);
            h = we::rhi::RHIBufferHandle::Invalid;
        }
    };
    destroyBuf(m_CameraBuffer);
    destroyBuf(m_EnvBuffer);
    destroyBuf(m_FrameBuffer);
    destroyBuf(m_CloudBuffer);

    if (m_Sampler != we::rhi::RHISamplerHandle::Invalid) {
        (void)m_Device->DestroySampler(m_Sampler);
        m_Sampler = we::rhi::RHISamplerHandle::Invalid;
    }
    if (m_UpsampleSampler != we::rhi::RHISamplerHandle::Invalid) {
        (void)m_Device->DestroySampler(m_UpsampleSampler);
        m_UpsampleSampler = we::rhi::RHISamplerHandle::Invalid;
    }

    DestroyLoadedDdsTexture(*m_Device, m_BaseShape);
    DestroyLoadedDdsTexture(*m_Device, m_DetailShape);
    DestroyLoadedDdsTexture(*m_Device, m_WeatherMap);
    DestroyLoadedDdsTexture(*m_Device, m_CurlNoise);

    if (m_Vs != we::rhi::RHIShaderHandle::Invalid) {
        (void)m_Device->DestroyShader(m_Vs);
        m_Vs = we::rhi::RHIShaderHandle::Invalid;
    }
    if (m_Ps != we::rhi::RHIShaderHandle::Invalid) {
        (void)m_Device->DestroyShader(m_Ps);
        m_Ps = we::rhi::RHIShaderHandle::Invalid;
    }
    if (m_UpsampleVs != we::rhi::RHIShaderHandle::Invalid) {
        (void)m_Device->DestroyShader(m_UpsampleVs);
        m_UpsampleVs = we::rhi::RHIShaderHandle::Invalid;
    }
    if (m_UpsamplePs != we::rhi::RHIShaderHandle::Invalid) {
        (void)m_Device->DestroyShader(m_UpsamplePs);
        m_UpsamplePs = we::rhi::RHIShaderHandle::Invalid;
    }

    m_PipelineColorFormat = we::rhi::Format::Unknown;
    m_UpsampleColorFormat = we::rhi::Format::Unknown;
    m_Device = nullptr;
    m_Ready = false;
}

bool VolumetricRenderer::LoadShaders() {
    const auto format = we::rhi::ShaderBytecodeLoader::ResolveFormat(m_Device);
    m_VsBytes = we::rhi::ShaderBytecodeLoader::Load("VolumetricClouds", "VS", format);
    m_PsBytes = we::rhi::ShaderBytecodeLoader::Load("VolumetricClouds", "PS", format);
    m_UpsampleVsBytes = we::rhi::ShaderBytecodeLoader::Load("CloudUpsample", "VS", format);
    m_UpsamplePsBytes = we::rhi::ShaderBytecodeLoader::Load("CloudUpsample", "PS", format);
    if (m_VsBytes.empty() || m_PsBytes.empty()
        || m_UpsampleVsBytes.empty() || m_UpsamplePsBytes.empty()) {
        return false;
    }

    we::rhi::ShaderDesc vsDesc{};
    vsDesc.stage = we::rhi::ShaderStage::Vertex;
    vsDesc.format = format;
    vsDesc.bytecode = m_VsBytes;
    vsDesc.entryPoint = "VSMain";
    vsDesc.debugName = "VolumetricClouds.VS";
    auto vs = m_Device->CreateShader(vsDesc);
    if (!vs) {
        return false;
    }
    m_Vs = *vs;

    we::rhi::ShaderDesc psDesc{};
    psDesc.stage = we::rhi::ShaderStage::Fragment;
    psDesc.format = format;
    psDesc.bytecode = m_PsBytes;
    psDesc.entryPoint = "PSMain";
    psDesc.debugName = "VolumetricClouds.PS";
    auto ps = m_Device->CreateShader(psDesc);
    if (!ps) {
        return false;
    }
    m_Ps = *ps;

    we::rhi::ShaderDesc upVsDesc{};
    upVsDesc.stage = we::rhi::ShaderStage::Vertex;
    upVsDesc.format = format;
    upVsDesc.bytecode = m_UpsampleVsBytes;
    upVsDesc.entryPoint = "VSMain";
    upVsDesc.debugName = "CloudUpsample.VS";
    auto upVs = m_Device->CreateShader(upVsDesc);
    if (!upVs) {
        return false;
    }
    m_UpsampleVs = *upVs;

    we::rhi::ShaderDesc upPsDesc{};
    upPsDesc.stage = we::rhi::ShaderStage::Fragment;
    upPsDesc.format = format;
    upPsDesc.bytecode = m_UpsamplePsBytes;
    upPsDesc.entryPoint = "PSMain";
    upPsDesc.debugName = "CloudUpsample.PS";
    auto upPs = m_Device->CreateShader(upPsDesc);
    if (!upPs) {
        return false;
    }
    m_UpsamplePs = *upPs;
    return true;
}

bool VolumetricRenderer::LoadNoiseTextures() {
    auto base = LoadDdsRgba8Texture(
        *m_Device, ResolveCloudAsset("CloudBaseShape128.dds"), "Cloud.BaseShape128");
    auto detail = LoadDdsRgba8Texture(
        *m_Device, ResolveCloudAsset("CloudDetailErosion32.dds"), "Cloud.DetailErosion32");
    auto curl = LoadDdsRgba8Texture(
        *m_Device, ResolveCloudAsset("CloudTurbulenceCurl128.dds"), "Cloud.TurbulenceCurl128");
    auto weather = LoadDdsRgba8Texture(
        *m_Device, ResolveCloudAsset("CloudWeatherMap1024.dds"), "Cloud.WeatherMap1024");

    if (!base || !detail || !curl) {
        if (base) {
            DestroyLoadedDdsTexture(*m_Device, *base);
        }
        if (detail) {
            DestroyLoadedDdsTexture(*m_Device, *detail);
        }
        if (curl) {
            DestroyLoadedDdsTexture(*m_Device, *curl);
        }
        if (weather) {
            DestroyLoadedDdsTexture(*m_Device, *weather);
        }
        return false;
    }

    // Weather map is required for Nubis macro structure, but never take down the
    // whole cloud pass if the DDS is missing — synthesize a mid-coverage 1x1.
    if (!weather) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            "VolumetricRenderer: CloudWeatherMap1024.dds missing — using 1x1 fallback.");
        we::rhi::TextureDesc texDesc{};
        texDesc.extent = {1, 1, 1};
        texDesc.format = we::rhi::Format::R8G8B8A8_UNORM;
        texDesc.usage = we::rhi::TextureUsage::Sampled | we::rhi::TextureUsage::TransferDst;
        texDesc.mipLevels = 1;
        texDesc.arrayLayers = 1;
        texDesc.debugName = "Cloud.WeatherMapFallback";
        auto texture = m_Device->CreateTexture(texDesc);
        if (!texture) {
            DestroyLoadedDdsTexture(*m_Device, *base);
            DestroyLoadedDdsTexture(*m_Device, *detail);
            DestroyLoadedDdsTexture(*m_Device, *curl);
            return false;
        }
        // R=180 coverage, G=0 precip, B=128 cumulus, A=200 altitude
        const uint8_t px[4] = {180, 0, 128, 200};
        we::rhi::TextureUpdateDesc update{};
        update.extent = {1, 1, 1};
        update.rowPitch = 4;
        update.data = std::span<const uint8_t>(px, 4);
        if (!m_Device->UpdateTexture(*texture, update)) {
            (void)m_Device->DestroyTexture(*texture);
            DestroyLoadedDdsTexture(*m_Device, *base);
            DestroyLoadedDdsTexture(*m_Device, *detail);
            DestroyLoadedDdsTexture(*m_Device, *curl);
            return false;
        }
        we::rhi::TextureViewDesc viewDesc{};
        viewDesc.texture = *texture;
        viewDesc.format = we::rhi::Format::R8G8B8A8_UNORM;
        viewDesc.debugName = "Cloud.WeatherMapFallback";
        auto view = m_Device->CreateTextureView(viewDesc);
        if (!view) {
            (void)m_Device->DestroyTexture(*texture);
            DestroyLoadedDdsTexture(*m_Device, *base);
            DestroyLoadedDdsTexture(*m_Device, *detail);
            DestroyLoadedDdsTexture(*m_Device, *curl);
            return false;
        }
        LoadedDdsTexture fallback{};
        fallback.texture = *texture;
        fallback.view = *view;
        fallback.width = 1;
        fallback.height = 1;
        fallback.depth = 1;
        fallback.mipLevels = 1;
        weather = fallback;
    }

    m_BaseShape = *base;
    m_DetailShape = *detail;
    m_WeatherMap = *weather;
    m_CurlNoise = *curl;
    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        "VolumetricRenderer: loaded weather map + noise DDS assets."
        " CloudShape mips=" + std::to_string(m_BaseShape.mipLevels)
        + " Detail mips=" + std::to_string(m_DetailShape.mipLevels));
    return true;
}

bool VolumetricRenderer::EnsurePipeline(we::rhi::Format colorFormat) {
    if (m_Pipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid
        && m_PipelineColorFormat == colorFormat) {
        return true;
    }
    if (m_Pipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid) {
        (void)m_Device->DestroyGraphicsPipeline(m_Pipeline);
        m_Pipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
    }

    we::rhi::GraphicsPipelineDesc pso{};
    pso.vertexShader = m_Vs;
    pso.fragmentShader = m_Ps;
    pso.layout = m_PipelineLayout;
    pso.topology = we::rhi::PrimitiveTopology::TriangleList;
    pso.cullMode = we::rhi::CullMode::None;
    pso.depthTest = false;
    pso.depthWrite = false;
    pso.depthCompare = we::rhi::CompareOp::Always;
    pso.colorFormat = colorFormat;
    pso.depthAttachment = false;
    pso.blend.enable = true;
    // Premultiplied: out = src.rgb + dst.rgb * (1 - src.a)
    pso.blend.srcColor = we::rhi::BlendFactor::One;
    pso.blend.dstColor = we::rhi::BlendFactor::OneMinusSrcAlpha;
    pso.blend.srcAlpha = we::rhi::BlendFactor::One;
    pso.blend.dstAlpha = we::rhi::BlendFactor::OneMinusSrcAlpha;
    pso.debugName = "Volumetric.PSO";
    auto pipeline = m_Device->CreateGraphicsPipeline(pso);
    if (!pipeline) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "VolumetricRenderer: CreateGraphicsPipeline failed.");
        return false;
    }
    m_Pipeline = *pipeline;
    m_PipelineColorFormat = colorFormat;
    return true;
}

bool VolumetricRenderer::EnsureUpsamplePipeline(we::rhi::Format colorFormat) {
    if (m_UpsamplePipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid
        && m_UpsampleColorFormat == colorFormat) {
        return true;
    }
    if (m_UpsamplePipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid) {
        (void)m_Device->DestroyGraphicsPipeline(m_UpsamplePipeline);
        m_UpsamplePipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
    }

    we::rhi::GraphicsPipelineDesc pso{};
    pso.vertexShader = m_UpsampleVs;
    pso.fragmentShader = m_UpsamplePs;
    pso.layout = m_UpsamplePipelineLayout;
    pso.topology = we::rhi::PrimitiveTopology::TriangleList;
    pso.cullMode = we::rhi::CullMode::None;
    pso.depthTest = false;
    pso.depthWrite = false;
    pso.depthCompare = we::rhi::CompareOp::Always;
    pso.colorFormat = colorFormat;
    pso.depthAttachment = false;
    pso.blend.enable = true;
    pso.blend.srcColor = we::rhi::BlendFactor::One;
    pso.blend.dstColor = we::rhi::BlendFactor::OneMinusSrcAlpha;
    pso.blend.srcAlpha = we::rhi::BlendFactor::One;
    pso.blend.dstAlpha = we::rhi::BlendFactor::OneMinusSrcAlpha;
    pso.debugName = "Volumetric.UpsamplePSO";
    auto pipeline = m_Device->CreateGraphicsPipeline(pso);
    if (!pipeline) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "VolumetricRenderer: CreateGraphicsPipeline(upsample) failed.");
        return false;
    }
    m_UpsamplePipeline = *pipeline;
    m_UpsampleColorFormat = colorFormat;
    return true;
}

bool VolumetricRenderer::EnsureOffscreenTarget(we::rhi::Extent2D fullExtent, float resolutionScale) {
    const float scale = (std::clamp)(resolutionScale, 0.35f, 1.0f);
    const uint32_t w = (std::max)(1u, static_cast<uint32_t>(
        static_cast<float>(fullExtent.width) * scale + 0.5f));
    const uint32_t h = (std::max)(1u, static_cast<uint32_t>(
        static_cast<float>(fullExtent.height) * scale + 0.5f));

    if (m_OffscreenColor != we::rhi::RHITextureHandle::Invalid
        && m_OffscreenExtent.width == w
        && m_OffscreenExtent.height == h
        && m_OffscreenFullExtent.width == fullExtent.width
        && m_OffscreenFullExtent.height == fullExtent.height
        && std::abs(m_OffscreenScale - scale) < 1.0e-4f) {
        return true;
    }

    DestroyOffscreenTarget();

    we::rhi::TextureDesc desc{};
    desc.extent = {w, h, 1};
    desc.format = we::rhi::Format::R16G16B16A16_SFLOAT;
    desc.usage = we::rhi::TextureUsage::ColorAttachment | we::rhi::TextureUsage::Sampled
        | we::rhi::TextureUsage::TransferSrc;
    desc.debugName = "Volumetric.Offscreen";
    auto tex = m_Device->CreateTexture(desc);
    if (!tex) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "VolumetricRenderer: failed to create offscreen cloud RT.");
        return false;
    }
    m_OffscreenColor = *tex;

    we::rhi::TextureViewDesc viewDesc{};
    viewDesc.texture = m_OffscreenColor;
    viewDesc.debugName = "Volumetric.OffscreenSRV";
    auto view = m_Device->CreateTextureView(viewDesc);
    if (!view) {
        DestroyOffscreenTarget();
        return false;
    }
    m_OffscreenView = *view;
    m_OffscreenExtent = {w, h};
    m_OffscreenFullExtent = fullExtent;
    m_OffscreenScale = scale;

    static bool s_LoggedScale = false;
    if (!s_LoggedScale) {
        s_LoggedScale = true;
        WE_LOG_INFO(we::LogCategory::Renderer.data(),
            std::string("Cloud resolution scale=") + std::to_string(scale)
            + " offscreen=" + std::to_string(w) + "x" + std::to_string(h)
            + " full=" + std::to_string(fullExtent.width) + "x"
            + std::to_string(fullExtent.height));
    }
    return true;
}

bool VolumetricRenderer::CreateDummyDepth() {
    // 1x1 RGBA8 is the well-tested UpdateTexture path (UI atlases); avoid R32
    // one-offs during bring-up. Depth Load() only needs a valid sampled image.
    we::rhi::TextureDesc desc{};
    desc.extent = {1, 1, 1};
    desc.format = we::rhi::Format::R8G8B8A8_UNORM;
    desc.usage = we::rhi::TextureUsage::Sampled | we::rhi::TextureUsage::TransferDst;
    desc.debugName = "Volumetric.DummyDepth";
    auto tex = m_Device->CreateTexture(desc);
    if (!tex) {
        return false;
    }
    m_DummyDepthTexture = *tex;

    const std::array<uint8_t, 4> pixel = {255, 255, 255, 255};
    we::rhi::TextureUpdateDesc update{};
    update.extent = {1, 1, 1};
    update.data = pixel;
    update.rowPitch = 4;
    if (!m_Device->UpdateTexture(m_DummyDepthTexture, update)) {
        (void)m_Device->DestroyTexture(m_DummyDepthTexture);
        m_DummyDepthTexture = we::rhi::RHITextureHandle::Invalid;
        return false;
    }

    we::rhi::TextureViewDesc viewDesc{};
    viewDesc.texture = m_DummyDepthTexture;
    viewDesc.format = we::rhi::Format::R8G8B8A8_UNORM;
    viewDesc.debugName = "Volumetric.DummyDepthSRV";
    auto view = m_Device->CreateTextureView(viewDesc);
    if (!view) {
        (void)m_Device->DestroyTexture(m_DummyDepthTexture);
        m_DummyDepthTexture = we::rhi::RHITextureHandle::Invalid;
        return false;
    }
    m_DummyDepthView = *view;
    return true;
}

void VolumetricRenderer::InvalidateDepthBinding() {
    if (!m_Device) {
        m_BoundDepth = we::rhi::RHITextureHandle::Invalid;
        m_DepthView = we::rhi::RHITextureViewHandle::Invalid;
        m_DepthBound = false;
        return;
    }
    if (m_DepthView != we::rhi::RHITextureViewHandle::Invalid) {
        (void)m_Device->DestroyTextureView(m_DepthView);
        m_DepthView = we::rhi::RHITextureViewHandle::Invalid;
    }
    m_BoundDepth = we::rhi::RHITextureHandle::Invalid;

    // Re-bind dummy so descriptor set binding 0 is never null.
    if (m_Ready && m_ResourceSet != we::rhi::RHIDescriptorSetHandle::Invalid
        && m_DummyDepthView != we::rhi::RHITextureViewHandle::Invalid) {
        we::rhi::DescriptorImageInfo depthInfo{};
        depthInfo.view = m_DummyDepthView;
        depthInfo.imageLayout = we::rhi::ResourceState::ShaderResource;
        we::rhi::WriteDescriptorSet write{};
        write.set = m_ResourceSet;
        write.binding = 0;
        write.type = we::rhi::DescriptorType::SampledImage;
        write.count = 1;
        write.imageInfos = &depthInfo;
        m_Device->UpdateDescriptorSets(std::span<const we::rhi::WriteDescriptorSet>(&write, 1));
        m_DepthBound = true;
    } else {
        m_DepthBound = false;
    }
}

bool VolumetricRenderer::EnsureDepthView(we::rhi::RHITextureHandle depth) {
    if (depth == we::rhi::RHITextureHandle::Invalid) {
        return false;
    }
    if (m_BoundDepth == depth && m_DepthView != we::rhi::RHITextureViewHandle::Invalid) {
        return true;
    }
    if (m_DepthView != we::rhi::RHITextureViewHandle::Invalid) {
        (void)m_Device->DestroyTextureView(m_DepthView);
        m_DepthView = we::rhi::RHITextureViewHandle::Invalid;
    }
    we::rhi::TextureViewDesc viewDesc{};
    viewDesc.texture = depth;
    viewDesc.format = we::rhi::Format::R32_SFLOAT;
    viewDesc.debugName = "Volumetric.DepthSRV";
    auto view = m_Device->CreateTextureView(viewDesc);
    if (!view) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "VolumetricRenderer: CreateTextureView(depth) failed.");
        return false;
    }
    m_DepthView = *view;
    m_BoundDepth = depth;
    return true;
}

bool VolumetricRenderer::BindResourceDescriptors(we::rhi::RHITextureHandle depth) {
    if (!EnsureDepthView(depth)) {
        return false;
    }
    we::rhi::DescriptorImageInfo depthInfo{};
    depthInfo.view = m_DepthView;
    depthInfo.imageLayout = we::rhi::ResourceState::ShaderResource;

    we::rhi::WriteDescriptorSet write{};
    write.set = m_ResourceSet;
    write.binding = 0;
    write.type = we::rhi::DescriptorType::SampledImage;
    write.count = 1;
    write.imageInfos = &depthInfo;
    m_Device->UpdateDescriptorSets(std::span<const we::rhi::WriteDescriptorSet>(&write, 1));
    m_DepthBound = true;
    return true;
}

void VolumetricRenderer::Draw(
    we::rhi::IRHICommandList& cmd,
    we::rhi::RHITextureHandle color,
    we::rhi::RHITextureHandle depth,
    we::rhi::Extent2D extent,
    const CameraUniform& camera,
    const SceneEnvironmentUniform& environment,
    const CloudUniform& clouds,
    const VolumetricQualitySettings& quality,
    uint32_t frameIndex,
    float resolutionScale)
{
    if (!m_Ready || !m_Device || color == we::rhi::RHITextureHandle::Invalid
        || depth == we::rhi::RHITextureHandle::Invalid
        || extent.width == 0 || extent.height == 0) {
        return;
    }

    if (!m_CloudProvider) {
        RegisterDefaultProviders();
    }

    const LocalFogUniform& localFog = m_LocalFogProvider->GetLocalFog();
    VolumetricPrepareContext prep{};
    prep.camera = &camera;
    prep.environment = &environment;
    prep.cloud = &clouds;
    prep.localFog = &localFog;
    prep.quality = &quality;
    prep.frameIndex = frameIndex;
    for (IVolumetricProvider* provider : m_Providers) {
        if (provider) {
            provider->PrepareFrame(prep);
        }
    }

    // Skip draw only when every provider is off.
    bool anyEnabled = false;
    for (const IVolumetricProvider* provider : m_Providers) {
        if (provider && provider->IsEnabled()) {
            anyEnabled = true;
            break;
        }
    }
    if (!anyEnabled) {
        return;
    }

    if (!EnsurePipeline(we::rhi::Format::R16G16B16A16_SFLOAT)) {
        return;
    }
    if (!BindResourceDescriptors(depth)) {
        static bool s_LoggedDepthFail = false;
        if (!s_LoggedDepthFail) {
            WE_LOG_WARN(we::LogCategory::Renderer.data(),
                "VolumetricRenderer: depth SRV bind failed — drawing with dummy depth.");
            s_LoggedDepthFail = true;
        }
    }

    const float scale = (std::clamp)(resolutionScale, 0.35f, 1.0f);
    (void)EnsureHistoryTargets(extent);

    const VolumetricFrameUniform frame = BuildFrameUniform(quality, frameIndex, scale);

    (void)m_Device->UpdateBuffer(
        m_CameraBuffer,
        std::span(reinterpret_cast<const uint8_t*>(&camera), sizeof(camera)));
    (void)m_Device->UpdateBuffer(
        m_EnvBuffer,
        std::span(reinterpret_cast<const uint8_t*>(&environment), sizeof(environment)));
    (void)m_Device->UpdateBuffer(
        m_FrameBuffer,
        std::span(reinterpret_cast<const uint8_t*>(&frame), sizeof(frame)));
    (void)m_Device->UpdateBuffer(
        m_CloudBuffer,
        std::span(reinterpret_cast<const uint8_t*>(&clouds), sizeof(clouds)));

    const bool useHalfRes = scale < 0.999f
        && EnsureOffscreenTarget(extent, scale)
        && EnsureUpsamplePipeline(we::rhi::Format::R16G16B16A16_SFLOAT);

    const we::rhi::RHITextureHandle marchTarget =
        useHalfRes ? m_OffscreenColor : color;
    const we::rhi::Extent2D marchExtent =
        useHalfRes ? m_OffscreenExtent : extent;
    const we::rhi::LoadOp marchLoad =
        useHalfRes ? we::rhi::LoadOp::Clear : we::rhi::LoadOp::Load;
    const we::rhi::Color4f clearColor{0.0f, 0.0f, 0.0f, 0.0f};

    if (useHalfRes) {
        cmd.TransitionTexture(
            m_OffscreenColor,
            m_OffscreenInSrv
                ? we::rhi::ResourceState::ShaderResource
                : we::rhi::ResourceState::Undefined,
            we::rhi::ResourceState::RenderTarget);
    }

    we::rhi::RenderingInfo info{};
    we::rhi::ColorAttachmentDesc colorAtt{};
    colorAtt.texture = marchTarget;
    colorAtt.loadOp = marchLoad;
    colorAtt.storeOp = we::rhi::StoreOp::Store;
    colorAtt.clearColor = clearColor;
    info.colorAttachments.push_back(colorAtt);
    info.renderArea = marchExtent;

    cmd.BeginRendering(info);
    cmd.SetViewport({0, 0,
        static_cast<float>(marchExtent.width),
        static_cast<float>(marchExtent.height),
        0.0f, 1.0f});
    cmd.SetScissor({0, 0, marchExtent.width, marchExtent.height});
    cmd.BindGraphicsPipeline(m_Pipeline);
    const we::rhi::RHIDescriptorSetHandle sets[] = {
        m_CameraSet, m_EnvSet, m_FrameSet, m_CloudSet, m_ResourceSet};
    cmd.BindDescriptorSets(
        we::rhi::PipelineBindPoint::Graphics,
        m_PipelineLayout,
        0,
        sets);
    cmd.Draw(3);
    cmd.EndRendering();

    m_HistoryWriteIndex ^= 1u;

    if (!useHalfRes) {
        return;
    }

    cmd.TransitionTexture(
        m_OffscreenColor,
        we::rhi::ResourceState::RenderTarget,
        we::rhi::ResourceState::ShaderResource);
    m_OffscreenInSrv = true;

    we::rhi::DescriptorImageInfo upInfo{};
    upInfo.sampler = m_UpsampleSampler;
    upInfo.view = m_OffscreenView;
    upInfo.imageLayout = we::rhi::ResourceState::ShaderResource;
    we::rhi::WriteDescriptorSet upWrite{};
    upWrite.set = m_UpsampleSet;
    upWrite.binding = 0;
    upWrite.type = we::rhi::DescriptorType::CombinedImageSampler;
    upWrite.count = 1;
    upWrite.imageInfos = &upInfo;
    m_Device->UpdateDescriptorSets(std::span<const we::rhi::WriteDescriptorSet>(&upWrite, 1));

    we::rhi::RenderingInfo upInfoPass{};
    we::rhi::ColorAttachmentDesc upAtt{};
    upAtt.texture = color;
    upAtt.loadOp = we::rhi::LoadOp::Load;
    upAtt.storeOp = we::rhi::StoreOp::Store;
    upInfoPass.colorAttachments.push_back(upAtt);
    upInfoPass.renderArea = extent;

    cmd.BeginRendering(upInfoPass);
    cmd.SetViewport({0, 0,
        static_cast<float>(extent.width),
        static_cast<float>(extent.height),
        0.0f, 1.0f});
    cmd.SetScissor({0, 0, extent.width, extent.height});
    cmd.BindGraphicsPipeline(m_UpsamplePipeline);
    const we::rhi::RHIDescriptorSetHandle upSets[] = {m_UpsampleSet};
    cmd.BindDescriptorSets(
        we::rhi::PipelineBindPoint::Graphics,
        m_UpsamplePipelineLayout,
        0,
        upSets);
    cmd.Draw(3);
    cmd.EndRendering();
}

void VolumetricRenderer::DestroyHistoryTargets() {
    if (!m_Device) {
        m_History[0] = we::rhi::RHITextureHandle::Invalid;
        m_History[1] = we::rhi::RHITextureHandle::Invalid;
        m_HistoryView[0] = we::rhi::RHITextureViewHandle::Invalid;
        m_HistoryView[1] = we::rhi::RHITextureViewHandle::Invalid;
        m_HistoryExtent = {};
        return;
    }
    for (int i = 0; i < 2; ++i) {
        if (m_HistoryView[i] != we::rhi::RHITextureViewHandle::Invalid) {
            (void)m_Device->DestroyTextureView(m_HistoryView[i]);
            m_HistoryView[i] = we::rhi::RHITextureViewHandle::Invalid;
        }
        if (m_History[i] != we::rhi::RHITextureHandle::Invalid) {
            (void)m_Device->DestroyTexture(m_History[i]);
            m_History[i] = we::rhi::RHITextureHandle::Invalid;
        }
    }
    m_HistoryExtent = {};
}

bool VolumetricRenderer::EnsureHistoryTargets(we::rhi::Extent2D extent) {
    if (!m_Device || extent.width == 0 || extent.height == 0) {
        return false;
    }
    if (m_History[0] != we::rhi::RHITextureHandle::Invalid
        && m_HistoryExtent.width == extent.width
        && m_HistoryExtent.height == extent.height) {
        return true;
    }
    DestroyHistoryTargets();

    for (int i = 0; i < 2; ++i) {
        we::rhi::TextureDesc desc{};
        desc.extent = {extent.width, extent.height, 1};
        desc.format = we::rhi::Format::R16G16B16A16_SFLOAT;
        desc.usage = we::rhi::TextureUsage::ColorAttachment | we::rhi::TextureUsage::Sampled;
        desc.mipLevels = 1;
        desc.arrayLayers = 1;
        desc.debugName = (i == 0) ? "Volumetric.History0" : "Volumetric.History1";
        auto tex = m_Device->CreateTexture(desc);
        if (!tex) {
            DestroyHistoryTargets();
            return false;
        }
        m_History[i] = *tex;

        we::rhi::TextureViewDesc viewDesc{};
        viewDesc.texture = m_History[i];
        viewDesc.format = we::rhi::Format::R16G16B16A16_SFLOAT;
        viewDesc.debugName = desc.debugName;
        auto view = m_Device->CreateTextureView(viewDesc);
        if (!view) {
            DestroyHistoryTargets();
            return false;
        }
        m_HistoryView[i] = *view;
    }
    m_HistoryExtent = extent;
    return true;
}

} // namespace we::runtime::renderer
