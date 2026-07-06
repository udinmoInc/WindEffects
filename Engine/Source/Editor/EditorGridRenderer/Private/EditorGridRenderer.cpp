#include "EditorGridRenderer.hpp"
#include "EditorFunctionConfig.hpp"
#include "EditorCamera.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Renderer/ShaderHelper.hpp"
#include "Renderer/Shader/ShaderManifest.hpp"
#include "Core/Logger.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace we::editor::grid {

namespace {

constexpr float kGridWorldPlaneY = 0.0f;

// std140 layout mirrored in EditorGrid.hlsl (vec4-packed for reliable alignment).
struct GridUniforms {
    glm::vec4 levelSizes;
    glm::vec4 levelFadeStart;
    glm::vec4 levelFadeEnd;

    glm::vec4 level0Color;
    glm::vec4 level1Color;
    glm::vec4 level2Color;
    glm::vec4 level3Color;
    glm::vec4 axisXColor;
    glm::vec4 axisZColor;

    glm::vec4 renderParams0;
    glm::vec4 renderParams1;
    glm::vec4 renderParams2;
    glm::vec4 snappedOrigin;

    glm::ivec4 gridFlags;
    glm::vec4 depthParams;
};

static_assert(sizeof(GridUniforms) == 240, "GridUniforms must match EditorGrid.hlsl std140 layout.");

double SnapToSpacing(double value, double spacing) {
    return std::floor(value / spacing) * spacing;
}

bool IsQuadOutsideFrustum(const glm::mat4& viewProj, const glm::vec3& center, float radius, float planeY) {
    auto isCornerOnScreen = [&](const glm::vec3& corner) -> bool {
        const glm::vec4 clip = viewProj * glm::vec4(corner, 1.0f);
        if (clip.w <= 1e-4f) {
            return false;
        }
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        return ndc.x >= -1.2f && ndc.x <= 1.2f && ndc.y >= -1.2f && ndc.y <= 1.2f;
    };

    const glm::vec3 corners[4] = {
        { center.x - radius, planeY, center.z - radius },
        { center.x + radius, planeY, center.z - radius },
        { center.x + radius, planeY, center.z + radius },
        { center.x - radius, planeY, center.z + radius },
    };

    for (const glm::vec3& corner : corners) {
        if (isCornerOnScreen(corner)) {
            return false;
        }
    }

    // Camera may be inside the ground patch — treat center visibility as sufficient.
    return !isCornerOnScreen(center);
}

} // namespace

EditorGridRenderer& EditorGridRenderer::Get() {
    static EditorGridRenderer instance;
    return instance;
}

void EditorGridRenderer::Initialize(const std::shared_ptr<we::runtime::renderer::VulkanContext>& context,
                                    VkRenderPass renderPass,
                                    VkDescriptorSetLayout cameraDescLayout) {
    if (m_Initialized) {
        return;
    }

    EditorFunctionConfig::Get().EnsureLoaded();

    m_Context = context;
    volkInitialize();
    volkLoadInstance(m_Context->GetInstance());
    volkLoadDevice(m_Context->GetDevice());
    CreateResources(renderPass, cameraDescLayout);
    m_Initialized = true;
}

void EditorGridRenderer::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    DestroyResources();
    m_Context.reset();
    m_Initialized = false;
}

void EditorGridRenderer::CreateResources(VkRenderPass renderPass, VkDescriptorSetLayout cameraDescLayout) {
    VkDevice device = m_Context->GetDevice();

    VkDescriptorSetLayoutBinding gridBinding{};
    gridBinding.binding = 0;
    gridBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    gridBinding.descriptorCount = 1;
    gridBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo gridLayoutInfo{};
    gridLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    gridLayoutInfo.bindingCount = 1;
    gridLayoutInfo.pBindings = &gridBinding;
    if (vkCreateDescriptorSetLayout(device, &gridLayoutInfo, nullptr, &m_GridDescLayout) != VK_SUCCESS) {
        HE_ERROR("EditorGridRenderer: failed to create descriptor set layout.");
        return;
    }

    m_Context->CreateBuffer(
        sizeof(GridUniforms),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_UniformBuffer,
        m_UniformMemory
    );

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Context->GetDescriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_GridDescLayout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &m_GridDescSet) != VK_SUCCESS) {
        HE_ERROR("EditorGridRenderer: failed to allocate descriptor set.");
        return;
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_UniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(GridUniforms);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_GridDescSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    CreatePipeline(renderPass, cameraDescLayout);
}

void EditorGridRenderer::DestroyResources() {
    if (!m_Context) {
        return;
    }

    VkDevice device = m_Context->GetDevice();
    if (m_Pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_Pipeline, nullptr);
    if (m_PipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    if (m_UniformBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, m_UniformBuffer, nullptr);
    if (m_UniformMemory != VK_NULL_HANDLE) vkFreeMemory(device, m_UniformMemory, nullptr);
    if (m_GridDescLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_GridDescLayout, nullptr);

    m_Pipeline = VK_NULL_HANDLE;
    m_PipelineLayout = VK_NULL_HANDLE;
    m_UniformBuffer = VK_NULL_HANDLE;
    m_UniformMemory = VK_NULL_HANDLE;
    m_GridDescSet = VK_NULL_HANDLE;
    m_GridDescLayout = VK_NULL_HANDLE;
}

void EditorGridRenderer::CreatePipeline(VkRenderPass renderPass, VkDescriptorSetLayout cameraDescLayout) {
    VkDevice device = m_Context->GetDevice();

    std::array<VkDescriptorSetLayout, 2> setLayouts = { m_GridDescLayout, cameraDescLayout };
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    layoutInfo.pSetLayouts = setLayouts.data();
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        HE_ERROR("EditorGridRenderer: failed to create pipeline layout.");
        return;
    }

    std::vector<char> vertCode = we::runtime::renderer::LoadShaderBytecode(
        we::runtime::renderer::shaders::EditorGrid, we::runtime::renderer::ShaderStage::Vertex);
    std::vector<char> fragCode = we::runtime::renderer::LoadShaderBytecode(
        we::runtime::renderer::shaders::EditorGrid, we::runtime::renderer::ShaderStage::Pixel);

    VkShaderModule vertModule = we::runtime::renderer::CreateShaderModule(device, vertCode);
    VkShaderModule fragModule = we::runtime::renderer::CreateShaderModule(device, fragCode);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = we::runtime::renderer::ShaderStageEntryPoint(we::runtime::renderer::ShaderStage::Vertex);

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = we::runtime::renderer::ShaderStageEntryPoint(we::runtime::renderer::ShaderStage::Pixel);

    std::array<VkPipelineShaderStageCreateInfo, 2> stages = { vertStage, fragStage };

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.blendEnable = VK_TRUE;
    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &blendAttachment;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
    pipelineInfo.pStages = stages.data();
    pipelineInfo.pVertexInputState = &vertexInput;
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

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);
        HE_ERROR("EditorGridRenderer: failed to create graphics pipeline.");
        return;
    }

    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
}

void EditorGridRenderer::UploadUniforms(const we::runtime::engine::EditorCamera& camera) const {
    EditorFunctionConfig::Get().ReloadIfChanged();
    const EditorGridConfig& config = EditorFunctionConfig::Get().GetGridConfig();

    const glm::vec3 cameraPos = camera.GetPosition();
    const double cameraX = static_cast<double>(cameraPos.x);
    const double cameraZ = static_cast<double>(cameraPos.z);

    const double finestSpacing = static_cast<double>(config.level0Size);
    const double snappedX = config.snapOriginToCamera ? SnapToSpacing(cameraX, finestSpacing) : 0.0;
    const double snappedZ = config.snapOriginToCamera ? SnapToSpacing(cameraZ, finestSpacing) : 0.0;

    GridUniforms uniforms{};
    uniforms.levelSizes = glm::vec4(config.level0Size, config.level1Size, config.level2Size, config.level3Size);
    uniforms.levelFadeStart = glm::vec4(config.level0FadeStart, config.level1FadeStart, config.level2FadeStart, config.level3FadeStart);
    uniforms.levelFadeEnd = glm::vec4(config.level0FadeEnd, config.level1FadeEnd, config.level2FadeEnd, config.level3FadeEnd);

    uniforms.level0Color = config.minorGridColor;
    uniforms.level1Color = config.mediumGridColor;
    uniforms.level2Color = config.largeGridColor;
    uniforms.level3Color = config.majorGridColor;
    uniforms.axisXColor = config.axisXColor;
    uniforms.axisZColor = config.axisZColor;

    uniforms.renderParams0 = glm::vec4(
        config.renderRadius,
        config.planeHeight,
        config.lineThicknessMinor,
        config.lineThicknessMajor);
    uniforms.renderParams1 = glm::vec4(
        config.baseOpacity,
        std::min(config.lineThicknessAxis, config.lineThicknessMajor * 1.1f),
        config.horizonGuardNdcMargin,
        config.antiAliasingEnabled ? config.antiAliasScale : 1.0f);
  uniforms.renderParams2 = glm::vec4(
        config.depthBiasConstant,
        config.depthBiasSlope,
        camera.GetGridLodDistance(),
        config.radiusFadeStart);

    const float cullRadius = config.enableDistanceCulling
        ? std::max(config.distanceCullRadius, config.renderRadius * 1.42f + 2.0f)
        : config.renderRadius * 1.42f + 2.0f;
    uniforms.snappedOrigin = glm::vec4(
        static_cast<float>(snappedX),
        config.planeHeight,
        static_cast<float>(snappedZ),
        cullRadius);
    uniforms.gridFlags = glm::ivec4(
        config.enabled ? 1 : 0,
        config.enableAxisLines ? 1 : 0,
        config.antiAliasingEnabled ? 1 : 0,
        0);
    uniforms.depthParams = glm::vec4(
        config.depthOffset,
        config.radiusFadeEnd,
        config.distanceFadeStart,
        config.distanceFadeEnd);

    void* data = nullptr;
    vkMapMemory(m_Context->GetDevice(), m_UniformMemory, 0, sizeof(GridUniforms), 0, &data);
    std::memcpy(data, &uniforms, sizeof(GridUniforms));
    vkUnmapMemory(m_Context->GetDevice(), m_UniformMemory);
}

void EditorGridRenderer::Render(VkCommandBuffer commandBuffer,
                                VkDescriptorSet cameraDescriptorSet,
                                const we::runtime::engine::EditorCamera& camera) const {
    if (!m_Initialized || m_Pipeline == VK_NULL_HANDLE) {
        return;
    }

    EditorFunctionConfig::Get().ReloadIfChanged();
    const EditorGridConfig& config = EditorFunctionConfig::Get().GetGridConfig();
    if (!config.enabled) {
        return;
    }

    const glm::vec3 cameraPos = camera.GetPosition();
    const double finestSpacing = static_cast<double>(config.level0Size);
    const double snappedX = config.snapOriginToCamera
        ? SnapToSpacing(static_cast<double>(cameraPos.x), finestSpacing) : 0.0;
    const double snappedZ = config.snapOriginToCamera
        ? SnapToSpacing(static_cast<double>(cameraPos.z), finestSpacing) : 0.0;
    const glm::vec3 gridCenter(
        static_cast<float>(snappedX),
        config.planeHeight,
        static_cast<float>(snappedZ)
    );

    if (config.enableFrustumCulling) {
        const glm::mat4 viewProj = camera.GetProjectionMatrix() * camera.GetViewMatrix();
        if (IsQuadOutsideFrustum(viewProj, gridCenter, config.renderRadius, config.planeHeight)) {
            // Never skip when the camera is inside the ground patch.
            const glm::vec2 cameraXZ(cameraPos.x, cameraPos.z);
            const glm::vec2 centerXZ(gridCenter.x, gridCenter.z);
            if (glm::length(cameraXZ - centerXZ) > config.renderRadius) {
                return;
            }
        }
    }

    const_cast<EditorGridRenderer*>(this)->UploadUniforms(camera);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    std::array<VkDescriptorSet, 2> descriptorSets = { m_GridDescSet, cameraDescriptorSet };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
        0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

} // namespace we::editor::grid
