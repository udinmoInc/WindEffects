#include "Rendering/UIRenderer.hpp"
#include "Core/DeviceContext.h"
#include "Core/ImageBarriers.h"
#include "Resource/ResourceManager.h"
#include "Shader/ShaderLibrary.h"
#include "Core/Logger.hpp"
#include "Core/Widget.hpp"
#include "Runtime/Core/AssetRegistry.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace we::UI {

UIRenderer::~UIRenderer() {
    Shutdown();
}

bool UIRenderer::Init(we::runtime::renderer::DeviceContext* context,
                      we::runtime::renderer::ResourceManager* resources,
                      VkFormat swapchainColorFormat) {
    m_Context = context;
    m_Resources = resources;
    m_ColorFormat = swapchainColorFormat;
    if (!m_Context || !m_Resources) {
        HE_ERROR("UIRenderer: DeviceContext or ResourceManager is null.");
        return false;
    }

    if (volkInitialize() != VK_SUCCESS) {
        HE_ERROR("UIRenderer: volkInitialize failed.");
        return false;
    }
    volkLoadInstance(m_Context->GetInstance());
    volkLoadDevice(m_Context->GetDevice());
    if (!vkCmdBindPipeline) {
        HE_ERROR("UIRenderer: Vulkan command function pointers were not loaded.");
        return false;
    }

    m_GpuUpload.Init(m_Context, m_Resources);

    VkDevice device = m_Context->GetDevice();

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
        HE_ERROR("UIRenderer: Failed to create UI texture descriptor set layout.");
        return false;
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 4096;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 2048;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
        HE_ERROR("UIRenderer: Failed to create descriptor pool.");
        return false;
    }

    m_FontAtlas = std::make_shared<FontAtlas>();
    const std::string uiFontPath = we::core::AssetRegistry::Get().GetFontPath("Font_UI");
    const std::string fontToLoad = uiFontPath.empty() ? "Assets/Fonts/Inter-Regular.ttf" : uiFontPath;
    if (!m_FontAtlas->Init(m_Context, m_Resources, &m_GpuUpload, fontToLoad, 32, 96, 1024, 1024)) {
        HE_ERROR("UIRenderer: Font atlas initialization failed.");
        return false;
    }

    m_IconRenderer = std::make_shared<IconRenderer>();
    if (!m_IconRenderer->Init(m_Context, m_Resources, &m_GpuUpload, m_DescriptorPool, m_TextureDescLayout)) {
        HE_ERROR("UIRenderer: Failed to initialize IconRenderer.");
    }

    CreateDummyTexture();
    m_DummyDescriptorSet = RegisterTexture(m_DummyImageView, m_DummySampler);
    if (m_DummyDescriptorSet == VK_NULL_HANDLE) {
        HE_ERROR("UIRenderer: Failed to allocate dummy texture descriptor set.");
        return false;
    }

    m_FontAtlas->GetDescriptorSetRef() = RegisterTexture(m_FontAtlas->GetImageView(), m_FontAtlas->GetSampler());
    if (m_FontAtlas->GetDescriptorSet() == VK_NULL_HANDLE) {
        HE_ERROR("UIRenderer: Failed to allocate font atlas descriptor set.");
        return false;
    }

    we::core::AssetRegistry::Get().RegisterTexture("Font_UI_Atlas", m_FontAtlas->GetImageView(), m_FontAtlas->GetSampler());
    we::core::AssetRegistry::Get().RegisterTexture("UI_DummyWhite", m_DummyImageView, m_DummySampler);

    CreatePipeline(swapchainColorFormat);

    HE_INFO("UIRenderer: GPU UI renderer initialized successfully.");
    return true;
}

void UIRenderer::CreateDummyTexture() {
    VkDevice device = m_Context->GetDevice();
    std::array<uint8_t, 4> pixel = {255, 255, 255, 255};

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    m_Resources->CreateBuffer(
        4,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    void* data = nullptr;
    vkMapMemory(device, stagingMemory, 0, 4, 0, &data);
    std::memcpy(data, pixel.data(), 4);
    vkUnmapMemory(device, stagingMemory);

    m_Resources->CreateImage(
        1, 1,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_DummyImage,
        m_DummyMemory);

    m_GpuUpload.SubmitOneTime([&](VkCommandBuffer cmd) {
        we::runtime::renderer::TransitionImageLayout(
            cmd,
            m_DummyImage,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {1, 1, 1};
        vkCmdCopyBufferToImage(cmd, stagingBuffer, m_DummyImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        we::runtime::renderer::TransitionImageLayout(
            cmd,
            m_DummyImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    });

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    m_DummyImageView = m_Resources->CreateImageView(m_DummyImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

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

void UIRenderer::CreatePipeline(VkFormat colorFormat) {
    VkDevice device = m_Context->GetDevice();

    VkShaderModule vertShaderModule = we::runtime::renderer::ShaderLibrary::Get().CreateShaderModule(
        device, "UI", we::runtime::renderer::ShaderStage::Vertex);
    VkShaderModule fragShaderModule = we::runtime::renderer::ShaderLibrary::Get().CreateShaderModule(
        device, "UI", we::runtime::renderer::ShaderStage::Pixel);

    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        throw std::runtime_error("UIRenderer: Failed to load UI shader modules.");
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "VSMain";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "PSMain";

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(UIVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
    attributeDescriptions[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UIVertex, pos)};
    attributeDescriptions[1] = {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(UIVertex, uv)};
    attributeDescriptions[2] = {0, 2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(UIVertex, color)};
    attributeDescriptions[3] = {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(UIVertex, sdfRect)};
    attributeDescriptions[4] = {0, 4, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(UIVertex, sdfParams)};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
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
    pipelineLayoutInfo.pSetLayouts = &m_TextureDescLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("UIRenderer: Failed to create UI pipeline layout.");
    }

    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &colorFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &pipelineRenderingInfo;
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
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("UIRenderer: Failed to create UI pipeline.");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

VkDescriptorSet UIRenderer::RegisterTexture(VkImageView imageView, VkSampler sampler) {
    if (!m_Context || m_DescriptorPool == VK_NULL_HANDLE || m_TextureDescLayout == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_TextureDescLayout;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
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
    if (descSet != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE && m_Context) {
        vkFreeDescriptorSets(m_Context->GetDevice(), m_DescriptorPool, 1, &descSet);
    }
}

void UIRenderer::BuildGeometry(const std::vector<DrawCommand>& commands, uint32_t width, uint32_t height) {
    m_Vertices.clear();
    m_Indices.clear();
    m_Batches.clear();

    for (const auto& cmd : commands) {
        VkDescriptorSet texSet = m_DummyDescriptorSet;
        if (cmd.type == DrawCommandType::Text) {
            if (!m_FontAtlas) {
                continue;
            }
            texSet = m_FontAtlas->GetDescriptorSet();
        } else if (cmd.type == DrawCommandType::Icon && m_IconRenderer) {
            const uint32_t iconSize = static_cast<uint32_t>(std::max(8.0f, cmd.fontSize));
            texSet = m_IconRenderer->GetLucideIcon(cmd.text, iconSize, cmd.color);
        } else if (cmd.type == DrawCommandType::Texture) {
            texSet = cmd.textureId;
        }

        if (texSet == VK_NULL_HANDLE) {
            if (cmd.type == DrawCommandType::Icon) {
                continue;
            }
            if (m_DummyDescriptorSet != VK_NULL_HANDLE) {
                texSet = m_DummyDescriptorSet;
            } else {
                continue;
            }
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
                } else if (cmd.type == DrawCommandType::Rect || cmd.type == DrawCommandType::Gradient) {
                    // Route solid UI chrome through the SDF branch so panel/titlebar
                    // rendering does not depend on the sampled dummy texture path.
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

void UIRenderer::UpdateBuffers(uint32_t frameIndex) {
    if (m_Pipeline == VK_NULL_HANDLE || !m_Context || !m_Resources) {
        return;
    }
    if (frameIndex >= static_cast<uint32_t>(we::runtime::renderer::kMaxFramesInFlight)) {
        return;
    }

    VkDevice device = m_Context->GetDevice();
    FrameGeometryBuffers& buffers = m_FrameBuffers[frameIndex];

    if (m_Vertices.empty() || m_Indices.empty()) {
        return;
    }

    VkDeviceSize vertexSize = sizeof(UIVertex) * m_Vertices.size();
    if (vertexSize > buffers.vertexBufferSize) {
        if (buffers.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffers.vertexBuffer, nullptr);
            vkFreeMemory(device, buffers.vertexMemory, nullptr);
        }
        buffers.vertexBufferSize = vertexSize * 2;
        m_Resources->CreateBuffer(
            buffers.vertexBufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            buffers.vertexBuffer,
            buffers.vertexMemory);
    }

    void* vData = nullptr;
    vkMapMemory(device, buffers.vertexMemory, 0, vertexSize, 0, &vData);
    std::memcpy(vData, m_Vertices.data(), static_cast<size_t>(vertexSize));
    vkUnmapMemory(device, buffers.vertexMemory);

    VkDeviceSize indexSize = sizeof(uint32_t) * m_Indices.size();
    if (indexSize > buffers.indexBufferSize) {
        if (buffers.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffers.indexBuffer, nullptr);
            vkFreeMemory(device, buffers.indexMemory, nullptr);
        }
        buffers.indexBufferSize = indexSize * 2;
        m_Resources->CreateBuffer(
            buffers.indexBufferSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            buffers.indexBuffer,
            buffers.indexMemory);
    }

    void* iData = nullptr;
    vkMapMemory(device, buffers.indexMemory, 0, indexSize, 0, &iData);
    std::memcpy(iData, m_Indices.data(), static_cast<size_t>(indexSize));
    vkUnmapMemory(device, buffers.indexMemory);
}

void UIRenderer::PrepareFrame(uint32_t width, uint32_t height, uint32_t frameIndex,
                              const std::shared_ptr<Widget>& root) {
    if (!root) {
        HE_ERROR("UIRenderer::PrepareFrame called with null root widget.");
        return;
    }

    // Layout then paint (update/Tick is handled by the editor main loop before this call)
    root->Measure(Size{ static_cast<float>(width), static_cast<float>(height) });
    root->Arrange(Rect{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) });

    PaintContext paintCtx;
    root->Paint(paintCtx);

    // Ensure the UI pass reaches the final presented swapchain image by drawing
    // a fullscreen red rectangle before any widget geometry.
    std::vector<DrawCommand> commands = paintCtx.GetCommands();
    {
        DrawCommand redFullscreen{};
        redFullscreen.type = DrawCommandType::Rect;
        redFullscreen.rect = Rect{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) };
        redFullscreen.color = Color{ 1.0f, 0.0f, 0.0f, 1.0f };
        redFullscreen.clipRect = redFullscreen.rect;
        redFullscreen.borderRadius = 0.0f;
        commands.insert(commands.begin(), redFullscreen);
    }

    // Generate and update GPU buffers outside any active render pass.
    BuildGeometry(commands, width, height);
    UpdateBuffers(frameIndex);

    m_LastFrameStats = {};
    m_LastFrameStats.drawCommands = static_cast<uint32_t>(commands.size());
    m_LastFrameStats.vertices = static_cast<uint32_t>(m_Vertices.size());
    m_LastFrameStats.indices = static_cast<uint32_t>(m_Indices.size());
    m_LastFrameStats.batches = static_cast<uint32_t>(m_Batches.size());
    m_LastFrameStats.width = width;
    m_LastFrameStats.height = height;

    static uint32_t s_LoggedFrames = 0;
    if (s_LoggedFrames < 5) {
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        if (!m_Vertices.empty()) {
            minX = maxX = m_Vertices[0].pos[0];
            minY = maxY = m_Vertices[0].pos[1];
            for (const auto& vertex : m_Vertices) {
                minX = std::min(minX, vertex.pos[0]);
                minY = std::min(minY, vertex.pos[1]);
                maxX = std::max(maxX, vertex.pos[0]);
                maxY = std::max(maxY, vertex.pos[1]);
            }
        }

        HE_INFO("[UIRenderer] Frame " + std::to_string(s_LoggedFrames)
            + ": layout=" + std::to_string(width) + "x" + std::to_string(height)
            + ", commands=" + std::to_string(m_LastFrameStats.drawCommands)
            + ", vertices=" + std::to_string(m_LastFrameStats.vertices)
            + ", indices=" + std::to_string(m_LastFrameStats.indices)
            + ", batches=" + std::to_string(m_LastFrameStats.batches)
            + ", bounds=(" + std::to_string(minX) + "," + std::to_string(minY)
            + ")-(" + std::to_string(maxX) + "," + std::to_string(maxY) + ")");

        const size_t sampleCount = std::min<size_t>(m_Vertices.size(), 6);
        for (size_t i = 0; i < sampleCount; ++i) {
            const auto& v = m_Vertices[i];
            HE_INFO("[UIRenderer] Vertex " + std::to_string(i)
                + ": pos=(" + std::to_string(v.pos[0]) + "," + std::to_string(v.pos[1])
                + ") uv=(" + std::to_string(v.uv[0]) + "," + std::to_string(v.uv[1])
                + ") colorA=" + std::to_string(v.color[3])
                + " sdfType=" + std::to_string(v.sdfParams[1]) + ")");
        }
        if (m_LastFrameStats.vertices == 0 || m_LastFrameStats.batches == 0) {
            HE_ERROR("[UIRenderer] Empty draw list â€” UI geometry was not generated.");
        }
        ++s_LoggedFrames;
    }
}

void UIRenderer::RecordDrawCommands(VkCommandBuffer cmd,
                                    VkImageView targetView,
                                    VkFormat targetFormat,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t frameIndex) {
    (void)targetFormat;

    static uint32_t s_RecordLogFrames = 0;

    if (cmd == VK_NULL_HANDLE) {
        HE_ERROR("UIRenderer::RecordDrawCommands: command buffer is null.");
        return;
    }
    if (!m_Context) {
        HE_ERROR("UIRenderer::RecordDrawCommands: Vulkan context is null.");
        return;
    }
    if (!vkCmdBindPipeline) {
        volkLoadDevice(m_Context->GetDevice());
    }
    if (!vkCmdBindPipeline) {
        HE_ERROR("UIRenderer::RecordDrawCommands: Vulkan command function pointers are not loaded.");
        return;
    }
    if (m_Pipeline == VK_NULL_HANDLE || m_PipelineLayout == VK_NULL_HANDLE) {
        if (s_RecordLogFrames < 3) {
            HE_ERROR("UIRenderer::RecordDrawCommands: pipeline or layout is null.");
        }
        return;
    }
    if (frameIndex >= static_cast<uint32_t>(we::runtime::renderer::kMaxFramesInFlight)) {
        HE_ERROR("UIRenderer::RecordDrawCommands: frame index out of range.");
        return;
    }
    if (m_Vertices.empty() || m_Batches.empty()) {
        if (s_RecordLogFrames < 3) {
            HE_ERROR("UIRenderer::RecordDrawCommands: no vertices or batches to draw.");
        }
        return;
    }

    const FrameGeometryBuffers& buffers = m_FrameBuffers[frameIndex];
    if (buffers.vertexBuffer == VK_NULL_HANDLE || buffers.indexBuffer == VK_NULL_HANDLE) {
        if (s_RecordLogFrames < 3) {
            HE_ERROR("UIRenderer::RecordDrawCommands: missing GPU buffers for current frame.");
        }
        return;
    }

    if (s_RecordLogFrames < 3) {
        HE_INFO("[UIRenderer] RecordDrawCommands: frame="
            + std::to_string(frameIndex)
            + " extent=" + std::to_string(width) + "x" + std::to_string(height)
            + " batches=" + std::to_string(m_Batches.size())
            + " vertices=" + std::to_string(m_Vertices.size())
            + " indices=" + std::to_string(m_Indices.size()));
    }

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = targetView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = {width, height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D fullScissor{};
    fullScissor.offset = {0, 0};
    fullScissor.extent = {width, height};
    vkCmdSetScissor(cmd, 0, 1, &fullScissor);

    float pushConstants[4];
    pushConstants[0] = 2.0f / static_cast<float>(width);
    pushConstants[1] = 2.0f / static_cast<float>(height);
    pushConstants[2] = -1.0f;
    pushConstants[3] = -1.0f;
    vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4, pushConstants);

    VkBuffer vertexBuffers[] = {buffers.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    uint32_t batchIndex = 0;
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

        VkDescriptorSet textureSet = batch.textureSet;
        if (textureSet == VK_NULL_HANDLE) {
            textureSet = m_DummyDescriptorSet;
        }
        if (textureSet == VK_NULL_HANDLE) {
            continue;
        }

        if (s_RecordLogFrames < 3 && batchIndex < 8) {
            HE_INFO("[UIRenderer] Batch "
                + std::to_string(batchIndex)
                + ": firstIndex=" + std::to_string(batch.firstIndex)
                + " indexCount=" + std::to_string(batch.indexCount)
                + " scissor=(" + std::to_string(fullScissor.offset.x) + "," + std::to_string(fullScissor.offset.y)
                + " " + std::to_string(fullScissor.extent.width) + "x" + std::to_string(fullScissor.extent.height) + ")");
        }

        // Bind Texture descriptor set
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &textureSet, 0, nullptr);

        // Render Indexed Triangles
        vkCmdDrawIndexed(cmd, batch.indexCount, 1, batch.firstIndex, batch.vertexOffset, 0);
        ++batchIndex;
    }

    vkCmdEndRendering(cmd);

    if (s_RecordLogFrames < 3) {
        ++s_RecordLogFrames;
    }
}

void UIRenderer::Render(VkCommandBuffer cmd,
                        VkImageView targetView,
                        VkFormat targetFormat,
                        uint32_t width,
                        uint32_t height,
                        uint32_t frameIndex,
                        const std::shared_ptr<Widget>& root) {
    PrepareFrame(width, height, frameIndex, root);
    RecordDrawCommands(cmd, targetView, targetFormat, width, height, frameIndex);
}

void UIRenderer::Shutdown() {
    if (m_IconRenderer) {
        m_IconRenderer->Shutdown();
        m_IconRenderer.reset();
    }

    if (!m_Context) {
        m_GpuUpload.Shutdown();
        return;
    }
    VkDevice device = m_Context->GetDevice();

    for (auto& buffers : m_FrameBuffers) {
        if (buffers.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffers.vertexBuffer, nullptr);
            buffers.vertexBuffer = VK_NULL_HANDLE;
        }
        if (buffers.vertexMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, buffers.vertexMemory, nullptr);
            buffers.vertexMemory = VK_NULL_HANDLE;
        }
        if (buffers.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffers.indexBuffer, nullptr);
            buffers.indexBuffer = VK_NULL_HANDLE;
        }
        if (buffers.indexMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, buffers.indexMemory, nullptr);
            buffers.indexMemory = VK_NULL_HANDLE;
        }
        buffers.vertexBufferSize = 0;
        buffers.indexBufferSize = 0;
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
    if (m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }

    m_GpuUpload.Shutdown();
    m_Context = nullptr;
    m_Resources = nullptr;
}

} // namespace we::editor::application::UI
