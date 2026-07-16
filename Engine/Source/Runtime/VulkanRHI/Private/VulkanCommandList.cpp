#include "VulkanDevice.h"
#include "VulkanFormats.h"

#include <string>
#include <vector>

namespace we::rhi::vulkan {
namespace {

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

RHIResult<void> VulkanQueue::Submit(IRHICommandList& commandList) {
    SubmitDesc desc{};
    desc.commandList = &commandList;
    return Submit(desc);
}

RHIResult<void> VulkanQueue::Submit(const SubmitDesc& desc) {
    if (!m_Queue || !desc.commandList || !m_Device) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Invalid queue submit.", "Submit");
    }
    auto* vkCmd = static_cast<VulkanCommandList*>(desc.commandList);
    VkCommandBuffer cmd = vkCmd->GetVkCommandBuffer();
    if (!cmd) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Command list has no buffer.", "Submit");
    }

    std::vector<VkSemaphore> waits;
    std::vector<VkPipelineStageFlags> waitStages;
    waits.reserve(desc.waitSemaphores.size());
    waitStages.reserve(desc.waitSemaphores.size());
    for (auto h : desc.waitSemaphores) {
        if (auto* s = m_Device->FindSemaphore(h); s && s->semaphore) {
            waits.push_back(s->semaphore);
            waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT);
        }
    }

    std::vector<VkSemaphore> signals;
    signals.reserve(desc.signalSemaphores.size());
    for (auto h : desc.signalSemaphores) {
        if (auto* s = m_Device->FindSemaphore(h); s && s->semaphore) {
            signals.push_back(s->semaphore);
        }
    }

    VkFence fence = VK_NULL_HANDLE;
    if (auto* f = m_Device->FindFence(desc.signalFence); f) {
        fence = f->fence;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waits.size());
    submitInfo.pWaitSemaphores = waits.empty() ? nullptr : waits.data();
    submitInfo.pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signals.size());
    submitInfo.pSignalSemaphores = signals.empty() ? nullptr : signals.data();

    if (vkQueueSubmit(m_Queue, 1, &submitInfo, fence) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkQueueSubmit failed.", "Submit");
    }
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanQueue::WaitIdle() {
    if (m_Queue) {
        vkQueueWaitIdle(m_Queue);
    }
    return RHIResult<void>::Success();
}

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

void VulkanCommandList::BindDescriptorSets(
    PipelineBindPoint bindPoint,
    RHIPipelineLayoutHandle layout,
    uint32_t firstSet,
    std::span<const RHIDescriptorSetHandle> sets)
{
    if (!m_Cmd || !m_Device || sets.empty()) {
        return;
    }
    auto* pipeLayout = m_Device->FindPipelineLayout(layout);
    if (!pipeLayout || !pipeLayout->layout) {
        return;
    }
    std::vector<VkDescriptorSet> vkSets;
    vkSets.reserve(sets.size());
    for (auto h : sets) {
        auto* set = m_Device->FindDescriptorSet(h);
        if (!set || !set->set) {
            return;
        }
        vkSets.push_back(set->set);
    }
    const VkPipelineBindPoint bp = bindPoint == PipelineBindPoint::Compute
        ? VK_PIPELINE_BIND_POINT_COMPUTE
        : VK_PIPELINE_BIND_POINT_GRAPHICS;
    vkCmdBindDescriptorSets(
        m_Cmd,
        bp,
        pipeLayout->layout,
        firstSet,
        static_cast<uint32_t>(vkSets.size()),
        vkSets.data(),
        0,
        nullptr);
}

void VulkanCommandList::PushConstants(
    RHIPipelineLayoutHandle layout,
    ShaderStageFlags stages,
    uint32_t offset,
    std::span<const uint8_t> data)
{
    if (!m_Cmd || !m_Device || data.empty()) {
        return;
    }
    auto* pipeLayout = m_Device->FindPipelineLayout(layout);
    if (!pipeLayout || !pipeLayout->layout) {
        return;
    }
    vkCmdPushConstants(
        m_Cmd,
        pipeLayout->layout,
        ToVkShaderStageFlags(stages),
        offset,
        static_cast<uint32_t>(data.size()),
        data.data());
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

void VulkanCommandList::DrawIndirect(RHIBufferHandle buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) {
    if (!m_Cmd || !m_Device || drawCount == 0) {
        return;
    }
    VulkanBuffer* buf = m_Device->FindBuffer(buffer);
    if (!buf || !buf->buffer) {
        return;
    }
    const uint32_t byteStride = stride ? stride : 16u;
    vkCmdDrawIndirect(m_Cmd, buf->buffer, offset, drawCount, byteStride);
}

void VulkanCommandList::DrawIndexedIndirect(RHIBufferHandle buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) {
    if (!m_Cmd || !m_Device || drawCount == 0) {
        return;
    }
    VulkanBuffer* buf = m_Device->FindBuffer(buffer);
    if (!buf || !buf->buffer) {
        return;
    }
    const uint32_t byteStride = stride ? stride : 20u;
    vkCmdDrawIndexedIndirect(m_Cmd, buf->buffer, offset, drawCount, byteStride);
}

void VulkanCommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    if (m_Cmd) {
        vkCmdDispatch(m_Cmd, groupCountX, groupCountY, groupCountZ);
    }
}

void VulkanCommandList::DispatchIndirect(RHIBufferHandle buffer, uint64_t offset) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    VulkanBuffer* buf = m_Device->FindBuffer(buffer);
    if (!buf || !buf->buffer) {
        return;
    }
    vkCmdDispatchIndirect(m_Cmd, buf->buffer, offset);
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

void VulkanCommandList::CopyTexture(RHITextureHandle src, RHITextureHandle dst, const TextureCopyRegion& region) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    VulkanTexture* s = m_Device->FindTexture(src);
    VulkanTexture* d = m_Device->FindTexture(dst);
    if (!s || !d) {
        return;
    }
    VkImageCopy copy{};
    copy.srcSubresource.aspectMask = IsDepthFormat(s->desc.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    copy.srcSubresource.mipLevel = region.src.mipLevel;
    copy.srcSubresource.baseArrayLayer = region.src.baseLayer;
    copy.srcSubresource.layerCount = region.src.layerCount;
    copy.srcOffset = {
        static_cast<int32_t>(region.srcOffsetX),
        static_cast<int32_t>(region.srcOffsetY),
        static_cast<int32_t>(region.srcOffsetZ)};
    copy.dstSubresource.aspectMask = IsDepthFormat(d->desc.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    copy.dstSubresource.mipLevel = region.dst.mipLevel;
    copy.dstSubresource.baseArrayLayer = region.dst.baseLayer;
    copy.dstSubresource.layerCount = region.dst.layerCount;
    copy.dstOffset = {
        static_cast<int32_t>(region.dstOffsetX),
        static_cast<int32_t>(region.dstOffsetY),
        static_cast<int32_t>(region.dstOffsetZ)};
    copy.extent = {region.extent.width, region.extent.height, region.extent.depth};
    vkCmdCopyImage(
        m_Cmd,
        s->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        d->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy);
}

void VulkanCommandList::BlitTexture(
    RHITextureHandle src,
    RHITextureHandle dst,
    const TextureBlitRegion& region,
    Filter filter)
{
    if (!m_Cmd || !m_Device) {
        return;
    }
    VulkanTexture* s = m_Device->FindTexture(src);
    VulkanTexture* d = m_Device->FindTexture(dst);
    if (!s || !d) {
        return;
    }
    VkImageBlit blit{};
    blit.srcSubresource.aspectMask = IsDepthFormat(s->desc.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = region.src.mipLevel;
    blit.srcSubresource.baseArrayLayer = region.src.baseLayer;
    blit.srcSubresource.layerCount = region.src.layerCount;
    blit.srcOffsets[0] = {region.srcOffset0X, region.srcOffset0Y, region.srcOffset0Z};
    blit.srcOffsets[1] = {region.srcOffset1X, region.srcOffset1Y, region.srcOffset1Z};
    blit.dstSubresource.aspectMask = IsDepthFormat(d->desc.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = region.dst.mipLevel;
    blit.dstSubresource.baseArrayLayer = region.dst.baseLayer;
    blit.dstSubresource.layerCount = region.dst.layerCount;
    blit.dstOffsets[0] = {region.dstOffset0X, region.dstOffset0Y, region.dstOffset0Z};
    blit.dstOffsets[1] = {region.dstOffset1X, region.dstOffset1Y, region.dstOffset1Z};
    vkCmdBlitImage(
        m_Cmd,
        s->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        d->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &blit,
        ToVkFilter(filter));
}

void VulkanCommandList::CopyBufferToTexture(RHIBufferHandle src, RHITextureHandle dst, const BufferImageCopyRegion& region) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    VulkanBuffer* s = m_Device->FindBuffer(src);
    VulkanTexture* d = m_Device->FindTexture(dst);
    if (!s || !d) {
        return;
    }
    VkBufferImageCopy copy{};
    copy.bufferOffset = region.bufferOffset;
    copy.bufferRowLength = region.bufferRowLength;
    copy.bufferImageHeight = region.bufferImageHeight;
    copy.imageSubresource.aspectMask = IsDepthFormat(d->desc.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = region.image.mipLevel;
    copy.imageSubresource.baseArrayLayer = region.image.baseLayer;
    copy.imageSubresource.layerCount = region.image.layerCount;
    copy.imageOffset = {
        static_cast<int32_t>(region.imageOffsetX),
        static_cast<int32_t>(region.imageOffsetY),
        static_cast<int32_t>(region.imageOffsetZ)};
    copy.imageExtent = {region.imageExtent.width, region.imageExtent.height, region.imageExtent.depth};
    vkCmdCopyBufferToImage(m_Cmd, s->buffer, d->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
}

void VulkanCommandList::CopyTextureToBuffer(RHITextureHandle src, RHIBufferHandle dst, const BufferImageCopyRegion& region) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    VulkanTexture* s = m_Device->FindTexture(src);
    VulkanBuffer* d = m_Device->FindBuffer(dst);
    if (!s || !d) {
        return;
    }
    VkBufferImageCopy copy{};
    copy.bufferOffset = region.bufferOffset;
    copy.bufferRowLength = region.bufferRowLength;
    copy.bufferImageHeight = region.bufferImageHeight;
    copy.imageSubresource.aspectMask = IsDepthFormat(s->desc.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = region.image.mipLevel;
    copy.imageSubresource.baseArrayLayer = region.image.baseLayer;
    copy.imageSubresource.layerCount = region.image.layerCount;
    copy.imageOffset = {
        static_cast<int32_t>(region.imageOffsetX),
        static_cast<int32_t>(region.imageOffsetY),
        static_cast<int32_t>(region.imageOffsetZ)};
    copy.imageExtent = {region.imageExtent.width, region.imageExtent.height, region.imageExtent.depth};
    vkCmdCopyImageToBuffer(m_Cmd, s->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, d->buffer, 1, &copy);
}

void VulkanCommandList::TransitionTexture(RHITextureHandle texture, ResourceState before, ResourceState after) {
    ResourceBarrierDesc barrier{};
    barrier.isTexture = true;
    barrier.texture.texture = texture;
    barrier.texture.before = before;
    barrier.texture.after = after;
    ResourceBarrier(std::span<const ResourceBarrierDesc>(&barrier, 1));
}

void VulkanCommandList::ResourceBarrier(std::span<const ResourceBarrierDesc> barriers) {
    if (!m_Cmd || !m_Device || barriers.empty()) {
        return;
    }

    std::vector<VkImageMemoryBarrier> imageBarriers;
    std::vector<VkBufferMemoryBarrier> bufferBarriers;
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    for (const auto& b : barriers) {
        if (b.isTexture) {
            VulkanTexture* tex = m_Device->FindTexture(b.texture.texture);
            if (!tex || !tex->image) {
                continue;
            }
            const ResourceState before = b.texture.before;
            const ResourceState after = b.texture.after;
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = ToVkImageLayout(before);
            barrier.newLayout = ToVkImageLayout(after);
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = tex->image;
            barrier.subresourceRange.aspectMask = IsDepthFormat(tex->desc.format)
                ? VK_IMAGE_ASPECT_DEPTH_BIT
                : VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = b.texture.baseMip;
            barrier.subresourceRange.levelCount = b.texture.mipCount == ~0u
                ? (tex->desc.mipLevels ? tex->desc.mipLevels : 1u)
                : b.texture.mipCount;
            barrier.subresourceRange.baseArrayLayer = b.texture.baseLayer;
            barrier.subresourceRange.layerCount = b.texture.layerCount == ~0u
                ? (tex->desc.arrayLayers ? tex->desc.arrayLayers : 1u)
                : b.texture.layerCount;
            barrier.srcAccessMask = AccessForState(before);
            barrier.dstAccessMask = AccessForState(after);
            if (barrier.srcAccessMask == 0) {
                barrier.srcAccessMask = AccessForLayout(barrier.oldLayout);
            }
            if (barrier.dstAccessMask == 0) {
                barrier.dstAccessMask = AccessForLayout(barrier.newLayout);
            }
            srcStage |= StageForState(before);
            dstStage |= StageForState(after);
            imageBarriers.push_back(barrier);
            tex->state = after;
        } else {
            VulkanBuffer* buf = m_Device->FindBuffer(b.buffer.buffer);
            if (!buf || !buf->buffer) {
                continue;
            }
            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcAccessMask = AccessForState(b.buffer.before);
            barrier.dstAccessMask = AccessForState(b.buffer.after);
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = buf->buffer;
            barrier.offset = b.buffer.offset;
            barrier.size = b.buffer.size;
            srcStage |= StageForState(b.buffer.before);
            dstStage |= StageForState(b.buffer.after);
            bufferBarriers.push_back(barrier);
            buf->state = b.buffer.after;
        }
    }

    if (imageBarriers.empty() && bufferBarriers.empty()) {
        return;
    }
    vkCmdPipelineBarrier(
        m_Cmd,
        srcStage,
        dstStage,
        0,
        0,
        nullptr,
        static_cast<uint32_t>(bufferBarriers.size()),
        bufferBarriers.empty() ? nullptr : bufferBarriers.data(),
        static_cast<uint32_t>(imageBarriers.size()),
        imageBarriers.empty() ? nullptr : imageBarriers.data());
}

void VulkanCommandList::WriteTimestamp(RHIQueryPoolHandle pool, uint32_t queryIndex) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    auto* q = m_Device->FindQueryPool(pool);
    if (!q || !q->pool || queryIndex >= q->desc.count) {
        return;
    }
    vkCmdWriteTimestamp(m_Cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, q->pool, queryIndex);
}

void VulkanCommandList::BeginQuery(RHIQueryPoolHandle pool, uint32_t queryIndex) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    auto* q = m_Device->FindQueryPool(pool);
    if (!q || !q->pool || queryIndex >= q->desc.count) {
        return;
    }
    const VkQueryType type = q->desc.type == QueryType::Occlusion
        ? VK_QUERY_TYPE_OCCLUSION
        : (q->desc.type == QueryType::PipelineStatistics
            ? VK_QUERY_TYPE_PIPELINE_STATISTICS
            : VK_QUERY_TYPE_TIMESTAMP);
    if (type == VK_QUERY_TYPE_TIMESTAMP) {
        return;
    }
    vkCmdBeginQuery(m_Cmd, q->pool, queryIndex, 0);
}

void VulkanCommandList::EndQuery(RHIQueryPoolHandle pool, uint32_t queryIndex) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    auto* q = m_Device->FindQueryPool(pool);
    if (!q || !q->pool || queryIndex >= q->desc.count) {
        return;
    }
    if (q->desc.type == QueryType::Timestamp) {
        vkCmdWriteTimestamp(m_Cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, q->pool, queryIndex);
        return;
    }
    vkCmdEndQuery(m_Cmd, q->pool, queryIndex);
}

void VulkanCommandList::ResetQueryPool(RHIQueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount) {
    if (!m_Cmd || !m_Device) {
        return;
    }
    auto* q = m_Device->FindQueryPool(pool);
    if (!q || !q->pool) {
        return;
    }
    vkCmdResetQueryPool(m_Cmd, q->pool, firstQuery, queryCount);
}

void VulkanCommandList::PushDebugGroup(std::string_view name) {
    if (!m_Cmd || !vkCmdBeginDebugUtilsLabelEXT) {
        return;
    }
    const std::string labelName(name);
    VkDebugUtilsLabelEXT label{};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = labelName.c_str();
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
    const std::string labelName(name);
    VkDebugUtilsLabelEXT label{};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = labelName.c_str();
    vkCmdInsertDebugUtilsLabelEXT(m_Cmd, &label);
}

} // namespace we::rhi::vulkan

