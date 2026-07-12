#include "Passes/SkyPass.h"
#include "Core/DeviceContext.h"
#include "Core/Validation.h"
#include "Core/AgentDebugLog.h"
#include "Pipeline/GraphicsPipelineFactory.h"
#include "Camera/CameraSystem.h"

namespace we::runtime::renderer {

SkyPass::SkyPass() : RenderPass("SkyPass") {}

SkyPass::~SkyPass() {
    Shutdown();
}

void SkyPass::Init(const SkyPassConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "SkyPass", "Already initialized.");
    WE_VALIDATE_INIT(config.deviceContext != nullptr, "SkyPass", "DeviceContext is null.");
    WE_VALIDATE_INIT(config.pipelineFactory != nullptr, "SkyPass", "PipelineFactory is null.");
    WE_VALIDATE_INIT(config.cameraDescriptorSetLayout != VK_NULL_HANDLE, "SkyPass", "Camera descriptor layout is null.");
    WE_VALIDATE_INIT(config.colorFormat != VK_FORMAT_UNDEFINED, "SkyPass", "Color format is undefined.");

    m_Config = config;

    GraphicsPipelineDesc pipelineDesc{};
    pipelineDesc.vertexShaderName = "ProceduralSky";
    pipelineDesc.pixelShaderName = "ProceduralSky";
    pipelineDesc.descriptorSetLayouts = { config.cameraDescriptorSetLayout };
    pipelineDesc.cullMode = VK_CULL_MODE_NONE;
    pipelineDesc.depthTest = false;
    pipelineDesc.depthWrite = false;
    pipelineDesc.colorFormat = config.colorFormat;

    m_Pipeline = config.pipelineFactory->CreateGraphicsPipeline(pipelineDesc, m_PipelineLayout);
    m_Initialized = true;
}

void SkyPass::Shutdown() {
    if (!m_Initialized) return;

    if (m_Config.pipelineFactory) {
        m_Config.pipelineFactory->DestroyPipeline(m_Pipeline);
        m_Config.pipelineFactory->DestroyPipelineLayout(m_PipelineLayout);
    }

    m_Pipeline = VK_NULL_HANDLE;
    m_PipelineLayout = VK_NULL_HANDLE;
    m_Initialized = false;
}

void SkyPass::Validate() {
    WE_VALIDATE_RENDER(m_Initialized, GetName().c_str(), "Pass not initialized.");
    WE_VALIDATE_RENDER(m_Pipeline != VK_NULL_HANDLE, GetName().c_str(), "Pipeline is invalid.");
    WE_VALIDATE_RENDER(m_PipelineLayout != VK_NULL_HANDLE, GetName().c_str(), "Pipeline layout is invalid.");
}

RenderPassIO SkyPass::DescribePassIO() const {
    RenderPassIO io{};
    io.colorResourceName = "SceneColor";
    io.color.write = true;
    io.color.requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    io.depthResourceName = "SceneDepth";
    io.depth.read = false;
    io.depth.write = false;
    io.depth.requiredLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    return io;
}

void SkyPass::Execute(const FrameContext& frame) {
    Validate();
    WE_VALIDATE_RENDER(frame.commandBuffer != VK_NULL_HANDLE, GetName().c_str(), "Command buffer is null.");
    WE_VALIDATE_RENDER(frame.camera != nullptr, GetName().c_str(), "Camera system is null.");
    WE_VALIDATE_RENDER(frame.colorImageView != VK_NULL_HANDLE, GetName().c_str(), "Color image view is null.");

    VkCommandBuffer cmd = frame.commandBuffer;

    char beginJson[256];
    std::snprintf(
        beginJson,
        sizeof(beginJson),
        "{"
        "\"passName\":\"SkyPass\","
        "\"frameIndex\":%u,"
        "\"imageIndex\":%u,"
        "\"pipeline\":%llu,"
        "\"pipelineLayout\":%llu,"
        "\"renderArea\":{\"x\":%d,\"y\":%d,\"w\":%u,\"h\":%u}"
        "}",
        frame.frameIndex,
        frame.imageIndex,
        static_cast<unsigned long long>(reinterpret_cast<uint64_t>(m_Pipeline)),
        static_cast<unsigned long long>(reinterpret_cast<uint64_t>(m_PipelineLayout)),
        frame.sceneRenderArea.offset.x,
        frame.sceneRenderArea.offset.y,
        frame.sceneRenderArea.extent.width,
        frame.sceneRenderArea.extent.height);

    // #region agent log
    AgentDebugLog(
        "H2",
        "SkyPass.cpp:Execute",
        "PASS_BEGIN",
        beginJson);
    // #endregion

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = frame.colorImageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.02f, 0.04f, 0.08f, 1.0f}};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = frame.sceneRenderArea;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = nullptr;

    vkCmdBeginRendering(cmd, &renderingInfo);

    VkViewport viewport{};
    viewport.x = static_cast<float>(frame.sceneRenderArea.offset.x);
    viewport.y = static_cast<float>(frame.sceneRenderArea.offset.y);
    viewport.width = static_cast<float>(frame.sceneRenderArea.extent.width);
    viewport.height = static_cast<float>(frame.sceneRenderArea.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = frame.sceneRenderArea;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    const VkDescriptorSet cameraSet = frame.camera->GetDescriptorSet(frame.frameIndex);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &cameraSet, 0, nullptr);

    vkCmdDraw(cmd, 3, 1, 0, 0);

    char drawJson[256];
    std::snprintf(
        drawJson,
        sizeof(drawJson),
        "{"
        "\"passName\":\"SkyPass\","
        "\"drawType\":\"vkCmdDraw\","
        "\"vertexCount\":3,"
        "\"instanceCount\":1,"
        "\"expectsColorWrite\":true,"
        "\"expectsDepthWrite\":false"
        "}");

    // #region agent log
    AgentDebugLog(
        "H2",
        "SkyPass.cpp:Execute",
        "PASS_DRAW",
        drawJson);
    // #endregion
    vkCmdEndRendering(cmd);
}

} // namespace we::runtime::renderer
