// ==============================================================================
// WindEffects — Renderer — ViewportCloudRenderer
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Graph/ViewportCloudRenderer.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Core/Paths.h"
#include "RHI/Desc.h"
#include "RHI/ShaderBytecode.h"

#include <algorithm>
#include <array>
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

ViewportCloudRenderer::~ViewportCloudRenderer() {
    Shutdown();
}

bool ViewportCloudRenderer::Init(we::rhi::IRHIDevice* device) {
    Shutdown();
    m_Device = device;
    if (!m_Device) {
        return false;
    }
    if (!LoadShaders()) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "ViewportCloudRenderer: failed to load VolumetricClouds shaders.");
        Shutdown();
        return false;
    }
    if (!LoadNoiseTextures()) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "ViewportCloudRenderer: failed to load cloud noise DDS assets.");
        Shutdown();
        return false;
    }

    we::rhi::DescriptorSetLayoutDesc cameraLayoutDesc{};
    cameraLayoutDesc.debugName = "ViewportCloud.CameraLayout";
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
    envLayoutDesc.debugName = "ViewportCloud.EnvLayout";
    envLayoutDesc.bindings.push_back({
        0, we::rhi::DescriptorType::UniformBuffer, 1,
        we::rhi::ShaderStageFlags::Vertex | we::rhi::ShaderStageFlags::Fragment});
    auto envLayout = m_Device->CreateDescriptorSetLayout(envLayoutDesc);
    if (!envLayout) {
        Shutdown();
        return false;
    }
    m_EnvLayout = *envLayout;

    we::rhi::DescriptorSetLayoutDesc cloudLayoutDesc{};
    cloudLayoutDesc.debugName = "ViewportCloud.CloudLayout";
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
    resourceLayoutDesc.debugName = "ViewportCloud.ResourceLayout";
    // t0: depth (Load) — SampledImage only. Never leave this binding null (NVIDIA AV).
    resourceLayoutDesc.bindings.push_back({
        0, we::rhi::DescriptorType::SampledImage, 1,
        we::rhi::ShaderStageFlags::Fragment});
    // t1/s1 combined base shape; t2/t3 sampled detail+curl (share s1).
    resourceLayoutDesc.bindings.push_back({
        1, we::rhi::DescriptorType::CombinedImageSampler, 1,
        we::rhi::ShaderStageFlags::Fragment});
    resourceLayoutDesc.bindings.push_back({
        2, we::rhi::DescriptorType::SampledImage, 1,
        we::rhi::ShaderStageFlags::Fragment});
    resourceLayoutDesc.bindings.push_back({
        3, we::rhi::DescriptorType::SampledImage, 1,
        we::rhi::ShaderStageFlags::Fragment});
    auto resourceLayout = m_Device->CreateDescriptorSetLayout(resourceLayoutDesc);
    if (!resourceLayout) {
        Shutdown();
        return false;
    }
    m_ResourceLayout = *resourceLayout;

    we::rhi::PipelineLayoutDesc pipelineLayoutDesc{};
    pipelineLayoutDesc.debugName = "ViewportCloud.PipelineLayout";
    pipelineLayoutDesc.setLayouts = {
        m_CameraLayout, m_EnvLayout, m_CloudLayout, m_ResourceLayout};
    auto pipelineLayout = m_Device->CreatePipelineLayout(pipelineLayoutDesc);
    if (!pipelineLayout) {
        Shutdown();
        return false;
    }
    m_PipelineLayout = *pipelineLayout;

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
    m_CameraBuffer = makeUbo(sizeof(CameraUniform), "ViewportCloud.CameraUBO");
    m_EnvBuffer = makeUbo(sizeof(SceneEnvironmentUniform), "ViewportCloud.EnvUBO");
    m_CloudBuffer = makeUbo(sizeof(CloudUniform), "ViewportCloud.CloudUBO");
    if (m_CameraBuffer == we::rhi::RHIBufferHandle::Invalid
        || m_EnvBuffer == we::rhi::RHIBufferHandle::Invalid
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
    sampDesc.debugName = "ViewportCloud.WrapSampler";
    auto sampler = m_Device->CreateSampler(sampDesc);
    if (!sampler) {
        Shutdown();
        return false;
    }
    m_Sampler = *sampler;

    we::rhi::DescriptorPoolDesc poolDesc{};
    poolDesc.maxSets = 8;
    poolDesc.debugName = "ViewportCloud.Pool";
    poolDesc.poolSizes.push_back({we::rhi::DescriptorType::UniformBuffer, 8});
    poolDesc.poolSizes.push_back({we::rhi::DescriptorType::SampledImage, 16});
    poolDesc.poolSizes.push_back({we::rhi::DescriptorType::CombinedImageSampler, 8});
    poolDesc.poolSizes.push_back({we::rhi::DescriptorType::Sampler, 4});
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
    m_CloudSet = allocSet(m_CloudLayout);
    m_ResourceSet = allocSet(m_ResourceLayout);
    if (m_CameraSet == we::rhi::RHIDescriptorSetHandle::Invalid
        || m_EnvSet == we::rhi::RHIDescriptorSetHandle::Invalid
        || m_CloudSet == we::rhi::RHIDescriptorSetHandle::Invalid
        || m_ResourceSet == we::rhi::RHIDescriptorSetHandle::Invalid) {
        Shutdown();
        return false;
    }

    we::rhi::DescriptorBufferInfo camInfo{m_CameraBuffer, 0, sizeof(CameraUniform)};
    we::rhi::DescriptorBufferInfo envInfo{m_EnvBuffer, 0, sizeof(SceneEnvironmentUniform)};
    we::rhi::DescriptorBufferInfo cloudInfo{m_CloudBuffer, 0, sizeof(CloudUniform)};
    we::rhi::WriteDescriptorSet writes[3]{};
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
    writes[2].set = m_CloudSet;
    writes[2].binding = 0;
    writes[2].type = we::rhi::DescriptorType::UniformBuffer;
    writes[2].count = 1;
    writes[2].bufferInfos = &cloudInfo;
    m_Device->UpdateDescriptorSets(writes);

    if (!CreateDummyDepth()) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "ViewportCloudRenderer: failed to create dummy depth SRV.");
        Shutdown();
        return false;
    }

    // Bind immutable noise textures as CombinedImageSampler (t1/s1 .. t3/s3).
    // Depth (binding 0) starts as the dummy so the set is never partially null.
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
    we::rhi::DescriptorImageInfo curlInfo{};
    curlInfo.view = m_CurlNoise.view;
    curlInfo.imageLayout = we::rhi::ResourceState::ShaderResource;

    we::rhi::WriteDescriptorSet texWrites[4]{};
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
    texWrites[3].imageInfos = &curlInfo;
    m_Device->UpdateDescriptorSets(texWrites);
    m_DepthBound = true;

    m_Ready = true;
    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        "ViewportCloudRenderer ready (VolumetricClouds / Nubis).");
    return true;
}

void ViewportCloudRenderer::Shutdown() {
    if (!m_Device) {
        m_Ready = false;
        return;
    }

    if (m_Pipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid) {
        (void)m_Device->DestroyGraphicsPipeline(m_Pipeline);
        m_Pipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
    }
    if (m_PipelineLayout != we::rhi::RHIPipelineLayoutHandle::Invalid) {
        (void)m_Device->DestroyPipelineLayout(m_PipelineLayout);
        m_PipelineLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
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
        m_CloudSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        m_ResourceSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    auto destroyLayout = [&](we::rhi::RHIDescriptorSetLayoutHandle& h) {
        if (h != we::rhi::RHIDescriptorSetLayoutHandle::Invalid) {
            (void)m_Device->DestroyDescriptorSetLayout(h);
            h = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
        }
    };
    destroyLayout(m_CameraLayout);
    destroyLayout(m_EnvLayout);
    destroyLayout(m_CloudLayout);
    destroyLayout(m_ResourceLayout);

    auto destroyBuf = [&](we::rhi::RHIBufferHandle& h) {
        if (h != we::rhi::RHIBufferHandle::Invalid) {
            (void)m_Device->DestroyBuffer(h);
            h = we::rhi::RHIBufferHandle::Invalid;
        }
    };
    destroyBuf(m_CameraBuffer);
    destroyBuf(m_EnvBuffer);
    destroyBuf(m_CloudBuffer);

    if (m_Sampler != we::rhi::RHISamplerHandle::Invalid) {
        (void)m_Device->DestroySampler(m_Sampler);
        m_Sampler = we::rhi::RHISamplerHandle::Invalid;
    }

    DestroyLoadedDdsTexture(*m_Device, m_BaseShape);
    DestroyLoadedDdsTexture(*m_Device, m_DetailShape);
    DestroyLoadedDdsTexture(*m_Device, m_CurlNoise);

    if (m_Vs != we::rhi::RHIShaderHandle::Invalid) {
        (void)m_Device->DestroyShader(m_Vs);
        m_Vs = we::rhi::RHIShaderHandle::Invalid;
    }
    if (m_Ps != we::rhi::RHIShaderHandle::Invalid) {
        (void)m_Device->DestroyShader(m_Ps);
        m_Ps = we::rhi::RHIShaderHandle::Invalid;
    }

    m_PipelineColorFormat = we::rhi::Format::Unknown;
    m_Device = nullptr;
    m_Ready = false;
}

bool ViewportCloudRenderer::LoadShaders() {
    const auto format = we::rhi::ShaderBytecodeLoader::ResolveFormat(m_Device);
    m_VsBytes = we::rhi::ShaderBytecodeLoader::Load("VolumetricClouds", "VS", format);
    m_PsBytes = we::rhi::ShaderBytecodeLoader::Load("VolumetricClouds", "PS", format);
    if (m_VsBytes.empty() || m_PsBytes.empty()) {
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
    return true;
}

bool ViewportCloudRenderer::LoadNoiseTextures() {
    auto base = LoadDdsRgba8Texture(
        *m_Device, ResolveCloudAsset("CloudBaseShape128.dds"), "Cloud.BaseShape128");
    auto detail = LoadDdsRgba8Texture(
        *m_Device, ResolveCloudAsset("CloudDetailErosion32.dds"), "Cloud.DetailErosion32");
    auto curl = LoadDdsRgba8Texture(
        *m_Device, ResolveCloudAsset("CloudTurbulenceCurl128.dds"), "Cloud.TurbulenceCurl128");
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
        return false;
    }
    m_BaseShape = *base;
    m_DetailShape = *detail;
    m_CurlNoise = *curl;
    return true;
}

bool ViewportCloudRenderer::EnsurePipeline(we::rhi::Format colorFormat) {
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
    pso.debugName = "ViewportCloud.PSO";
    auto pipeline = m_Device->CreateGraphicsPipeline(pso);
    if (!pipeline) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "ViewportCloudRenderer: CreateGraphicsPipeline failed.");
        return false;
    }
    m_Pipeline = *pipeline;
    m_PipelineColorFormat = colorFormat;
    return true;
}

bool ViewportCloudRenderer::CreateDummyDepth() {
    // 1x1 RGBA8 is the well-tested UpdateTexture path (UI atlases); avoid R32
    // one-offs during bring-up. Depth Load() only needs a valid sampled image.
    we::rhi::TextureDesc desc{};
    desc.extent = {1, 1, 1};
    desc.format = we::rhi::Format::R8G8B8A8_UNORM;
    desc.usage = we::rhi::TextureUsage::Sampled | we::rhi::TextureUsage::TransferDst;
    desc.debugName = "ViewportCloud.DummyDepth";
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
    viewDesc.debugName = "ViewportCloud.DummyDepthSRV";
    auto view = m_Device->CreateTextureView(viewDesc);
    if (!view) {
        (void)m_Device->DestroyTexture(m_DummyDepthTexture);
        m_DummyDepthTexture = we::rhi::RHITextureHandle::Invalid;
        return false;
    }
    m_DummyDepthView = *view;
    return true;
}

void ViewportCloudRenderer::InvalidateDepthBinding() {
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

bool ViewportCloudRenderer::EnsureDepthView(we::rhi::RHITextureHandle depth) {
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
    viewDesc.debugName = "ViewportCloud.DepthSRV";
    auto view = m_Device->CreateTextureView(viewDesc);
    if (!view) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "ViewportCloudRenderer: CreateTextureView(depth) failed.");
        return false;
    }
    m_DepthView = *view;
    m_BoundDepth = depth;
    return true;
}

bool ViewportCloudRenderer::BindResourceDescriptors(we::rhi::RHITextureHandle depth) {
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

void ViewportCloudRenderer::Draw(
    we::rhi::IRHICommandList& cmd,
    we::rhi::RHITextureHandle color,
    we::rhi::RHITextureHandle depth,
    we::rhi::Extent2D extent,
    const CameraUniform& camera,
    const SceneEnvironmentUniform& environment,
    const CloudUniform& clouds)
{
    if (!m_Ready || !m_Device || color == we::rhi::RHITextureHandle::Invalid
        || depth == we::rhi::RHITextureHandle::Invalid
        || extent.width == 0 || extent.height == 0
        || clouds.enabled < 0.5f) {
        return;
    }
    if (!EnsurePipeline(we::rhi::Format::R16G16B16A16_SFLOAT)) {
        return;
    }
    if (!BindResourceDescriptors(depth)) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            "ViewportCloudRenderer: skipping draw — depth SRV bind failed.");
        return;
    }

    (void)m_Device->UpdateBuffer(
        m_CameraBuffer,
        std::span(reinterpret_cast<const uint8_t*>(&camera), sizeof(camera)));
    (void)m_Device->UpdateBuffer(
        m_EnvBuffer,
        std::span(reinterpret_cast<const uint8_t*>(&environment), sizeof(environment)));
    (void)m_Device->UpdateBuffer(
        m_CloudBuffer,
        std::span(reinterpret_cast<const uint8_t*>(&clouds), sizeof(clouds)));

    we::rhi::RenderingInfo info{};
    we::rhi::ColorAttachmentDesc colorAtt{};
    colorAtt.texture = color;
    colorAtt.loadOp = we::rhi::LoadOp::Load;
    colorAtt.storeOp = we::rhi::StoreOp::Store;
    info.colorAttachments.push_back(colorAtt);
    info.renderArea = extent;

    cmd.BeginRendering(info);
    cmd.SetViewport({0, 0,
        static_cast<float>(extent.width),
        static_cast<float>(extent.height),
        0.0f, 1.0f});
    cmd.SetScissor({0, 0, extent.width, extent.height});
    cmd.BindGraphicsPipeline(m_Pipeline);
    const we::rhi::RHIDescriptorSetHandle sets[] = {
        m_CameraSet, m_EnvSet, m_CloudSet, m_ResourceSet};
    cmd.BindDescriptorSets(
        we::rhi::PipelineBindPoint::Graphics,
        m_PipelineLayout,
        0,
        sets);
    cmd.Draw(3);
    cmd.EndRendering();
}

} // namespace we::runtime::renderer
