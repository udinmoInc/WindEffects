#include "Rendering/UIRenderer.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Renderer/ShaderHelper.hpp"
#include "Core/Logger.hpp"
#include "Core/Widget.hpp"
#include "Runtime/Core/AssetRegistry.hpp"
#include <array>
#include <cmath>

namespace we::UI {

UIRenderer::~UIRenderer() {
    Shutdown();
}

bool UIRenderer::Init(const std::shared_ptr<we::runtime::renderer::VulkanContext>& context, VkRenderPass renderPass) {
    m_Context = context;
    VkDevice device = m_Context->GetDevice();

    // Initialize volk for this DLL's context
    volkInitialize();
    volkLoadInstance(m_Context->GetInstance());
    volkLoadDevice(device);


    HE_INFO("UIRenderer: Initializing UI descriptor layouts...");
    // 1. Create Descriptor Set Layout for combined image sampler
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_TextureDescLayout) != VK_SUCCESS) {
        HE_ERROR("UIRenderer: Failed to create UI texture descriptor set layout!");
        return false;
    }

    // 2. Initialize Font Atlas
    m_FontAtlas = std::make_shared<FontAtlas>();
    const std::string uiFontPath = we::core::AssetRegistry::Get().GetFontPath("Font_UI");
    const std::string fontToLoad = uiFontPath.empty() ? "Assets/Fonts/Inter-Regular.ttf" : uiFontPath;
    if (!m_FontAtlas->Init(m_Context, fontToLoad, 32, 96, 1024, 1024)) {
        throw std::runtime_error("Failed to initialize HouseUI Renderer!");
    }

    m_IconRenderer = std::make_shared<IconRenderer>();
    if (!m_IconRenderer->Init(m_Context, m_TextureDescLayout)) {
        HE_ERROR("UIRenderer: Failed to initialize IconRenderer.");
    }

    // 3. Create Dummy White Texture
    CreateDummyTexture();

    // 4. Create font descriptor set
    
    // Register textures for Font and Dummy
    m_DummyDescriptorSet = RegisterTexture(m_DummyImageView, m_DummySampler);
    // Allocate Font descriptor set
    VkDescriptorSetAllocateInfo fontAllocInfo{};
    fontAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    fontAllocInfo.descriptorPool = m_Context->GetDescriptorPool();
    fontAllocInfo.descriptorSetCount = 1;
    fontAllocInfo.pSetLayouts = &m_TextureDescLayout;

    if (vkAllocateDescriptorSets(device, &fontAllocInfo, &m_FontAtlas->GetDescriptorSetRef()) == VK_SUCCESS) {
        VkDescriptorImageInfo fontImageInfo{};
        fontImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        fontImageInfo.imageView = m_FontAtlas->GetImageView();
        fontImageInfo.sampler = m_FontAtlas->GetSampler();

        VkWriteDescriptorSet fontWrite{};
        fontWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        fontWrite.dstSet = m_FontAtlas->GetDescriptorSetRef();
        fontWrite.dstBinding = 0;
        fontWrite.dstArrayElement = 0;
        fontWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        fontWrite.descriptorCount = 1;
        fontWrite.pImageInfo = &fontImageInfo;

        vkUpdateDescriptorSets(device, 1, &fontWrite, 0, nullptr);
    } else {
        // Fallback: register via manual layout
        m_FontAtlas->GetDescriptorSetRef() = RegisterTexture(m_FontAtlas->GetImageView(), m_FontAtlas->GetSampler());
    }

    we::core::AssetRegistry::Get().RegisterTexture("Font_UI_Atlas", m_FontAtlas->GetImageView(), m_FontAtlas->GetSampler());
    we::core::AssetRegistry::Get().RegisterTexture("UI_DummyWhite", m_DummyImageView, m_DummySampler);

    // 5. Create pipeline
    CreatePipeline(renderPass);

    HE_INFO("UIRenderer: Custom Slate-like UI renderer initialized successfully.");
    return true;
}

void UIRenderer::CreateDummyTexture() {
    VkDevice device = m_Context->GetDevice();

    // 1x1 white pixel data
    std::array<uint8_t, 4> pixel = { 255, 255, 255, 255 };

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    m_Context->CreateBuffer(
        4,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, 4, 0, &data);
    memcpy(data, pixel.data(), 4);
    vkUnmapMemory(device, stagingBufferMemory);

    m_Context->CreateImage(
        1, 1,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_DummyImage,
        m_DummyMemory
    );

    m_Context->TransitionImageLayout(
        m_DummyImage,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    VkCommandBuffer cmd = m_Context->BeginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = { 1, 1, 1 };

    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_DummyImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    m_Context->EndSingleTimeCommands(cmd);

    m_Context->TransitionImageLayout(
        m_DummyImage,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    m_DummyImageView = m_Context->CreateImageView(m_DummyImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    vkCreateSampler(device, &samplerInfo, nullptr, &m_DummySampler);
}

void UIRenderer::CreatePipeline(VkRenderPass renderPass) {
    VkDevice device = m_Context->GetDevice();

    std::vector<char> vertCode = we::runtime::renderer::LoadShaderBytecode("UI", we::runtime::renderer::ShaderStage::Vertex);
    std::vector<char> fragCode = we::runtime::renderer::LoadShaderBytecode("UI", we::runtime::renderer::ShaderStage::Pixel);

    VkShaderModule vertShaderModule = we::runtime::renderer::CreateShaderModule(device, vertCode);
    VkShaderModule fragShaderModule = we::runtime::renderer::CreateShaderModule(device, fragCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = we::runtime::renderer::ShaderStageEntryPoint(we::runtime::renderer::ShaderStage::Vertex);

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = we::runtime::renderer::ShaderStageEntryPoint(we::runtime::renderer::ShaderStage::Pixel);

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

    // Vertex attributes
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(UIVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

    // Position (vec2)
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(UIVertex, pos);

    // UV (vec2)
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(UIVertex, uv);

    // Color (vec4)
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(UIVertex, color);

    // SDF Rect (vec4)
    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(UIVertex, sdfRect);

    // SDF Params (vec4)
    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(UIVertex, sdfParams);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 5;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Push constant range (Scale & Translate)
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 4;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_TextureDescLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create UI pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
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

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create UI pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

VkDescriptorSet UIRenderer::RegisterTexture(VkImageView imageView, VkSampler sampler) {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Context->GetDescriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_TextureDescLayout;

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(m_Context->GetDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &descriptorWrite, 0, nullptr);

    return descriptorSet;
}

void UIRenderer::UpdateTexture(VkDescriptorSet descriptorSet, VkImageView imageView, VkSampler sampler) {
    if (descriptorSet == VK_NULL_HANDLE || !m_Context) {
        return;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &descriptorWrite, 0, nullptr);
}

void UIRenderer::UnregisterTexture(VkDescriptorSet descSet) {
    if (descSet != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(m_Context->GetDevice(), m_Context->GetDescriptorPool(), 1, &descSet);
    }
}

void UIRenderer::BuildGeometry(const std::vector<DrawCommand>& commands, uint32_t width, uint32_t height) {
    m_Vertices.clear();
    m_Indices.clear();
    m_Batches.clear();

    for (const auto& cmd : commands) {
        VkDescriptorSet texSet = m_DummyDescriptorSet;
        if (cmd.type == DrawCommandType::Text) {
            texSet = m_FontAtlas->GetDescriptorSet();
        } else if (cmd.type == DrawCommandType::Icon && m_IconRenderer) {
            const uint32_t iconSize = static_cast<uint32_t>(std::max(8.0f, cmd.fontSize));
            texSet = m_IconRenderer->GetLucideIcon(cmd.text, iconSize, cmd.color);
        } else if (cmd.type == DrawCommandType::Texture) {
            texSet = cmd.textureId;
        }

        if (cmd.type == DrawCommandType::Icon && texSet == VK_NULL_HANDLE) {
            continue;
        }

        uint32_t startIndex = static_cast<uint32_t>(m_Vertices.size());
        uint32_t cmdIndexCount = 0;

        if (cmd.type == DrawCommandType::Rect || cmd.type == DrawCommandType::Texture || cmd.type == DrawCommandType::Icon || cmd.type == DrawCommandType::Gradient || cmd.type == DrawCommandType::Shadow || cmd.type == DrawCommandType::RoundedOutline) {
            float x = cmd.rect.x;
            float y = cmd.rect.y;
            float w = cmd.rect.width;
            float h = cmd.rect.height;

            if (cmd.type == DrawCommandType::Shadow) {
                // Approximate soft shadow using multiple expanded semi-transparent rects
                const int numLayers = 4;
                float shadowSpread = cmd.blur / numLayers;
                float baseAlpha = cmd.color.a / (numLayers * 1.5f);
                
                for (int i = 0; i < numLayers; ++i) {
                    float expand = (i + 1) * shadowSpread;
                    float sx = x - expand;
                    float sy = y - expand;
                    float sw = w + expand * 2.0f;
                    float sh = h + expand * 2.0f;
                    float alpha = baseAlpha * (1.0f - (float)i / numLayers);
                    
                    float type = 1.0f; // SDF Rect
                    float r = cmd.borderRadius + expand; // Increase corner radius for outer layers
                    
                    UIVertex v0{ {sx,      sy},      {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, alpha}, {sx, sy, sw, sh}, {r, type, 0.0f, 0.0f} };
                    UIVertex v1{ {sx + sw, sy},      {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, alpha}, {sx, sy, sw, sh}, {r, type, 0.0f, 0.0f} };
                    UIVertex v2{ {sx + sw, sy + sh}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, alpha}, {sx, sy, sw, sh}, {r, type, 0.0f, 0.0f} };
                    UIVertex v3{ {sx,      sy + sh}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, alpha}, {sx, sy, sw, sh}, {r, type, 0.0f, 0.0f} };

                    uint32_t currentStartIndex = static_cast<uint32_t>(m_Vertices.size());
                    m_Vertices.push_back(v0); m_Vertices.push_back(v1); m_Vertices.push_back(v2); m_Vertices.push_back(v3);
                    m_Indices.push_back(currentStartIndex + 0); m_Indices.push_back(currentStartIndex + 1); m_Indices.push_back(currentStartIndex + 2);
                    m_Indices.push_back(currentStartIndex + 2); m_Indices.push_back(currentStartIndex + 3); m_Indices.push_back(currentStartIndex + 0);
                    
                    cmdIndexCount += 6;
                }
            } else {
                float type = 0.0f;
                if (cmd.type == DrawCommandType::RoundedOutline) {
                    type = 2.0f;
                } else if ((cmd.type == DrawCommandType::Rect || cmd.type == DrawCommandType::Gradient) && cmd.borderRadius > 0.0f) {
                    type = 1.0f;
                }

                const float outlineThickness = (cmd.type == DrawCommandType::RoundedOutline) ? cmd.thickness : 0.0f;
                
                Color colorTop = cmd.color;
                Color colorBottom = (cmd.type == DrawCommandType::Gradient || cmd.type == DrawCommandType::Texture) ? cmd.colorBottom : cmd.color;
                if (cmd.type == DrawCommandType::Icon) {
                    colorTop = Color::White();
                    colorBottom = Color::White();
                }

                UIVertex v0{ {x,     y},     {0.0f, 0.0f}, {colorTop.r, colorTop.g, colorTop.b, colorTop.a},       {x, y, w, h}, {cmd.borderRadius, type, outlineThickness, 0.0f} };
                UIVertex v1{ {x + w, y},     {1.0f, 0.0f}, {colorTop.r, colorTop.g, colorTop.b, colorTop.a},       {x, y, w, h}, {cmd.borderRadius, type, outlineThickness, 0.0f} };
                UIVertex v2{ {x + w, y + h}, {1.0f, 1.0f}, {colorBottom.r, colorBottom.g, colorBottom.b, colorBottom.a}, {x, y, w, h}, {cmd.borderRadius, type, outlineThickness, 0.0f} };
                UIVertex v3{ {x,     y + h}, {0.0f, 1.0f}, {colorBottom.r, colorBottom.g, colorBottom.b, colorBottom.a}, {x, y, w, h}, {cmd.borderRadius, type, outlineThickness, 0.0f} };

                // If it's a flat rect or gradient, map to center of dummy texture
                if (cmd.type == DrawCommandType::Rect || cmd.type == DrawCommandType::Gradient || cmd.type == DrawCommandType::RoundedOutline) {
                    v0.uv[0] = 0.5f; v0.uv[1] = 0.5f;
                    v1.uv[0] = 0.5f; v1.uv[1] = 0.5f;
                    v2.uv[0] = 0.5f; v2.uv[1] = 0.5f;
                    v3.uv[0] = 0.5f; v3.uv[1] = 0.5f;
                }

                m_Vertices.push_back(v0);
                m_Vertices.push_back(v1);
                m_Vertices.push_back(v2);
                m_Vertices.push_back(v3);

                m_Indices.push_back(startIndex + 0);
                m_Indices.push_back(startIndex + 1);
                m_Indices.push_back(startIndex + 2);
                m_Indices.push_back(startIndex + 2);
                m_Indices.push_back(startIndex + 3);
                m_Indices.push_back(startIndex + 0);

                cmdIndexCount = 6;
            }

        } else if (cmd.type == DrawCommandType::Line) {
            float dx = cmd.lineEnd.x - cmd.lineStart.x;
            float dy = cmd.lineEnd.y - cmd.lineStart.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len > 0.0f) {
                dx /= len;
                dy /= len;
                float px = -dy * (cmd.thickness * 0.5f);
                float py = dx * (cmd.thickness * 0.5f);

                float sx = cmd.lineStart.x;
                float sy = cmd.lineStart.y;
                float sw = dx * len;
                float sh = dy * len;
                
                UIVertex v0{ {cmd.lineStart.x + px, cmd.lineStart.y + py}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {sx, sy, sw, sh}, {0.0f, 0.0f, 0.0f, 0.0f} };
                UIVertex v1{ {cmd.lineStart.x - px, cmd.lineStart.y - py}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {sx, sy, sw, sh}, {0.0f, 0.0f, 0.0f, 0.0f} };
                UIVertex v2{ {cmd.lineEnd.x - px,   cmd.lineEnd.y - py},   {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {sx, sy, sw, sh}, {0.0f, 0.0f, 0.0f, 0.0f} };
                UIVertex v3{ {cmd.lineEnd.x + px,   cmd.lineEnd.y + py},   {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {sx, sy, sw, sh}, {0.0f, 0.0f, 0.0f, 0.0f} };

                m_Vertices.push_back(v0);
                m_Vertices.push_back(v1);
                m_Vertices.push_back(v2);
                m_Vertices.push_back(v3);

                m_Indices.push_back(startIndex + 0);
                m_Indices.push_back(startIndex + 1);
                m_Indices.push_back(startIndex + 2);
                m_Indices.push_back(startIndex + 2);
                m_Indices.push_back(startIndex + 3);
                m_Indices.push_back(startIndex + 0);

                cmdIndexCount = 6;
            }

        } else if (cmd.type == DrawCommandType::Text) {
            float xpos = cmd.rect.x;
            float ypos = cmd.rect.y;

            float scale = 1.0f;
            if (m_FontAtlas && m_FontAtlas->GetFontHeight() > 0.0f) {
                scale = cmd.fontSize / m_FontAtlas->GetFontHeight();
            }
            float logicalX = 0.0f;
            float logicalY = 0.0f;
            float startX = xpos;
            float startY = ypos + cmd.fontSize * 0.85f;

            for (char c : cmd.text) {
                GlyphInfo q;
                if (m_FontAtlas && m_FontAtlas->GetCharQuad(c, &logicalX, &logicalY, q)) {
                    uint32_t charStart = static_cast<uint32_t>(m_Vertices.size());

                    float px0 = startX + q.x0 * scale;
                    float py0 = startY + q.y0 * scale;
                    float px1 = startX + q.x1 * scale;
                    float py1 = startY + q.y1 * scale;
                    float w = px1 - px0;
                    float h = py1 - py0;

                    UIVertex v0{ {px0, py0}, {q.u0, q.v0}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {px0, py0, w, h}, {0.0f, 0.0f, 0.0f, 0.0f} };
                    UIVertex v1{ {px1, py0}, {q.u1, q.v0}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {px0, py0, w, h}, {0.0f, 0.0f, 0.0f, 0.0f} };
                    UIVertex v2{ {px1, py1}, {q.u1, q.v1}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {px0, py0, w, h}, {0.0f, 0.0f, 0.0f, 0.0f} };
                    UIVertex v3{ {px0, py1}, {q.u0, q.v1}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {px0, py0, w, h}, {0.0f, 0.0f, 0.0f, 0.0f} };

                    m_Vertices.push_back(v0);
                    m_Vertices.push_back(v1);
                    m_Vertices.push_back(v2);
                    m_Vertices.push_back(v3);

                    m_Indices.push_back(charStart + 0);
                    m_Indices.push_back(charStart + 1);
                    m_Indices.push_back(charStart + 2);
                    m_Indices.push_back(charStart + 2);
                    m_Indices.push_back(charStart + 3);
                    m_Indices.push_back(charStart + 0);

                    cmdIndexCount += 6;
                }
            }
        }

        if (cmdIndexCount > 0) {
            // Apply Scissor Clipping boundary checking
            Rect scissorRect = cmd.clipRect;
            if (scissorRect.x < 0.0f) scissorRect.x = 0.0f;
            if (scissorRect.y < 0.0f) scissorRect.y = 0.0f;
            if (scissorRect.width > width) scissorRect.width = static_cast<float>(width);
            if (scissorRect.height > height) scissorRect.height = static_cast<float>(height);

            // Group into dynamic draw batches
            if (!m_Batches.empty() && 
                m_Batches.back().textureSet == texSet &&
                std::abs(m_Batches.back().scissor.x - scissorRect.x) < 0.01f &&
                std::abs(m_Batches.back().scissor.y - scissorRect.y) < 0.01f &&
                std::abs(m_Batches.back().scissor.width - scissorRect.width) < 0.01f &&
                std::abs(m_Batches.back().scissor.height - scissorRect.height) < 0.01f) {
                
                m_Batches.back().indexCount += cmdIndexCount;
            } else {
                DrawBatch batch{};
                batch.indexCount = cmdIndexCount;
                batch.firstIndex = static_cast<uint32_t>(m_Indices.size()) - cmdIndexCount;
                batch.vertexOffset = 0; // Relative offsets
                batch.textureSet = texSet;
                batch.scissor = scissorRect;
                m_Batches.push_back(batch);
            }
        }
    }
}

void UIRenderer::UpdateBuffers() {
    VkDevice device = m_Context->GetDevice();

    if (m_Vertices.empty()) return;

    // 1. Vertex Buffer
    VkDeviceSize vertexSize = sizeof(UIVertex) * m_Vertices.size();
    if (vertexSize > m_VertexBufferSize) {
        if (m_VertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_VertexBuffer, nullptr);
            vkFreeMemory(device, m_VertexMemory, nullptr);
        }
        m_VertexBufferSize = vertexSize * 2; // Allocate double for padding
        m_Context->CreateBuffer(
            m_VertexBufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_VertexBuffer,
            m_VertexMemory
        );
    }

    void* vData;
    vkMapMemory(device, m_VertexMemory, 0, vertexSize, 0, &vData);
    memcpy(vData, m_Vertices.data(), vertexSize);
    vkUnmapMemory(device, m_VertexMemory);

    // 2. Index Buffer
    VkDeviceSize indexSize = sizeof(uint32_t) * m_Indices.size();
    if (indexSize > m_IndexBufferSize) {
        if (m_IndexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_IndexBuffer, nullptr);
            vkFreeMemory(device, m_IndexMemory, nullptr);
        }
        m_IndexBufferSize = indexSize * 2;
        m_Context->CreateBuffer(
            m_IndexBufferSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_IndexBuffer,
            m_IndexMemory
        );
    }

    void* iData;
    vkMapMemory(device, m_IndexMemory, 0, indexSize, 0, &iData);
    memcpy(iData, m_Indices.data(), indexSize);
    vkUnmapMemory(device, m_IndexMemory);
}

void UIRenderer::Render(VkCommandBuffer cmd, uint32_t width, uint32_t height, const std::shared_ptr<Widget>& root) {
    if (!root) {
        HE_ERROR("UIRenderer::Render called with null root widget.");
        return;
    }

    // 1. Layout then paint (update/Tick is handled by the editor main loop before this call)
    root->Measure(Size{ static_cast<float>(width), static_cast<float>(height) });
    root->Arrange(Rect{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) });

    PaintContext paintCtx;
    root->Paint(paintCtx);

    const auto& commands = paintCtx.GetCommands();

    // 2. Generate and update GPU buffers
    BuildGeometry(commands, width, height);
    UpdateBuffers();

    m_LastFrameStats = {};
    m_LastFrameStats.drawCommands = static_cast<uint32_t>(commands.size());
    m_LastFrameStats.vertices = static_cast<uint32_t>(m_Vertices.size());
    m_LastFrameStats.indices = static_cast<uint32_t>(m_Indices.size());
    m_LastFrameStats.batches = static_cast<uint32_t>(m_Batches.size());
    m_LastFrameStats.width = width;
    m_LastFrameStats.height = height;

    static uint32_t s_LoggedFrames = 0;
    if (s_LoggedFrames < 5) {
        HE_INFO("[UIRenderer] Frame " + std::to_string(s_LoggedFrames)
            + ": layout=" + std::to_string(width) + "x" + std::to_string(height)
            + ", commands=" + std::to_string(m_LastFrameStats.drawCommands)
            + ", vertices=" + std::to_string(m_LastFrameStats.vertices)
            + ", indices=" + std::to_string(m_LastFrameStats.indices)
            + ", batches=" + std::to_string(m_LastFrameStats.batches));
        if (m_LastFrameStats.vertices == 0 || m_LastFrameStats.batches == 0) {
            HE_ERROR("[UIRenderer] Empty draw list — UI geometry was not generated.");
        }
        ++s_LoggedFrames;
    }

    // 3. Always bind pipeline and set dynamic state even if no geometry.
    //    An early return here would leave the active render pass open,
    //    causing vkQueueSubmit to fail with VK_ERROR_DEVICE_LOST.
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    // Viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    // Full-screen default scissor
    VkRect2D fullScissor{};
    fullScissor.offset = { 0, 0 };
    fullScissor.extent = { width, height };
    vkCmdSetScissor(cmd, 0, 1, &fullScissor);

    // Push Constants (Scale & Translate to NDC)
    float pushConstants[4];
    pushConstants[0] = 2.0f / static_cast<float>(width);
    pushConstants[1] = 2.0f / static_cast<float>(height);
    pushConstants[2] = -1.0f;
    pushConstants[3] = -1.0f;
    vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4, pushConstants);

    if (m_Vertices.empty() || m_Batches.empty()) return;

    // Bind Vertex & Index buffers
    VkBuffer vertexBuffers[] = { m_VertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // 4. Draw Batches
    for (const auto& batch : m_Batches) {
        // Apply Scissor clip rectangle
        VkRect2D scissor{};
        scissor.offset.x = static_cast<int32_t>(batch.scissor.x);
        scissor.offset.y = static_cast<int32_t>(batch.scissor.y);
        scissor.extent.width = static_cast<uint32_t>(batch.scissor.width);
        scissor.extent.height = static_cast<uint32_t>(batch.scissor.height);

        // Sanity clamp scissor to viewport bounds
        if (scissor.offset.x < 0) scissor.offset.x = 0;
        if (scissor.offset.y < 0) scissor.offset.y = 0;
        if (scissor.offset.x + static_cast<int32_t>(scissor.extent.width) > static_cast<int32_t>(width)) {
            int32_t diff = static_cast<int32_t>(width) - scissor.offset.x;
            scissor.extent.width = (diff > 0) ? static_cast<uint32_t>(diff) : 0u;
        }
        if (scissor.offset.y + static_cast<int32_t>(scissor.extent.height) > static_cast<int32_t>(height)) {
            int32_t diff = static_cast<int32_t>(height) - scissor.offset.y;
            scissor.extent.height = (diff > 0) ? static_cast<uint32_t>(diff) : 0u;
        }

        // Skip degenerate batches
        if (scissor.extent.width == 0 || scissor.extent.height == 0) continue;

        vkCmdSetScissor(cmd, 0, 1, &scissor);

        // Bind Texture descriptor set
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &batch.textureSet, 0, nullptr);

        // Render Indexed Triangles
        vkCmdDrawIndexed(cmd, batch.indexCount, 1, batch.firstIndex, batch.vertexOffset, 0);
    }
}

void UIRenderer::Shutdown() {
    if (m_IconRenderer) {
        m_IconRenderer->Shutdown();
        m_IconRenderer.reset();
    }

    if (!m_Context) return;
    VkDevice device = m_Context->GetDevice();

    if (m_VertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_VertexBuffer, nullptr);
        m_VertexBuffer = VK_NULL_HANDLE;
    }
    if (m_VertexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_VertexMemory, nullptr);
        m_VertexMemory = VK_NULL_HANDLE;
    }
    if (m_IndexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_IndexBuffer, nullptr);
        m_IndexBuffer = VK_NULL_HANDLE;
    }
    if (m_IndexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_IndexMemory, nullptr);
        m_IndexMemory = VK_NULL_HANDLE;
    }

    if (m_DummySampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_DummySampler, nullptr);
        m_DummySampler = VK_NULL_HANDLE;
    }
    if (m_DummyImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_DummyImageView, nullptr);
        m_DummyImageView = VK_NULL_HANDLE;
    }
    if (m_DummyImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_DummyImage, nullptr);
        m_DummyImage = VK_NULL_HANDLE;
    }
    if (m_DummyMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_DummyMemory, nullptr);
        m_DummyMemory = VK_NULL_HANDLE;
    }

    if (m_FontAtlas) {
        m_FontAtlas->Shutdown();
        m_FontAtlas.reset();
    }

    if (m_Pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }
    if (m_PipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }
    if (m_TextureDescLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_TextureDescLayout, nullptr);
        m_TextureDescLayout = VK_NULL_HANDLE;
    }
}

} // namespace we::editor::application::UI
