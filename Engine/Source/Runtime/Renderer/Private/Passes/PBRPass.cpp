#include "Passes/PBRPass.h"
#include "Core/DeviceContext.h"
#include "Core/Validation.h"
#include "Pipeline/GraphicsPipelineFactory.h"
#include "Camera/CameraSystem.h"
#include "Lighting/DirectionalLight.h"
#include "Resource/DepthTarget.h"
#include "Scene/SceneRenderer.h"
#include "Scene/MeshPrimitives.h"

#include <cstddef>
#include <cstdio>
#include <chrono>

namespace {

#ifndef WE_ENABLE_AGENT_DEBUG_LOG
#define WE_ENABLE_AGENT_DEBUG_LOG 0
#endif

#if WE_ENABLE_AGENT_DEBUG_LOG
void AgentDebugLog(const char* hypothesisId, const char* location, const char* message, const char* dataJson) {
    using namespace std::chrono;
    const auto now = time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count();

    std::FILE* f = std::fopen("f:/Coding/windeffects/debug-a0cfa5.log", "a");
    if (!f) return;

    std::fprintf(
        f,
        "{\"sessionId\":\"a0cfa5\",\"id\":\"log_%lld\",\"timestamp\":%lld,"
        "\"runId\":\"pre-fix\",\"hypothesisId\":\"%s\",\"location\":\"%s\","
        "\"message\":\"%s\",\"data\":%s}\n",
        static_cast<long long>(now),
        static_cast<long long>(now),
        hypothesisId,
        location,
        message,
        dataJson ? dataJson : "null");

    std::fclose(f);
}
#else
void AgentDebugLog(const char*, const char*, const char*, const char*) {}
#endif

} // namespace

namespace we::runtime::renderer {

PBRPass::PBRPass() : RenderPass("PBRPass") {}

PBRPass::~PBRPass() {
    Shutdown();
}

void PBRPass::Init(const PBRPassConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "PBRPass", "Already initialized.");
    WE_VALIDATE_INIT(config.deviceContext != nullptr, "PBRPass", "DeviceContext is null.");
    WE_VALIDATE_INIT(config.pipelineFactory != nullptr, "PBRPass", "PipelineFactory is null.");
    WE_VALIDATE_INIT(config.cameraDescriptorSetLayout != VK_NULL_HANDLE, "PBRPass", "Camera descriptor layout is null.");
    WE_VALIDATE_INIT(config.objectDescriptorSetLayout != VK_NULL_HANDLE, "PBRPass", "Object descriptor layout is null.");
    WE_VALIDATE_INIT(config.lightDescriptorSetLayout != VK_NULL_HANDLE, "PBRPass", "Light descriptor layout is null.");
    WE_VALIDATE_INIT(config.colorFormat != VK_FORMAT_UNDEFINED, "PBRPass", "Color format is undefined.");
    WE_VALIDATE_INIT(config.depthFormat != VK_FORMAT_UNDEFINED, "PBRPass", "Depth format is undefined.");

    m_Config = config;

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(MeshVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attributes(3);
    attributes[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MeshVertex, position)};
    attributes[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MeshVertex, normal)};
    attributes[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(MeshVertex, uv)};

    GraphicsPipelineDesc pipelineDesc{};
    pipelineDesc.vertexShaderName = "BasicMesh";
    pipelineDesc.pixelShaderName = "BasicMesh";
    pipelineDesc.descriptorSetLayouts = {
        config.cameraDescriptorSetLayout,
        config.objectDescriptorSetLayout,
        config.lightDescriptorSetLayout
    };
    pipelineDesc.vertexBindings = { binding };
    pipelineDesc.vertexAttributes = attributes;
    pipelineDesc.depthTest = true;
    pipelineDesc.depthWrite = true;
    pipelineDesc.colorFormat = config.colorFormat;
    pipelineDesc.depthFormat = config.depthFormat;

    m_Pipeline = config.pipelineFactory->CreateGraphicsPipeline(pipelineDesc, m_PipelineLayout);
    m_Initialized = true;
}

void PBRPass::Shutdown() {
    if (!m_Initialized) return;

    if (m_Config.pipelineFactory) {
        m_Config.pipelineFactory->DestroyPipeline(m_Pipeline);
        m_Config.pipelineFactory->DestroyPipelineLayout(m_PipelineLayout);
    }

    m_Pipeline = VK_NULL_HANDLE;
    m_PipelineLayout = VK_NULL_HANDLE;
    m_Initialized = false;
}

void PBRPass::Validate() {
    WE_VALIDATE_RENDER(m_Initialized, GetName().c_str(), "Pass not initialized.");
    WE_VALIDATE_RENDER(m_Pipeline != VK_NULL_HANDLE, GetName().c_str(), "Pipeline is invalid.");
    WE_VALIDATE_RENDER(m_PipelineLayout != VK_NULL_HANDLE, GetName().c_str(), "Pipeline layout is invalid.");
}

RenderPassIO PBRPass::DescribePassIO() const {
    RenderPassIO io{};
    io.colorResourceName = "SceneColor";
    io.color.read = true;
    io.color.write = true;
    io.color.requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    io.depthResourceName = "SceneDepth";
    io.depth.read = true;
    io.depth.write = true;
    io.depth.requiredLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    return io;
}

void PBRPass::Execute(const FrameContext& frame) {
    Validate();
    WE_VALIDATE_RENDER(frame.commandBuffer != VK_NULL_HANDLE, GetName().c_str(), "Command buffer is null.");
    WE_VALIDATE_RENDER(frame.camera != nullptr, GetName().c_str(), "Camera system is null.");
    WE_VALIDATE_RENDER(frame.directionalLight != nullptr, GetName().c_str(), "Directional light is null.");
    WE_VALIDATE_RENDER(frame.depthTarget != nullptr, GetName().c_str(), "Depth target is null.");
    WE_VALIDATE_RENDER(frame.sceneRenderer != nullptr, GetName().c_str(), "Scene renderer is null.");

    VkCommandBuffer cmd = frame.commandBuffer;

    char beginJson[256];
    std::snprintf(
        beginJson,
        sizeof(beginJson),
        "{"
        "\"passName\":\"PBRPass\","
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
        "PBRPass.cpp:Execute",
        "PASS_BEGIN",
        beginJson);
    // #endregion

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = frame.colorImageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = frame.depthTarget->GetImageView();
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = frame.sceneRenderArea;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

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

    const VkDescriptorSet sets[3] = {
        frame.camera->GetDescriptorSet(frame.frameIndex),
        frame.sceneRenderer->GetObjectDescriptorSet(frame.frameIndex),
        frame.directionalLight->GetDescriptorSet(frame.frameIndex)
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 3, sets, 0, nullptr);

    frame.sceneRenderer->DrawMeshes(cmd);

    char drawJson[256];
    std::snprintf(
        drawJson,
        sizeof(drawJson),
        "{"
        "\"passName\":\"PBRPass\","
        "\"drawType\":\"vkCmdDrawIndexed\","
        "\"expectsColorWrite\":true,"
        "\"expectsDepthWrite\":true"
        "}");

    // #region agent log
    AgentDebugLog(
        "H5",
        "PBRPass.cpp:Execute",
        "PASS_DRAW",
        drawJson);
    // #endregion

    vkCmdEndRendering(cmd);
}

} // namespace we::runtime::renderer
