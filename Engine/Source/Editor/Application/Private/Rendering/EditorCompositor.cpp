#include "Rendering/EditorCompositor.h"

#include "Shader/ShaderLibrary.h"
#include "Resource/ResourceManager.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <vector>

namespace we::editor::rendering {

namespace {

VkSampler CreateLinearClampSampler(VkDevice device) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 1.0f;

    VkSampler sampler = VK_NULL_HANDLE;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return sampler;
}

} // namespace

EditorCompositor::EditorCompositor() = default;

EditorCompositor::~EditorCompositor() {
    Shutdown();
}

bool EditorCompositor::Init(VkPhysicalDevice physicalDevice,
                            VkDevice logicalDevice,
                            we::runtime::renderer::ResourceManager* resourceManager,
                            VkFormat swapchainFormat,
                            uint32_t maxFramesInFlight) {
    m_PhysicalDevice = physicalDevice;
    m_LogicalDevice = logicalDevice;
    m_ResourceManager = resourceManager;
    m_SwapchainFormat = swapchainFormat;
    m_MaxFramesInFlight = maxFramesInFlight;

    if (!m_PhysicalDevice || !m_LogicalDevice || !m_ResourceManager || maxFramesInFlight == 0 || maxFramesInFlight > 2) {
        return false;
    }

    if (!CreateDescriptorResources()) {
        return false;
    }
    if (!CreatePipeline()) {
        return false;
    }
    if (!CreateCompositorGeometry()) {
        return false;
    }

    return true;
}

void EditorCompositor::Shutdown() {
    if (m_LogicalDevice != VK_NULL_HANDLE) {
        DestroyPipelineAndLayout();

        if (m_DescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }

        if (m_TextureDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_LogicalDevice, m_TextureDescriptorSetLayout, nullptr);
            m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
        }

        if (m_Sampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_LogicalDevice, m_Sampler, nullptr);
            m_Sampler = VK_NULL_HANDLE;
        }

        for (uint32_t i = 0; i < m_MaxFramesInFlight; ++i) {
            auto& fg = m_FrameGeometry[i];
            if (fg.vertexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_LogicalDevice, fg.vertexBuffer, nullptr);
                fg.vertexBuffer = VK_NULL_HANDLE;
            }
            if (fg.vertexMemory != VK_NULL_HANDLE) {
                vkFreeMemory(m_LogicalDevice, fg.vertexMemory, nullptr);
                fg.vertexMemory = VK_NULL_HANDLE;
            }

            if (fg.indexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_LogicalDevice, fg.indexBuffer, nullptr);
                fg.indexBuffer = VK_NULL_HANDLE;
            }
            if (fg.indexMemory != VK_NULL_HANDLE) {
                vkFreeMemory(m_LogicalDevice, fg.indexMemory, nullptr);
                fg.indexMemory = VK_NULL_HANDLE;
            }
        }
    }

    m_ResourceManager = nullptr;
    m_PhysicalDevice = VK_NULL_HANDLE;
    m_LogicalDevice = VK_NULL_HANDLE;
}

void EditorCompositor::DestroyPipelineAndLayout() {
    if (m_LogicalDevice != VK_NULL_HANDLE) {
        if (m_GraphicsPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_LogicalDevice, m_GraphicsPipeline, nullptr);
            m_GraphicsPipeline = VK_NULL_HANDLE;
        }
        if (m_PipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
            m_PipelineLayout = VK_NULL_HANDLE;
        }
    }
}

bool EditorCompositor::CreateDescriptorResources() {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &m_TextureDescriptorSetLayout) != VK_SUCCESS) {
        return false;
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = m_MaxFramesInFlight;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = m_MaxFramesInFlight;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
        return false;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = m_MaxFramesInFlight;
    std::vector<VkDescriptorSetLayout> layouts(m_MaxFramesInFlight, m_TextureDescriptorSetLayout);
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> allocated(m_MaxFramesInFlight, VK_NULL_HANDLE);
    if (vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, allocated.data()) != VK_SUCCESS) {
        return false;
    }

    m_TextureDescriptorSets[0] = VK_NULL_HANDLE;
    m_TextureDescriptorSets[1] = VK_NULL_HANDLE;
    for (uint32_t i = 0; i < m_MaxFramesInFlight; ++i) {
        m_TextureDescriptorSets[i] = allocated[i];
    }

    m_Sampler = CreateLinearClampSampler(m_LogicalDevice);
    if (m_Sampler == VK_NULL_HANDLE) {
        return false;
    }

    return true;
}

bool EditorCompositor::CreatePipeline() {
    if (m_SwapchainFormat == VK_FORMAT_UNDEFINED) {
        return false;
    }

    VkShaderModule vertShaderModule = we::runtime::renderer::ShaderLibrary::Get().CreateShaderModule(
        m_LogicalDevice, "UI", we::runtime::renderer::ShaderStage::Vertex);
    VkShaderModule fragShaderModule = we::runtime::renderer::ShaderLibrary::Get().CreateShaderModule(
        m_LogicalDevice, "UI", we::runtime::renderer::ShaderStage::Pixel);

    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        return false;
    }

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "VSMain";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "PSMain";

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(EditorCompositorVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
    attributeDescriptions[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(EditorCompositorVertex, position)};
    attributeDescriptions[1] = {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(EditorCompositorVertex, uv)};
    attributeDescriptions[2] = {0, 2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(EditorCompositorVertex, color)};
    attributeDescriptions[3] = {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(EditorCompositorVertex, sdfRect)};
    attributeDescriptions[4] = {0, 4, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(EditorCompositorVertex, sdfParams)};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 4;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_TextureDescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        return false;
    }

    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &m_SwapchainFormat;
    pipelineRenderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &pipelineRenderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS) {
        return false;
    }

    vkDestroyShaderModule(m_LogicalDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_LogicalDevice, vertShaderModule, nullptr);

    return true;
}

bool EditorCompositor::CreateCompositorGeometry() {
    if (!m_ResourceManager) {
        return false;
    }

    for (uint32_t frameIndex = 0; frameIndex < m_MaxFramesInFlight; ++frameIndex) {
        auto& fg = m_FrameGeometry[frameIndex];

        // Textured quad (4 verts) + crosshair (2 axis-aligned rounded rects => 8 verts).
        VkDeviceSize vertexSize = sizeof(EditorCompositorVertex) * 12;
        m_ResourceManager->CreateBuffer(
            vertexSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            fg.vertexBuffer,
            fg.vertexMemory);

        // 3 shapes * 2 triangles * 3 indices
        VkDeviceSize indexSize = sizeof(uint32_t) * 18;
        m_ResourceManager->CreateBuffer(
            indexSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            fg.indexBuffer,
            fg.indexMemory);

        // Upload indices once (relative vertex indices per shape).
        uint32_t shapeIndices[18] = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            8, 9, 10, 10, 11, 8
        };
        void* mapped = nullptr;
        if (fg.indexMemory != VK_NULL_HANDLE) {
            vkMapMemory(m_LogicalDevice, fg.indexMemory, 0, indexSize, 0, &mapped);
            std::memcpy(mapped, shapeIndices, static_cast<size_t>(indexSize));
            vkUnmapMemory(m_LogicalDevice, fg.indexMemory);
        }
    }

    return true;
}

void EditorCompositor::UpdateVertexBuffer(uint32_t frameIndex, VkRect2D viewportDestRect, bool drawCrosshair) {
    if (frameIndex >= m_MaxFramesInFlight) {
        return;
    }

    auto& fg = m_FrameGeometry[frameIndex];
    if (fg.vertexBuffer == VK_NULL_HANDLE || fg.vertexMemory == VK_NULL_HANDLE) {
        return;
    }

    const float x0 = static_cast<float>(viewportDestRect.offset.x);
    const float y0 = static_cast<float>(viewportDestRect.offset.y);
    const float x1 = static_cast<float>(viewportDestRect.offset.x + static_cast<int32_t>(viewportDestRect.extent.width));
    const float y1 = static_cast<float>(viewportDestRect.offset.y + static_cast<int32_t>(viewportDestRect.extent.height));

    EditorCompositorVertex vertices[12]{};
    // Textured quad (viewport).
    vertices[0] = EditorCompositorVertex{{x0, y0}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0, 0, 0}, {0, 0, 0, 0}};
    vertices[1] = EditorCompositorVertex{{x1, y0}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0, 0, 0}, {0, 0, 0, 0}};
    vertices[2] = EditorCompositorVertex{{x1, y1}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0, 0, 0}, {0, 0, 0, 0}};
    vertices[3] = EditorCompositorVertex{{x0, y1}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0, 0, 0}, {0, 0, 0, 0}};

    if (drawCrosshair) {
        const float centerX = x0 + (x1 - x0) * 0.5f;
        const float centerY = y0 + (y1 - y0) * 0.5f;
        constexpr float kArm = 8.0f;
        constexpr float kThickness = 1.5f;
        const float alpha = 0.9f;

        // Horizontal line as thin rounded rect.
        const float hx0 = centerX - kArm;
        const float hy0 = centerY - kThickness * 0.5f;
        const float hw = 2.0f * kArm;
        const float hh = kThickness;

        vertices[4] = EditorCompositorVertex{{hx0, hy0}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, alpha}, {hx0, hy0, hw, hh}, {0.0f, 1.0f, 0.0f, 0.0f}};
        vertices[5] = EditorCompositorVertex{{hx0 + hw, hy0}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, alpha}, {hx0, hy0, hw, hh}, {0.0f, 1.0f, 0.0f, 0.0f}};
        vertices[6] = EditorCompositorVertex{{hx0 + hw, hy0 + hh}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, alpha}, {hx0, hy0, hw, hh}, {0.0f, 1.0f, 0.0f, 0.0f}};
        vertices[7] = EditorCompositorVertex{{hx0, hy0 + hh}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, alpha}, {hx0, hy0, hw, hh}, {0.0f, 1.0f, 0.0f, 0.0f}};

        // Vertical line as thin rounded rect.
        const float vx0 = centerX - kThickness * 0.5f;
        const float vy0 = centerY - kArm;
        const float vw = kThickness;
        const float vh = 2.0f * kArm;

        vertices[8] = EditorCompositorVertex{{vx0, vy0}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, alpha}, {vx0, vy0, vw, vh}, {0.0f, 1.0f, 0.0f, 0.0f}};
        vertices[9] = EditorCompositorVertex{{vx0 + vw, vy0}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, alpha}, {vx0, vy0, vw, vh}, {0.0f, 1.0f, 0.0f, 0.0f}};
        vertices[10] = EditorCompositorVertex{{vx0 + vw, vy0 + vh}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, alpha}, {vx0, vy0, vw, vh}, {0.0f, 1.0f, 0.0f, 0.0f}};
        vertices[11] = EditorCompositorVertex{{vx0, vy0 + vh}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, alpha}, {vx0, vy0, vw, vh}, {0.0f, 1.0f, 0.0f, 0.0f}};
    }

    void* mapped = nullptr;
    vkMapMemory(m_LogicalDevice, fg.vertexMemory, 0, sizeof(vertices), 0, &mapped);
    std::memcpy(mapped, vertices, sizeof(vertices));
    vkUnmapMemory(m_LogicalDevice, fg.vertexMemory);
}

void EditorCompositor::UpdateViewportDescriptorSet(uint32_t frameIndex, VkImageView viewportColorImageView) {
    if (frameIndex >= m_MaxFramesInFlight || viewportColorImageView == VK_NULL_HANDLE) {
        return;
    }

    VkDescriptorSet set = m_TextureDescriptorSets[frameIndex];
    if (set == VK_NULL_HANDLE) {
        return;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = viewportColorImageView;
    imageInfo.sampler = m_Sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = set;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_LogicalDevice, 1, &descriptorWrite, 0, nullptr);
}

void EditorCompositor::CompositeViewport(VkCommandBuffer cmd,
                                        uint32_t frameIndex,
                                        VkImageView swapchainView,
                                        VkExtent2D swapchainExtent,
                                        VkRect2D viewportDestRect,
                                        VkImageView viewportColorImageView,
                                        bool drawCrosshair) {
    if (!cmd || swapchainView == VK_NULL_HANDLE || viewportColorImageView == VK_NULL_HANDLE) {
        return;
    }

    if (swapchainExtent.width == 0 || swapchainExtent.height == 0) {
        return;
    }

    if (viewportDestRect.extent.width == 0 || viewportDestRect.extent.height == 0) {
        return;
    }

    UpdateVertexBuffer(frameIndex, viewportDestRect, drawCrosshair);
    UpdateViewportDescriptorSet(frameIndex, viewportColorImageView);

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = swapchainView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = swapchainExtent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

    // Full viewport dynamic state; quad vertices define the actual destination region.
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D fullScissor{};
    fullScissor.offset = {0, 0};
    fullScissor.extent = swapchainExtent;
    vkCmdSetScissor(cmd, 0, 1, &fullScissor);

    // Transform pixel-space vertices to NDC in the UI2 vertex shader.
    float pushConstants[4];
    pushConstants[0] = 2.0f / static_cast<float>(swapchainExtent.width);
    pushConstants[1] = 2.0f / static_cast<float>(swapchainExtent.height);
    pushConstants[2] = -1.0f;
    pushConstants[3] = -1.0f;
    vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4, pushConstants);

    VkDescriptorSet set = m_TextureDescriptorSets[frameIndex];
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &set, 0, nullptr);

    VkBuffer vertexBuffers[] = {m_FrameGeometry[frameIndex].vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, m_FrameGeometry[frameIndex].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // Shape 0: viewport texture.
    vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

    // Shape 1-2: crosshair (optional).
    if (drawCrosshair) {
        vkCmdDrawIndexed(cmd, 6, 1, 6, 0, 0);
        vkCmdDrawIndexed(cmd, 6, 1, 12, 0, 0);
    }

    vkCmdEndRendering(cmd);
}

} // namespace we::editor::rendering

