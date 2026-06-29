#include "Renderer/GridRenderer.hpp"
#include "Renderer/ShaderHelper.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace we::runtime::renderer {

namespace {

constexpr float kLodSizes[] = { 0.01f, 0.1f, 1.0f, 10.0f, 100.0f, 1000.0f };
constexpr int kLodCount = static_cast<int>(sizeof(kLodSizes) / sizeof(kLodSizes[0]));

// Target minor-line width in pixels at the look-at distance (tuned for UE5-like 1 m at ~15 m orbit).
constexpr float kTargetPixelsPerCell = 56.0f;

struct GridLodSelection {
    float cellA = 1.0f;
    float cellB = 1.0f;
    float blend = 0.0f;
    float originSnap = 1.0f;
};

float Smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / std::max(edge1 - edge0, 1e-6f), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

GridLodSelection SelectGridLod(float cameraDistance, float fovDegrees, float viewportHeight) {
    GridLodSelection result{};

    const float dist = std::max(cameraDistance, 0.5f);
    const float tanHalfFov = std::tan(fovDegrees * 0.5f * 3.14159265f / 180.0f);
    const float visibleMeters = 2.0f * dist * tanHalfFov;
    const float metersPerPixel = visibleMeters / std::max(viewportHeight, 1.0f);
    float idealCell = metersPerPixel * kTargetPixelsPerCell;
    idealCell = std::clamp(idealCell, kLodSizes[0], kLodSizes[kLodCount - 1]);

    int idxA = 0;
    for (int i = 0; i < kLodCount - 1; ++i) {
        const float boundary = std::sqrt(kLodSizes[i] * kLodSizes[i + 1]);
        if (idealCell >= boundary) {
            idxA = i + 1;
        }
    }

    const int idxB = std::min(idxA + 1, kLodCount - 1);
    result.cellA = kLodSizes[idxA];
    result.cellB = kLodSizes[idxB];

    if (idxA < idxB) {
        const float boundary = std::sqrt(result.cellA * result.cellB);
        const float blendWidth = boundary * 0.04f;
        result.blend = Smoothstep(boundary - blendWidth * 0.5f, boundary + blendWidth * 0.5f, idealCell);
    }

    result.originSnap = (result.blend > 0.5f) ? result.cellB : result.cellA;
    return result;
}

} // namespace

GridRenderer::GridRenderer(const std::shared_ptr<VulkanContext>& context, VkRenderPass renderPass, VkDescriptorSetLayout cameraDescLayout)
    : m_Context(context), m_CameraDescLayout(cameraDescLayout) {
    VkDescriptorSetLayoutBinding gridBinding{};
    gridBinding.binding = 0;
    gridBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    gridBinding.descriptorCount = 1;
    gridBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo gridLayoutInfo{};
    gridLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    gridLayoutInfo.bindingCount = 1;
    gridLayoutInfo.pBindings = &gridBinding;
    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &gridLayoutInfo, nullptr, &m_GridDescLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create grid settings descriptor set layout!");
    }

    m_Context->CreateBuffer(
        sizeof(GridSettings),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_GridSettingsBuffer,
        m_GridSettingsMemory
    );

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Context->GetDescriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_GridDescLayout;
    if (vkAllocateDescriptorSets(m_Context->GetDevice(), &allocInfo, &m_GridDescSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate grid settings descriptor set!");
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_GridSettingsBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(GridSettings);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_GridDescSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &write, 0, nullptr);

    UpdateGridSettingsBufferIfDirty();
    CreatePipeline(renderPass);
}

GridRenderer::~GridRenderer() {
    VkDevice device = m_Context->GetDevice();
    if (m_Pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_Pipeline, nullptr);
    }
    if (m_PipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    }
    if (m_GridSettingsBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_GridSettingsBuffer, nullptr);
    }
    if (m_GridSettingsMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_GridSettingsMemory, nullptr);
    }
    if (m_GridDescLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_GridDescLayout, nullptr);
    }
}

void GridRenderer::SetGridFadeDistance(float distance) {
    m_GridSettings.fadeDistance = std::max(10.0f, distance);
    m_GridSettingsDirty = true;
}

void GridRenderer::SetGridLodIntensity(float intensity) {
    m_GridSettings.lodIntensity = std::clamp(intensity, 0.0f, 2.0f);
    m_GridSettingsDirty = true;
}

void GridRenderer::UpdateFromCamera(float lodDistance, float fovDegrees, float viewportHeight, float projYScale) {
    const GridLodSelection lod = SelectGridLod(lodDistance, fovDegrees, viewportHeight);

    m_GridSettings.cellSizeA = lod.cellA;
    m_GridSettings.cellSizeB = lod.cellB;
    m_GridSettings.lodBlend = lod.blend;
    m_GridSettings.originSnap = lod.originSnap;

    const float zoomFactor = 2.0f / std::max(std::abs(projYScale), 1e-4f);
    m_GridSettings.thicknessScale = std::clamp(zoomFactor * 0.45f, 0.40f, 1.6f);
    m_GridSettingsDirty = true;
}

void GridRenderer::UpdateGridSettingsBufferIfDirty() {
    if (!m_GridSettingsDirty || m_GridSettingsMemory == VK_NULL_HANDLE) {
        return;
    }

    void* data = nullptr;
    vkMapMemory(m_Context->GetDevice(), m_GridSettingsMemory, 0, sizeof(GridSettings), 0, &data);
    std::memcpy(data, &m_GridSettings, sizeof(GridSettings));
    vkUnmapMemory(m_Context->GetDevice(), m_GridSettingsMemory);
    m_GridSettingsDirty = false;
}

void GridRenderer::CreatePipeline(VkRenderPass renderPass) {
    VkDevice device = m_Context->GetDevice();

    std::vector<char> vertCode = LoadShaderBytecode("Grid", ShaderStage::Vertex);
    std::vector<char> fragCode = LoadShaderBytecode("Grid", ShaderStage::Pixel);

    VkShaderModule vertShaderModule = CreateShaderModule(device, vertCode);
    VkShaderModule fragShaderModule = CreateShaderModule(device, fragCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = ShaderStageEntryPoint(ShaderStage::Vertex);

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = ShaderStageEntryPoint(ShaderStage::Pixel);

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    std::array<VkDescriptorSetLayout, 2> setLayouts = { m_CameraDescLayout, m_GridDescLayout };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create grid pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create grid graphics pipeline!");
    }

    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
}

void GridRenderer::Draw(VkCommandBuffer cmd, VkDescriptorSet cameraDescSet) const {
    const_cast<GridRenderer*>(this)->UpdateGridSettingsBufferIfDirty();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    VkDescriptorSet sets[] = { cameraDescSet, m_GridDescSet };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 2, sets, 0, nullptr);
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

} // namespace we::runtime::renderer
