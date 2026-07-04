#include "Renderer/SceneRenderer.hpp"
#include "Core/Logger.hpp"
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
    // 1. Create descriptor set layout for procedural editor sky UBO + camera
    VkDescriptorSetLayoutBinding skyBinding{};
    skyBinding.binding = 0;
    skyBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    skyBinding.descriptorCount = 1;
    skyBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo skyLayoutInfo{};
    skyLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    skyLayoutInfo.bindingCount = 1;
    skyLayoutInfo.pBindings = &skyBinding;
    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &skyLayoutInfo, nullptr, &m_SkyDescLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create procedural sky descriptor set layout!");
    }

    m_Context->CreateBuffer(
        sizeof(EditorBackgroundSettings),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_SkyBuffer,
        m_SkyBufferMemory
    );

    VkDescriptorSetAllocateInfo skyAllocInfo{};
    skyAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    skyAllocInfo.descriptorPool = m_Context->GetDescriptorPool();
    skyAllocInfo.descriptorSetCount = 1;
    skyAllocInfo.pSetLayouts = &m_SkyDescLayout;
    if (vkAllocateDescriptorSets(m_Context->GetDevice(), &skyAllocInfo, &m_SkyDescSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate procedural sky descriptor set!");
    }

    VkDescriptorBufferInfo skyBufferInfo{};
    skyBufferInfo.buffer = m_SkyBuffer;
    skyBufferInfo.offset = 0;
    skyBufferInfo.range = sizeof(EditorBackgroundSettings);

    VkWriteDescriptorSet skyWrite{};
    skyWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    skyWrite.dstSet = m_SkyDescSet;
    skyWrite.dstBinding = 0;
    skyWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    skyWrite.descriptorCount = 1;
    skyWrite.pBufferInfo = &skyBufferInfo;
    vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &skyWrite, 0, nullptr);

    UpdateEditorBackgroundBufferIfDirty();

    // 2. Create Descriptor Set Layout for objects (includes both camera and object UBOs)
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
        m_EnvironmentBufferMemory
    );

    // 3. Create Pipelines
    CreatePipelines(renderPass);
    HE_INFO("SceneRenderer: Pipelines created.");

    // 4. Create Ground Plane and Cube Meshes
    CreateMeshes();
    HE_INFO("SceneRenderer: Meshes created.");
}

SceneRenderer::~SceneRenderer() {
    VkDevice device = m_Context->GetDevice();

    DestroyMeshes();

    if (m_SkyboxPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_SkyboxPipeline, nullptr);
    if (m_LitPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_LitPipeline, nullptr);
    if (m_UnlitPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_UnlitPipeline, nullptr);
    if (m_WireframePipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_WireframePipeline, nullptr);

    if (m_SkyPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_SkyPipelineLayout, nullptr);
    if (m_PipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    if (m_SkyBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, m_SkyBuffer, nullptr);
    if (m_SkyBufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, m_SkyBufferMemory, nullptr);
    if (m_EnvironmentBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, m_EnvironmentBuffer, nullptr);
    if (m_EnvironmentBufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, m_EnvironmentBufferMemory, nullptr);
    if (m_SkyDescLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_SkyDescLayout, nullptr);
    if (m_ObjectDescLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_ObjectDescLayout, nullptr);
}

void SceneRenderer::CreatePipelines(VkRenderPass renderPass) {
    VkDevice device = m_Context->GetDevice();

    // Mesh pipeline layout (takes the object descriptor layout)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_ObjectDescLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create scene object pipeline layout!");
    }

    // Sky pipeline layout: sky settings + shared camera UBO
    std::array<VkDescriptorSetLayout, 2> skySetLayouts = { m_SkyDescLayout, m_CameraDescLayout };
    VkPipelineLayoutCreateInfo skyPipelineLayoutInfo{};
    skyPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    skyPipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(skySetLayouts.size());
    skyPipelineLayoutInfo.pSetLayouts = skySetLayouts.data();
    if (vkCreatePipelineLayout(device, &skyPipelineLayoutInfo, nullptr, &m_SkyPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create procedural sky pipeline layout!");
    }

    // Common configurations
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // -------------------------------------------------------------------------
    // 1. Skybox / Background Gradient Pipeline
    // -------------------------------------------------------------------------
    {
        std::vector<char> vertCode = LoadShaderBytecode("EditorBackground", ShaderStage::Vertex);
        std::vector<char> fragCode = LoadShaderBytecode("EditorBackground", ShaderStage::Pixel);
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

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE; // Background doesn't test depth
        depthStencil.depthWriteEnable = VK_FALSE;

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
        pipelineInfo.layout = m_SkyPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_SkyboxPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create skybox pipeline!");
        }

        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);
    }

    // -------------------------------------------------------------------------
    // 2. Lit / Unlit / Wireframe Pipelines (Mesh rendering)
    // -------------------------------------------------------------------------
    {
        std::vector<char> vertCode = LoadShaderBytecode("SceneObject", ShaderStage::Vertex);
        std::vector<char> fragCode = LoadShaderBytecode("SceneObject", ShaderStage::Pixel);
        HE_INFO("SceneRenderer: Creating SceneObject shader modules...");
        VkShaderModule vertModule = CreateShaderModule(device, vertCode);
        VkShaderModule fragModule = CreateShaderModule(device, fragCode);
        HE_INFO("SceneRenderer: Creating lit mesh pipeline...");

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

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;

        // 2a. Lit Pipeline (Solid, Cull Back)
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Standard for counter-clockwise triangles

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

        VkResult litPipelineResult = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_LitPipeline);
        if (litPipelineResult != VK_SUCCESS) {
            HE_ERROR("Failed to create lit mesh pipeline (VkResult=" + std::to_string(static_cast<int>(litPipelineResult)) + ")");
            vkDestroyShaderModule(device, vertModule, nullptr);
            vkDestroyShaderModule(device, fragModule, nullptr);
            return;
        }
        HE_INFO("SceneRenderer: Lit mesh pipeline created.");

        // 2b. Unlit Pipeline (Same as Lit but we use it differently in code by setting mode uniform, though pipeline is identical)
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_UnlitPipeline) != VK_SUCCESS) {
            HE_ERROR("Failed to create unlit mesh pipeline.");
            vkDestroyShaderModule(device, vertModule, nullptr);
            vkDestroyShaderModule(device, fragModule, nullptr);
            return;
        }
        HE_INFO("SceneRenderer: Unlit mesh pipeline created.");

        // 2c. Wireframe Pipeline (Non-solid, Cull None)
        rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_WireframePipeline) != VK_SUCCESS) {
            HE_ERROR("Failed to create wireframe mesh pipeline.");
            vkDestroyShaderModule(device, vertModule, nullptr);
            vkDestroyShaderModule(device, fragModule, nullptr);
            return;
        }
        HE_INFO("SceneRenderer: Wireframe mesh pipeline created.");

        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);
    }
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

void SceneRenderer::DrawEditorBackground(VkCommandBuffer cmd, VkDescriptorSet cameraDescSet) const {
    if (!m_EnableEditorBackground) {
        return;
    }

    const_cast<SceneRenderer*>(this)->UpdateEditorBackgroundBufferIfDirty();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyboxPipeline);
    VkDescriptorSet descriptorSets[] = { m_SkyDescSet, cameraDescSet };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyPipelineLayout, 0, 2, descriptorSets, 0, nullptr);
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void SceneRenderer::SetEditorBackgroundSettings(const EditorBackgroundSettings& settings) {
    auto neutralize = [](const glm::vec3& c) {
        const float g = (c.r + c.g + c.b) / 3.0f;
        return glm::vec3(g);
    };

    EditorBackgroundSettings sanitized = settings;
    sanitized.zenithColor = neutralize(sanitized.zenithColor);
    sanitized.upperSkyColor = neutralize(sanitized.upperSkyColor);
    sanitized.midSkyColor = neutralize(sanitized.midSkyColor);
    sanitized.horizonColor = neutralize(sanitized.horizonColor);
    sanitized.bottomColor = neutralize(sanitized.bottomColor);
    sanitized.backgroundBrightness = std::clamp(sanitized.backgroundBrightness, 0.85f, 1.0f);
    sanitized.gradientStrength = std::clamp(sanitized.gradientStrength, 0.0f, 1.0f);
    sanitized.horizonFade = 0.0f;
    sanitized.backgroundContrast = 1.0f;

    m_EditorBackgroundSettings = sanitized;
    m_EditorBackgroundDirty = true;
}

void SceneRenderer::UpdateEditorBackgroundBufferIfDirty() {
    if (!m_EditorBackgroundDirty || m_SkyBufferMemory == VK_NULL_HANDLE) {
        return;
    }

    void* data = nullptr;
    vkMapMemory(m_Context->GetDevice(), m_SkyBufferMemory, 0, sizeof(EditorBackgroundSettings), 0, &data);
    std::memcpy(data, &m_EditorBackgroundSettings, sizeof(EditorBackgroundSettings));
    vkUnmapMemory(m_Context->GetDevice(), m_SkyBufferMemory);
    m_EditorBackgroundDirty = false;
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
