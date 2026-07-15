#include "Passes/VolumetricCloudsPass.h"
#include "Core/DeviceContext.h"
#include "Core/Validation.h"
#include "Core/Logger.h"
#include "Core/LogCategory.h"
#include "Core/ImageBarriers.h"
#include "Pipeline/GraphicsPipelineFactory.h"
#include "Camera/CameraSystem.h"
#include "Lighting/EnvironmentUniform.h"
#include "Resource/ResourceManager.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace we::runtime::renderer {
namespace {

float Halton(uint32_t index, uint32_t base) {
    float f = 1.0f;
    float r = 0.0f;
    while (index > 0) {
        f /= static_cast<float>(base);
        r += f * static_cast<float>(index % base);
        index /= base;
    }
    return r;
}

void SetViewportScissor(VkCommandBuffer cmd, VkRect2D area) {
    VkViewport viewport{};
    viewport.x = static_cast<float>(area.offset.x);
    viewport.y = static_cast<float>(area.offset.y);
    viewport.width = static_cast<float>(area.extent.width);
    viewport.height = static_cast<float>(area.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &area);
}

} // namespace

VolumetricCloudsPass::VolumetricCloudsPass() : RenderPass("VolumetricCloudsPass") {}

VolumetricCloudsPass::~VolumetricCloudsPass() {
    Shutdown();
}

void VolumetricCloudsPass::Init(const VolumetricCloudsPassConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "VolumetricCloudsPass", "Already initialized.");
    WE_VALIDATE_INIT(config.deviceContext != nullptr, "VolumetricCloudsPass", "DeviceContext is null.");
    WE_VALIDATE_INIT(config.pipelineFactory != nullptr, "VolumetricCloudsPass", "PipelineFactory is null.");
    WE_VALIDATE_INIT(config.resourceManager != nullptr, "VolumetricCloudsPass", "ResourceManager is null.");
    WE_VALIDATE_INIT(config.cameraDescriptorSetLayout != VK_NULL_HANDLE, "VolumetricCloudsPass", "Camera layout null.");
    WE_VALIDATE_INIT(config.environmentDescriptorSetLayout != VK_NULL_HANDLE, "VolumetricCloudsPass", "Env layout null.");
    WE_VALIDATE_INIT(config.colorFormat != VK_FORMAT_UNDEFINED, "VolumetricCloudsPass", "Color format undefined.");

    m_Config = config;
    m_ResolutionScale = std::clamp(config.resolutionScale, 0.25f, 1.0f);

    CreateCloudDescriptorLayout();

    const std::vector<VkDescriptorSetLayout> layouts = {
        config.cameraDescriptorSetLayout,
        config.environmentDescriptorSetLayout,
        m_CloudSetLayout
    };

    // 1) Half-res raymarch → RGBA16F cloud RT (no blend).
    {
        GraphicsPipelineDesc desc{};
        desc.vertexShaderName = "VolumetricCloudsPass";
        desc.pixelShaderName = "VolumetricCloudsPass";
        desc.descriptorSetLayouts = layouts;
        desc.cullMode = VK_CULL_MODE_NONE;
        desc.depthTest = false;
        desc.depthWrite = false;
        desc.colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        desc.blendEnable = false;
        m_RaymarchPipeline = config.pipelineFactory->CreateGraphicsPipeline(desc, m_RaymarchLayout);
    }

    // 2) Temporal resolve → history (no blend).
    {
        GraphicsPipelineDesc desc{};
        desc.vertexShaderName = "CloudTemporalResolve";
        desc.pixelShaderName = "CloudTemporalResolve";
        desc.descriptorSetLayouts = layouts;
        desc.cullMode = VK_CULL_MODE_NONE;
        desc.depthTest = false;
        desc.depthWrite = false;
        desc.colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        desc.blendEnable = false;
        m_TemporalPipeline = config.pipelineFactory->CreateGraphicsPipeline(desc, m_TemporalLayout);
    }

    // 3) Bilateral upsample + alpha blend onto SceneColor.
    {
        GraphicsPipelineDesc desc{};
        desc.vertexShaderName = "CloudCompositePass";
        desc.pixelShaderName = "CloudCompositePass";
        desc.descriptorSetLayouts = layouts;
        desc.cullMode = VK_CULL_MODE_NONE;
        desc.depthTest = false;
        desc.depthWrite = false;
        desc.colorFormat = config.colorFormat;
        desc.blendEnable = true;
        desc.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        desc.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        desc.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        desc.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        m_CompositePipeline = config.pipelineFactory->CreateGraphicsPipeline(desc, m_CompositeLayout);
    }

    for (uint32_t i = 0; i < 2; ++i) {
        config.resourceManager->CreateBuffer(
            sizeof(CloudFrameUniformData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_CloudFrameBuffers[i],
            m_CloudFrameMemories[i]);
    }

    CreateCloudDescriptorSets();

    CloudRenderTargetsConfig targetsConfig{};
    targetsConfig.deviceContext = config.deviceContext;
    targetsConfig.resourceManager = config.resourceManager;
    targetsConfig.fullWidth = 64;
    targetsConfig.fullHeight = 64;
    targetsConfig.resolutionScale = m_ResolutionScale;
    m_Targets.Init(targetsConfig);

    m_Initialized = true;
    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        "[Clouds] VolumetricCloudsPass initialized (half-res raymarch + temporal + bilateral composite).");
}

void VolumetricCloudsPass::Shutdown() {
    if (!m_Initialized && (m_RaymarchPipeline == VK_NULL_HANDLE)) {
        return;
    }

    m_Targets.Shutdown();

    if (m_Config.deviceContext) {
        VkDevice device = m_Config.deviceContext->GetDevice();
        if (m_CloudPool) {
            vkDestroyDescriptorPool(device, m_CloudPool, nullptr);
            m_CloudPool = VK_NULL_HANDLE;
        }
        if (m_CloudSetLayout) {
            vkDestroyDescriptorSetLayout(device, m_CloudSetLayout, nullptr);
            m_CloudSetLayout = VK_NULL_HANDLE;
        }
        for (uint32_t i = 0; i < 2; ++i) {
            if (m_CloudFrameBuffers[i]) {
                vkDestroyBuffer(device, m_CloudFrameBuffers[i], nullptr);
                m_CloudFrameBuffers[i] = VK_NULL_HANDLE;
            }
            if (m_CloudFrameMemories[i]) {
                vkFreeMemory(device, m_CloudFrameMemories[i], nullptr);
                m_CloudFrameMemories[i] = VK_NULL_HANDLE;
            }
            m_CloudSets[i] = VK_NULL_HANDLE;
        }
    }

    if (m_Config.pipelineFactory) {
        m_Config.pipelineFactory->DestroyPipeline(m_RaymarchPipeline);
        m_Config.pipelineFactory->DestroyPipelineLayout(m_RaymarchLayout);
        m_Config.pipelineFactory->DestroyPipeline(m_TemporalPipeline);
        m_Config.pipelineFactory->DestroyPipelineLayout(m_TemporalLayout);
        m_Config.pipelineFactory->DestroyPipeline(m_CompositePipeline);
        m_Config.pipelineFactory->DestroyPipelineLayout(m_CompositeLayout);
    }

    m_RaymarchPipeline = VK_NULL_HANDLE;
    m_RaymarchLayout = VK_NULL_HANDLE;
    m_TemporalPipeline = VK_NULL_HANDLE;
    m_TemporalLayout = VK_NULL_HANDLE;
    m_CompositePipeline = VK_NULL_HANDLE;
    m_CompositeLayout = VK_NULL_HANDLE;
    m_HasHistory = false;
    m_Initialized = false;
}

void VolumetricCloudsPass::CreateCloudDescriptorLayout() {
    VkDescriptorSetLayoutBinding bindings[5]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 5;
    layoutInfo.pBindings = bindings;
    WE_VALIDATE_INIT(
        vkCreateDescriptorSetLayout(m_Config.deviceContext->GetDevice(), &layoutInfo, nullptr, &m_CloudSetLayout) == VK_SUCCESS,
        "VolumetricCloudsPass",
        "Failed to create cloud descriptor layout.");
}

void VolumetricCloudsPass::CreateCloudDescriptorSets() {
    VkDescriptorPoolSize poolSizes[3]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 2;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    poolSizes[1].descriptorCount = 6; // 2 sets * (color+history+depth)
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    poolSizes[2].descriptorCount = 2;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 3;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 2;
    WE_VALIDATE_INIT(
        vkCreateDescriptorPool(m_Config.deviceContext->GetDevice(), &poolInfo, nullptr, &m_CloudPool) == VK_SUCCESS,
        "VolumetricCloudsPass",
        "Failed to create cloud descriptor pool.");

    VkDescriptorSetLayout layouts[2] = { m_CloudSetLayout, m_CloudSetLayout };
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_CloudPool;
    allocInfo.descriptorSetCount = 2;
    allocInfo.pSetLayouts = layouts;
    WE_VALIDATE_INIT(
        vkAllocateDescriptorSets(m_Config.deviceContext->GetDevice(), &allocInfo, m_CloudSets) == VK_SUCCESS,
        "VolumetricCloudsPass",
        "Failed to allocate cloud descriptor sets.");

    for (uint32_t i = 0; i < 2; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_CloudFrameBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CloudFrameUniformData);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_CloudSets[i];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;
        vkUpdateDescriptorSets(m_Config.deviceContext->GetDevice(), 1, &write, 0, nullptr);
    }
}

void VolumetricCloudsPass::UpdateCloudDescriptors(
    VkDescriptorSet set,
    VkImageView image1,
    VkImageView image2,
    VkImageView depthView) {
    VkDescriptorImageInfo imageInfos[3]{};
    imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfos[0].imageView = image1;
    imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfos[1].imageView = image2;
    imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    imageInfos[2].imageView = (depthView != VK_NULL_HANDLE) ? depthView : image1;

    VkDescriptorImageInfo samplerInfo{};
    samplerInfo.sampler = m_Targets.Sampler();

    VkWriteDescriptorSet writes[4]{};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = set;
    writes[0].dstBinding = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &imageInfos[0];

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = set;
    writes[1].dstBinding = 2;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &imageInfos[1];

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = set;
    writes[2].dstBinding = 3;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    writes[2].descriptorCount = 1;
    writes[2].pImageInfo = &samplerInfo;

    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = set;
    writes[3].dstBinding = 4;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writes[3].descriptorCount = 1;
    writes[3].pImageInfo = &imageInfos[2];

    vkUpdateDescriptorSets(m_Config.deviceContext->GetDevice(), 4, writes, 0, nullptr);
}

void VolumetricCloudsPass::EnsureTargets(uint32_t fullWidth, uint32_t fullHeight) {
    const uint32_t prevW = m_Targets.CloudWidth();
    const uint32_t prevH = m_Targets.CloudHeight();
    m_Targets.Resize(fullWidth, fullHeight, m_ResolutionScale);
    if (m_Targets.CloudWidth() != prevW || m_Targets.CloudHeight() != prevH) {
        m_HasHistory = false;
    }
}

void VolumetricCloudsPass::UploadCloudFrame(uint32_t frameIndex, const FrameContext& frame, bool historyValid) {
    CloudFrameUniformData data{};
#if WE_HAS_GLM
    data.prevViewProj = m_PrevViewProj;
    data.prevCameraPos = m_PrevCameraPos;
    data.historyValid = historyValid ? 1.0f : 0.0f;
    data.invFullResolution = glm::vec2(
        1.0f / std::max(1u, frame.sceneRenderArea.extent.width),
        1.0f / std::max(1u, frame.sceneRenderArea.extent.height));
    data.invCloudResolution = glm::vec2(
        1.0f / std::max(1u, m_Targets.CloudWidth()),
        1.0f / std::max(1u, m_Targets.CloudHeight()));
    const float jx = Halton(m_FrameCounter % 8u + 1u, 2u) - 0.5f;
    const float jy = Halton(m_FrameCounter % 8u + 1u, 3u) - 0.5f;
    data.temporalJitter = glm::vec2(jx, jy);
    data.resolutionScale = m_ResolutionScale;
    data.maxMarchDistance = 28000.0f;
    data.temporalBlend = 0.85f;
    data.emptySkipThreshold = 0.035f;
    data.frameCounter = m_FrameCounter;
    data.domainRadius = 14000.0f;
#endif

    void* mapped = nullptr;
    const uint32_t slot = frameIndex & 1u;
    WE_VALIDATE_RENDER(
        vkMapMemory(m_Config.deviceContext->GetDevice(), m_CloudFrameMemories[slot], 0, sizeof(data), 0, &mapped) == VK_SUCCESS,
        "VolumetricCloudsPass",
        "Failed to map cloud frame UBO.");
    std::memcpy(mapped, &data, sizeof(data));
    vkUnmapMemory(m_Config.deviceContext->GetDevice(), m_CloudFrameMemories[slot]);
}

void VolumetricCloudsPass::Validate() {
    WE_VALIDATE_RENDER(m_Initialized, GetName().c_str(), "Pass not initialized.");
    WE_VALIDATE_RENDER((m_RaymarchPipeline != VK_NULL_HANDLE), GetName().c_str(), "Raymarch pipeline invalid.");
    WE_VALIDATE_RENDER((m_TemporalPipeline != VK_NULL_HANDLE), GetName().c_str(), "Temporal pipeline invalid.");
    WE_VALIDATE_RENDER((m_CompositePipeline != VK_NULL_HANDLE), GetName().c_str(), "Composite pipeline invalid.");
}

RenderPassIO VolumetricCloudsPass::DescribePassIO() const {
    RenderPassIO io{};
    io.colorResourceName = "SceneColor";
    io.color.write = true;
    io.color.read = true;
    io.color.requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    io.depthResourceName = "SceneDepth";
    io.depth.read = true;
    io.depth.write = false;
    io.depth.requiredLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    return io;
}

void VolumetricCloudsPass::Execute(const FrameContext& frame) {
    Validate();
    if (!frame.environmentUniform || !frame.camera) {
        return;
    }

    const SceneEnvironmentUniform& env = frame.environmentUniform->GetData(frame.frameIndex);
    if (env.enableClouds < 0.5f) {
        static bool s_LoggedDisabled = false;
        if (!s_LoggedDisabled) {
            WE_LOG_INFO(we::LogCategory::Renderer.data(),
                "[Clouds] Pass skipped: enableClouds=0 (actor disabled or missing EntityId).");
            s_LoggedDisabled = true;
        }
        m_HasHistory = false;
        return;
    }

    EnsureTargets(frame.sceneRenderArea.extent.width, frame.sceneRenderArea.extent.height);

    const uint32_t historyRead = m_HistoryWrite ^ 1u;
    const uint32_t uboSlot = frame.frameIndex & 1u;
    UploadCloudFrame(frame.frameIndex, frame, m_HasHistory);

    const bool hasSceneDepth = frame.depthTarget != nullptr && frame.depthTarget->GetImageView() != VK_NULL_HANDLE;
    VkImageView depthView = hasSceneDepth
        ? frame.depthTarget->GetImageView()
        : m_Targets.FallbackDepthView();

    if (!hasSceneDepth && m_Targets.FallbackDepthImage() != VK_NULL_HANDLE) {
        if (m_Targets.FallbackDepthLayout() != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
            TransitionDepthImageLayout(
                frame.commandBuffer,
                m_Targets.FallbackDepthImage(),
                m_Targets.FallbackDepthLayout(),
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
            m_Targets.FallbackDepthLayout() = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        }
    }

    // Seed image bindings for raymarch (UBO-only stage still needs valid image views).
    UpdateCloudDescriptors(
        m_CloudSets[uboSlot],
        m_Targets.ScratchView(),
        m_Targets.HistoryView(historyRead),
        depthView);

    VkCommandBuffer cmd = frame.commandBuffer;
    const VkRect2D cloudArea{{0, 0}, {m_Targets.CloudWidth(), m_Targets.CloudHeight()}};

    const VkDescriptorSet sets[3] = {
        frame.camera->GetDescriptorSet(frame.frameIndex),
        frame.environmentUniform->GetDescriptorSet(frame.frameIndex),
        m_CloudSets[uboSlot]
    };

    // --- Stage 1: raymarch into scratch ---
    TransitionImageLayout(
        cmd,
        m_Targets.ScratchImage(),
        m_Targets.ScratchLayout(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        m_Targets.ScratchLayout() == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : VK_ACCESS_SHADER_READ_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        m_Targets.ScratchLayout() == VK_IMAGE_LAYOUT_UNDEFINED
            ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
            : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    m_Targets.ScratchLayout() = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    {
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = m_Targets.ScratchView();
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = cloudArea;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(cmd, &renderingInfo);
        SetViewportScissor(cmd, cloudArea);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_RaymarchPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_RaymarchLayout, 0, 3, sets, 0, nullptr);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);
    }

    TransitionImageLayout(
        cmd,
        m_Targets.ScratchImage(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    m_Targets.ScratchLayout() = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // History read for temporal.
    if (m_Targets.HistoryLayout(historyRead) != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        TransitionImageLayout(
            cmd,
            m_Targets.HistoryImage(historyRead),
            m_Targets.HistoryLayout(historyRead),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            0,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        m_Targets.HistoryLayout(historyRead) = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    TransitionImageLayout(
        cmd,
        m_Targets.HistoryImage(m_HistoryWrite),
        m_Targets.HistoryLayout(m_HistoryWrite),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        m_Targets.HistoryLayout(m_HistoryWrite) == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            ? VK_ACCESS_SHADER_READ_BIT
            : 0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        m_Targets.HistoryLayout(m_HistoryWrite) == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
            : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    m_Targets.HistoryLayout(m_HistoryWrite) = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // --- Stage 2: temporal resolve → history write ---
    UpdateCloudDescriptors(
        m_CloudSets[uboSlot],
        m_Targets.ScratchView(),
        m_Targets.HistoryView(historyRead),
        depthView);

    {
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = m_Targets.HistoryView(m_HistoryWrite);
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = cloudArea;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(cmd, &renderingInfo);
        SetViewportScissor(cmd, cloudArea);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TemporalPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TemporalLayout, 0, 3, sets, 0, nullptr);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);
    }

    TransitionImageLayout(
        cmd,
        m_Targets.HistoryImage(m_HistoryWrite),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    m_Targets.HistoryLayout(m_HistoryWrite) = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // --- Stage 3: bilateral upsample + composite onto SceneColor ---
    UpdateCloudDescriptors(
        m_CloudSets[uboSlot],
        m_Targets.HistoryView(m_HistoryWrite),
        m_Targets.HistoryView(m_HistoryWrite),
        depthView);

    {
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
        SetViewportScissor(cmd, frame.sceneRenderArea);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CompositePipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CompositeLayout, 0, 3, sets, 0, nullptr);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);
    }

#if WE_HAS_GLM
    const CameraUniform& cam = frame.camera->GetLastUploaded(frame.frameIndex);
    m_PrevViewProj = cam.proj * cam.view;
    m_PrevCameraPos = cam.position;
#endif
    m_HasHistory = true;
    m_HistoryWrite ^= 1u;
    ++m_FrameCounter;

    {
        static int s_DrawLog = 0;
        if (s_DrawLog < 3 || (s_DrawLog % 120) == 0) {
            WE_LOG_INFO(
                we::LogCategory::Renderer.data(),
                std::string("[Clouds] Pass execute halfRes=")
                + std::to_string(m_Targets.CloudWidth()) + "x" + std::to_string(m_Targets.CloudHeight())
                + " scale=" + std::to_string(m_ResolutionScale)
                + " coverage=" + std::to_string(env.cloudCoverage)
                + " shape=" + std::to_string(env.cloudShapeNoise)
                + " history=" + (m_HasHistory ? "1" : "0")
                + " debug=" + std::to_string(env.atmosphereDebugMode));
        }
        ++s_DrawLog;
    }
}

} // namespace we::runtime::renderer
