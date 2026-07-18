#include "VulkanDevice.h"
#include "VulkanFormats.h"
#include "VulkanPlatformSurface.h"
#include "VulkanInternal.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "RHI/ShaderBytecode.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace we::rhi::vulkan {
RHIResult<RHIDescriptorSetLayoutHandle> VulkanDevice::CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(desc.bindings.size());
    for (const auto& b : desc.bindings) {
        VkDescriptorSetLayoutBinding vb{};
        vb.binding = b.binding;
        vb.descriptorType = ToVkDescriptorType(b.type);
        vb.descriptorCount = b.count ? b.count : 1u;
        vb.stageFlags = ToVkShaderStageFlags(b.stages);
        bindings.push_back(vb);
    }

    VulkanDescriptorSetLayout layout{};
    layout.desc = desc;
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.empty() ? nullptr : bindings.data();
    if (vkCreateDescriptorSetLayout(m_Device, &info, nullptr, &layout.layout) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateDescriptorSetLayout failed.", "CreateDescriptorSetLayout");
    }
    const auto handle = static_cast<RHIDescriptorSetLayoutHandle>(AllocHandle());
    m_DescriptorSetLayouts.emplace(static_cast<uint64_t>(handle), std::move(layout));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyDescriptorSetLayout(RHIDescriptorSetLayoutHandle handle) {
    if (m_DescriptorSetLayouts.find(static_cast<uint64_t>(handle)) == m_DescriptorSetLayouts.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown descriptor set layout.", "DestroyDescriptorSetLayout");
    }
    EnqueueDeferred(DeferredKind::DescriptorSetLayout, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHIDescriptorPoolHandle> VulkanDevice::CreateDescriptorPool(const DescriptorPoolDesc& desc) {
    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(desc.poolSizes.size());
    for (const auto& s : desc.poolSizes) {
        VkDescriptorPoolSize vs{};
        vs.type = ToVkDescriptorType(s.type);
        vs.descriptorCount = s.count ? s.count : 1u;
        sizes.push_back(vs);
    }
    if (sizes.empty()) {
        sizes.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});
    }

    VulkanDescriptorPool pool{};
    pool.desc = desc;
    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    info.maxSets = desc.maxSets ? desc.maxSets : 1u;
    info.poolSizeCount = static_cast<uint32_t>(sizes.size());
    info.pPoolSizes = sizes.data();
    if (vkCreateDescriptorPool(m_Device, &info, nullptr, &pool.pool) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateDescriptorPool failed.", "CreateDescriptorPool");
    }
    const auto handle = static_cast<RHIDescriptorPoolHandle>(AllocHandle());
    m_DescriptorPools.emplace(static_cast<uint64_t>(handle), std::move(pool));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyDescriptorPool(RHIDescriptorPoolHandle handle) {
    if (m_DescriptorPools.find(static_cast<uint64_t>(handle)) == m_DescriptorPools.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown descriptor pool.", "DestroyDescriptorPool");
    }
    EnqueueDeferred(DeferredKind::DescriptorPool, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::ResetDescriptorPool(RHIDescriptorPoolHandle handle) {
    auto it = m_DescriptorPools.find(static_cast<uint64_t>(handle));
    if (it == m_DescriptorPools.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown descriptor pool.", "ResetDescriptorPool");
    }
    vkResetDescriptorPool(m_Device, it->second.pool, 0);
    for (auto setIt = m_DescriptorSets.begin(); setIt != m_DescriptorSets.end();) {
        if (setIt->second.pool == handle) {
            setIt = m_DescriptorSets.erase(setIt);
        } else {
            ++setIt;
        }
    }
    return RHIResult<void>::Success();
}

RHIResult<RHIDescriptorSetHandle> VulkanDevice::AllocateDescriptorSet(const DescriptorSetAllocateDesc& desc) {
    auto poolIt = m_DescriptorPools.find(static_cast<uint64_t>(desc.pool));
    auto layoutIt = m_DescriptorSetLayouts.find(static_cast<uint64_t>(desc.layout));
    if (poolIt == m_DescriptorPools.end() || layoutIt == m_DescriptorSetLayouts.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Invalid pool or layout.", "AllocateDescriptorSet");
    }

    VulkanDescriptorSet set{};
    set.pool = desc.pool;
    set.layout = desc.layout;
    VkDescriptorSetAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = poolIt->second.pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layoutIt->second.layout;
    if (vkAllocateDescriptorSets(m_Device, &info, &set.set) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkAllocateDescriptorSets failed.", "AllocateDescriptorSet");
    }
    const auto handle = static_cast<RHIDescriptorSetHandle>(AllocHandle());
    m_DescriptorSets.emplace(static_cast<uint64_t>(handle), set);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

void VulkanDevice::UpdateDescriptorSets(std::span<const WriteDescriptorSet> writes) {
    if (writes.empty()) {
        return;
    }
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;
    struct PendingWrite {
        WriteDescriptorSet src{};
        bool isBuffer = false;
        size_t infoOffset = 0;
    };
    std::vector<PendingWrite> pending;
    pending.reserve(writes.size());

    for (const auto& w : writes) {
        if (!FindDescriptorSet(w.set) || w.count == 0) {
            continue;
        }
        PendingWrite p{};
        p.src = w;
        if (w.bufferInfos) {
            p.isBuffer = true;
            p.infoOffset = bufferInfos.size();
            for (uint32_t i = 0; i < w.count; ++i) {
                const auto& bi = w.bufferInfos[i];
                auto* buf = FindBuffer(bi.buffer);
                VkDescriptorBufferInfo info{};
                info.buffer = buf ? buf->buffer : VK_NULL_HANDLE;
                info.offset = bi.offset;
                info.range = bi.range == ~0ull
                    ? (buf && buf->desc.size > bi.offset ? buf->desc.size - bi.offset : VK_WHOLE_SIZE)
                    : bi.range;
                bufferInfos.push_back(info);
            }
        } else if (w.imageInfos) {
            p.isBuffer = false;
            p.infoOffset = imageInfos.size();
            for (uint32_t i = 0; i < w.count; ++i) {
                const auto& ii = w.imageInfos[i];
                auto* view = FindTextureView(ii.view);
                auto* sampler = FindSampler(ii.sampler);
                VkDescriptorImageInfo info{};
                info.sampler = sampler ? sampler->sampler : VK_NULL_HANDLE;
                info.imageView = view ? view->view : VK_NULL_HANDLE;
                info.imageLayout = ToVkImageLayout(ii.imageLayout);
                imageInfos.push_back(info);
            }
        } else {
            continue;
        }
        pending.push_back(p);
    }

    std::vector<VkWriteDescriptorSet> vkWrites;
    vkWrites.reserve(pending.size());
    for (const auto& p : pending) {
        auto* set = FindDescriptorSet(p.src.set);
        if (!set) {
            continue;
        }
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set->set;
        write.dstBinding = p.src.binding;
        write.dstArrayElement = p.src.arrayElement;
        write.descriptorType = ToVkDescriptorType(p.src.type);
        write.descriptorCount = p.src.count;
        if (p.isBuffer) {
            write.pBufferInfo = bufferInfos.data() + p.infoOffset;
        } else {
            write.pImageInfo = imageInfos.data() + p.infoOffset;
        }
        vkWrites.push_back(write);
    }

    if (!vkWrites.empty()) {
        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(vkWrites.size()), vkWrites.data(), 0, nullptr);
    }
}

RHIResult<RHIPipelineLayoutHandle> VulkanDevice::CreatePipelineLayout(const PipelineLayoutDesc& desc) {
    std::vector<VkDescriptorSetLayout> setLayouts;
    setLayouts.reserve(desc.setLayouts.size());
    for (auto h : desc.setLayouts) {
        auto it = m_DescriptorSetLayouts.find(static_cast<uint64_t>(h));
        if (it == m_DescriptorSetLayouts.end()) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown descriptor set layout.", "CreatePipelineLayout");
        }
        setLayouts.push_back(it->second.layout);
    }

    std::vector<VkPushConstantRange> ranges;
    if (!desc.pushConstantRanges.empty()) {
        ranges.reserve(desc.pushConstantRanges.size());
        for (const auto& r : desc.pushConstantRanges) {
            VkPushConstantRange vr{};
            vr.stageFlags = ToVkShaderStageFlags(r.stages);
            vr.offset = r.offset;
            vr.size = r.size;
            ranges.push_back(vr);
        }
    } else if (desc.pushConstantSize > 0) {
        VkPushConstantRange vr{};
        vr.stageFlags = ToVkShaderStageFlags(desc.pushConstantStages);
        vr.offset = 0;
        vr.size = desc.pushConstantSize;
        ranges.push_back(vr);
    }

    VulkanPipelineLayout layout{};
    layout.desc = desc;
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    info.pSetLayouts = setLayouts.empty() ? nullptr : setLayouts.data();
    info.pushConstantRangeCount = static_cast<uint32_t>(ranges.size());
    info.pPushConstantRanges = ranges.empty() ? nullptr : ranges.data();
    if (vkCreatePipelineLayout(m_Device, &info, nullptr, &layout.layout) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreatePipelineLayout failed.", "CreatePipelineLayout");
    }
    const auto handle = static_cast<RHIPipelineLayoutHandle>(AllocHandle());
    m_PipelineLayouts.emplace(static_cast<uint64_t>(handle), std::move(layout));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyPipelineLayout(RHIPipelineLayoutHandle handle) {
    if (m_PipelineLayouts.find(static_cast<uint64_t>(handle)) == m_PipelineLayouts.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown pipeline layout.", "DestroyPipelineLayout");
    }
    EnqueueDeferred(DeferredKind::PipelineLayout, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHIGraphicsPipelineHandle> VulkanDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) {
    auto* vert = FindShader(desc.vertexShader);
    auto* frag = FindShader(desc.fragmentShader);
    auto* layout = FindPipelineLayout(desc.layout);
    if (!vert || !frag || !layout || !vert->module || !frag->module || !layout->layout) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Missing shaders or layout.", "CreateGraphicsPipeline");
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert->module;
    stages[0].pName = vert->entryPoint.c_str();
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag->module;
    stages[1].pName = frag->entryPoint.c_str();

    std::vector<VkVertexInputBindingDescription> bindings;
    bindings.reserve(desc.vertexBindings.size());
    for (const auto& b : desc.vertexBindings) {
        VkVertexInputBindingDescription vb{};
        vb.binding = b.binding;
        vb.stride = b.stride;
        vb.inputRate = b.perInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
        bindings.push_back(vb);
    }
    std::vector<VkVertexInputAttributeDescription> attrs;
    attrs.reserve(desc.vertexAttributes.size());
    for (const auto& a : desc.vertexAttributes) {
        VkVertexInputAttributeDescription va{};
        va.location = a.location;
        va.binding = a.binding;
        va.format = ToVkFormat(a.format);
        va.offset = a.offset;
        attrs.push_back(va);
    }

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInput.pVertexBindingDescriptions = bindings.empty() ? nullptr : bindings.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
    vertexInput.pVertexAttributeDescriptions = attrs.empty() ? nullptr : attrs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = ToVkTopology(desc.topology);

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = desc.rasterizer.depthClamp ? VK_TRUE : VK_FALSE;
    rasterizer.polygonMode = desc.rasterizer.fillMode == FillMode::Wireframe
        ? VK_POLYGON_MODE_LINE
        : VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = ToVkCullMode(desc.cullMode);
    rasterizer.frontFace = desc.rasterizer.frontCounterClockwise
        ? VK_FRONT_FACE_COUNTER_CLOCKWISE
        : VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = desc.rasterizer.depthBiasEnable ? VK_TRUE : VK_FALSE;
    rasterizer.depthBiasConstantFactor = desc.rasterizer.depthBiasConstant;
    rasterizer.depthBiasClamp = desc.rasterizer.depthBiasClamp;
    rasterizer.depthBiasSlopeFactor = desc.rasterizer.depthBiasSlope;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = ToVkSampleCount(
        desc.multisample.sampleCount ? desc.multisample.sampleCount : 1u);
    multisampling.alphaToCoverageEnable = desc.multisample.alphaToCoverage ? VK_TRUE : VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = desc.depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = desc.depthWrite ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = ToVkCompareOp(desc.depthCompare);
    depthStencil.stencilTestEnable = desc.depthStencil.stencilTest ? VK_TRUE : VK_FALSE;
    if (desc.depthStencil.stencilTest) {
        auto applyFace = [](VkStencilOpState& face, const StencilOpStateDesc& src) {
            face.failOp = ToVkStencilOp(src.failOp);
            face.passOp = ToVkStencilOp(src.passOp);
            face.depthFailOp = ToVkStencilOp(src.depthFailOp);
            face.compareOp = ToVkCompareOp(src.compareOp);
            face.compareMask = src.compareMask;
            face.writeMask = src.writeMask;
            face.reference = src.reference;
        };
        applyFace(depthStencil.front, desc.depthStencil.front);
        applyFace(depthStencil.back, desc.depthStencil.back);
    }

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
    const size_t colorCount = !desc.colorFormats.empty() ? desc.colorFormats.size() : 1u;
    colorBlendAttachments.resize(colorCount);
    for (size_t i = 0; i < colorCount; ++i) {
        const BlendStateDesc& blend = !desc.colorBlends.empty()
            ? desc.colorBlends[std::min(i, desc.colorBlends.size() - 1)]
            : desc.blend;
        auto& att = colorBlendAttachments[i];
        att.colorWriteMask = 0;
        if (blend.writeMask & 0x1) att.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
        if (blend.writeMask & 0x2) att.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
        if (blend.writeMask & 0x4) att.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
        if (blend.writeMask & 0x8) att.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
        if (att.colorWriteMask == 0) {
            att.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        }
        att.blendEnable = blend.enable ? VK_TRUE : VK_FALSE;
        att.srcColorBlendFactor = ToVkBlendFactor(blend.srcColor);
        att.dstColorBlendFactor = ToVkBlendFactor(blend.dstColor);
        att.colorBlendOp = ToVkBlendOp(blend.colorOp);
        att.srcAlphaBlendFactor = ToVkBlendFactor(blend.srcAlpha);
        att.dstAlphaBlendFactor = ToVkBlendFactor(blend.dstAlpha);
        att.alphaBlendOp = ToVkBlendOp(blend.alphaOp);
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();

    const VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    std::vector<VkFormat> colorVkFormats;
    if (!desc.colorFormats.empty()) {
        colorVkFormats.reserve(desc.colorFormats.size());
        for (Format f : desc.colorFormats) {
            colorVkFormats.push_back(ToVkFormat(f));
        }
    } else {
        colorVkFormats.push_back(ToVkFormat(desc.colorFormat));
    }
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorVkFormats.size());
    renderingInfo.pColorAttachmentFormats = colorVkFormats.data();
    renderingInfo.depthAttachmentFormat = desc.depthAttachment ? ToVkFormat(desc.depthFormat) : VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = layout->layout;

    VulkanGraphicsPipeline pipeline{};
    pipeline.desc = desc;
    pipeline.layout = layout->layout;
    if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.pipeline) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateGraphicsPipelines failed.", "CreateGraphicsPipeline");
    }
    const auto handle = static_cast<RHIGraphicsPipelineHandle>(AllocHandle());
    m_GraphicsPipelines.emplace(static_cast<uint64_t>(handle), pipeline);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyGraphicsPipeline(RHIGraphicsPipelineHandle handle) {
    if (m_GraphicsPipelines.find(static_cast<uint64_t>(handle)) == m_GraphicsPipelines.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown graphics pipeline.", "DestroyGraphicsPipeline");
    }
    EnqueueDeferred(DeferredKind::GraphicsPipeline, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHIComputePipelineHandle> VulkanDevice::CreateComputePipeline(const ComputePipelineDesc& desc) {
    auto* shader = FindShader(desc.computeShader);
    auto* layout = FindPipelineLayout(desc.layout);
    if (!shader || !layout || !shader->module || !layout->layout) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Missing compute shader or layout.", "CreateComputePipeline");
    }

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = shader->module;
    stage.pName = shader->entryPoint.c_str();

    VkComputePipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    info.stage = stage;
    info.layout = layout->layout;

    VulkanComputePipeline pipeline{};
    pipeline.desc = desc;
    pipeline.layout = layout->layout;
    if (vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline.pipeline) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateComputePipelines failed.", "CreateComputePipeline");
    }
    const auto handle = static_cast<RHIComputePipelineHandle>(AllocHandle());
    m_ComputePipelines.emplace(static_cast<uint64_t>(handle), pipeline);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyComputePipeline(RHIComputePipelineHandle handle) {
    if (m_ComputePipelines.find(static_cast<uint64_t>(handle)) == m_ComputePipelines.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown compute pipeline.", "DestroyComputePipeline");
    }
    EnqueueDeferred(DeferredKind::ComputePipeline, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

} // namespace we::rhi::vulkan
