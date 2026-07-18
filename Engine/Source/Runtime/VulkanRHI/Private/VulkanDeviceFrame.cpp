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
RHIResult<RHIFenceHandle> VulkanDevice::CreateFence(const FenceDesc& desc) {
    VulkanFence fence{};
    fence.desc = desc;
    VkFenceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (desc.signaled) {
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }
    if (vkCreateFence(m_Device, &info, nullptr, &fence.fence) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateFence failed.", "CreateFence");
    }
    const auto handle = static_cast<RHIFenceHandle>(AllocHandle());
    m_Fences.emplace(static_cast<uint64_t>(handle), fence);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyFence(RHIFenceHandle handle) {
    if (!FindFence(handle)) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown fence.", "DestroyFence");
    }
    EnqueueDeferred(DeferredKind::Fence, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::WaitForFences(std::span<const RHIFenceHandle> fences, bool waitAll, uint64_t timeoutNs) {
    std::vector<VkFence> vkFences;
    vkFences.reserve(fences.size());
    for (auto h : fences) {
        if (auto* f = FindFence(h); f && f->fence) {
            vkFences.push_back(f->fence);
        }
    }
    if (vkFences.empty()) {
        return RHIResult<void>::Success();
    }
    const VkResult result = vkWaitForFences(
        m_Device,
        static_cast<uint32_t>(vkFences.size()),
        vkFences.data(),
        waitAll ? VK_TRUE : VK_FALSE,
        timeoutNs);
    if (result != VK_SUCCESS && result != VK_TIMEOUT) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkWaitForFences failed.", "WaitForFences");
    }
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::ResetFences(std::span<const RHIFenceHandle> fences) {
    std::vector<VkFence> vkFences;
    vkFences.reserve(fences.size());
    for (auto h : fences) {
        if (auto* f = FindFence(h); f && f->fence) {
            vkFences.push_back(f->fence);
        }
    }
    if (!vkFences.empty()) {
        vkResetFences(m_Device, static_cast<uint32_t>(vkFences.size()), vkFences.data());
    }
    return RHIResult<void>::Success();
}

bool VulkanDevice::IsFenceSignaled(RHIFenceHandle handle) {
    auto* f = FindFence(handle);
    if (!f || !f->fence) {
        return false;
    }
    return vkGetFenceStatus(m_Device, f->fence) == VK_SUCCESS;
}

RHIResult<RHISemaphoreHandle> VulkanDevice::CreateSemaphore(const SemaphoreDesc& desc) {
    VulkanSemaphore semaphore{};
    semaphore.desc = desc;
    VkSemaphoreCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(m_Device, &info, nullptr, &semaphore.semaphore) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateSemaphore failed.", "CreateSemaphore");
    }
    const auto handle = static_cast<RHISemaphoreHandle>(AllocHandle());
    m_Semaphores.emplace(static_cast<uint64_t>(handle), semaphore);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroySemaphore(RHISemaphoreHandle handle) {
    if (!FindSemaphore(handle)) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown semaphore.", "DestroySemaphore");
    }
    EnqueueDeferred(DeferredKind::Semaphore, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::SubmitOneTime(std::function<void(VkCommandBuffer)> record) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(m_Device, &allocInfo, &cmd) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to allocate one-time command buffer.", "SubmitOneTime");
    }
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    record(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(m_GraphicsQueue.GetVkQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_GraphicsQueue.GetVkQueue());
    vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &cmd);
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::UpdateTexture(RHITextureHandle handle, const TextureUpdateDesc& update) {
    auto* tex = FindTexture(handle);
    if (!tex || !tex->image) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture.", "UpdateTexture");
    }
    if (update.data.empty()) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Empty texture update.", "UpdateTexture");
    }

    BufferDesc stagingDesc{};
    stagingDesc.size = update.data.size();
    stagingDesc.usage = BufferUsage::TransferSrc;
    stagingDesc.memory = MemoryUsage::HostVisible;
    auto staging = CreateBuffer(stagingDesc);
    if (!staging) {
        return staging.error;
    }
    if (void* mapped = MapBuffer(*staging)) {
        std::memcpy(mapped, update.data.data(), update.data.size());
        UnmapBuffer(*staging);
    }

    BufferImageCopyRegion region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    if (update.rowPitch > 0 && update.extent.width > 0) {
        // Desc documents rowPitch in bytes; Vulkan bufferRowLength is in texels.
        region.bufferRowLength = update.rowPitch / 4u;
        if (region.bufferRowLength == 0) {
            region.bufferRowLength = update.extent.width;
        }
    }
    region.image.mipLevel = update.mipLevel;
    region.image.baseLayer = update.arrayLayer;
    region.image.layerCount = 1;
    region.imageOffsetX = update.offsetX;
    region.imageOffsetY = update.offsetY;
    region.imageOffsetZ = update.offsetZ;
    region.imageExtent = update.extent;

    auto result = SubmitOneTime([&](VkCommandBuffer cmd) {
        VulkanCommandList list(this);
        list.SetCommandBuffer(cmd);
        list.Begin();
        const ResourceState old = tex->state;
        list.TransitionTexture(handle, old, ResourceState::CopyDst);
        list.CopyBufferToTexture(*staging, handle, region);
        list.TransitionTexture(handle, ResourceState::CopyDst, old == ResourceState::Undefined ? ResourceState::ShaderResource : old);
        list.End();
    });

    DestroyBuffer(*staging);
    return result;
}

IRHICommandList* VulkanDevice::BeginFrame() {
    if (!m_Valid || m_FrameActive) {
        return nullptr;
    }

    vkWaitForFences(m_Device, 1, &m_InFlight[m_FrameSlot], VK_TRUE, UINT64_MAX);
    vkResetFences(m_Device, 1, &m_InFlight[m_FrameSlot]);

    if (m_Swapchain) {
        m_Swapchain->SetFrameSlot(m_FrameSlot);
        auto acquire = m_Swapchain->AcquireNextImageForSlot(m_FrameSlot);
        if (!acquire) {
            if (acquire.error.code == RHIErrorCode::OutOfDate) {
                m_Swapchain->SetFrameSlot(m_FrameSlot);
                acquire = m_Swapchain->AcquireNextImageForSlot(m_FrameSlot);
            }
            if (!acquire) {
                return nullptr;
            }
        }
    }

    VkCommandBuffer cmd = m_CommandBuffers[m_FrameSlot];
    vkResetCommandBuffer(cmd, 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    m_FrameCmd.SetCommandBuffer(cmd);
    m_FrameCmd.Begin();
    m_FrameActive = true;
    m_ActiveCmd = &m_FrameCmd;
    return &m_FrameCmd;
}

RHIResult<void> VulkanDevice::Submit(IRHICommandList* commandList) {
    if (!commandList || !m_FrameActive) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "No active frame command list.", "Submit");
    }
    commandList->End();
    auto* vkCmd = static_cast<VulkanCommandList*>(commandList);
    VkCommandBuffer cmd = vkCmd->GetVkCommandBuffer();
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkEndCommandBuffer failed.", "Submit");
    }

    VkSemaphore waitSemaphores[] = {m_ImageAvailable[m_FrameSlot]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT};
    VkSemaphore signalSemaphores[] = {m_RenderFinished[m_FrameSlot]};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    if (m_Swapchain) {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
    }
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    if (vkQueueSubmit(m_GraphicsQueue.GetVkQueue(), 1, &submitInfo, m_InFlight[m_FrameSlot]) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkQueueSubmit failed.", "Submit");
    }
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::Present() {
    if (!m_Swapchain) {
        return RHIResult<void>::Success();
    }
    return m_Swapchain->PresentForSlot(m_FrameSlot);
}

RHIResult<void> VulkanDevice::EndFrame() {
    m_FrameActive = false;
    m_ActiveCmd = nullptr;
    m_FrameSlot = (m_FrameSlot + 1) % m_FramesInFlight;
    ++m_FrameNumber;
    ++m_Diagnostics.lastFrame.frameIndex;
    TickDeferredDestruction();
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::WaitIdle() {
    if (m_Device) {
        vkDeviceWaitIdle(m_Device);
    }
    return RHIResult<void>::Success();
}

void VulkanDevice::SetResourceName(RHIBufferHandle handle, std::string_view name) {
    if (!vkSetDebugUtilsObjectNameEXT) {
        return;
    }
    auto* buf = FindBuffer(handle);
    if (!buf) {
        return;
    }
    const std::string n(name);
    VkDebugUtilsObjectNameInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.objectType = VK_OBJECT_TYPE_BUFFER;
    info.objectHandle = reinterpret_cast<uint64_t>(buf->buffer);
    info.pObjectName = n.c_str();
    vkSetDebugUtilsObjectNameEXT(m_Device, &info);
}

void VulkanDevice::SetResourceName(RHITextureHandle handle, std::string_view name) {
    if (!vkSetDebugUtilsObjectNameEXT) {
        return;
    }
    auto* tex = FindTexture(handle);
    if (!tex) {
        return;
    }
    const std::string n(name);
    VkDebugUtilsObjectNameInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.objectType = VK_OBJECT_TYPE_IMAGE;
    info.objectHandle = reinterpret_cast<uint64_t>(tex->image);
    info.pObjectName = n.c_str();
    vkSetDebugUtilsObjectNameEXT(m_Device, &info);
}

void VulkanDevice::DestroyImmediate(DeferredKind kind, uint64_t handle) {
    switch (kind) {
    case DeferredKind::Buffer: {
        auto it = m_Buffers.find(handle);
        if (it == m_Buffers.end()) {
            break;
        }
        if (it->second.mapped) {
            vkUnmapMemory(m_Device, it->second.memory);
        }
        vkDestroyBuffer(m_Device, it->second.buffer, nullptr);
        vkFreeMemory(m_Device, it->second.memory, nullptr);
        m_Buffers.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::Texture: {
        auto it = m_Textures.find(handle);
        if (it == m_Textures.end() || it->second.isSwapchain) {
            break;
        }
        if (it->second.view) {
            vkDestroyImageView(m_Device, it->second.view, nullptr);
        }
        if (it->second.ownsImage && it->second.image) {
            vkDestroyImage(m_Device, it->second.image, nullptr);
        }
        if (it->second.memory) {
            vkFreeMemory(m_Device, it->second.memory, nullptr);
        }
        m_Textures.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::TextureView: {
        auto it = m_TextureViews.find(handle);
        if (it == m_TextureViews.end()) {
            break;
        }
        vkDestroyImageView(m_Device, it->second.view, nullptr);
        m_TextureViews.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::Sampler: {
        auto it = m_Samplers.find(handle);
        if (it == m_Samplers.end()) {
            break;
        }
        vkDestroySampler(m_Device, it->second.sampler, nullptr);
        m_Samplers.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::Shader: {
        auto it = m_Shaders.find(handle);
        if (it == m_Shaders.end()) {
            break;
        }
        vkDestroyShaderModule(m_Device, it->second.module, nullptr);
        m_Shaders.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::DescriptorSetLayout: {
        auto it = m_DescriptorSetLayouts.find(handle);
        if (it == m_DescriptorSetLayouts.end()) {
            break;
        }
        vkDestroyDescriptorSetLayout(m_Device, it->second.layout, nullptr);
        m_DescriptorSetLayouts.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::DescriptorPool: {
        auto it = m_DescriptorPools.find(handle);
        if (it == m_DescriptorPools.end()) {
            break;
        }
        for (auto setIt = m_DescriptorSets.begin(); setIt != m_DescriptorSets.end();) {
            if (static_cast<uint64_t>(setIt->second.pool) == handle) {
                setIt = m_DescriptorSets.erase(setIt);
            } else {
                ++setIt;
            }
        }
        vkDestroyDescriptorPool(m_Device, it->second.pool, nullptr);
        m_DescriptorPools.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::PipelineLayout: {
        auto it = m_PipelineLayouts.find(handle);
        if (it == m_PipelineLayouts.end()) {
            break;
        }
        vkDestroyPipelineLayout(m_Device, it->second.layout, nullptr);
        m_PipelineLayouts.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::GraphicsPipeline: {
        auto it = m_GraphicsPipelines.find(handle);
        if (it == m_GraphicsPipelines.end()) {
            break;
        }
        if (it->second.pipeline) {
            vkDestroyPipeline(m_Device, it->second.pipeline, nullptr);
        }
        m_GraphicsPipelines.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::ComputePipeline: {
        auto it = m_ComputePipelines.find(handle);
        if (it == m_ComputePipelines.end()) {
            break;
        }
        if (it->second.pipeline) {
            vkDestroyPipeline(m_Device, it->second.pipeline, nullptr);
        }
        m_ComputePipelines.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::Fence: {
        auto it = m_Fences.find(handle);
        if (it == m_Fences.end()) {
            break;
        }
        vkDestroyFence(m_Device, it->second.fence, nullptr);
        m_Fences.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::Semaphore: {
        auto it = m_Semaphores.find(handle);
        if (it == m_Semaphores.end()) {
            break;
        }
        vkDestroySemaphore(m_Device, it->second.semaphore, nullptr);
        m_Semaphores.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::CommandPool: {
        auto it = m_UserCommandPools.find(handle);
        if (it == m_UserCommandPools.end()) {
            break;
        }
        if (it->second.pool) {
            vkDestroyCommandPool(m_Device, it->second.pool, nullptr);
        }
        m_UserCommandPools.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::QueryPool: {
        auto it = m_QueryPools.find(handle);
        if (it == m_QueryPools.end()) {
            break;
        }
        if (it->second.pool) {
            vkDestroyQueryPool(m_Device, it->second.pool, nullptr);
        }
        m_QueryPools.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    }
}

void VulkanDevice::TickDeferredDestruction() {
    while (!m_Deferred.empty() && m_Deferred.front().frame <= m_FrameNumber) {
        auto item = m_Deferred.front();
        m_Deferred.pop_front();
        DestroyImmediate(item.kind, item.handle);
    }
}

RHIResult<RHICommandPoolHandle> VulkanDevice::CreateCommandPool(const CommandPoolDesc& desc) {
    if (!m_Device) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Device null.", "CreateCommandPool");
    }
    VulkanCommandPool pool{};
    pool.desc = desc;
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = m_GraphicsFamily;
    if (HasFlag(desc.flags, CommandPoolFlags::Transient)) {
        info.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    }
    if (HasFlag(desc.flags, CommandPoolFlags::ResetCommandBuffer)) {
        info.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    }
    if (vkCreateCommandPool(m_Device, &info, nullptr, &pool.pool) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateCommandPool failed.", "CreateCommandPool");
    }
    const auto handle = static_cast<RHICommandPoolHandle>(AllocHandle());
    m_UserCommandPools.emplace(static_cast<uint64_t>(handle), std::move(pool));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyCommandPool(RHICommandPoolHandle handle) {
    if (m_UserCommandPools.find(static_cast<uint64_t>(handle)) == m_UserCommandPools.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown command pool.", "DestroyCommandPool");
    }
    EnqueueDeferred(DeferredKind::CommandPool, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::ResetCommandPool(RHICommandPoolHandle handle) {
    auto it = m_UserCommandPools.find(static_cast<uint64_t>(handle));
    if (it == m_UserCommandPools.end() || !it->second.pool) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown command pool.", "ResetCommandPool");
    }
    vkResetCommandPool(m_Device, it->second.pool, 0);
    return RHIResult<void>::Success();
}

RHIResult<IRHICommandList*> VulkanDevice::AllocateCommandList(RHICommandPoolHandle poolHandle) {
    auto it = m_UserCommandPools.find(static_cast<uint64_t>(poolHandle));
    if (it == m_UserCommandPools.end() || !it->second.pool) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown command pool.", "AllocateCommandList");
    }
    VkCommandBufferAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool = it->second.pool;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(m_Device, &alloc, &cmd) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkAllocateCommandBuffers failed.", "AllocateCommandList");
    }
    auto list = std::make_unique<VulkanCommandList>(this);
    list->SetCommandBuffer(cmd);
    IRHICommandList* raw = list.get();
    it->second.lists.push_back(std::move(list));
    it->second.buffers.push_back(cmd);
    return raw;
}

RHIResult<RHIQueryPoolHandle> VulkanDevice::CreateQueryPool(const QueryPoolDesc& desc) {
    if (!m_Device || desc.count == 0) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Invalid query pool desc.", "CreateQueryPool");
    }
    VulkanQueryPool pool{};
    pool.desc = desc;
    VkQueryPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    info.queryCount = desc.count;
    switch (desc.type) {
    case QueryType::Occlusion:
        info.queryType = VK_QUERY_TYPE_OCCLUSION;
        break;
    case QueryType::PipelineStatistics:
        info.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
        break;
    case QueryType::Timestamp:
    default:
        info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        break;
    }
    if (vkCreateQueryPool(m_Device, &info, nullptr, &pool.pool) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateQueryPool failed.", "CreateQueryPool");
    }
    const auto handle = static_cast<RHIQueryPoolHandle>(AllocHandle());
    m_QueryPools.emplace(static_cast<uint64_t>(handle), pool);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyQueryPool(RHIQueryPoolHandle handle) {
    if (m_QueryPools.find(static_cast<uint64_t>(handle)) == m_QueryPools.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown query pool.", "DestroyQueryPool");
    }
    EnqueueDeferred(DeferredKind::QueryPool, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::GetQueryPoolResults(
    RHIQueryPoolHandle handle,
    uint32_t firstQuery,
    uint32_t queryCount,
    std::span<uint64_t> results,
    bool wait)
{
    auto* pool = FindQueryPool(handle);
    if (!pool || !pool->pool) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown query pool.", "GetQueryPoolResults");
    }
    if (results.size() < queryCount) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Results span too small.", "GetQueryPoolResults");
    }
    VkResult r = vkGetQueryPoolResults(
        m_Device,
        pool->pool,
        firstQuery,
        queryCount,
        results.size() * sizeof(uint64_t),
        results.data(),
        sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT | (wait ? VK_QUERY_RESULT_WAIT_BIT : 0));
    if (r != VK_SUCCESS && r != VK_NOT_READY) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkGetQueryPoolResults failed.", "GetQueryPoolResults");
    }
    return RHIResult<void>::Success();
}

ResourceState VulkanDevice::GetTextureState(RHITextureHandle handle) const {
    auto it = m_Textures.find(static_cast<uint64_t>(handle));
    return it == m_Textures.end() ? ResourceState::Undefined : it->second.state;
}

ResourceState VulkanDevice::GetBufferState(RHIBufferHandle handle) const {
    auto it = m_Buffers.find(static_cast<uint64_t>(handle));
    return it == m_Buffers.end() ? ResourceState::Undefined : it->second.state;
}

VulkanQueryPool* VulkanDevice::FindQueryPool(RHIQueryPoolHandle handle) {
    auto it = m_QueryPools.find(static_cast<uint64_t>(handle));
    return it == m_QueryPools.end() ? nullptr : &it->second;
}

std::unique_ptr<IRHIDevice> CreateVulkanDevice(const DeviceDesc& desc) {
    auto device = std::make_unique<VulkanDevice>(desc);
    if (!device->IsValid()) {
        return nullptr;
    }
    return device;
}

} // namespace we::rhi::vulkan
} // namespace we::rhi::vulkan
