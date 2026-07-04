#include "Renderer/SceneRenderer.hpp"
#include "Renderer/RenderDiagnostics.hpp"
#include "Renderer/FrameStats.hpp"
#include "Renderer/RendererConfig.hpp"
#include "Core/DiagnosticMacros.hpp"
#include "Core/LogCategory.hpp"
#include "Renderer/ShaderHelper.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#if WE_HAS_GLM
#include <glm/geometric.hpp>
#endif
#include <cstring>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

SceneRenderer::SceneRenderer(const std::shared_ptr<VulkanContext>& context, VkRenderPass renderPass, VkDescriptorSetLayout cameraDescLayout)
    : m_Context(context), m_CameraDescLayout(cameraDescLayout) {
    VkDescriptorSetLayoutBinding cameraBinding{};
    cameraBinding.binding = 0;
    cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraBinding.descriptorCount = 1;
    cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding objectBinding{};
    objectBinding.binding = 1;
    objectBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    objectBinding.descriptorCount = 1;
    objectBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding environmentBinding{};
    environmentBinding.binding = 2;
    environmentBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    environmentBinding.descriptorCount = 1;
    environmentBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = { cameraBinding, objectBinding, environmentBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &layoutInfo, nullptr, &m_ObjectDescLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create object descriptor set layout!");
    }

    m_Context->CreateBuffer(
        sizeof(SceneEnvironmentUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_EnvironmentBuffer,
        m_EnvironmentBufferMemory);

    VkDescriptorSetLayoutBinding envBinding{};
    envBinding.binding = 0;
    envBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    envBinding.descriptorCount = 1;
    envBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo envLayoutInfo{};
    envLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    envLayoutInfo.bindingCount = 1;
    envLayoutInfo.pBindings = &envBinding;
    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &envLayoutInfo, nullptr, &m_EnvironmentDescLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create environment descriptor set layout!");
    }

    VkDescriptorSetAllocateInfo envAllocInfo{};
    envAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    envAllocInfo.descriptorPool = m_Context->GetDescriptorPool();
    envAllocInfo.descriptorSetCount = 1;
    envAllocInfo.pSetLayouts = &m_EnvironmentDescLayout;
    if (vkAllocateDescriptorSets(m_Context->GetDevice(), &envAllocInfo, &m_EnvironmentDescSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate environment descriptor set!");
    }

    VkDescriptorBufferInfo envBufferInfo{};
    envBufferInfo.buffer = m_EnvironmentBuffer;
    envBufferInfo.offset = 0;
    envBufferInfo.range = sizeof(SceneEnvironmentUniform);

    VkWriteDescriptorSet envWrite{};
    envWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    envWrite.dstSet = m_EnvironmentDescSet;
    envWrite.dstBinding = 0;
    envWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    envWrite.descriptorCount = 1;
    envWrite.pBufferInfo = &envBufferInfo;
    vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &envWrite, 0, nullptr);

    m_LUTGenerator = std::make_unique<AtmosphereLUTGenerator>(m_Context);

    std::array<VkDescriptorSetLayoutBinding, 3> fogLutBindings{};
    for (uint32_t i = 0; i < fogLutBindings.size(); ++i) {
        fogLutBindings[i].binding = i;
        fogLutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        fogLutBindings[i].descriptorCount = 1;
        fogLutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo fogLutLayoutInfo{};
    fogLutLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    fogLutLayoutInfo.bindingCount = static_cast<uint32_t>(fogLutBindings.size());
    fogLutLayoutInfo.pBindings = fogLutBindings.data();
    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &fogLutLayoutInfo, nullptr, &m_FogLutDescLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fog LUT descriptor set layout!");
    }

    VkDescriptorSetAllocateInfo fogLutAllocInfo{};
    fogLutAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    fogLutAllocInfo.descriptorPool = m_Context->GetDescriptorPool();
    fogLutAllocInfo.descriptorSetCount = 1;
    fogLutAllocInfo.pSetLayouts = &m_FogLutDescLayout;
    if (vkAllocateDescriptorSets(m_Context->GetDevice(), &fogLutAllocInfo, &m_FogLutDescSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate fog LUT descriptor set!");
    }

    VkDescriptorSetLayoutBinding postStorageBinding{};
    postStorageBinding.binding = 0;
    postStorageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    postStorageBinding.descriptorCount = 1;
    postStorageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo postLayoutInfo{};
    postLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    postLayoutInfo.bindingCount = 1;
    postLayoutInfo.pBindings = &postStorageBinding;
    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &postLayoutInfo, nullptr, &m_PostStorageDescLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create post storage descriptor set layout!");
    }

    VkDescriptorSetAllocateInfo postAllocInfo{};
    postAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    postAllocInfo.descriptorPool = m_Context->GetDescriptorPool();
    postAllocInfo.descriptorSetCount = 1;
    postAllocInfo.pSetLayouts = &m_PostStorageDescLayout;
    if (vkAllocateDescriptorSets(m_Context->GetDevice(), &postAllocInfo, &m_PostStorageDescSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate post storage descriptor set!");
    }

    CreatePipelines(renderPass);
    ValidateCreatedPipelines();
    CreateMeshes();
}

SceneRenderer::~SceneRenderer() {
    VkDevice device = m_Context->GetDevice();
    DestroyMeshes();

    if (m_PostExposurePipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_PostExposurePipeline, nullptr);
    if (m_SkyAtmospherePipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_SkyAtmospherePipeline, nullptr);
    if (m_VolumetricCloudsPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_VolumetricCloudsPipeline, nullptr);
    if (m_FogCompositePipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_FogCompositePipeline, nullptr);
    if (m_LitPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_LitPipeline, nullptr);
    if (m_UnlitPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_UnlitPipeline, nullptr);
    if (m_WireframePipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_WireframePipeline, nullptr);

    if (m_PostPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_PostPipelineLayout, nullptr);
    if (m_FogPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_FogPipelineLayout, nullptr);
    if (m_EnvironmentPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_EnvironmentPipelineLayout, nullptr);
    if (m_PipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);

    m_LUTGenerator.reset();

    if (m_EnvironmentBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, m_EnvironmentBuffer, nullptr);
    if (m_EnvironmentBufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, m_EnvironmentBufferMemory, nullptr);
    if (m_PostStorageDescLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_PostStorageDescLayout, nullptr);
    if (m_FogLutDescLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_FogLutDescLayout, nullptr);
    if (m_EnvironmentDescLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_EnvironmentDescLayout, nullptr);
    if (m_ObjectDescLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_ObjectDescLayout, nullptr);
}

void SceneRenderer::CreatePipelines(VkRenderPass renderPass) {
    VkDevice device = m_Context->GetDevice();

    VkPipelineLayoutCreateInfo meshLayoutInfo{};
    meshLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    meshLayoutInfo.setLayoutCount = 1;
    meshLayoutInfo.pSetLayouts = &m_ObjectDescLayout;
    if (vkCreatePipelineLayout(device, &meshLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create scene object pipeline layout!");
    }

    std::array<VkDescriptorSetLayout, 3> envSetLayouts = {
        m_EnvironmentDescLayout,
        m_CameraDescLayout,
        m_LUTGenerator->GetSampleLayout()
    };
    VkPipelineLayoutCreateInfo envLayoutInfo{};
    envLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    envLayoutInfo.setLayoutCount = static_cast<uint32_t>(envSetLayouts.size());
    envLayoutInfo.pSetLayouts = envSetLayouts.data();
    if (vkCreatePipelineLayout(device, &envLayoutInfo, nullptr, &m_EnvironmentPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create environment pipeline layout!");
    }

    std::array<VkDescriptorSetLayout, 3> fogSetLayouts = {
        m_EnvironmentDescLayout,
        m_CameraDescLayout,
        m_FogLutDescLayout
    };
    VkPipelineLayoutCreateInfo fogLayoutInfo{};
    fogLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    fogLayoutInfo.setLayoutCount = static_cast<uint32_t>(fogSetLayouts.size());
    fogLayoutInfo.pSetLayouts = fogSetLayouts.data();
    if (vkCreatePipelineLayout(device, &fogLayoutInfo, nullptr, &m_FogPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fog pipeline layout!");
    }

    std::array<VkDescriptorSetLayout, 2> postSetLayouts = { m_EnvironmentDescLayout, m_PostStorageDescLayout };
    VkPipelineLayoutCreateInfo postLayoutInfo{};
    postLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    postLayoutInfo.setLayoutCount = static_cast<uint32_t>(postSetLayouts.size());
    postLayoutInfo.pSetLayouts = postSetLayouts.data();
    if (vkCreatePipelineLayout(device, &postLayoutInfo, nullptr, &m_PostPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create post pipeline layout!");
    }

    m_SkyAtmospherePipeline = CreateFullscreenPipeline(renderPass, m_EnvironmentPipelineLayout, "AtmospherePass", false, VK_COMPARE_OP_ALWAYS, false, false);
    m_VolumetricCloudsPipeline = CreateFullscreenPipeline(renderPass, m_EnvironmentPipelineLayout, "VolumetricCloudsPass", false, VK_COMPARE_OP_ALWAYS, false, true);
    m_FogCompositePipeline = CreateFullscreenPipeline(renderPass, m_FogPipelineLayout, "FogCompositePass", true, VK_COMPARE_OP_GREATER, false, true);

    {
        try {
            std::vector<char> computeCode = LoadShaderBytecode("PostExposureCS", ShaderStage::Compute);
            if (!computeCode.empty()) {
            VkShaderModule computeModule = CreateShaderModule(device, computeCode);
            VkPipelineShaderStageCreateInfo computeStage{};
            computeStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            computeStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            computeStage.module = computeModule;
            computeStage.pName = "CSMain";

            VkComputePipelineCreateInfo computeInfo{};
            computeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            computeInfo.stage = computeStage;
            computeInfo.layout = m_PostPipelineLayout;
            if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computeInfo, nullptr, &m_PostExposurePipeline) != VK_SUCCESS) {
                HE_WARN("SceneRenderer: failed to create post exposure compute pipeline.");
            }
            vkDestroyShaderModule(device, computeModule, nullptr);
            }
        } catch (const std::exception& ex) {
            HE_WARN(std::string("SceneRenderer: PostExposureCS unavailable: ") + ex.what());
        }
    }

    // Scene object pipelines
    std::vector<char> vertCode = LoadShaderBytecode("SceneObject", ShaderStage::Vertex);
    std::vector<char> fragCode = LoadShaderBytecode("SceneObject", ShaderStage::Pixel);
    VkShaderModule vertModule = CreateShaderModule(device, vertCode);
    VkShaderModule fragModule = CreateShaderModule(device, fragCode);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = ShaderStageEntryPoint(ShaderStage::Vertex);

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = ShaderStageEntryPoint(ShaderStage::Pixel);

    std::array<VkPipelineShaderStageCreateInfo, 2> stages = { vertStage, fragStage };

    VkVertexInputBindingDescription bindingDesc = Vertex::GetBindingDescription();
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();

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
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
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

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_LitPipeline) != VK_SUCCESS) {
        HE_ERROR("Failed to create lit mesh pipeline.");
    }
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_UnlitPipeline) != VK_SUCCESS) {
        HE_ERROR("Failed to create unlit mesh pipeline.");
    }
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_WireframePipeline) != VK_SUCCESS) {
        HE_ERROR("Failed to create wireframe mesh pipeline.");
    }

    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
}

void SceneRenderer::ValidateCreatedPipelines() const {
    auto& diag = RenderDiagnostics::Get();
    diag.ValidatePipeline("SkyAtmosphere", m_SkyAtmospherePipeline);
    diag.ValidatePipeline("VolumetricClouds", m_VolumetricCloudsPipeline);
    diag.ValidatePipeline("FogComposite", m_FogCompositePipeline);
    diag.ValidatePipeline("PostExposure", m_PostExposurePipeline);
    diag.ValidatePipeline("SceneObjectLit", m_LitPipeline);
}

bool SceneRenderer::ValidateRenderFrame(VkFramebuffer framebuffer, uint32_t width, uint32_t height) const {
    auto& diag = RenderDiagnostics::Get();
    diag.ResetFrame();
    diag.ValidateFramebuffer(framebuffer, width, height);
    diag.ValidateEnvironmentUniform(m_SceneEnvironment);
    return !diag.HasCritical();
}

VkPipeline SceneRenderer::CreateFullscreenPipeline(
    VkRenderPass renderPass,
    VkPipelineLayout layout,
    const char* shaderName,
    bool depthTest,
    VkCompareOp depthCompare,
    bool depthWrite,
    bool alphaBlend) const {

    VkDevice device = m_Context->GetDevice();
    std::vector<char> vertCode = LoadShaderBytecode(shaderName, ShaderStage::Vertex);
    std::vector<char> fragCode = LoadShaderBytecode(shaderName, ShaderStage::Pixel);
    if (vertCode.empty() || fragCode.empty()) {
        RenderDiagnostics::Get().ValidateShaderBytecode(shaderName, !vertCode.empty(), !fragCode.empty());
        return VK_NULL_HANDLE;
    }

    VkShaderModule vertModule = CreateShaderModule(device, vertCode);
    VkShaderModule fragModule = CreateShaderModule(device, fragCode);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = ShaderStageEntryPoint(ShaderStage::Vertex);

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = ShaderStageEntryPoint(ShaderStage::Pixel);

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
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = depthCompare;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if (alphaBlend) {
        blendAttachment.blendEnable = VK_TRUE;
        blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &blendAttachment;

    std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages.data();
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        HE_WARN(std::string("SceneRenderer: failed to create pipeline for ") + shaderName);
    }

    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
    return pipeline;
}

void SceneRenderer::CreateMeshes() {
    // 1. Create Ground Plane Mesh
    // Large square on XZ plane
    std::vector<Vertex> planeVertices = {
        { {-50.0f, 0.0f, -50.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
        { { 50.0f, 0.0f, -50.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
        { { 50.0f, 0.0f,  50.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
        { {-50.0f, 0.0f,  50.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} }
    };
    std::vector<uint32_t> planeIndices = {
        0, 2, 1, 0, 3, 2 // Counter-clockwise winding
    };
    CreateMeshBuffers("Plane", planeVertices, planeIndices);

    // 2. Create Cube Mesh
    // 3D cube from -0.5 to 0.5
    std::vector<Vertex> cubeVertices = {
        // Front Face (Normal: 0, 0, 1)
        { {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} },
        { { 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} },
        { { 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
        { {-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} },

        // Back Face (Normal: 0, 0, -1)
        { { 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} },
        { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} },
        { {-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} },
        { { 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} },

        // Top Face (Normal: 0, 1, 0)
        { {-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
        { { 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
        { { 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
        { {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },

        // Bottom Face (Normal: 0, -1, 0)
        { {-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} },
        { { 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} },
        { { 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} },
        { {-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} },

        // Left Face (Normal: -1, 0, 0)
        { {-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
        { {-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} },
        { {-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} },
        { {-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} },

        // Right Face (Normal: 1, 0, 0)
        { { 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
        { { 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} },
        { { 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} },
        { { 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} }
    };
    std::vector<uint32_t> cubeIndices = {
        0, 2, 1,     0, 3, 2,     // Front
        4, 6, 5,     4, 7, 6,     // Back
        8, 10, 9,    8, 11, 10,   // Top
        12, 14, 13,  12, 15, 14,  // Bottom
        16, 18, 17,  16, 19, 18,  // Left
        20, 22, 21,  20, 23, 22   // Right
    };
    CreateMeshBuffers("Cube", cubeVertices, cubeIndices);
}

void SceneRenderer::CreateMeshBuffers(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    MeshBuffer buffer{};
    buffer.indexCount = static_cast<uint32_t>(indices.size());

    VkDeviceSize vertexSize = sizeof(Vertex) * vertices.size();
    VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();

    // Allocate vertices on host-visible memory for simplicity
    m_Context->CreateBuffer(
        vertexSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer.vertexBuffer,
        buffer.vertexBufferMemory
    );

    // Copy vertex data
    void* vertexData;
    vkMapMemory(m_Context->GetDevice(), buffer.vertexBufferMemory, 0, vertexSize, 0, &vertexData);
    memcpy(vertexData, vertices.data(), vertexSize);
    vkUnmapMemory(m_Context->GetDevice(), buffer.vertexBufferMemory);

    // Allocate indices
    m_Context->CreateBuffer(
        indexSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer.indexBuffer,
        buffer.indexBufferMemory
    );

    // Copy index data
    void* indexData;
    vkMapMemory(m_Context->GetDevice(), buffer.indexBufferMemory, 0, indexSize, 0, &indexData);
    memcpy(indexData, indices.data(), indexSize);
    vkUnmapMemory(m_Context->GetDevice(), buffer.indexBufferMemory);

    m_Meshes[name] = buffer;
}

void SceneRenderer::DestroyMeshes() {
    VkDevice device = m_Context->GetDevice();
    for (auto& [name, buffer] : m_Meshes) {
        if (buffer.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffer.vertexBuffer, nullptr);
            vkFreeMemory(device, buffer.vertexBufferMemory, nullptr);
        }
        if (buffer.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffer.indexBuffer, nullptr);
            vkFreeMemory(device, buffer.indexBufferMemory, nullptr);
        }
    }
    m_Meshes.clear();
}

void SceneRenderer::PrepareAtmosphereLUTs(VkCommandBuffer /*cmd*/) {
    RefreshEnvironmentDescriptorBindings();
    if (m_LUTGenerator) {
        m_LUTGenerator->EnsureGenerated(m_SceneEnvironment);
    }
}

void SceneRenderer::DrawSkyAtmospherePass(VkCommandBuffer cmd, VkDescriptorSet cameraDescSet) const {
    GpuDebugScope debugScope(cmd, "SkyAtmosphere");
    if (m_SkyAtmospherePipeline == VK_NULL_HANDLE || !m_LUTGenerator) {
        static bool s_Reported = false;
        if (!s_Reported) {
            RenderDiagnostics::Get().ValidatePipeline("SkyAtmosphere", m_SkyAtmospherePipeline);
            s_Reported = true;
        }
        return;
    }
    const_cast<SceneRenderer*>(this)->PrepareAtmosphereLUTs(cmd);
    RenderDiagnostics::Get().ValidateDescriptorSet("SkyAtmosphere", 0, m_EnvironmentDescSet);
    RenderDiagnostics::Get().ValidateDescriptorSet("SkyAtmosphere", 1, cameraDescSet);
    RenderDiagnostics::Get().ValidateDescriptorSet("SkyAtmosphere", 2, m_LUTGenerator->GetSampleSet());
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyAtmospherePipeline);
    VkDescriptorSet sets[] = { m_EnvironmentDescSet, cameraDescSet, m_LUTGenerator->GetSampleSet() };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_EnvironmentPipelineLayout, 0, 3, sets, 0, nullptr);
    vkCmdDraw(cmd, 6, 1, 0, 0);
    FrameStatsCollector::Get().AddDrawCall(2);
}

void SceneRenderer::DrawVolumetricCloudsPass(VkCommandBuffer cmd, VkDescriptorSet cameraDescSet) const {
    if (m_VolumetricCloudsPipeline == VK_NULL_HANDLE || m_SceneEnvironment.enableClouds < 0.5f) return;
    RefreshEnvironmentDescriptorBindings();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VolumetricCloudsPipeline);
    VkDescriptorSet sets[] = { m_EnvironmentDescSet, cameraDescSet, m_LUTGenerator->GetSampleSet() };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_EnvironmentPipelineLayout, 0, 3, sets, 0, nullptr);
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void SceneRenderer::DrawFogCompositePass(
    VkCommandBuffer cmd,
    VkDescriptorSet cameraDescSet,
    VkImageView depthImageView,
    VkSampler sampler) const {

    if (m_FogCompositePipeline == VK_NULL_HANDLE || m_SceneEnvironment.enableVolumetricFog < 0.5f || !m_LUTGenerator) return;
    const_cast<SceneRenderer*>(this)->PrepareAtmosphereLUTs(cmd);
    RefreshEnvironmentDescriptorBindings();

    if (depthImageView != m_BoundFogDepthView) {
        const auto& lutImages = m_LUTGenerator->GetImages();
        VkDescriptorImageInfo imageInfos[3] = {
            { sampler, depthImageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL },
            { lutImages.sampler, lutImages.aerialView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { lutImages.sampler, lutImages.skyViewView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        };

        VkWriteDescriptorSet writes[3]{};
        for (uint32_t i = 0; i < 3; ++i) {
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = m_FogLutDescSet;
            writes[i].dstBinding = i;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i].descriptorCount = 1;
            writes[i].pImageInfo = &imageInfos[i];
        }
        vkUpdateDescriptorSets(m_Context->GetDevice(), 3, writes, 0, nullptr);
        m_BoundFogDepthView = depthImageView;
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_FogCompositePipeline);
    VkDescriptorSet sets[] = { m_EnvironmentDescSet, cameraDescSet, m_FogLutDescSet };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_FogPipelineLayout, 0, 3, sets, 0, nullptr);
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void SceneRenderer::ApplyPostExposure(
    VkCommandBuffer cmd,
    VkImage colorImage,
    VkImageView colorImageView,
    uint32_t width,
    uint32_t height) const {

    if (m_PostExposurePipeline == VK_NULL_HANDLE || colorImage == VK_NULL_HANDLE || colorImageView == VK_NULL_HANDLE || width == 0 || height == 0) {
        RenderDiagnostics::Get().ValidatePipeline("PostExposure", m_PostExposurePipeline);
        return;
    }

    GpuDebugScope debugScope(cmd, "PostExposure");

    RefreshEnvironmentDescriptorBindings();

    if (colorImageView != m_BoundPostColorView) {
        VkDescriptorImageInfo storageInfo{};
        storageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        storageInfo.imageView = colorImageView;

        VkWriteDescriptorSet storageWrite{};
        storageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        storageWrite.dstSet = m_PostStorageDescSet;
        storageWrite.dstBinding = 0;
        storageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        storageWrite.descriptorCount = 1;
        storageWrite.pImageInfo = &storageInfo;
        vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &storageWrite, 0, nullptr);
        m_BoundPostColorView = colorImageView;
    }

    m_Context->TransitionImageLayout(colorImage, kOffscreenColorFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PostExposurePipeline);
    VkDescriptorSet sets[] = { m_EnvironmentDescSet, m_PostStorageDescSet };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PostPipelineLayout, 0, 2, sets, 0, nullptr);
    vkCmdDispatch(cmd, (width + 7) / 8, (height + 7) / 8, 1);

    m_Context->TransitionImageLayout(colorImage, kOffscreenColorFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void SceneRenderer::DrawMesh(VkCommandBuffer cmd, const std::string& meshName, VkDescriptorSet descriptorSet, int mode) const {
    RefreshEnvironmentDescriptorBindings();

    auto it = m_Meshes.find(meshName);
    if (it == m_Meshes.end()) {
        HE_WARN("Mesh " + meshName + " not found!");
        return;
    }

    const MeshBuffer& mesh = it->second;

    // Bind the correct pipeline
    VkPipeline pipeline = m_LitPipeline;
    if (mode == 1) pipeline = m_UnlitPipeline;
    else if (mode == 2) pipeline = m_WireframePipeline;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
}

void SceneRenderer::UpdateObjectDescriptorSet(VkDescriptorSet descriptorSet, VkBuffer cameraBuffer, VkBuffer objectBuffer) const {
    VkDescriptorBufferInfo cameraBufferInfo{};
    cameraBufferInfo.buffer = cameraBuffer;
    cameraBufferInfo.offset = 0;
    cameraBufferInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo objectBufferInfo{};
    objectBufferInfo.buffer = objectBuffer;
    objectBufferInfo.offset = 0;
    objectBufferInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo environmentBufferInfo{};
    environmentBufferInfo.buffer = m_EnvironmentBuffer;
    environmentBufferInfo.offset = 0;
    environmentBufferInfo.range = sizeof(SceneEnvironmentUniform);

    std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &cameraBufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &objectBufferInfo;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = descriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &environmentBufferInfo;

    vkUpdateDescriptorSets(m_Context->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void SceneRenderer::SetSceneEnvironment(const SceneEnvironmentUniform& environment) {
    if (std::memcmp(&m_SceneEnvironment, &environment, sizeof(SceneEnvironmentUniform)) != 0) {
        if (m_LUTGenerator) {
            m_LUTGenerator->Invalidate();
        }
    }
    m_SceneEnvironment = environment;
    m_SceneEnvironmentDirty = true;
}

void SceneRenderer::RefreshEnvironmentDescriptorBindings() const {
    if (!m_SceneEnvironmentDirty || m_EnvironmentBuffer == VK_NULL_HANDLE) {
        return;
    }

    void* data = nullptr;
    vkMapMemory(m_Context->GetDevice(), m_EnvironmentBufferMemory, 0, sizeof(SceneEnvironmentUniform), 0, &data);
    std::memcpy(data, &m_SceneEnvironment, sizeof(SceneEnvironmentUniform));
    vkUnmapMemory(m_Context->GetDevice(), m_EnvironmentBufferMemory);
    m_SceneEnvironmentDirty = false;
}

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
