#include "Graph/ViewportGridRenderer.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "RHI/Desc.h"
#include "RHI/ShaderBytecode.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

namespace we::runtime::renderer {
namespace {

// Must match EditorGrid.hlsl GridSettings (std140).
struct GridSettingsUBO {
    float levelSizes[4]{1.f, 10.f, 100.f, 1000.f};
    float levelFadeStart[4]{25.f, 80.f, 400.f, 100000.f};
    float levelFadeEnd[4]{80.f, 400.f, 4000.f, 100000.f};

    float level0Color[4]{0.27f, 0.27f, 0.27f, 0.30f};
    float level1Color[4]{0.31f, 0.31f, 0.31f, 0.36f};
    float level2Color[4]{0.34f, 0.34f, 0.34f, 0.40f};
    float level3Color[4]{0.37f, 0.37f, 0.37f, 0.45f};
    float axisXColor[4]{0.57f, 0.29f, 0.29f, 0.55f};
    float axisZColor[4]{0.29f, 0.57f, 0.29f, 0.55f};

    float renderParams0[4]{512.f, 0.f, 1.f, 1.1f};       // radius, planeY, minor, major
    float renderParams1[4]{1.f, 1.1f, 0.002f, 1.f};      // opacity, axisThick, horizon, aa
    float renderParams2[4]{-2.f, -1.f, 0.f, 0.82f};       // biasC, biasS, camDist, radiusFadeStart
    float snappedOrigin[4]{0.f, 0.f, 0.f, 768.f};        // xyz + cull radius

    int gridFlags[4]{1, 1, 1, 0};                        // enable, axis, aa, pad
    float depthParams[4]{0.002f, 1.f, 50.f, 500.f};      // depthOffset, radiusFadeEnd, distFade*
};

static_assert(sizeof(GridSettingsUBO) % 16 == 0, "GridSettingsUBO must be 16-byte aligned");

} // namespace

ViewportGridRenderer::~ViewportGridRenderer() {
    Shutdown();
}

bool ViewportGridRenderer::Init(we::rhi::IRHIDevice* device) {
    Shutdown();
    m_Device = device;
    if (!m_Device) {
        return false;
    }
    if (!LoadShaders()) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "ViewportGridRenderer: failed to load EditorGrid shaders.");
        Shutdown();
        return false;
    }

    we::rhi::DescriptorSetLayoutDesc gridLayoutDesc{};
    gridLayoutDesc.debugName = "ViewportGrid.GridLayout";
    gridLayoutDesc.bindings.push_back({
        0,
        we::rhi::DescriptorType::UniformBuffer,
        1,
        we::rhi::ShaderStageFlags::Vertex | we::rhi::ShaderStageFlags::Fragment});
    auto gridLayout = m_Device->CreateDescriptorSetLayout(gridLayoutDesc);
    if (!gridLayout) {
        Shutdown();
        return false;
    }
    m_GridLayout = *gridLayout;

    we::rhi::DescriptorSetLayoutDesc cameraLayoutDesc{};
    cameraLayoutDesc.debugName = "ViewportGrid.CameraLayout";
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

    we::rhi::PipelineLayoutDesc pipelineLayoutDesc{};
    pipelineLayoutDesc.debugName = "ViewportGrid.PipelineLayout";
    pipelineLayoutDesc.setLayouts = {m_GridLayout, m_CameraLayout};
    auto pipelineLayout = m_Device->CreatePipelineLayout(pipelineLayoutDesc);
    if (!pipelineLayout) {
        Shutdown();
        return false;
    }
    m_PipelineLayout = *pipelineLayout;

    we::rhi::BufferDesc gridBuf{};
    gridBuf.size = sizeof(GridSettingsUBO);
    gridBuf.usage = we::rhi::BufferUsage::Uniform;
    gridBuf.memory = we::rhi::MemoryUsage::HostVisible;
    gridBuf.debugName = "ViewportGrid.SettingsUBO";
    auto gBuf = m_Device->CreateBuffer(gridBuf);
    if (!gBuf) {
        Shutdown();
        return false;
    }
    m_GridBuffer = *gBuf;

    we::rhi::BufferDesc cameraBuf{};
    cameraBuf.size = sizeof(CameraUniform);
    cameraBuf.usage = we::rhi::BufferUsage::Uniform;
    cameraBuf.memory = we::rhi::MemoryUsage::HostVisible;
    cameraBuf.debugName = "ViewportGrid.CameraUBO";
    auto cBuf = m_Device->CreateBuffer(cameraBuf);
    if (!cBuf) {
        Shutdown();
        return false;
    }
    m_CameraBuffer = *cBuf;

    we::rhi::DescriptorPoolDesc poolDesc{};
    poolDesc.maxSets = 4;
    poolDesc.debugName = "ViewportGrid.Pool";
    poolDesc.poolSizes.push_back({we::rhi::DescriptorType::UniformBuffer, 8});
    auto pool = m_Device->CreateDescriptorPool(poolDesc);
    if (!pool) {
        Shutdown();
        return false;
    }
    m_Pool = *pool;

    we::rhi::DescriptorSetAllocateDesc gridSetDesc{};
    gridSetDesc.pool = m_Pool;
    gridSetDesc.layout = m_GridLayout;
    auto gridSet = m_Device->AllocateDescriptorSet(gridSetDesc);
    if (!gridSet) {
        Shutdown();
        return false;
    }
    m_GridSet = *gridSet;

    we::rhi::DescriptorSetAllocateDesc camSetDesc{};
    camSetDesc.pool = m_Pool;
    camSetDesc.layout = m_CameraLayout;
    auto camSet = m_Device->AllocateDescriptorSet(camSetDesc);
    if (!camSet) {
        Shutdown();
        return false;
    }
    m_CameraSet = *camSet;

    we::rhi::DescriptorBufferInfo gridInfo{m_GridBuffer, 0, sizeof(GridSettingsUBO)};
    we::rhi::DescriptorBufferInfo camInfo{m_CameraBuffer, 0, sizeof(CameraUniform)};
    we::rhi::WriteDescriptorSet writes[2]{};
    writes[0].set = m_GridSet;
    writes[0].binding = 0;
    writes[0].type = we::rhi::DescriptorType::UniformBuffer;
    writes[0].count = 1;
    writes[0].bufferInfos = &gridInfo;
    writes[1].set = m_CameraSet;
    writes[1].binding = 0;
    writes[1].type = we::rhi::DescriptorType::UniformBuffer;
    writes[1].count = 1;
    writes[1].bufferInfos = &camInfo;
    m_Device->UpdateDescriptorSets(writes);

    m_Ready = true;
    WE_LOG_INFO(we::LogCategory::Renderer.data(), "ViewportGridRenderer ready (EditorGrid).");
    return true;
}

void ViewportGridRenderer::Shutdown() {
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
        m_GridSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        m_CameraSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    }
    if (m_GridLayout != we::rhi::RHIDescriptorSetLayoutHandle::Invalid) {
        (void)m_Device->DestroyDescriptorSetLayout(m_GridLayout);
        m_GridLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    }
    if (m_CameraLayout != we::rhi::RHIDescriptorSetLayoutHandle::Invalid) {
        (void)m_Device->DestroyDescriptorSetLayout(m_CameraLayout);
        m_CameraLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    }
    if (m_GridBuffer != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->DestroyBuffer(m_GridBuffer);
        m_GridBuffer = we::rhi::RHIBufferHandle::Invalid;
    }
    if (m_CameraBuffer != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->DestroyBuffer(m_CameraBuffer);
        m_CameraBuffer = we::rhi::RHIBufferHandle::Invalid;
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

bool ViewportGridRenderer::LoadShaders() {
    const auto format = we::rhi::ShaderBytecodeLoader::ResolveFormat(m_Device);
    m_VsBytes = we::rhi::ShaderBytecodeLoader::Load("EditorGrid", "VS", format);
    m_PsBytes = we::rhi::ShaderBytecodeLoader::Load("EditorGrid", "PS", format);
    if (m_VsBytes.empty() || m_PsBytes.empty()) {
        return false;
    }

    we::rhi::ShaderDesc vsDesc{};
    vsDesc.stage = we::rhi::ShaderStage::Vertex;
    vsDesc.format = format;
    vsDesc.bytecode = m_VsBytes;
    vsDesc.entryPoint = "VSMain";
    vsDesc.debugName = "EditorGrid.VS";
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
    psDesc.debugName = "EditorGrid.PS";
    auto ps = m_Device->CreateShader(psDesc);
    if (!ps) {
        return false;
    }
    m_Ps = *ps;
    return true;
}

bool ViewportGridRenderer::EnsurePipeline(
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
    pso.depthTest = true;
    pso.depthWrite = true;
    pso.depthCompare = we::rhi::CompareOp::LessOrEqual;
    pso.blend.enable = true;
    pso.blend.srcColor = we::rhi::BlendFactor::SrcAlpha;
    pso.blend.dstColor = we::rhi::BlendFactor::OneMinusSrcAlpha;
    pso.blend.srcAlpha = we::rhi::BlendFactor::One;
    pso.blend.dstAlpha = we::rhi::BlendFactor::OneMinusSrcAlpha;
    pso.colorFormat = colorFormat;
    pso.depthFormat = depthFormat;
    pso.depthAttachment = hasDepth;
    pso.debugName = "ViewportGrid.PSO";
    auto pipeline = m_Device->CreateGraphicsPipeline(pso);
    if (!pipeline) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            "ViewportGridRenderer: CreateGraphicsPipeline failed.");
        return false;
    }
    m_Pipeline = *pipeline;
    m_PipelineColorFormat = colorFormat;
    m_PipelineDepthFormat = depthFormat;
    m_PipelineHasDepth = hasDepth;
    return true;
}

void ViewportGridRenderer::UploadGridSettings(const CameraUniform& camera) {
    GridSettingsUBO settings{};
#if WE_HAS_GLM
    const float camDist = std::max(glm::length(camera.position), 1.0f);
    settings.renderParams2[2] = camDist;
    settings.snappedOrigin[0] = camera.position.x;
    settings.snappedOrigin[1] = 0.0f;
    settings.snappedOrigin[2] = camera.position.z;
#else
    const float camDist = std::max(
        std::sqrt(camera.position[0] * camera.position[0]
            + camera.position[1] * camera.position[1]
            + camera.position[2] * camera.position[2]),
        1.0f);
    settings.renderParams2[2] = camDist;
    settings.snappedOrigin[0] = camera.position[0];
    settings.snappedOrigin[2] = camera.position[2];
#endif
    (void)m_Device->UpdateBuffer(
        m_GridBuffer,
        std::span(reinterpret_cast<const uint8_t*>(&settings), sizeof(settings)));
}

void ViewportGridRenderer::Draw(
    we::rhi::IRHICommandList& cmd,
    we::rhi::RHITextureHandle color,
    we::rhi::RHITextureHandle depth,
    we::rhi::Extent2D extent,
    const CameraUniform& camera)
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

    UploadGridSettings(camera);
    (void)m_Device->UpdateBuffer(
        m_CameraBuffer,
        std::span(reinterpret_cast<const uint8_t*>(&camera), sizeof(camera)));

    we::rhi::RenderingInfo info{};
    we::rhi::ColorAttachmentDesc colorAtt{};
    colorAtt.texture = color;
    colorAtt.loadOp = we::rhi::LoadOp::Load;
    colorAtt.storeOp = we::rhi::StoreOp::Store;
    info.colorAttachments.push_back(colorAtt);
    if (hasDepth) {
        info.depth.enabled = true;
        info.depth.texture = depth;
        info.depth.loadOp = we::rhi::LoadOp::Load;
        info.depth.storeOp = we::rhi::StoreOp::Store;
    }
    info.renderArea = extent;

    cmd.BeginRendering(info);
    cmd.SetViewport({0, 0,
        static_cast<float>(extent.width),
        static_cast<float>(extent.height),
        0.0f, 1.0f});
    cmd.SetScissor({0, 0, extent.width, extent.height});
    cmd.BindGraphicsPipeline(m_Pipeline);
    const we::rhi::RHIDescriptorSetHandle sets[] = {m_GridSet, m_CameraSet};
    cmd.BindDescriptorSets(
        we::rhi::PipelineBindPoint::Graphics,
        m_PipelineLayout,
        0,
        sets);
    cmd.Draw(3);
    cmd.EndRendering();
}

} // namespace we::runtime::renderer
