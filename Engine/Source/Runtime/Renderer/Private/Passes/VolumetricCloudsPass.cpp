#include "Passes/VolumetricCloudsPass.h"
#include "Core/DeviceContext.h"
#include "Core/Validation.h"
#include "Pipeline/GraphicsPipelineFactory.h"
#include "Camera/CameraSystem.h"
#include "Lighting/EnvironmentUniform.h"

namespace we::runtime::renderer {

VolumetricCloudsPass::VolumetricCloudsPass() : RenderPass("VolumetricCloudsPass") {}

VolumetricCloudsPass::~VolumetricCloudsPass() {
    Shutdown();
}

void VolumetricCloudsPass::Init(const VolumetricCloudsPassConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "VolumetricCloudsPass", "Already initialized.");
    WE_VALIDATE_INIT(config.deviceContext != nullptr, "VolumetricCloudsPass", "DeviceContext is null.");
    WE_VALIDATE_INIT(config.pipelineFactory != nullptr, "VolumetricCloudsPass", "PipelineFactory is null.");
    WE_VALIDATE_INIT(config.cameraDescriptorSetLayout != VK_NULL_HANDLE, "VolumetricCloudsPass", "Camera layout null.");
    WE_VALIDATE_INIT(config.environmentDescriptorSetLayout != VK_NULL_HANDLE, "VolumetricCloudsPass", "Env layout null.");
    WE_VALIDATE_INIT(config.colorFormat != VK_FORMAT_UNDEFINED, "VolumetricCloudsPass", "Color format undefined.");

    m_Config = config;

    GraphicsPipelineDesc pipelineDesc{};
    pipelineDesc.vertexShaderName = "VolumetricCloudsPass";
    pipelineDesc.pixelShaderName = "VolumetricCloudsPass";
    pipelineDesc.descriptorSetLayouts = {
        config.cameraDescriptorSetLayout,
        config.environmentDescriptorSetLayout
    };
    pipelineDesc.cullMode = VK_CULL_MODE_NONE;
    pipelineDesc.depthTest = false;
    pipelineDesc.depthWrite = false;
    pipelineDesc.colorFormat = config.colorFormat;
    pipelineDesc.blendEnable = true;
    pipelineDesc.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipelineDesc.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineDesc.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineDesc.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    m_Pipeline = config.pipelineFactory->CreateGraphicsPipeline(pipelineDesc, m_PipelineLayout);
    m_Initialized = true;
}

void VolumetricCloudsPass::Shutdown() {
    if (!m_Initialized) return;
    if (m_Config.pipelineFactory) {
        m_Config.pipelineFactory->DestroyPipeline(m_Pipeline);
        m_Config.pipelineFactory->DestroyPipelineLayout(m_PipelineLayout);
    }
    m_Pipeline = VK_NULL_HANDLE;
    m_PipelineLayout = VK_NULL_HANDLE;
    m_Initialized = false;
}

void VolumetricCloudsPass::Validate() {
    WE_VALIDATE_RENDER(m_Initialized, GetName().c_str(), "Pass not initialized.");
    WE_VALIDATE_RENDER(m_Pipeline != VK_NULL_HANDLE, GetName().c_str(), "Pipeline invalid.");
}

RenderPassIO VolumetricCloudsPass::DescribePassIO() const {
    RenderPassIO io{};
    io.colorResourceName = "SceneColor";
    io.color.write = true;
    io.color.read = true;
    io.color.requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    io.depthResourceName = "SceneDepth";
    io.depth.read = false;
    io.depth.write = false;
    io.depth.requiredLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    return io;
}

void VolumetricCloudsPass::Execute(const FrameContext& frame) {
    Validate();
    if (!frame.environmentUniform) {
        return;
    }

    WE_VALIDATE_RENDER(frame.commandBuffer != VK_NULL_HANDLE, GetName().c_str(), "Command buffer null.");
    WE_VALIDATE_RENDER(frame.camera != nullptr, GetName().c_str(), "Camera null.");
    WE_VALIDATE_RENDER(frame.colorImageView != VK_NULL_HANDLE, GetName().c_str(), "Color view null.");

    const SceneEnvironmentUniform& env = frame.environmentUniform->GetData(frame.frameIndex);
    if (env.enableClouds < 0.5f) {
        return;
    }

    VkCommandBuffer cmd = frame.commandBuffer;

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = frame.colorImageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = frame.sceneRenderArea;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

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

    const VkDescriptorSet sets[2] = {
        frame.camera->GetDescriptorSet(frame.frameIndex),
        frame.environmentUniform->GetDescriptorSet(frame.frameIndex)
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 2, sets, 0, nullptr);
    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRendering(cmd);
}

} // namespace we::runtime::renderer
