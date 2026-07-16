#include "Graph/ViewportSkyRenderer.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "RHI/Desc.h"
#include "RHI/ShaderBytecode.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace we::runtime::renderer {

ViewportSkyRenderer::~ViewportSkyRenderer() {
    Shutdown();
}

bool ViewportSkyRenderer::Init(we::rhi::IRHIDevice* device) {
    Shutdown();
    m_Device = device;
    if (!m_Device) {
        return false;
    }
    if (!LoadShaders()) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "ViewportSkyRenderer: failed to load ProceduralSky shaders.");
        Shutdown();
        return false;
    }

    we::rhi::DescriptorSetLayoutDesc cameraLayoutDesc{};
    cameraLayoutDesc.debugName = "ViewportSky.CameraLayout";
    cameraLayoutDesc.bindings.push_back({
        0,
        we::rhi::DescriptorType::UniformBuffer,
        1,
        we::rhi::ShaderStageFlags::Vertex | we::rhi::ShaderStageFlags::Fragment});
    auto cameraLayout = m_Device->CreateDescriptorSetLayout(cameraLayoutDesc);
    if (!cameraLayout) {
        Shutdown();
        return false;
    }
    m_CameraLayout = *cameraLayout;

    we::rhi::DescriptorSetLayoutDesc envLayoutDesc{};
    envLayoutDesc.debugName = "ViewportSky.EnvLayout";
    envLayoutDesc.bindings.push_back({
        0,
        we::rhi::DescriptorType::UniformBuffer,
        1,
        we::rhi::ShaderStageFlags::Vertex | we::rhi::ShaderStageFlags::Fragment});
    auto envLayout = m_Device->CreateDescriptorSetLayout(envLayoutDesc);
    if (!envLayout) {
        Shutdown();
        return false;
    }
    m_EnvLayout = *envLayout;

    we::rhi::PipelineLayoutDesc pipelineLayoutDesc{};
    pipelineLayoutDesc.debugName = "ViewportSky.PipelineLayout";
    pipelineLayoutDesc.setLayouts = {m_CameraLayout, m_EnvLayout};
    auto pipelineLayout = m_Device->CreatePipelineLayout(pipelineLayoutDesc);
    if (!pipelineLayout) {
        Shutdown();
        return false;
    }
    m_PipelineLayout = *pipelineLayout;

    we::rhi::BufferDesc cameraBuf{};
    cameraBuf.size = sizeof(CameraUniform);
    cameraBuf.usage = we::rhi::BufferUsage::Uniform;
    cameraBuf.memory = we::rhi::MemoryUsage::HostVisible;
    cameraBuf.debugName = "ViewportSky.CameraUBO";
    auto camBuf = m_Device->CreateBuffer(cameraBuf);
    if (!camBuf) {
        Shutdown();
        return false;
    }
    m_CameraBuffer = *camBuf;

    we::rhi::BufferDesc envBuf{};
    envBuf.size = sizeof(SceneEnvironmentUniform);
    envBuf.usage = we::rhi::BufferUsage::Uniform;
    envBuf.memory = we::rhi::MemoryUsage::HostVisible;
    envBuf.debugName = "ViewportSky.EnvUBO";
    auto environmentBuf = m_Device->CreateBuffer(envBuf);
    if (!environmentBuf) {
        Shutdown();
        return false;
    }
    m_EnvBuffer = *environmentBuf;

    we::rhi::DescriptorPoolDesc poolDesc{};
    poolDesc.maxSets = 4;
    poolDesc.debugName = "ViewportSky.Pool";
    poolDesc.poolSizes.push_back({we::rhi::DescriptorType::UniformBuffer, 8});
    auto pool = m_Device->CreateDescriptorPool(poolDesc);
    if (!pool) {
        Shutdown();
        return false;
    }
    m_Pool = *pool;

    we::rhi::DescriptorSetAllocateDesc camSetDesc{};
    camSetDesc.pool = m_Pool;
    camSetDesc.layout = m_CameraLayout;
    auto camSet = m_Device->AllocateDescriptorSet(camSetDesc);
    if (!camSet) {
        Shutdown();
        return false;
    }
    m_CameraSet = *camSet;

    we::rhi::DescriptorSetAllocateDesc envSetDesc{};
    envSetDesc.pool = m_Pool;
    envSetDesc.layout = m_EnvLayout;
    auto envSet = m_Device->AllocateDescriptorSet(envSetDesc);
    if (!envSet) {
        Shutdown();
        return false;
    }
    m_EnvSet = *envSet;

    we::rhi::DescriptorBufferInfo camInfo{m_CameraBuffer, 0, sizeof(CameraUniform)};
    we::rhi::DescriptorBufferInfo envInfo{m_EnvBuffer, 0, sizeof(SceneEnvironmentUniform)};
    we::rhi::WriteDescriptorSet writes[2]{};
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
    m_Device->UpdateDescriptorSets(writes);

    m_Ready = true;
    WE_LOG_INFO(we::LogCategory::Renderer.data(), "ViewportSkyRenderer ready (ProceduralSky).");
    return true;
}

void ViewportSkyRenderer::Shutdown() {
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
    if (m_Pool != we::rhi::RHIDescriptorPoolHandle::Invalid) {
        (void)m_Device->DestroyDescriptorPool(m_Pool);
        m_Pool = we::rhi::RHIDescriptorPoolHandle::Invalid;
        m_CameraSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        m_EnvSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    }
    if (m_CameraLayout != we::rhi::RHIDescriptorSetLayoutHandle::Invalid) {
        (void)m_Device->DestroyDescriptorSetLayout(m_CameraLayout);
        m_CameraLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    }
    if (m_EnvLayout != we::rhi::RHIDescriptorSetLayoutHandle::Invalid) {
        (void)m_Device->DestroyDescriptorSetLayout(m_EnvLayout);
        m_EnvLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    }
    if (m_CameraBuffer != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->DestroyBuffer(m_CameraBuffer);
        m_CameraBuffer = we::rhi::RHIBufferHandle::Invalid;
    }
    if (m_EnvBuffer != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->DestroyBuffer(m_EnvBuffer);
        m_EnvBuffer = we::rhi::RHIBufferHandle::Invalid;
    }
    if (m_Vs != we::rhi::RHIShaderHandle::Invalid) {
        (void)m_Device->DestroyShader(m_Vs);
        m_Vs = we::rhi::RHIShaderHandle::Invalid;
    }
    if (m_Ps != we::rhi::RHIShaderHandle::Invalid) {
        (void)m_Device->DestroyShader(m_Ps);
        m_Ps = we::rhi::RHIShaderHandle::Invalid;
    }

    m_PipelineColorFormat = we::rhi::Format::Unknown;
    m_PipelineDepthFormat = we::rhi::Format::Unknown;
    m_PipelineHasDepth = false;
    m_Device = nullptr;
    m_Ready = false;
}

bool ViewportSkyRenderer::LoadShaders() {
    const auto format = we::rhi::ShaderBytecodeLoader::ResolveFormat(m_Device);
    m_VsBytes = we::rhi::ShaderBytecodeLoader::Load("ProceduralSky", "VS", format);
    m_PsBytes = we::rhi::ShaderBytecodeLoader::Load("ProceduralSky", "PS", format);
    if (m_VsBytes.empty() || m_PsBytes.empty()) {
        return false;
    }

    we::rhi::ShaderDesc vsDesc{};
    vsDesc.stage = we::rhi::ShaderStage::Vertex;
    vsDesc.format = format;
    vsDesc.bytecode = m_VsBytes;
    vsDesc.entryPoint = "VSMain";
    vsDesc.debugName = "ProceduralSky.VS";
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
    psDesc.debugName = "ProceduralSky.PS";
    auto ps = m_Device->CreateShader(psDesc);
    if (!ps) {
        return false;
    }
    m_Ps = *ps;
    return true;
}

bool ViewportSkyRenderer::EnsurePipeline(
    we::rhi::Format colorFormat,
    we::rhi::Format depthFormat,
    bool hasDepth)
{
    if (m_Pipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid
        && m_PipelineColorFormat == colorFormat
        && m_PipelineDepthFormat == depthFormat
        && m_PipelineHasDepth == hasDepth) {
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
    pso.depthFormat = depthFormat;
    pso.depthAttachment = hasDepth;
    pso.debugName = "ViewportSky.PSO";
    auto pipeline = m_Device->CreateGraphicsPipeline(pso);
    if (!pipeline) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "ViewportSkyRenderer: CreateGraphicsPipeline failed.");
        return false;
    }
    m_Pipeline = *pipeline;
    m_PipelineColorFormat = colorFormat;
    m_PipelineDepthFormat = depthFormat;
    m_PipelineHasDepth = hasDepth;
    return true;
}

void ViewportSkyRenderer::UploadFrameUniforms(
    const CameraUniform& camera,
    const SceneEnvironmentUniform& environment)
{
    if (!m_Ready || !m_Device) {
        return;
    }
    if (m_CameraBuffer != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->UpdateBuffer(
            m_CameraBuffer,
            std::span(reinterpret_cast<const uint8_t*>(&camera), sizeof(camera)));
    }
    if (m_EnvBuffer != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->UpdateBuffer(
            m_EnvBuffer,
            std::span(reinterpret_cast<const uint8_t*>(&environment), sizeof(environment)));
    }
}

void ViewportSkyRenderer::Draw(
    we::rhi::IRHICommandList& cmd,
    we::rhi::RHITextureHandle color,
    we::rhi::RHITextureHandle depth,
    we::rhi::Extent2D extent,
    const CameraUniform& camera,
    const SceneEnvironmentUniform& environment)
{
    if (!m_Ready || !m_Device || color == we::rhi::RHITextureHandle::Invalid
        || extent.width == 0 || extent.height == 0) {
        return;
    }

    const bool hasDepth = depth != we::rhi::RHITextureHandle::Invalid;
    if (!EnsurePipeline(
            we::rhi::Format::R16G16B16A16_SFLOAT,
            we::rhi::Format::D32_SFLOAT,
            hasDepth)) {
        return;
    }

    UploadFrameUniforms(camera, environment);

    we::rhi::RenderingInfo info{};
    we::rhi::ColorAttachmentDesc colorAtt{};
    colorAtt.texture = color;
    colorAtt.loadOp = we::rhi::LoadOp::Clear;
    colorAtt.storeOp = we::rhi::StoreOp::Store;
    colorAtt.clearColor = {0.12f, 0.14f, 0.18f, 1.0f};
    info.colorAttachments.push_back(colorAtt);
    if (hasDepth) {
        info.depth.enabled = true;
        info.depth.texture = depth;
        info.depth.loadOp = we::rhi::LoadOp::Clear;
        info.depth.storeOp = we::rhi::StoreOp::Store;
        info.depth.clearDepth = 1.0f;
    }
    info.renderArea = extent;

    cmd.BeginRendering(info);
    cmd.SetViewport({0, 0,
        static_cast<float>(extent.width),
        static_cast<float>(extent.height),
        0.0f, 1.0f});
    cmd.SetScissor({0, 0, extent.width, extent.height});
    cmd.BindGraphicsPipeline(m_Pipeline);
    const we::rhi::RHIDescriptorSetHandle sets[] = {m_CameraSet, m_EnvSet};
    cmd.BindDescriptorSets(
        we::rhi::PipelineBindPoint::Graphics,
        m_PipelineLayout,
        0,
        sets);
    cmd.Draw(3);
    cmd.EndRendering();
}

} // namespace we::runtime::renderer
