#include "Renderer/SceneRenderer.hpp"
#include "Renderer/RenderGpuInvestigator.hpp"
#include "Renderer/PostProcessStack.hpp"
#include "Renderer/RenderDiagnostics.hpp"
#include "Renderer/FrameStats.hpp"
#include "Renderer/RendererConfig.hpp"
#include "Renderer/RendererDebug.hpp"
#include "Renderer/RenderPipelineInvestigator.hpp"
#include "Renderer/AtmosphereLUTInputs.hpp"
#include "Renderer/RenderForensics.hpp"
#include "Core/DiagnosticMacros.hpp"
#include "Core/LogCategory.hpp"
#include "Renderer/ShaderHelper.hpp"
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#if WE_HAS_GLM
#include <glm/geometric.hpp>
#endif
#include <cstring>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

namespace {

VkRenderPass CreateColorOnlyRenderPass(VkDevice device, VkFormat format, VkAttachmentLoadOp loadOp) {
    VkAttachmentDescription color{};
    color.format = format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = loadOp;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &color;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return renderPass;
}

void CreateCloudImage(
    const VulkanContext& context,
    uint32_t width,
    uint32_t height,
    VkImage& image,
    VkDeviceMemory& memory,
    VkImageView& view) {
    context.CreateImage(
        width,
        height,
        kOffscreenColorFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        image,
        memory);
    view = context.CreateImageView(image, kOffscreenColorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

void UpdateCloudHistoryDescriptorSet(
    VkDevice device,
    VkDescriptorSet set,
    VkSampler sampler,
    VkImageView scratchView,
    VkImageView historyView) {
    VkDescriptorImageInfo scratchInfo{};
    scratchInfo.sampler = sampler;
    scratchInfo.imageView = scratchView;
    scratchInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo historyInfo{};
    historyInfo.sampler = sampler;
    historyInfo.imageView = historyView;
    historyInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::array<VkWriteDescriptorSet, 2> writes{};
    for (uint32_t i = 0; i < writes.size(); ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = set;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].descriptorCount = 1;
    }
    writes[0].pImageInfo = &scratchInfo;
    writes[1].pImageInfo = &historyInfo;
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void UpdateCloudCompositeDescriptorSet(VkDevice device, VkDescriptorSet set, VkSampler sampler, VkImageView view) {
    VkDescriptorImageInfo info{};
    info.sampler = sampler;
    info.imageView = view;
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &info;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

} // namespace

SceneRenderer::SceneRenderer(
    const std::shared_ptr<VulkanContext>& context,
    VkRenderPass renderPass,
    VkRenderPass renderPassLoad,
    VkDescriptorSetLayout cameraDescLayout)
    : m_Context(context),
      m_CameraDescLayout(cameraDescLayout),
      m_OffscreenRenderPass(renderPass),
      m_OffscreenRenderPassLoad(renderPassLoad) {
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

    m_LUTGenerator = nullptr;
    m_PostProcess = nullptr;
    if (RendererDebug::Get().ShouldCreateAtmosphereResources()) {
        m_LUTGenerator = std::make_unique<AtmosphereLUTGenerator>(m_Context);
    }
    if (RendererDebug::Get().ShouldCreatePostProcessResources()) {
        m_PostProcess = std::make_unique<PostProcessStack>(m_Context);
    }

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

    std::array<VkDescriptorSetLayoutBinding, 2> cloudHistoryBindings{};
    for (uint32_t i = 0; i < cloudHistoryBindings.size(); ++i) {
        cloudHistoryBindings[i].binding = i;
        cloudHistoryBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cloudHistoryBindings[i].descriptorCount = 1;
        cloudHistoryBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkDescriptorSetLayoutCreateInfo cloudHistoryLayoutInfo{};
    cloudHistoryLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    cloudHistoryLayoutInfo.bindingCount = static_cast<uint32_t>(cloudHistoryBindings.size());
    cloudHistoryLayoutInfo.pBindings = cloudHistoryBindings.data();
    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &cloudHistoryLayoutInfo, nullptr, &m_CloudHistoryDescLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create cloud history descriptor set layout!");
    }

    VkDescriptorSetAllocateInfo cloudHistoryAllocInfo{};
    cloudHistoryAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    cloudHistoryAllocInfo.descriptorPool = m_Context->GetDescriptorPool();
    cloudHistoryAllocInfo.descriptorSetCount = 1;
    cloudHistoryAllocInfo.pSetLayouts = &m_CloudHistoryDescLayout;
    if (vkAllocateDescriptorSets(m_Context->GetDevice(), &cloudHistoryAllocInfo, &m_CloudHistoryDescSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate cloud history descriptor set!");
    }

    VkDescriptorSetLayoutBinding cloudCompositeBinding{};
    cloudCompositeBinding.binding = 0;
    cloudCompositeBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    cloudCompositeBinding.descriptorCount = 1;
    cloudCompositeBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo cloudCompositeLayoutInfo{};
    cloudCompositeLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    cloudCompositeLayoutInfo.bindingCount = 1;
    cloudCompositeLayoutInfo.pBindings = &cloudCompositeBinding;
    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &cloudCompositeLayoutInfo, nullptr, &m_CloudCompositeDescLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create cloud composite descriptor set layout!");
    }

    VkDescriptorSetAllocateInfo cloudCompositeAllocInfo{};
    cloudCompositeAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    cloudCompositeAllocInfo.descriptorPool = m_Context->GetDescriptorPool();
    cloudCompositeAllocInfo.descriptorSetCount = 1;
    cloudCompositeAllocInfo.pSetLayouts = &m_CloudCompositeDescLayout;
    if (vkAllocateDescriptorSets(m_Context->GetDevice(), &cloudCompositeAllocInfo, &m_CloudCompositeDescSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate cloud composite descriptor set!");
    }

    VkSamplerCreateInfo cloudSamplerInfo{};
    cloudSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    cloudSamplerInfo.magFilter = VK_FILTER_LINEAR;
    cloudSamplerInfo.minFilter = VK_FILTER_LINEAR;
    cloudSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    cloudSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    cloudSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    cloudSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    cloudSamplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(m_Context->GetDevice(), &cloudSamplerInfo, nullptr, &m_CloudSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create cloud sampler!");
    }

    CreatePipelines(renderPass);
    ValidateCreatedPipelines();
    CreateMeshes();
}

SceneRenderer::~SceneRenderer() {
    VkDevice device = m_Context->GetDevice();
    DestroyMeshes();

    m_PostProcess.reset();
    DestroyCloudTemporalResources();
    if (m_CloudSampler != VK_NULL_HANDLE) vkDestroySampler(device, m_CloudSampler, nullptr);
    if (m_CloudCompositePipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_CloudCompositePipeline, nullptr);
    if (m_CloudTemporalPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_CloudTemporalPipeline, nullptr);
    if (m_SkyAtmospherePipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_SkyAtmospherePipeline, nullptr);
    if (m_VolumetricCloudsPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_VolumetricCloudsPipeline, nullptr);
    if (m_FogCompositePipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_FogCompositePipeline, nullptr);
    if (m_LitPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_LitPipeline, nullptr);
    if (m_UnlitPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_UnlitPipeline, nullptr);
    if (m_WireframePipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_WireframePipeline, nullptr);

    if (m_CloudCompositePipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_CloudCompositePipelineLayout, nullptr);
    if (m_CloudPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_CloudPipelineLayout, nullptr);
    if (m_FogPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_FogPipelineLayout, nullptr);
    if (m_EnvironmentPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_EnvironmentPipelineLayout, nullptr);
    if (m_PipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);

    m_LUTGenerator.reset();

    if (m_CloudCompositeDescLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_CloudCompositeDescLayout, nullptr);
    if (m_CloudHistoryDescLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_CloudHistoryDescLayout, nullptr);

    if (m_EnvironmentBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, m_EnvironmentBuffer, nullptr);
    if (m_EnvironmentBufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, m_EnvironmentBufferMemory, nullptr);
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

    m_SkyAtmospherePipeline = CreateFullscreenPipeline(renderPass, m_EnvironmentPipelineLayout, "AtmospherePass", false, VK_COMPARE_OP_ALWAYS, false, false);
    m_VolumetricCloudsPipeline = CreateFullscreenPipeline(renderPass, m_EnvironmentPipelineLayout, "VolumetricCloudsPass", false, VK_COMPARE_OP_ALWAYS, false, false);

    std::array<VkDescriptorSetLayout, 3> cloudSetLayouts = {
        m_EnvironmentDescLayout,
        m_CameraDescLayout,
        m_CloudHistoryDescLayout
    };
    VkPipelineLayoutCreateInfo cloudLayoutInfo{};
    cloudLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    cloudLayoutInfo.setLayoutCount = static_cast<uint32_t>(cloudSetLayouts.size());
    cloudLayoutInfo.pSetLayouts = cloudSetLayouts.data();
    if (vkCreatePipelineLayout(device, &cloudLayoutInfo, nullptr, &m_CloudPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create cloud pipeline layout!");
    }

    VkPipelineLayoutCreateInfo cloudCompositeLayoutInfo{};
    cloudCompositeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    cloudCompositeLayoutInfo.setLayoutCount = 1;
    cloudCompositeLayoutInfo.pSetLayouts = &m_CloudCompositeDescLayout;
    if (vkCreatePipelineLayout(device, &cloudCompositeLayoutInfo, nullptr, &m_CloudCompositePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create cloud composite pipeline layout!");
    }

    m_CloudTemporalPipeline = CreateFullscreenPipeline(renderPass, m_CloudPipelineLayout, "CloudTemporalResolve", false, VK_COMPARE_OP_ALWAYS, false, false);
    m_CloudCompositePipeline = CreateFullscreenPipeline(renderPass, m_CloudCompositePipelineLayout, "CloudCompositePass", false, VK_COMPARE_OP_ALWAYS, false, true);
    m_FogCompositePipeline = CreateFullscreenPipeline(renderPass, m_FogPipelineLayout, "FogCompositePass", true, VK_COMPARE_OP_GREATER, false, true);

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
    diag.ValidatePipeline("SceneObjectLit", m_LitPipeline);
}

bool SceneRenderer::ValidateRenderFrame(VkFramebuffer framebuffer, uint32_t width, uint32_t height) const {
    m_LastOffscreenFramebuffer = framebuffer;
    m_LastOffscreenWidth = width;
    m_LastOffscreenHeight = height;
    auto& diag = RenderDiagnostics::Get();
    diag.ResetFrame();
    diag.ValidateFramebuffer(framebuffer, width, height);
    if (RendererDebug::Get().ShouldUploadEnvironmentUniform()) {
        diag.ValidateEnvironmentUniform(m_SceneEnvironment);
    }
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
    if (!RendererDebug::Get().IsFeatureEnabled(MinimalRendererStage::SkyAtmosphere)) {
        return;
    }

    auto& diag = RenderDiagnostics::Get();
    auto& stats = FrameStatsCollector::Get();

    RefreshEnvironmentDescriptorBindings();
    if (!m_LUTGenerator) {
        stats.SetAtmosphereLutReady(false);
        stats.SetPassStatus("AtmosphereLUT", "failed");
        diag.RecordPassStatus("AtmosphereLUT", PassExecutionStatus::Failed, "LUT generator is null");
        diag.Emit(
            DiagnosticSeverity::Error,
            LogCategory::Environment.data(),
            "Atmosphere LUT generator was not created.",
            "Verify SceneRenderer constructed AtmosphereLUTGenerator successfully.");
        return;
    }

    const auto lutStart = std::chrono::steady_clock::now();
    const bool generated = m_LUTGenerator->EnsureGenerated(m_SceneEnvironment);
    stats.RecordPassMs("AtmosphereLUT",
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - lutStart).count());

    if (!generated) {
        stats.SetAtmosphereLutReady(false);
        stats.SetPassStatus("AtmosphereLUT", "failed");
        diag.RecordPassStatus("AtmosphereLUT", PassExecutionStatus::Failed, m_LUTGenerator->GetLastError().c_str());
        diag.ValidateAtmosphereLUTs(false, 0, 0);
        return;
    }

    const auto& dims = m_LUTGenerator->GetDimensions();
    const auto& images = m_LUTGenerator->GetImages();
    constexpr VkFormat kLUTFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

    diag.ValidateAtmosphereLUTs(true, dims.transmittanceW, dims.skyViewW);
    diag.ValidateAtmosphereLUTResource("Transmittance", images.transmittance, images.transmittanceView, images.sampler, dims.transmittanceW, dims.transmittanceH, kLUTFormat);
    diag.ValidateAtmosphereLUTResource("MultiScattering", images.multiScatter, images.multiScatterView, images.sampler, dims.multiScatterSize, dims.multiScatterSize, kLUTFormat);
    diag.ValidateAtmosphereLUTResource("SkyView", images.skyView, images.skyViewView, images.sampler, dims.skyViewW, dims.skyViewH, kLUTFormat);
    diag.ValidateAtmosphereLUTResource("AerialPerspective", images.aerial, images.aerialView, images.sampler, dims.aerialSize, dims.aerialSize, kLUTFormat);
    diag.ValidateDescriptorSet("SkyAtmosphere", 2, m_LUTGenerator->GetSampleSet());

    std::string validationError;
    const bool resourcesValid = m_LUTGenerator->ValidateResources(&validationError);
    stats.SetAtmosphereLutReady(resourcesValid);
    stats.SetSkyPipelineValid(m_SkyAtmospherePipeline != VK_NULL_HANDLE);
    stats.SetPostPipelineValid(m_PostProcess && m_PostProcess->IsReady());

    if (!resourcesValid) {
        stats.SetPassStatus("AtmosphereLUT", "failed");
        diag.RecordPassStatus("AtmosphereLUT", PassExecutionStatus::Failed, validationError.c_str());
        diag.Emit(
            DiagnosticSeverity::Error,
            LogCategory::Environment.data(),
            validationError,
            "Sky Atmosphere pass will be skipped until all LUT resources are valid.");
        return;
    }

    stats.SetPassStatus("AtmosphereLUT", "executed");
    diag.RecordPassStatus("AtmosphereLUT", PassExecutionStatus::Executed);
}

bool SceneRenderer::AreAtmosphereLUTsReady() const {
    if (!RendererDebug::Get().IsFeatureEnabled(MinimalRendererStage::SkyAtmosphere)) {
        return true;
    }
    return m_LUTGenerator && m_LUTGenerator->IsReady();
}

void SceneRenderer::LogAtmospherePipelineDiagnostics() const {
    auto logStage = [](const char* name, bool pass, const char* failReason, const char* file, int line) {
        if (pass) {
            WE_LOG_INFO(LogCategory::Environment.data(), std::string("PASS - ") + name);
        } else {
            WE_LOG_ERROR(
                LogCategory::Environment.data(),
                std::string("FAIL - ") + name + ": " + failReason + " (" + file + ":" + std::to_string(line) + ")");
        }
    };

    const bool generatorCreated = m_LUTGenerator != nullptr;
    logStage("Atmosphere initialized", generatorCreated, "AtmosphereLUTGenerator was not created", __FILE__, __LINE__);

    bool lutGenerationExecuted = false;
    bool transmittanceReady = false;
    bool skyViewReady = false;
    bool multiScatterReady = false;
    bool aerialReady = false;
    bool gpuImagesCreated = false;
    bool imageViewsCreated = false;
    bool samplersCreated = false;
    bool descriptorUpdated = false;

    if (m_LUTGenerator) {
        lutGenerationExecuted = m_LUTGenerator->IsReady();
        const auto& images = m_LUTGenerator->GetImages();
        transmittanceReady = images.transmittanceView != VK_NULL_HANDLE;
        multiScatterReady = images.multiScatterView != VK_NULL_HANDLE;
        skyViewReady = images.skyViewView != VK_NULL_HANDLE;
        aerialReady = images.aerialView != VK_NULL_HANDLE;
        gpuImagesCreated = images.transmittance != VK_NULL_HANDLE
            && images.multiScatter != VK_NULL_HANDLE
            && images.skyView != VK_NULL_HANDLE
            && images.aerial != VK_NULL_HANDLE;
        imageViewsCreated = transmittanceReady && multiScatterReady && skyViewReady && aerialReady;
        samplersCreated = images.sampler != VK_NULL_HANDLE;
        descriptorUpdated = m_LUTGenerator->GetSampleSet() != VK_NULL_HANDLE && lutGenerationExecuted;
    }

    logStage("LUT generation executed", lutGenerationExecuted, m_LUTGenerator ? m_LUTGenerator->GetLastError().c_str() : "generator null", __FILE__, __LINE__);
    logStage("Transmittance ready", transmittanceReady, "transmittance image view is null", "AtmosphereLUTGenerator.cpp", 162);
    logStage("SkyView ready", skyViewReady, "sky-view image view is null", "AtmosphereLUTGenerator.cpp", 164);
    logStage("MultiScatter ready", multiScatterReady, "multi-scatter image view is null", "AtmosphereLUTGenerator.cpp", 163);
    logStage("AerialPerspective ready", aerialReady, "aerial-perspective image view is null", "AtmosphereLUTGenerator.cpp", 165);
    logStage("GPU images created", gpuImagesCreated, "one or more LUT VkImage handles are null", "AtmosphereLUTGenerator.cpp", 161);
    logStage("Image views created", imageViewsCreated, "one or more LUT VkImageView handles are null", "AtmosphereLUTGenerator.cpp", 158);
    logStage("Samplers created", samplersCreated, "LUT sampler is null", "AtmosphereLUTGenerator.cpp", 174);
    logStage("Descriptor sets updated", descriptorUpdated, "LUT sample descriptor set is null or LUTs not ready", "AtmosphereLUTGenerator.cpp", 315);

    const bool pipelineCreated = m_SkyAtmospherePipeline != VK_NULL_HANDLE;
    logStage("Pipeline created", pipelineCreated, "SkyAtmosphere pipeline handle is null", "SceneRenderer.cpp", 224);

    const bool skyExecuted = RenderDiagnostics::Get().GetPassStatus("SkyAtmosphere") == PassExecutionStatus::Executed;
    const bool cloudsExecuted = RenderDiagnostics::Get().GetPassStatus("VolumetricClouds") == PassExecutionStatus::Executed;
    const bool fogExecuted = RenderDiagnostics::Get().GetPassStatus("FogComposite") == PassExecutionStatus::Executed;
    const auto& stats = FrameStatsCollector::Get().GetStats();

    logStage("Descriptor sets bound", skyExecuted, stats.skyPassStatus.c_str(), "SceneRenderer.cpp", 706);
    logStage("SkyAtmosphere pass executed", skyExecuted, stats.skyPassStatus.c_str(), "SceneRenderer.cpp", 706);
    logStage("Clouds executed", cloudsExecuted, stats.cloudsPassStatus.c_str(), "SceneRenderer.cpp", 736);
    logStage("Fog executed", fogExecuted, stats.fogPassStatus.c_str(), "SceneRenderer.cpp", 770);
    logStage("Tonemap executed", stats.postPassStatus == "executed", stats.postPassStatus.c_str(), "SceneRenderer.cpp", 870);
}

void SceneRenderer::LogAtmospherePipelineDiagnosticsOnChange() const {
    auto fingerprintStage = [](uint64_t& fingerprint, bool pass) {
        fingerprint = (fingerprint << 1) | (pass ? 1ull : 0ull);
    };

    uint64_t fingerprint = 0;
    const bool generatorCreated = m_LUTGenerator != nullptr;
    fingerprintStage(fingerprint, generatorCreated);

    bool lutGenerationExecuted = false;
    bool transmittanceReady = false;
    bool skyViewReady = false;
    bool multiScatterReady = false;
    bool aerialReady = false;
    bool gpuImagesCreated = false;
    bool imageViewsCreated = false;
    bool samplersCreated = false;
    bool descriptorUpdated = false;

    if (m_LUTGenerator) {
        lutGenerationExecuted = m_LUTGenerator->IsReady();
        const auto& images = m_LUTGenerator->GetImages();
        transmittanceReady = images.transmittanceView != VK_NULL_HANDLE;
        multiScatterReady = images.multiScatterView != VK_NULL_HANDLE;
        skyViewReady = images.skyViewView != VK_NULL_HANDLE;
        aerialReady = images.aerialView != VK_NULL_HANDLE;
        gpuImagesCreated = images.transmittance != VK_NULL_HANDLE
            && images.multiScatter != VK_NULL_HANDLE
            && images.skyView != VK_NULL_HANDLE
            && images.aerial != VK_NULL_HANDLE;
        imageViewsCreated = transmittanceReady && multiScatterReady && skyViewReady && aerialReady;
        samplersCreated = images.sampler != VK_NULL_HANDLE;
        descriptorUpdated = m_LUTGenerator->GetSampleSet() != VK_NULL_HANDLE && lutGenerationExecuted;
    }

    fingerprintStage(fingerprint, lutGenerationExecuted);
    fingerprintStage(fingerprint, transmittanceReady);
    fingerprintStage(fingerprint, skyViewReady);
    fingerprintStage(fingerprint, multiScatterReady);
    fingerprintStage(fingerprint, aerialReady);
    fingerprintStage(fingerprint, gpuImagesCreated);
    fingerprintStage(fingerprint, imageViewsCreated);
    fingerprintStage(fingerprint, samplersCreated);
    fingerprintStage(fingerprint, descriptorUpdated);
    fingerprintStage(fingerprint, m_SkyAtmospherePipeline != VK_NULL_HANDLE);

    const bool skyExecuted = RenderDiagnostics::Get().GetPassStatus("SkyAtmosphere") == PassExecutionStatus::Executed;
    const bool cloudsExecuted = RenderDiagnostics::Get().GetPassStatus("VolumetricClouds") == PassExecutionStatus::Executed;
    const bool fogExecuted = RenderDiagnostics::Get().GetPassStatus("FogComposite") == PassExecutionStatus::Executed;
    const auto& stats = FrameStatsCollector::Get().GetStats();

    fingerprintStage(fingerprint, skyExecuted);
    fingerprintStage(fingerprint, cloudsExecuted);
    fingerprintStage(fingerprint, fogExecuted);
    fingerprintStage(fingerprint, stats.postPassStatus == "executed");

    if (m_AtmosphereDiagInitialized && fingerprint == m_LastAtmosphereDiagFingerprint) {
        return;
    }

    m_LastAtmosphereDiagFingerprint = fingerprint;
    m_AtmosphereDiagInitialized = true;
    LogAtmospherePipelineDiagnostics();
}

void SceneRenderer::DestroyCloudTemporalResources() {
    VkDevice device = m_Context->GetDevice();
    if (m_CloudScratchFramebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device, m_CloudScratchFramebuffer, nullptr);
        m_CloudScratchFramebuffer = VK_NULL_HANDLE;
    }
    for (VkFramebuffer& framebuffer : m_CloudHistoryFramebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
            framebuffer = VK_NULL_HANDLE;
        }
    }
    if (m_CloudScratchRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, m_CloudScratchRenderPass, nullptr);
        m_CloudScratchRenderPass = VK_NULL_HANDLE;
    }
    if (m_CloudHistoryRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, m_CloudHistoryRenderPass, nullptr);
        m_CloudHistoryRenderPass = VK_NULL_HANDLE;
    }
    if (m_CloudScratchView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_CloudScratchView, nullptr);
        m_CloudScratchView = VK_NULL_HANDLE;
    }
    if (m_CloudScratchImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_CloudScratchImage, nullptr);
        m_CloudScratchImage = VK_NULL_HANDLE;
    }
    if (m_CloudScratchMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_CloudScratchMemory, nullptr);
        m_CloudScratchMemory = VK_NULL_HANDLE;
    }
    for (size_t i = 0; i < m_CloudHistoryImages.size(); ++i) {
        if (m_CloudHistoryViews[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(device, m_CloudHistoryViews[i], nullptr);
            m_CloudHistoryViews[i] = VK_NULL_HANDLE;
        }
        if (m_CloudHistoryImages[i] != VK_NULL_HANDLE) {
            vkDestroyImage(device, m_CloudHistoryImages[i], nullptr);
            m_CloudHistoryImages[i] = VK_NULL_HANDLE;
        }
        if (m_CloudHistoryMemories[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, m_CloudHistoryMemories[i], nullptr);
            m_CloudHistoryMemories[i] = VK_NULL_HANDLE;
        }
    }
    m_CloudTemporalWidth = 0;
    m_CloudTemporalHeight = 0;
    m_CloudHistoryWriteIndex = 0;
    m_CloudHistoryGpuValid = false;
    m_CloudTemporalInitialized = false;
}

void SceneRenderer::EnsureCloudTemporalResources(uint32_t width, uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);
    if (width == m_CloudTemporalWidth && height == m_CloudTemporalHeight && m_CloudScratchImage != VK_NULL_HANDLE) {
        return;
    }

    if (m_CloudScratchImage != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_Context->GetDevice());
    }
    DestroyCloudTemporalResources();
    m_CloudTemporalWidth = width;
    m_CloudTemporalHeight = height;

    VkDevice device = m_Context->GetDevice();
    CreateCloudImage(*m_Context, width, height, m_CloudScratchImage, m_CloudScratchMemory, m_CloudScratchView);
    for (size_t i = 0; i < m_CloudHistoryImages.size(); ++i) {
        CreateCloudImage(*m_Context, width, height, m_CloudHistoryImages[i], m_CloudHistoryMemories[i], m_CloudHistoryViews[i]);
    }

    m_CloudScratchRenderPass = CreateColorOnlyRenderPass(device, kOffscreenColorFormat, VK_ATTACHMENT_LOAD_OP_CLEAR);
    m_CloudHistoryRenderPass = CreateColorOnlyRenderPass(device, kOffscreenColorFormat, VK_ATTACHMENT_LOAD_OP_DONT_CARE);

    VkFramebufferCreateInfo scratchFbInfo{};
    scratchFbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    scratchFbInfo.renderPass = m_CloudScratchRenderPass;
    scratchFbInfo.attachmentCount = 1;
    scratchFbInfo.pAttachments = &m_CloudScratchView;
    scratchFbInfo.width = width;
    scratchFbInfo.height = height;
    scratchFbInfo.layers = 1;
    if (vkCreateFramebuffer(device, &scratchFbInfo, nullptr, &m_CloudScratchFramebuffer) != VK_SUCCESS) {
        HE_ERROR("Failed to create cloud scratch framebuffer.");
        return;
    }

    for (size_t i = 0; i < m_CloudHistoryFramebuffers.size(); ++i) {
        VkFramebufferCreateInfo historyFbInfo = scratchFbInfo;
        historyFbInfo.renderPass = m_CloudHistoryRenderPass;
        historyFbInfo.pAttachments = &m_CloudHistoryViews[i];
        if (vkCreateFramebuffer(device, &historyFbInfo, nullptr, &m_CloudHistoryFramebuffers[i]) != VK_SUCCESS) {
            HE_ERROR("Failed to create cloud history framebuffer.");
            return;
        }
    }

    m_CloudTemporalInitialized = true;
}

void SceneRenderer::UploadCloudTemporalEnvironmentFlags() const {
    void* data = nullptr;
    vkMapMemory(m_Context->GetDevice(), m_EnvironmentBufferMemory, 0, sizeof(SceneEnvironmentUniform), 0, &data);
    std::memcpy(data, &m_SceneEnvironment, sizeof(SceneEnvironmentUniform));
    vkUnmapMemory(m_Context->GetDevice(), m_EnvironmentBufferMemory);
    const_cast<SceneRenderer*>(this)->m_SceneEnvironmentDirty = false;
}

void SceneRenderer::InvalidateCloudTemporalHistory() {
    m_CloudHistoryGpuValid = false;
    m_CloudTemporalInitialized = false;
    m_SceneEnvironment.cloudHistoryValid = 0;
    m_SceneEnvironmentDirty = true;
    UploadCloudTemporalEnvironmentFlags();
}

void SceneRenderer::UpdateCloudTemporalState(
    const glm::vec3& cameraPos,
    const glm::vec3& cameraForward,
    const glm::mat4& /*view*/,
    const glm::mat4& /*proj*/) {

    constexpr float kPositionResetMeters = 5.0f;
    constexpr float kViewDotResetThreshold = 0.9659f; // ~15 degrees

    bool resetHistory = !m_CloudTemporalInitialized || !m_CloudHistoryGpuValid;
    if (m_CloudTemporalInitialized) {
        const float positionDelta = glm::length(cameraPos - m_PrevCloudCameraPos);
        const glm::vec3 prevForward = glm::normalize(m_PrevCloudCameraForward);
        const glm::vec3 nextForward = glm::normalize(cameraForward);
        const float viewAlignment = glm::dot(prevForward, nextForward);
        if (positionDelta > kPositionResetMeters || viewAlignment < kViewDotResetThreshold) {
            resetHistory = true;
        }
    }

    if (resetHistory) {
        m_CloudHistoryGpuValid = false;
        m_SceneEnvironment.cloudHistoryValid = 0;
    } else {
        m_SceneEnvironment.cloudHistoryValid = 1;
    }

    m_PrevCloudCameraPos = cameraPos;
    m_PrevCloudCameraForward = cameraForward;
    m_SceneEnvironmentDirty = true;
    UploadCloudTemporalEnvironmentFlags();
}

void SceneRenderer::DrawSkyAtmospherePass(VkCommandBuffer cmd, VkDescriptorSet cameraDescSet) const {
    if (!RendererDebug::Get().IsFeatureEnabled(MinimalRendererStage::SkyAtmosphere)) {
        return;
    }

    auto& diag = RenderDiagnostics::Get();
    auto& stats = FrameStatsCollector::Get();
    GpuDebugScope debugScope(cmd, "SkyAtmosphere");

    if (m_SkyAtmospherePipeline == VK_NULL_HANDLE) {
        stats.SetPassStatus("SkyAtmosphere", "failed");
        diag.RecordPassStatus("SkyAtmosphere", PassExecutionStatus::Failed, "pipeline is null");
        diag.ValidatePipeline("SkyAtmosphere", m_SkyAtmospherePipeline);
        return;
    }

    const bool validationActive = m_SceneEnvironment.atmosphereDebugMode != 0;

    if (!validationActive) {
        if (!m_LUTGenerator || !m_LUTGenerator->IsReady()) {
            const_cast<SceneRenderer*>(this)->PrepareAtmosphereLUTs(cmd);
        }

        if (!m_LUTGenerator || !m_LUTGenerator->IsReady()) {
            stats.SetPassStatus("SkyAtmosphere", "skipped");
            const char* reason = m_LUTGenerator ? m_LUTGenerator->GetLastError().c_str() : "LUT generator is null";
            if (reason == nullptr || reason[0] == '\0') {
                reason = "required atmosphere LUTs are missing";
            }
            diag.RecordPassStatus("SkyAtmosphere", PassExecutionStatus::Skipped, reason);
            diag.Emit(
                DiagnosticSeverity::Error,
                LogCategory::Environment.data(),
                std::string("Sky Atmosphere pass aborted: ") + reason,
                "Fix atmosphere LUT generation before rendering the sky.");
            return;
        }
    } else if (!m_LUTGenerator || !m_LUTGenerator->IsReady()) {
        const_cast<SceneRenderer*>(this)->PrepareAtmosphereLUTs(cmd);
    }

    const_cast<SceneRenderer*>(this)->RefreshEnvironmentDescriptorBindings();
    RenderDiagnostics::Get().ValidateDescriptorSet("SkyAtmosphere", 0, m_EnvironmentDescSet);
    RenderDiagnostics::Get().ValidateDescriptorSet("SkyAtmosphere", 1, cameraDescSet);
    RenderDiagnostics::Get().ValidateDescriptorSet("SkyAtmosphere", 2, m_LUTGenerator->GetSampleSet());

    if (RenderForensics::Get().IsActive()) {
        ForensicPassMetadata meta{};
        meta.outputTarget = "OffscreenColor";
        meta.pipelineName = "SkyAtmosphere";
        meta.vertexShader = "AtmospherePass_VS";
        meta.pixelShader = "AtmospherePass_PS";
        meta.descriptorSets = "environment(0),camera(1),atmosphereLUTs(2)";
        meta.depthState = "ALWAYS";
        meta.blendState = "REPLACE";
        meta.boundTextures = "transmittance,multiScatter,skyView,aerialPerspective LUTs";
        meta.sourceFile = __FILE__;
        meta.sourceLine = __LINE__;
        RenderForensics::Get().RecordPassMetadata(ForensicPassId::SkyAtmosphere, meta);
    }

    if (m_EnvironmentDescSet == VK_NULL_HANDLE || cameraDescSet == VK_NULL_HANDLE || m_LUTGenerator->GetSampleSet() == VK_NULL_HANDLE) {
        stats.SetPassStatus("SkyAtmosphere", "failed");
        diag.RecordPassStatus("SkyAtmosphere", PassExecutionStatus::Failed, "descriptor set binding is null");
        return;
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyAtmospherePipeline);
    VkDescriptorSet sets[] = { m_EnvironmentDescSet, cameraDescSet, m_LUTGenerator->GetSampleSet() };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_EnvironmentPipelineLayout, 0, 3, sets, 0, nullptr);
    vkCmdDraw(cmd, 6, 1, 0, 0);
    FrameStatsCollector::Get().AddDrawCall(2);
    stats.SetPassStatus("SkyAtmosphere", "executed");
    diag.RecordPassStatus("SkyAtmosphere", PassExecutionStatus::Executed);
}

void SceneRenderer::DrawVolumetricCloudsPass(VkCommandBuffer cmd, VkDescriptorSet cameraDescSet) const {
    if (!RendererDebug::Get().IsFeatureEnabled(MinimalRendererStage::VolumetricClouds)) {
        return;
    }

    auto& diag = RenderDiagnostics::Get();
    auto& stats = FrameStatsCollector::Get();
    GpuDebugScope debugScope(cmd, "VolumetricClouds");

    if (m_VolumetricCloudsPipeline == VK_NULL_HANDLE || m_CloudTemporalPipeline == VK_NULL_HANDLE || m_CloudCompositePipeline == VK_NULL_HANDLE) {
        stats.SetPassStatus("VolumetricClouds", "failed");
        diag.RecordPassStatus("VolumetricClouds", PassExecutionStatus::Failed, "pipeline is null");
        return;
    }
    if (m_SceneEnvironment.enableClouds < 0.5f) {
        stats.SetPassStatus("VolumetricClouds", "skipped");
        diag.RecordPassStatus("VolumetricClouds", PassExecutionStatus::Skipped, "clouds disabled in environment settings");
        return;
    }
    if (!AreAtmosphereLUTsReady()) {
        stats.SetPassStatus("VolumetricClouds", "skipped");
        diag.RecordPassStatus("VolumetricClouds", PassExecutionStatus::Skipped, "atmosphere LUTs not ready");
        return;
    }
    if (m_LastOffscreenFramebuffer == VK_NULL_HANDLE || m_LastOffscreenWidth == 0 || m_LastOffscreenHeight == 0) {
        stats.SetPassStatus("VolumetricClouds", "failed");
        diag.RecordPassStatus("VolumetricClouds", PassExecutionStatus::Failed, "offscreen framebuffer is not ready");
        return;
    }

    auto* mutableThis = const_cast<SceneRenderer*>(this);
    mutableThis->EnsureCloudTemporalResources(m_LastOffscreenWidth, m_LastOffscreenHeight);
    if (m_CloudScratchFramebuffer == VK_NULL_HANDLE || m_CloudScratchRenderPass == VK_NULL_HANDLE) {
        stats.SetPassStatus("VolumetricClouds", "failed");
        diag.RecordPassStatus("VolumetricClouds", PassExecutionStatus::Failed, "cloud temporal resources are not ready");
        return;
    }

    const_cast<SceneRenderer*>(this)->RefreshEnvironmentDescriptorBindings();

    const uint32_t historyReadIndex = m_CloudHistoryWriteIndex;
    const uint32_t historyWriteIndex = 1u - m_CloudHistoryWriteIndex;
    const VkDevice device = m_Context->GetDevice();

    UpdateCloudHistoryDescriptorSet(
        device,
        m_CloudHistoryDescSet,
        m_CloudSampler,
        m_CloudScratchView,
        m_CloudHistoryViews[historyReadIndex]);
    UpdateCloudCompositeDescriptorSet(
        device,
        m_CloudCompositeDescSet,
        m_CloudSampler,
        m_CloudHistoryViews[historyWriteIndex]);

    const VkViewport viewport{
        0.0f,
        0.0f,
        static_cast<float>(m_LastOffscreenWidth),
        static_cast<float>(m_LastOffscreenHeight),
        0.0f,
        1.0f
    };
    const VkRect2D scissor{{0, 0}, {m_LastOffscreenWidth, m_LastOffscreenHeight}};

    vkCmdEndRenderPass(cmd);

    VkClearValue clearValue{};
    clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

    VkRenderPassBeginInfo scratchPassInfo{};
    scratchPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    scratchPassInfo.renderPass = m_CloudScratchRenderPass;
    scratchPassInfo.framebuffer = m_CloudScratchFramebuffer;
    scratchPassInfo.renderArea.offset = {0, 0};
    scratchPassInfo.renderArea.extent = {m_LastOffscreenWidth, m_LastOffscreenHeight};
    scratchPassInfo.clearValueCount = 1;
    scratchPassInfo.pClearValues = &clearValue;
    vkCmdBeginRenderPass(cmd, &scratchPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VolumetricCloudsPipeline);
    VkDescriptorSet raymarchSets[] = {m_EnvironmentDescSet, cameraDescSet, m_LUTGenerator->GetSampleSet()};
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_EnvironmentPipelineLayout, 0, 3, raymarchSets, 0, nullptr);
    vkCmdDraw(cmd, 6, 1, 0, 0);
    vkCmdEndRenderPass(cmd);

    VkRenderPassBeginInfo historyPassInfo = scratchPassInfo;
    historyPassInfo.renderPass = m_CloudHistoryRenderPass;
    historyPassInfo.framebuffer = m_CloudHistoryFramebuffers[historyWriteIndex];
    historyPassInfo.clearValueCount = 0;
    historyPassInfo.pClearValues = nullptr;
    vkCmdBeginRenderPass(cmd, &historyPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CloudTemporalPipeline);
    VkDescriptorSet temporalSets[] = {m_EnvironmentDescSet, cameraDescSet, m_CloudHistoryDescSet};
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CloudPipelineLayout, 0, 3, temporalSets, 0, nullptr);
    vkCmdDraw(cmd, 6, 1, 0, 0);
    vkCmdEndRenderPass(cmd);

    VkRenderPassBeginInfo offscreenPassInfo{};
    offscreenPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    offscreenPassInfo.renderPass = m_OffscreenRenderPassLoad;
    offscreenPassInfo.framebuffer = m_LastOffscreenFramebuffer;
    offscreenPassInfo.renderArea.offset = {0, 0};
    offscreenPassInfo.renderArea.extent = {m_LastOffscreenWidth, m_LastOffscreenHeight};
    vkCmdBeginRenderPass(cmd, &offscreenPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CloudCompositePipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CloudCompositePipelineLayout, 0, 1, &m_CloudCompositeDescSet, 0, nullptr);
    vkCmdDraw(cmd, 6, 1, 0, 0);

    mutableThis->m_CloudHistoryWriteIndex = historyWriteIndex;
    mutableThis->m_CloudHistoryGpuValid = true;

    stats.SetPassStatus("VolumetricClouds", "executed");
    diag.RecordPassStatus("VolumetricClouds", PassExecutionStatus::Executed);
}

void SceneRenderer::DrawFogCompositePass(
    VkCommandBuffer cmd,
    VkDescriptorSet cameraDescSet,
    VkImageView depthImageView,
    VkSampler sampler) const {

    if (!RendererDebug::Get().IsFeatureEnabled(MinimalRendererStage::Fog)) {
        return;
    }

    auto& diag = RenderDiagnostics::Get();
    auto& stats = FrameStatsCollector::Get();

    if (m_FogCompositePipeline == VK_NULL_HANDLE) {
        stats.SetPassStatus("FogComposite", "failed");
        diag.RecordPassStatus("FogComposite", PassExecutionStatus::Failed, "pipeline is null");
        return;
    }
    if (m_SceneEnvironment.enableVolumetricFog < 0.5f) {
        stats.SetPassStatus("FogComposite", "skipped");
        diag.RecordPassStatus("FogComposite", PassExecutionStatus::Skipped, "volumetric fog disabled in environment settings");
        diag.Emit(
            DiagnosticSeverity::Warning,
            LogCategory::Environment.data(),
            "Fog Composite pass skipped: volumetric fog disabled in environment settings.",
            "Enable volumetric fog on ExponentialHeightFog or set EnableVolumetricFog=true in function.ini.");
        return;
    }
    if (!m_LUTGenerator || !m_LUTGenerator->IsReady()) {
        stats.SetPassStatus("FogComposite", "skipped");
        diag.RecordPassStatus("FogComposite", PassExecutionStatus::Skipped, "atmosphere LUTs not ready");
        return;
    }

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
    if (RenderForensics::Get().IsActive()) {
        ForensicPassMetadata meta{};
        meta.inputTarget = "OffscreenColor+Depth";
        meta.outputTarget = "OffscreenColor";
        meta.pipelineName = "FogComposite";
        meta.vertexShader = "FogCompositePass_VS";
        meta.pixelShader = "FogCompositePass_PS";
        meta.descriptorSets = "environment(0),camera(1),depth+aerial+skyLUT(2)";
        meta.depthState = "GREATER reverse-Z";
        meta.blendState = "ALPHA_BLEND";
        meta.boundTextures = "depth,aerialPerspectiveLUT,skyViewLUT";
        meta.sourceFile = __FILE__;
        meta.sourceLine = __LINE__;
        RenderForensics::Get().RecordPassMetadata(ForensicPassId::Fog, meta);
    }
    VkDescriptorSet sets[] = { m_EnvironmentDescSet, cameraDescSet, m_FogLutDescSet };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_FogPipelineLayout, 0, 3, sets, 0, nullptr);
    vkCmdDraw(cmd, 6, 1, 0, 0);
    stats.SetPassStatus("FogComposite", "executed");
    diag.RecordPassStatus("FogComposite", PassExecutionStatus::Executed);
}

void SceneRenderer::ResizePostProcess(uint32_t width, uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);
    if (m_PostProcess) {
        m_PostProcess->Resize(width, height);
    }
    // Cloud images are shared across in-flight frames. Tearing them down every frame
    // (even when the viewport size is unchanged) races the GPU and causes DEVICE_LOST.
    if ((width != m_CloudTemporalWidth || height != m_CloudTemporalHeight) && m_CloudScratchImage != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_Context->GetDevice());
        DestroyCloudTemporalResources();
    }
}

void SceneRenderer::ApplyPostExposure(
    VkCommandBuffer cmd,
    VkImage colorImage,
    VkImageView colorImageView,
    uint32_t width,
    uint32_t height) const {

    if (!RenderPipelineInvestigator::Get().ShouldRunPostProcess()) {
        return;
    }

    if (!m_PostProcess) {
        return;
    }

    GpuDebugScope debugScope(cmd, "PostProcess");
    RefreshEnvironmentDescriptorBindings();
    m_PostProcess->Apply(cmd, m_EnvironmentDescSet, colorImage, colorImageView, width, height);
}

void SceneRenderer::FlushPostProcessReadbacks() const {
    if (m_PostProcess) {
        m_PostProcess->FlushGpuAverageLuminance();
    }
}

void SceneRenderer::DrawMesh(VkCommandBuffer cmd, const std::string& meshName, VkDescriptorSet descriptorSet, int mode) const {
    if (!RendererDebug::Get().ShouldUseDirectionalLighting()) {
        mode = 1;
    }

    if (RendererDebug::Get().ShouldUploadEnvironmentUniform()) {
        RefreshEnvironmentDescriptorBindings();
    }

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
    if (!RendererDebug::Get().ShouldUploadEnvironmentUniform()) {
        return;
    }

    if (std::memcmp(&m_SceneEnvironment, &environment, sizeof(SceneEnvironmentUniform)) != 0) {
        if (m_LUTGenerator && AtmosphereLUTInputsChanged(m_SceneEnvironment, environment)) {
            m_LUTGenerator->Invalidate();
        }
        m_SceneEnvironment = environment;
        m_SceneEnvironmentDirty = true;
    }
}

const std::vector<RenderDebuggerLUTStats>& SceneRenderer::GetLUTDebugStats() const {
    static const std::vector<RenderDebuggerLUTStats> kEmpty{};
    if (!m_LUTGenerator) return kEmpty;
    return m_LUTGenerator->GetLUTDebugStats();
}

const std::vector<GpuCachedLUTData>& SceneRenderer::GetCachedLUTData() const {
    static const std::vector<GpuCachedLUTData> kEmpty{};
    if (!m_LUTGenerator) return kEmpty;
    return m_LUTGenerator->GetCachedLUTData();
}

std::vector<GpuResourceCatalogEntry> SceneRenderer::BuildGpuResourceCatalog(
    uint32_t offscreenWidth,
    uint32_t offscreenHeight) const {

    std::vector<GpuResourceCatalogEntry> catalog;

    auto add = [&](const char* id, const char* name, uint32_t w, uint32_t h,
                   const char* format, const char* producer,
                   std::initializer_list<const char*> consumers, bool exists) {
        GpuResourceCatalogEntry e{};
        e.id = id;
        e.displayName = name;
        e.width = w;
        e.height = h;
        e.format = format;
        e.memoryBytes = static_cast<uint64_t>(w) * h * 8;
        e.producerPass = producer;
        for (const char* c : consumers) e.consumerPasses.push_back(c);
        e.exists = exists;
        catalog.push_back(std::move(e));
    };

    const char* hdrFmt = "R16G16B16A16_SFLOAT";
    const char* depthFmt = "D32_SFLOAT";
    const char* lutFmt = "R32G32B32A32_SFLOAT";

    add("SceneHDR", "Scene HDR", offscreenWidth, offscreenHeight, hdrFmt, "Clear",
        {"SkyAtmosphere", "Geometry", "PostExposure"}, true);
    add("SceneLDR", "Scene LDR", offscreenWidth, offscreenHeight, hdrFmt, "ToneMapping",
        {"UI"}, true);
    add("SceneDepth", "Scene Depth", offscreenWidth, offscreenHeight, depthFmt, "Clear",
        {"Geometry", "Fog", "Clouds"}, true);
    add("Normal", "Normal", 0, 0, "n/a", "n/a", {}, false);
    add("Velocity", "Velocity", 0, 0, "n/a", "n/a", {}, false);

    if (m_LUTGenerator) {
        const auto& dims = m_LUTGenerator->GetDimensions();
        add("TransmittanceLUT", "Transmittance LUT", dims.transmittanceW, dims.transmittanceH, lutFmt,
            "AtmosphereLUT", {"SkyAtmosphere", "Fog"}, m_LUTGenerator->IsReady());
        add("SkyViewLUT", "SkyView LUT", dims.skyViewW, dims.skyViewH, lutFmt,
            "AtmosphereLUT", {"SkyAtmosphere"}, m_LUTGenerator->IsReady());
        add("MultiScatterLUT", "Multiple Scattering LUT", dims.multiScatterSize, dims.multiScatterSize, lutFmt,
            "AtmosphereLUT", {"SkyAtmosphere"}, m_LUTGenerator->IsReady());
        add("AerialLUT", "Aerial Perspective LUT", dims.aerialSize, dims.aerialSize, lutFmt,
            "AtmosphereLUT", {"Fog"}, m_LUTGenerator->IsReady());
    }

    if (m_PostProcess) {
        for (const auto& pp : m_PostProcess->GetResourceDescriptors()) {
            add(pp.name.c_str(), pp.name.c_str(), pp.width, pp.height, pp.format.c_str(),
                "PostExposure", {"PostExposure"}, true);
        }
    }

    add("CloudScratch", "Volumetric Cloud Scratch", offscreenWidth, offscreenHeight, hdrFmt,
        "VolumetricClouds", {"VolumetricClouds"}, m_CloudScratchFramebuffer != VK_NULL_HANDLE);
    add("ShadowMaps", "Shadow Maps", 0, 0, "n/a", "n/a", {}, false);
    add("Backbuffer", "Final Backbuffer", offscreenWidth, offscreenHeight, "B8G8R8A8_UNORM",
        "Present", {}, true);

    return catalog;
}

void SceneRenderer::RefreshEnvironmentDescriptorBindings() const {
    if (!RendererDebug::Get().ShouldUploadEnvironmentUniform()) {
        return;
    }

    if (!m_SceneEnvironmentDirty || m_EnvironmentBuffer == VK_NULL_HANDLE) {
        return;
    }

    void* data = nullptr;
    vkMapMemory(m_Context->GetDevice(), m_EnvironmentBufferMemory, 0, sizeof(SceneEnvironmentUniform), 0, &data);
    std::memcpy(data, &m_SceneEnvironment, sizeof(SceneEnvironmentUniform));
    vkUnmapMemory(m_Context->GetDevice(), m_EnvironmentBufferMemory);
    m_SceneEnvironmentDirty = false;
}

bool SceneRenderer::IsSkyPipelineCreated() const {
    return m_SkyAtmospherePipeline != VK_NULL_HANDLE;
}

bool SceneRenderer::IsSkyPassReady() const {
    return IsSkyPipelineCreated() && AreAtmosphereLUTsReady();
}

bool SceneRenderer::IsPostPassReady() const {
    return m_PostProcess && m_PostProcess->IsReady();
}

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
