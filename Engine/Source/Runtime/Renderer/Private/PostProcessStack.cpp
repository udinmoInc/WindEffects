#include "Renderer/PostProcessStack.hpp"
#include "Renderer/RenderDiagnostics.hpp"
#include "Renderer/RenderPipelineInvestigator.hpp"
#include "Renderer/FrameStats.hpp"
#include "Renderer/ShaderHelper.hpp"
#include "Renderer/RenderForensics.hpp"
#include "Core/DiagnosticMacros.hpp"
#include "Core/LogCategory.hpp"

#include <array>
#include <stdexcept>
#include <algorithm>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

namespace {

void DestroyImageResources(VkDevice device, VkImage& image, VkDeviceMemory& memory, VkImageView& view) {
    if (view != VK_NULL_HANDLE) {
        vkDestroyImageView(device, view, nullptr);
        view = VK_NULL_HANDLE;
    }
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
}

void CreateStorageImage(
    const VulkanContext& context,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImage& image,
    VkDeviceMemory& memory,
    VkImageView& view) {
    context.CreateImage(
        std::max(1u, width),
        std::max(1u, height),
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        image,
        memory);
    view = context.CreateImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
}

VkDescriptorSetLayoutBinding MakeStorageBinding(uint32_t binding) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    return layoutBinding;
}

VkDescriptorSetLayoutBinding MakeSampledBinding(uint32_t binding) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    return layoutBinding;
}

} // namespace

PostProcessStack::PostProcessStack(const std::shared_ptr<VulkanContext>& context)
    : m_Context(context) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_Context->GetDevice(), &samplerInfo, nullptr, &m_LinearSampler) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessStack: failed to create sampler.");
    }

    std::array<VkDescriptorSetLayoutBinding, 4> postBindings = {
        MakeSampledBinding(0),
        MakeSampledBinding(1),
        MakeSampledBinding(2),
        MakeStorageBinding(3),
    };

    VkDescriptorSetLayoutCreateInfo postLayoutInfo{};
    postLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    postLayoutInfo.bindingCount = static_cast<uint32_t>(postBindings.size());
    postLayoutInfo.pBindings = postBindings.data();
    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &postLayoutInfo, nullptr, &m_PostDescLayout) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessStack: failed to create descriptor layout.");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Context->GetDescriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_PostDescLayout;
    if (vkAllocateDescriptorSets(m_Context->GetDevice(), &allocInfo, &m_PostDescSet) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessStack: failed to allocate descriptor set.");
    }

    VkDescriptorSetLayoutBinding sampledBinding = MakeSampledBinding(0);
    VkDescriptorSetLayoutBinding storageBinding = MakeStorageBinding(0);
    VkDescriptorSetLayoutCreateInfo singleLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    singleLayoutInfo.bindingCount = 1;

    singleLayoutInfo.pBindings = &sampledBinding;
    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &singleLayoutInfo, nullptr, &m_SceneSampleLayout) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessStack: failed to create scene sample layout.");
    }

    singleLayoutInfo.pBindings = &storageBinding;
    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &singleLayoutInfo, nullptr, &m_StorageLayout) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessStack: failed to create storage layout.");
    }

    VkDescriptorSetAllocateInfo singleAlloc{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    singleAlloc.descriptorPool = m_Context->GetDescriptorPool();
    singleAlloc.descriptorSetCount = 1;

    singleAlloc.pSetLayouts = &m_SceneSampleLayout;
    vkAllocateDescriptorSets(m_Context->GetDevice(), &singleAlloc, &m_SceneSampleSet);
    singleAlloc.pSetLayouts = &m_StorageLayout;
    vkAllocateDescriptorSets(m_Context->GetDevice(), &singleAlloc, &m_StorageSetA);
    vkAllocateDescriptorSets(m_Context->GetDevice(), &singleAlloc, &m_StorageSetB);
    vkAllocateDescriptorSets(m_Context->GetDevice(), &singleAlloc, &m_StorageSampleSet);

    VkDescriptorSetLayoutBinding envBinding{};
    envBinding.binding = 0;
    envBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    envBinding.descriptorCount = 1;
    envBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    VkDescriptorSetLayoutCreateInfo envLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    envLayoutInfo.bindingCount = 1;
    envLayoutInfo.pBindings = &envBinding;
    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &envLayoutInfo, nullptr, &m_EnvDescLayout) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessStack: failed to create env layout.");
    }

    CreatePipelines();

    m_Context->CreateBuffer(
        sizeof(float),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_LuminanceReadbackBuffer,
        m_LuminanceReadbackMemory);
}

PostProcessStack::~PostProcessStack() {
    DestroyPipelines();
    DestroyResources();

    VkDevice device = m_Context->GetDevice();
    if (m_LinearSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_LinearSampler, nullptr);
    }
    if (m_PostDescLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_PostDescLayout, nullptr);
    }
    if (m_SceneSampleLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_SceneSampleLayout, nullptr);
    }
    if (m_StorageLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_StorageLayout, nullptr);
    }
    if (m_EnvDescLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_EnvDescLayout, nullptr);
    }
    if (m_LuminanceReadbackBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_LuminanceReadbackBuffer, nullptr);
        m_LuminanceReadbackBuffer = VK_NULL_HANDLE;
    }
    if (m_LuminanceReadbackMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_LuminanceReadbackMemory, nullptr);
        m_LuminanceReadbackMemory = VK_NULL_HANDLE;
    }
}

bool PostProcessStack::IsReady() const {
    return m_CompositePipeline != VK_NULL_HANDLE
        && m_BloomPrefilterPipeline != VK_NULL_HANDLE
        && m_LuminanceReducePipeline != VK_NULL_HANDLE;
}

void PostProcessStack::Resize(uint32_t width, uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);
    if (width == m_Width && height == m_Height && m_BloomA.image != VK_NULL_HANDLE) {
        return;
    }

    vkDeviceWaitIdle(m_Context->GetDevice());
    DestroyResources();
    m_Width = width;
    m_Height = height;
    m_BloomWidth = std::max(1u, width / 2);
    m_BloomHeight = std::max(1u, height / 2);
    m_TileWidth = std::max(1u, (width + 15) / 16);
    m_TileHeight = std::max(1u, (height + 15) / 16);
    CreateResources(width, height);
}

void PostProcessStack::CreateResources(uint32_t width, uint32_t height) {
    CreateStorageImage(*m_Context, m_BloomWidth, m_BloomHeight, kOffscreenColorFormat, m_BloomA.image, m_BloomA.memory, m_BloomA.view);
    CreateStorageImage(*m_Context, m_BloomWidth, m_BloomHeight, kOffscreenColorFormat, m_BloomB.image, m_BloomB.memory, m_BloomB.view);
    CreateStorageImage(*m_Context, m_TileWidth, m_TileHeight, VK_FORMAT_R32_SFLOAT, m_LuminanceTiles.image, m_LuminanceTiles.memory, m_LuminanceTiles.view);
    CreateStorageImage(*m_Context, 1, 1, VK_FORMAT_R32_SFLOAT, m_LuminanceAvg.image, m_LuminanceAvg.memory, m_LuminanceAvg.view);

    const VkImage bloomImages[] = { m_BloomA.image, m_BloomB.image };
    for (VkImage image : bloomImages) {
        m_Context->TransitionImageLayout(image, kOffscreenColorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    }
    m_Context->TransitionImageLayout(m_LuminanceTiles.image, VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    m_Context->TransitionImageLayout(m_LuminanceAvg.image, VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
}

void PostProcessStack::DestroyResources() {
    VkDevice device = m_Context->GetDevice();
    DestroyImageResources(device, m_BloomA.image, m_BloomA.memory, m_BloomA.view);
    DestroyImageResources(device, m_BloomB.image, m_BloomB.memory, m_BloomB.view);
    DestroyImageResources(device, m_LuminanceTiles.image, m_LuminanceTiles.memory, m_LuminanceTiles.view);
    DestroyImageResources(device, m_LuminanceAvg.image, m_LuminanceAvg.memory, m_LuminanceAvg.view);
}

VkPipeline PostProcessStack::CreateComputePipeline(const char* shaderName, VkPipelineLayout layout) const {
    VkDevice device = m_Context->GetDevice();
    try {
        const std::vector<char> code = LoadShaderBytecode(shaderName, ShaderStage::Compute);
        if (code.empty()) {
            HE_WARN(std::string("PostProcessStack: missing shader bytecode for ") + shaderName);
            return VK_NULL_HANDLE;
        }

        VkShaderModule module = CreateShaderModule(device, code);
        VkPipelineShaderStageCreateInfo stage{};
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stage.module = module;
        stage.pName = "CSMain";

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = stage;
        pipelineInfo.layout = layout;

        VkPipeline pipeline = VK_NULL_HANDLE;
        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
            HE_WARN(std::string("PostProcessStack: failed to create pipeline for ") + shaderName);
        }
        vkDestroyShaderModule(device, module, nullptr);
        return pipeline;
    } catch (const std::exception& ex) {
        HE_WARN(std::string("PostProcessStack: ") + shaderName + " unavailable: " + ex.what());
        return VK_NULL_HANDLE;
    }
}

void PostProcessStack::CreatePipelines() {
    VkDevice device = m_Context->GetDevice();

    auto createLayout = [&](const std::vector<VkDescriptorSetLayout>& layouts, VkPushConstantRange* pushRange, VkPipelineLayout& outLayout) {
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
        layoutInfo.pSetLayouts = layouts.data();
        if (pushRange != nullptr) {
            layoutInfo.pushConstantRangeCount = 1;
            layoutInfo.pPushConstantRanges = pushRange;
        }
        if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &outLayout) != VK_SUCCESS) {
            throw std::runtime_error("PostProcessStack: failed to create pipeline layout.");
        }
    };

    createLayout({ m_SceneSampleLayout, m_StorageLayout }, nullptr, m_LuminanceReduceLayout);
    m_LuminanceReducePipeline = CreateComputePipeline("LuminanceReduceCS", m_LuminanceReduceLayout);

    createLayout({ m_SceneSampleLayout, m_StorageLayout }, nullptr, m_LuminanceAvgLayout);
    m_LuminanceAvgPipeline = CreateComputePipeline("LuminanceAvgCS", m_LuminanceAvgLayout);

    createLayout({ m_SceneSampleLayout, m_StorageLayout }, nullptr, m_BloomPrefilterLayout);
    m_BloomPrefilterPipeline = CreateComputePipeline("BloomPrefilterCS", m_BloomPrefilterLayout);

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(float) * 4;
    createLayout({ m_SceneSampleLayout, m_StorageLayout }, &pushRange, m_BloomBlurLayout);
    m_BloomBlurPipeline = CreateComputePipeline("BloomBlurCS", m_BloomBlurLayout);

    createLayout({ m_EnvDescLayout, m_PostDescLayout }, nullptr, m_CompositeLayout);
    m_CompositePipeline = CreateComputePipeline("PostCompositeCS", m_CompositeLayout);
}

void PostProcessStack::DestroyPipelines() {
    VkDevice device = m_Context->GetDevice();
    auto destroy = [&](VkPipeline& pipeline) {
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline, nullptr);
            pipeline = VK_NULL_HANDLE;
        }
    };
    destroy(m_LuminanceReducePipeline);
    destroy(m_LuminanceAvgPipeline);
    destroy(m_BloomPrefilterPipeline);
    destroy(m_BloomBlurPipeline);
    destroy(m_CompositePipeline);

    auto destroyLayout = [&](VkPipelineLayout& layout) {
        if (layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, layout, nullptr);
            layout = VK_NULL_HANDLE;
        }
    };
    destroyLayout(m_LuminanceReduceLayout);
    destroyLayout(m_LuminanceAvgLayout);
    destroyLayout(m_BloomPrefilterLayout);
    destroyLayout(m_BloomBlurLayout);
    destroyLayout(m_CompositeLayout);
}

void PostProcessStack::Apply(
    VkCommandBuffer cmd,
    VkDescriptorSet environmentDescSet,
    VkImage sceneImage,
    VkImageView sceneImageView,
    uint32_t width,
    uint32_t height) const {

    auto& diag = RenderDiagnostics::Get();
    auto& stats = FrameStatsCollector::Get();

    if (!IsReady() || sceneImage == VK_NULL_HANDLE || sceneImageView == VK_NULL_HANDLE) {
        stats.SetPassStatus("PostExposure", "failed");
        stats.SetPassStatus("ToneMapping", "failed");
        stats.SetPassStatus("Bloom", "failed");
        diag.RecordPassStatus("PostExposure", PassExecutionStatus::Failed, "post stack not ready");
        return;
    }

    if (m_Width != width || m_Height != height) {
        const_cast<PostProcessStack*>(this)->Resize(width, height);
    }

    // Render pass leaves the scene target in SHADER_READ_ONLY; post compute needs GENERAL storage.
    m_Context->CmdTransitionImageLayout(
        cmd,
        sceneImage,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_GENERAL);

    auto writeImage = [&](VkDescriptorSet dstSet, uint32_t binding, VkImageView view, VkDescriptorType type, VkImageLayout layout) {
        VkDescriptorImageInfo info{};
        if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            info.sampler = m_LinearSampler;
        }
        info.imageView = view;
        info.imageLayout = layout;
        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = dstSet;
        write.dstBinding = binding;
        write.descriptorCount = 1;
        write.descriptorType = type;
        write.pImageInfo = &info;
        vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &write, 0, nullptr);
    };

    writeImage(m_SceneSampleSet, 0, sceneImageView, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_GENERAL);
    writeImage(m_StorageSetA, 0, m_LuminanceTiles.view, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_IMAGE_LAYOUT_GENERAL);

    auto& pipelineInv = RenderPipelineInvestigator::Get();
    const bool applyBloom = pipelineInv.ShouldApplyBloom();
    const bool applyAutoExposure = pipelineInv.ShouldApplyAutoExposure();

    auto& forensics = RenderForensics::Get();
    const bool forensicActive = forensics.IsActive();
    int lumExec = -1;
    int bloomExec = -1;
    int exposureExec = -1;
    int toneMapExec = -1;

    if (applyAutoExposure) {
        if (forensicActive) {
            ForensicPassMetadata lumMeta{};
            lumMeta.inputTarget = "OffscreenColor";
            lumMeta.outputTarget = "LuminanceTiles";
            lumMeta.width = width;
            lumMeta.height = height;
            lumMeta.format = "R16G16B16A16_SFLOAT";
            lumMeta.inputLayout = "GENERAL";
            lumMeta.outputLayout = "GENERAL";
            lumMeta.loadOp = "LOAD";
            lumMeta.storeOp = "STORE";
            lumMeta.pipelineName = "LuminanceReduce";
            lumMeta.computeShader = "LuminanceReduceCS";
            lumMeta.descriptorSets = "sceneSample(0),luminanceTiles(1)";
            lumExec = WE_FORENSIC_PASS_BEGIN(forensics, ForensicPassId::LuminanceReduce, lumMeta);
        }

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_LuminanceReducePipeline);
        VkDescriptorSet reduceSets[] = { m_SceneSampleSet, m_StorageSetA };
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_LuminanceReduceLayout, 0, 2, reduceSets, 0, nullptr);
        vkCmdDispatch(cmd, (m_TileWidth + 7) / 8, (m_TileHeight + 7) / 8, 1);
        m_Context->CmdComputeImageBarrier(cmd, m_LuminanceTiles.image);

        writeImage(m_StorageSampleSet, 0, m_LuminanceTiles.view, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_GENERAL);
        writeImage(m_StorageSetB, 0, m_LuminanceAvg.view, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_IMAGE_LAYOUT_GENERAL);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_LuminanceAvgPipeline);
        VkDescriptorSet avgSets[] = { m_StorageSampleSet, m_StorageSetB };
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_LuminanceAvgLayout, 0, 2, avgSets, 0, nullptr);
        vkCmdDispatch(cmd, 1, 1, 1);
        m_Context->CmdComputeImageBarrier(cmd, m_LuminanceAvg.image);

        if (m_LuminanceReadbackBuffer != VK_NULL_HANDLE) {
            VkBufferImageCopy region{};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = { 1, 1, 1 };
            vkCmdCopyImageToBuffer(
                cmd,
                m_LuminanceAvg.image,
                VK_IMAGE_LAYOUT_GENERAL,
                m_LuminanceReadbackBuffer,
                1,
                &region);
            m_LuminanceReadbackPending = true;
        }
    }

    writeImage(m_StorageSetA, 0, m_BloomA.view, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_IMAGE_LAYOUT_GENERAL);

    if (applyBloom) {
        if (forensicActive) {
            ForensicPassMetadata bloomMeta{};
            bloomMeta.inputTarget = "OffscreenColor";
            bloomMeta.outputTarget = "BloomA";
            bloomMeta.width = m_BloomWidth;
            bloomMeta.height = m_BloomHeight;
            bloomMeta.format = "R16G16B16A16_SFLOAT";
            bloomMeta.inputLayout = "GENERAL";
            bloomMeta.outputLayout = "GENERAL";
            bloomMeta.pipelineName = "BloomPrefilter+Blur";
            bloomMeta.computeShader = "BloomPrefilterCS,BloomBlurCS";
            bloomMeta.descriptorSets = "sceneSample(0),bloomStorage(1)";
            bloomExec = WE_FORENSIC_PASS_BEGIN(forensics, ForensicPassId::Bloom, bloomMeta);
        }

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BloomPrefilterPipeline);
        VkDescriptorSet prefilterSets[] = { m_SceneSampleSet, m_StorageSetA };
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BloomPrefilterLayout, 0, 2, prefilterSets, 0, nullptr);
        vkCmdDispatch(cmd, (m_BloomWidth + 7) / 8, (m_BloomHeight + 7) / 8, 1);
        m_Context->CmdComputeImageBarrier(cmd, m_BloomA.image);

        auto runBlur = [&](VkImageView source, VkDescriptorSet targetSet, float dirX, float dirY, VkImage targetImage) {
            writeImage(m_StorageSampleSet, 0, source, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_GENERAL);
            const float pushData[4] = { dirX, dirY, 0.0f, 0.0f };
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BloomBlurPipeline);
            vkCmdPushConstants(cmd, m_BloomBlurLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushData), pushData);
            VkDescriptorSet blurSets[] = { m_StorageSampleSet, targetSet };
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BloomBlurLayout, 0, 2, blurSets, 0, nullptr);
            vkCmdDispatch(cmd, (m_BloomWidth + 7) / 8, (m_BloomHeight + 7) / 8, 1);
            m_Context->CmdComputeImageBarrier(cmd, targetImage);
        };

        runBlur(m_BloomA.view, m_StorageSetB, 1.0f, 0.0f, m_BloomB.image);
        runBlur(m_BloomB.view, m_StorageSetA, 0.0f, 1.0f, m_BloomA.image);
    } else {
        stats.SetPassStatus("Bloom", "skipped");
        diag.RecordPassStatus("Bloom", PassExecutionStatus::Skipped, "pipeline isolation gate");
    }
    m_Context->CmdComputeImageBarrier(cmd, sceneImage);

    writeImage(m_PostDescSet, 0, sceneImageView, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_GENERAL);
    writeImage(m_PostDescSet, 1, m_BloomA.view, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_GENERAL);
    writeImage(m_PostDescSet, 2, m_LuminanceAvg.view, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_GENERAL);
    writeImage(m_PostDescSet, 3, sceneImageView, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_IMAGE_LAYOUT_GENERAL);

    if (forensicActive) {
        const auto& cam = forensics.GetCurrentCameraLog();
        ForensicPassMetadata exposureMeta{};
        exposureMeta.inputTarget = "OffscreenColor";
        exposureMeta.outputTarget = "OffscreenColor";
        exposureMeta.width = width;
        exposureMeta.height = height;
        exposureMeta.format = "R16G16B16A16_SFLOAT";
        exposureMeta.inputLayout = "GENERAL";
        exposureMeta.outputLayout = "GENERAL";
        exposureMeta.loadOp = "LOAD";
        exposureMeta.storeOp = "STORE";
        exposureMeta.pipelineName = "PostComposite";
        exposureMeta.computeShader = "PostCompositeCS (exposure stage)";
        exposureMeta.descriptorSets = "environment(0),scene+bloom+luminanceAvg+sceneOut(1)";
        exposureMeta.boundTextures = "sceneColor,bloom,luminanceAvg";
        exposureExec = WE_FORENSIC_PASS_BEGIN(forensics, ForensicPassId::Exposure, exposureMeta);
        forensics.RecordExposureDetails(exposureExec, cam.ev100, cam.exposureMultiplier, cam.avgSceneLuminance);
        if (forensics.IsReadbackEnabled()) {
            forensics.EnqueueReadback(
                cmd, *m_Context, sceneImage, width, height,
                ForensicPassId::Exposure, VK_IMAGE_LAYOUT_GENERAL, exposureExec);
        }

        ForensicPassMetadata toneMeta = exposureMeta;
        toneMeta.computeShader = "PostCompositeCS (tone-map stage)";
        toneMeta.outputLayout = "SHADER_READ_ONLY_OPTIMAL";
        toneMapExec = WE_FORENSIC_PASS_BEGIN(forensics, ForensicPassId::ToneMapping, toneMeta);
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_CompositePipeline);
    VkDescriptorSet compositeSets[] = { environmentDescSet, m_PostDescSet };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_CompositeLayout, 0, 2, compositeSets, 0, nullptr);
    vkCmdDispatch(cmd, (width + 7) / 8, (height + 7) / 8, 1);

    m_Context->CmdTransitionImageLayout(
        cmd,
        sceneImage,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    stats.SetPassStatus("PostExposure", "executed");
    stats.SetPassStatus("ToneMapping", "executed");
    if (applyBloom) {
        stats.SetPassStatus("Bloom", "executed");
        diag.RecordPassStatus("Bloom", PassExecutionStatus::Executed);
    }
    diag.RecordPassStatus("PostExposure", PassExecutionStatus::Executed);
    diag.RecordPassStatus("ToneMapping", PassExecutionStatus::Executed);
}

void PostProcessStack::FlushGpuAverageLuminance() const {
    if (!m_LuminanceReadbackPending || m_LuminanceReadbackBuffer == VK_NULL_HANDLE) {
        return;
    }

    const VkDevice device = m_Context->GetDevice();
    void* mapped = nullptr;
    if (vkMapMemory(device, m_LuminanceReadbackMemory, 0, sizeof(float), 0, &mapped) != VK_SUCCESS || mapped == nullptr) {
        return;
    }

    const float avg = *static_cast<const float*>(mapped);
    vkUnmapMemory(device, m_LuminanceReadbackMemory);
    m_LuminanceReadbackPending = false;
    FrameStatsCollector::Get().SetGpuAverageLuminance(avg);
}

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
