#include "VulkanDevice.h"
#include "VulkanFormats.h"
#include "VulkanPlatformSurface.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Platform/Platform.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <set>
#include <string>

namespace we::rhi::vulkan {
namespace {

bool AreValidationLayersAvailable() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (layerCount == 0) {
        return false;
    }
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    for (const auto& layer : layers) {
        if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
            return true;
        }
    }
    return false;
}

bool ValidationRequestedFromEnvironment() {
    if (const char* env = std::getenv("WE_VULKAN_VALIDATION")) {
        return env[0] == '1' || std::strcmp(env, "true") == 0;
    }
    return false;
}

bool ValidationDisabledFromEnvironment() {
    if (const char* env = std::getenv("WE_VULKAN_VALIDATION")) {
        return env[0] == '0' || std::strcmp(env, "false") == 0;
    }
    return false;
}

uint32_t SelectInstanceApiVersion() {
    uint32_t requested = VK_API_VERSION_1_3;
    const auto enumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
        vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (!enumerateInstanceVersion) {
        return requested;
    }
    uint32_t supported = 0;
    if (enumerateInstanceVersion(&supported) != VK_SUCCESS) {
        return requested;
    }
    return std::min(requested, supported);
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*)
{
    if (!pCallbackData || !pCallbackData->pMessage) {
        return VK_FALSE;
    }
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(), std::string("Vulkan: ") + pCallbackData->pMessage);
    }
    return VK_FALSE;
}

VkAccessFlags AccessForLayout(VkImageLayout layout) {
    switch (layout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return VK_ACCESS_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return VK_ACCESS_TRANSFER_READ_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return VK_ACCESS_SHADER_READ_BIT;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: return 0;
    default: return 0;
    }
}

VkPipelineStageFlags StageForLayout(VkImageLayout layout) {
    switch (layout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    default:
        return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
}

} // namespace

// --- Queue --------------------------------------------------------------------

RHIResult<void> VulkanQueue::Submit(IRHICommandList&) {
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanQueue::WaitIdle() {
    if (m_Queue) {
        vkQueueWaitIdle(m_Queue);
    }
    return RHIResult<void>::Success();
}

// --- Command list -------------------------------------------------------------

VulkanCommandList::VulkanCommandList(VulkanDevice* device)
    : m_Device(device)
{
}

void VulkanCommandList::Begin() {
    m_Recording = true;
}

void VulkanCommandList::End() {
    m_Recording = false;
}

void VulkanCommandList::BeginRendering(const RenderingInfo& info) {
    if (!m_Cmd || !m_Device) {
        return;
    }

    std::vector<VkRenderingAttachmentInfo> colors;
    colors.reserve(info.colorAttachments.size());
    for (const auto& att : info.colorAttachments) {
        VulkanTexture* tex = m_Device->FindTexture(att.texture);
        VkRenderingAttachmentInfo color{};
        color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color.imageView = tex ? tex->view : VK_NULL_HANDLE;
        color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color.loadOp = att.loadOp == LoadOp::Clear ? VK_ATTACHMENT_LOAD_OP_CLEAR
            : (att.loadOp == LoadOp::DontCare ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_LOAD);
        color.storeOp = att.storeOp == StoreOp::DontCare ? VK_ATTACHMENT_STORE_OP_DONT_CARE
                                                        : VK_ATTACHMENT_STORE_OP_STORE;
        color.clearValue.color = {{att.clearColor.r, att.clearColor.g, att.clearColor.b, att.clearColor.a}};
        colors.push_back(color);

        if (tex && tex->state != ResourceState::RenderTarget) {
            TransitionTexture(att.texture, tex->state, ResourceState::RenderTarget);
        }
    }

    VkRenderingAttachmentInfo depthInfo{};
    VkRenderingAttachmentInfo* depthPtr = nullptr;
    if (info.depth.enabled) {
        VulkanTexture* depthTex = m_Device->FindTexture(info.depth.texture);
        depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthInfo.imageView = depthTex ? depthTex->view : VK_NULL_HANDLE;
        depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthInfo.loadOp = info.depth.loadOp == LoadOp::Clear ? VK_ATTACHMENT_LOAD_OP_CLEAR
            : (info.depth.loadOp == LoadOp::DontCare ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_LOAD);
        depthInfo.storeOp = info.depth.storeOp == StoreOp::DontCare ? VK_ATTACHMENT_STORE_OP_DONT_CARE
                                                                     : VK_ATTACHMENT_STORE_OP_STORE;
        depthInfo.clearValue.depthStencil = {info.depth.clearDepth, info.depth.clearStencil};
        depthPtr = &depthInfo;
        if (depthTex && depthTex->state != ResourceState::DepthWrite) {
            TransitionTexture(info.depth.texture, depthTex->state, ResourceState::DepthWrite);
        }
    }

    VkRenderingInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering.renderArea.offset = {0, 0};
    rendering.renderArea.extent = {
        info.renderArea.width ? info.renderArea.width : 1u,
        info.renderArea.height ? info.renderArea.height : 1u};
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = static_cast<uint32_t>(colors.size());
    rendering.pColorAttachments = colors.data();
    rendering.pDepthAttachment = depthPtr;

    vkCmdBeginRendering(m_Cmd, &rendering);
    m_InRendering = true;
}

void VulkanCommandList::EndRendering() {
    if (m_Cmd && m_InRendering) {
        vkCmdEndRendering(m_Cmd);
        m_InRendering = false;
    }
}

void VulkanCommandList::SetViewport(const Viewport& viewport) {
    if (!m_Cmd) {
        return;
    }
    VkViewport vp{};
    vp.x = viewport.x;
    vp.y = viewport.y;
    vp.width = viewport.width;
    vp.height = viewport.height;
    vp.minDepth = viewport.minDepth;
    vp.maxDepth = viewport.maxDepth;
    vkCmdSetViewport(m_Cmd, 0, 1, &vp);
}

void VulkanCommandList::SetScissor(const Scissor& scissor) {
    if (!m_Cmd) {
        return;
    }
    VkRect2D rect{};
    rect.offset = {scissor.x, scissor.y};
    rect.extent = {scissor.width, scissor.height};
    vkCmdSetScissor(m_Cmd, 0, 1, &rect);
}

void VulkanCommandList::BindGraphicsPipeline(RHIGraphicsPipelineHandle pipeline) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    if (auto* p = m_Device->FindGraphicsPipeline(pipeline); p && p->pipeline) {
        vkCmdBindPipeline(m_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p->pipeline);
    }
}

void VulkanCommandList::BindComputePipeline(RHIComputePipelineHandle pipeline) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    if (auto* p = m_Device->FindComputePipeline(pipeline); p && p->pipeline) {
        vkCmdBindPipeline(m_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE, p->pipeline);
    }
}

void VulkanCommandList::BindVertexBuffer(uint32_t binding, RHIBufferHandle buffer, uint64_t offset) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    VulkanBuffer* buf = m_Device->FindBuffer(buffer);
    if (!buf) {
        return;
    }
    VkDeviceSize off = offset;
    vkCmdBindVertexBuffers(m_Cmd, binding, 1, &buf->buffer, &off);
}

void VulkanCommandList::BindIndexBuffer(RHIBufferHandle buffer, uint64_t offset, IndexType type) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    VulkanBuffer* buf = m_Device->FindBuffer(buffer);
    if (!buf) {
        return;
    }
    const VkIndexType indexType = type == IndexType::UInt16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
    vkCmdBindIndexBuffer(m_Cmd, buf->buffer, offset, indexType);
}

void VulkanCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    if (m_Cmd) {
        vkCmdDraw(m_Cmd, vertexCount, instanceCount, firstVertex, firstInstance);
    }
}

void VulkanCommandList::DrawIndexed(
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t firstIndex,
    int32_t vertexOffset,
    uint32_t firstInstance)
{
    if (m_Cmd) {
        vkCmdDrawIndexed(m_Cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
}

void VulkanCommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    if (m_Cmd) {
        vkCmdDispatch(m_Cmd, groupCountX, groupCountY, groupCountZ);
    }
}

void VulkanCommandList::CopyBuffer(
    RHIBufferHandle src,
    RHIBufferHandle dst,
    uint64_t size,
    uint64_t srcOffset,
    uint64_t dstOffset)
{
    if (!m_Cmd || !m_Device) {
        return;
    }
    VulkanBuffer* s = m_Device->FindBuffer(src);
    VulkanBuffer* d = m_Device->FindBuffer(dst);
    if (!s || !d) {
        return;
    }
    VkBufferCopy region{};
    region.srcOffset = srcOffset;
    region.dstOffset = dstOffset;
    region.size = size;
    vkCmdCopyBuffer(m_Cmd, s->buffer, d->buffer, 1, &region);
}

void VulkanCommandList::TransitionTexture(RHITextureHandle texture, ResourceState before, ResourceState after) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    VulkanTexture* tex = m_Device->FindTexture(texture);
    if (!tex || !tex->image) {
        return;
    }

    const VkImageLayout oldLayout = ToVkImageLayout(before);
    const VkImageLayout newLayout = ToVkImageLayout(after);
    if (oldLayout == newLayout) {
        tex->state = after;
        return;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = tex->image;
    barrier.subresourceRange.aspectMask = IsDepthFormat(tex->desc.format)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = tex->desc.mipLevels ? tex->desc.mipLevels : 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = tex->desc.arrayLayers ? tex->desc.arrayLayers : 1;
    barrier.srcAccessMask = AccessForLayout(oldLayout);
    barrier.dstAccessMask = AccessForLayout(newLayout);

    vkCmdPipelineBarrier(
        m_Cmd,
        StageForLayout(oldLayout),
        StageForLayout(newLayout),
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    tex->state = after;
}

void VulkanCommandList::PushDebugGroup(std::string_view name) {
    if (!m_Cmd || !vkCmdBeginDebugUtilsLabelEXT) {
        return;
    }
    VkDebugUtilsLabelEXT label{};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = name.data();
    vkCmdBeginDebugUtilsLabelEXT(m_Cmd, &label);
}

void VulkanCommandList::PopDebugGroup() {
    if (m_Cmd && vkCmdEndDebugUtilsLabelEXT) {
        vkCmdEndDebugUtilsLabelEXT(m_Cmd);
    }
}

void VulkanCommandList::InsertDebugMarker(std::string_view name) {
    if (!m_Cmd || !vkCmdInsertDebugUtilsLabelEXT) {
        return;
    }
    VkDebugUtilsLabelEXT label{};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = name.data();
    vkCmdInsertDebugUtilsLabelEXT(m_Cmd, &label);
}

} // namespace we::rhi::vulkan
