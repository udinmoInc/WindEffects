#include "Rendering/UIRenderer2.hpp"
#include "Rendering/UIGeometryCache.hpp"
#include "Rendering/UIBatchGenerator.hpp"
#include "Rendering/UITextureAtlasManager.hpp"
#include "Rendering/UICommandBuffer.hpp"
#include "Rendering/UICompositor.hpp"
#include "Rendering/UIStateManager.hpp"
#include "Core/Widget.hpp"
#include "Core/PaintContext.hpp"
#include "Rendering/FontAtlas.hpp"
#include "Rendering/IconRenderer.hpp"
#include "Rendering/UiGpuUpload.h"
#include "Core/Logger.hpp"
#include "Runtime/Core/AssetRegistry.hpp"
#include <volk.h>
#include <cstring>
#include <chrono>

namespace we::UI {

UIRenderer2::UIRenderer2()
    : m_MaxFramesInFlight(2)
    , m_CurrentFrameIndex(0)
    , m_CurrentWidth(0)
    , m_CurrentHeight(0)
    , m_FullRedrawNeeded(true)
{
}

UIRenderer2::~UIRenderer2() {
    Shutdown();
}

bool UIRenderer2::Init(VkPhysicalDevice physicalDevice,
                       VkDevice logicalDevice,
                       VkQueue graphicsQueue,
                       uint32_t graphicsQueueFamilyIndex,
                       VkFormat swapchainFormat,
                       uint32_t maxFramesInFlight,
                       we::runtime::renderer::DeviceContext* deviceContext,
                       we::runtime::renderer::ResourceManager* resourceManager) {
    HE_INFO("UIRenderer2: Init called");
    
    // Validate parameters
    if (!physicalDevice || !logicalDevice || !graphicsQueue) {
        HE_ERROR("UIRenderer2: Invalid Vulkan parameters");
        return false;
    }
    if (!deviceContext || !resourceManager) {
        HE_ERROR("UIRenderer2: DeviceContext or ResourceManager is null");
        return false;
    }

    HE_INFO("UIRenderer2: Parameters validated");

    m_PhysicalDevice = physicalDevice;
    m_LogicalDevice = logicalDevice;
    m_GraphicsQueue = graphicsQueue;
    m_GraphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
    m_SwapchainFormat = swapchainFormat;
    m_MaxFramesInFlight = maxFramesInFlight;
    m_DeviceContext = deviceContext;
    m_ResourceManager = resourceManager;
    
    HE_INFO("UIRenderer2: Parameters set, skipping component initialization for debugging");
    
    // TODO: Re-enable component initialization after fixing crash
    // For now, just return true to allow the app to start
    HE_INFO("UIRenderer2: Initialization deferred - components not yet implemented");
    return true;
}

void UIRenderer2::Shutdown() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    // Shutdown components
    if (m_GeometryCache) {
        m_GeometryCache->Shutdown();
        m_GeometryCache.reset();
    }
    
    if (m_BatchGenerator) {
        m_BatchGenerator->Shutdown();
        m_BatchGenerator.reset();
    }
    
    if (m_TextureAtlasManager) {
        m_TextureAtlasManager->Shutdown();
        m_TextureAtlasManager.reset();
    }
    
    if (m_CommandBuffer) {
        m_CommandBuffer->Shutdown();
        m_CommandBuffer.reset();
    }
    
    if (m_Compositor) {
        m_Compositor->Shutdown();
        m_Compositor.reset();
    }
    
    if (m_StateManager) {
        m_StateManager->Shutdown();
        m_StateManager.reset();
    }
    
    // Shutdown font and icon renderers
    if (m_IconRenderer) {
        m_IconRenderer->Shutdown();
        m_IconRenderer.reset();
    }
    
    if (m_FontAtlas) {
        m_FontAtlas->Shutdown();
        m_FontAtlas.reset();
    }
    
    // Destroy Vulkan resources
    if (m_GraphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_LogicalDevice, m_GraphicsPipeline, nullptr);
        m_GraphicsPipeline = VK_NULL_HANDLE;
    }
    
    if (m_PipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }
    
    if (m_TextureDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_LogicalDevice, m_TextureDescriptorSetLayout, nullptr);
        m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
    }
    
    if (m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
    
    // Destroy geometry buffers
    for (auto& frame : m_FrameGeometry) {
        if (frame.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_LogicalDevice, frame.vertexBuffer, nullptr);
            frame.vertexBuffer = VK_NULL_HANDLE;
        }
        if (frame.vertexMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_LogicalDevice, frame.vertexMemory, nullptr);
            frame.vertexMemory = VK_NULL_HANDLE;
        }
        if (frame.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_LogicalDevice, frame.indexBuffer, nullptr);
            frame.indexBuffer = VK_NULL_HANDLE;
        }
        if (frame.indexMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_LogicalDevice, frame.indexMemory, nullptr);
            frame.indexMemory = VK_NULL_HANDLE;
        }
    }
    m_FrameGeometry.clear();
}

void UIRenderer2::BeginFrame(uint32_t frameIndex, uint32_t width, uint32_t height) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    m_CurrentFrameIndex = frameIndex;
    m_CurrentWidth = width;
    m_CurrentHeight = height;
    
    // Reset frame stats
    m_FrameStats = {};
    
    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Evict old geometry cache entries
    m_GeometryCache->EvictOldEntries(frameIndex, 60); // Evict after 60 frames
    
    // Begin command recording
    m_CommandBuffer->BeginRecording(frameIndex);
    
    // Begin batch generation
    m_BatchGenerator->BeginBatching(width, height);
    
    m_FrameStats.cpuTimeMs = 0.0f;
}

void UIRenderer2::RenderWidget(const std::shared_ptr<Widget>& root) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (!root) {
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Layout and paint widget tree
    root->Measure(Size{static_cast<float>(m_CurrentWidth), static_cast<float>(m_CurrentHeight)});
    root->Arrange(Rect{0.0f, 0.0f, static_cast<float>(m_CurrentWidth), static_cast<float>(m_CurrentHeight)});
    
    PaintContext paintCtx;
    root->Paint(paintCtx);
    
    // Process paint commands and generate geometry
    // This would convert paint commands to UIVertex2 and indices
    // For now, this is a placeholder for the full implementation
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_FrameStats.cpuTimeMs += std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

void UIRenderer2::EndFrame(VkCommandBuffer cmd, VkImageView swapchainImageView) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    // End batch generation
    m_BatchGenerator->EndBatching();
    
    // Update frame stats
    m_FrameStats.drawCalls = m_CommandBuffer->GetDrawCallCount();
    m_FrameStats.vertices = m_BatchGenerator->GetVertexCount();
    m_FrameStats.indices = m_BatchGenerator->GetIndexCount();
    m_FrameStats.batches = m_BatchGenerator->GetBatchCount();
    m_FrameStats.cacheHits = m_GeometryCache->GetCacheHitCount();
    m_FrameStats.cacheMisses = m_GeometryCache->GetCacheMissCount();
    m_FrameStats.dirtyRegions = static_cast<uint32_t>(m_DirtyRegions.size());
    
    // Save current Vulkan state
    SavedVulkanState savedState;
    m_StateManager->SaveState(cmd, savedState);
    
    // Begin UI composite pass
    m_Compositor->BeginComposite(cmd, swapchainImageView, m_CurrentWidth, m_CurrentHeight);
    
    // Bind UI pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
    
    // Set viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_CurrentWidth);
    viewport.height = static_cast<float>(m_CurrentHeight);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    
    // Set scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {m_CurrentWidth, m_CurrentHeight};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    
    // Set push constants
    float pushConstants[4];
    pushConstants[0] = 2.0f / static_cast<float>(m_CurrentWidth);
    pushConstants[1] = 2.0f / static_cast<float>(m_CurrentHeight);
    pushConstants[2] = -1.0f;
    pushConstants[3] = -1.0f;
    vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4, pushConstants);
    
    // Bind geometry buffers
    VkBuffer vertexBuffer = m_CommandBuffer->GetVertexBuffer(m_CurrentFrameIndex);
    VkBuffer indexBuffer = m_CommandBuffer->GetIndexBuffer(m_CurrentFrameIndex);
    
    if (vertexBuffer != VK_NULL_HANDLE && indexBuffer != VK_NULL_HANDLE) {
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize vertexOffsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, vertexOffsets);
        vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        // Draw batches
        const auto& batches = m_BatchGenerator->GetBatches();
        for (const auto& batch : batches) {
            // Bind texture descriptor set
            if (batch.textureSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                       m_PipelineLayout, 0, 1, &batch.textureSet, 0, nullptr);
            }
            
            // Set scissor for this batch
            VkRect2D batchScissor{};
            batchScissor.offset.x = static_cast<int32_t>(batch.scissor[0]);
            batchScissor.offset.y = static_cast<int32_t>(batch.scissor[1]);
            batchScissor.extent.width = static_cast<uint32_t>(batch.scissor[2]);
            batchScissor.extent.height = static_cast<uint32_t>(batch.scissor[3]);
            vkCmdSetScissor(cmd, 0, 1, &batchScissor);
            
            // Draw
            vkCmdDrawIndexed(cmd, batch.indexCount, 1, batch.firstIndex, batch.vertexOffset, 0);
        }
    }
    
    // End composite pass
    m_Compositor->EndComposite(cmd);
    
    // Restore Vulkan state
    m_StateManager->RestoreState(cmd, savedState);
    
    // End command recording
    m_CommandBuffer->EndRecording();
    
    // Clear dirty regions
    m_DirtyRegions.clear();
    m_FullRedrawNeeded = false;
}

VkDescriptorSet UIRenderer2::RegisterTexture(VkImageView imageView, VkSampler sampler) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (m_DescriptorPool == VK_NULL_HANDLE || m_TextureDescriptorSetLayout == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_TextureDescriptorSetLayout;
    
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        HE_ERROR("UIRenderer2: Failed to allocate descriptor set");
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
    
    vkUpdateDescriptorSets(m_LogicalDevice, 1, &descriptorWrite, 0, nullptr);
    
    return descriptorSet;
}

void UIRenderer2::UpdateTexture(VkDescriptorSet descriptorSet, VkImageView imageView, VkSampler sampler) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (descriptorSet == VK_NULL_HANDLE) {
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
    
    vkUpdateDescriptorSets(m_LogicalDevice, 1, &descriptorWrite, 0, nullptr);
}

void UIRenderer2::UnregisterTexture(VkDescriptorSet descriptorSet) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (descriptorSet != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(m_LogicalDevice, m_DescriptorPool, 1, &descriptorSet);
    }
}

FontAtlas* UIRenderer2::GetFontAtlas() const {
    return m_FontAtlas.get();
}

IconRenderer* UIRenderer2::GetIconRenderer() const {
    return m_IconRenderer.get();
}

void UIRenderer2::ResetStats() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_FrameStats = {};
    m_GeometryCache->ResetStats();
}

void UIRenderer2::InvalidateRegion(const UIDirtyRegion& region) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_DirtyRegions.push_back(region);
}

void UIRenderer2::InvalidateAll() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_FullRedrawNeeded = true;
    m_GeometryCache->InvalidateAll();
}

void UIRenderer2::SaveVulkanState(VkCommandBuffer cmd) {
    // State saving is handled by UIStateManager
}

void UIRenderer2::RestoreVulkanState(VkCommandBuffer cmd) {
    // State restoration is handled by UIStateManager
}

void UIRenderer2::CreateGraphicsPipeline(VkFormat colorFormat) {
    // TODO: Load shaders from UI2.hlsl using ShaderLibrary
    // For now, skip pipeline creation to allow compilation
    // The pipeline will be created when shader loading is implemented
    HE_WARN("UIRenderer2: Pipeline creation deferred - shader loading not yet implemented");
    (void)colorFormat;
}

void UIRenderer2::CreateDescriptorSetLayouts() {
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
    
    if (vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &m_TextureDescriptorSetLayout) != VK_SUCCESS) {
        HE_ERROR("UIRenderer2: Failed to create descriptor set layout");
    }
}

void UIRenderer2::CreatePipelineLayout() {
    // Pipeline layout is created in CreateGraphicsPipeline
}

void UIRenderer2::CreateDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 4096;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 2048;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    
    if (vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
        HE_ERROR("UIRenderer2: Failed to create descriptor pool");
    }
}

void UIRenderer2::CreateGeometryBuffers(uint32_t frameIndex) {
    if (frameIndex >= m_FrameGeometry.size()) {
        return;
    }
    
    FrameGeometry& frame = m_FrameGeometry[frameIndex];
    
    // Create vertex buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = 1024 * 1024; // 1MB initial
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr, &frame.vertexBuffer) != VK_SUCCESS) {
        HE_ERROR("UIRenderer2: Failed to create vertex buffer");
        return;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_LogicalDevice, frame.vertexBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = 0; // Would need proper memory type selection
    
    if (vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, &frame.vertexMemory) != VK_SUCCESS) {
        vkDestroyBuffer(m_LogicalDevice, frame.vertexBuffer, nullptr);
        frame.vertexBuffer = VK_NULL_HANDLE;
        return;
    }
    
    vkBindBufferMemory(m_LogicalDevice, frame.vertexBuffer, frame.vertexMemory, 0);
    frame.vertexSize = memRequirements.size;
    
    // Create index buffer
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    
    if (vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr, &frame.indexBuffer) != VK_SUCCESS) {
        HE_ERROR("UIRenderer2: Failed to create index buffer");
        return;
    }
    
    vkGetBufferMemoryRequirements(m_LogicalDevice, frame.indexBuffer, &memRequirements);
    
    allocInfo.allocationSize = memRequirements.size;
    
    if (vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, &frame.indexMemory) != VK_SUCCESS) {
        vkDestroyBuffer(m_LogicalDevice, frame.indexBuffer, nullptr);
        frame.indexBuffer = VK_NULL_HANDLE;
        return;
    }
    
    vkBindBufferMemory(m_LogicalDevice, frame.indexBuffer, frame.indexMemory, 0);
    frame.indexSize = memRequirements.size;
}

void UIRenderer2::UpdateGeometryBuffers(uint32_t frameIndex) {
    if (frameIndex >= m_FrameGeometry.size()) {
        return;
    }
    
    FrameGeometry& frame = m_FrameGeometry[frameIndex];
    
    // Update buffers with data from batch generator
    const auto& vertices = m_BatchGenerator->GetVertices();
    const auto& indices = m_BatchGenerator->GetIndices();
    
    if (vertices.empty() || indices.empty()) {
        return;
    }
    
    VkDeviceSize vertexSize = vertices.size() * sizeof(UIVertex2);
    VkDeviceSize indexSize = indices.size() * sizeof(uint32_t);
    
    // Check if buffers need to be resized
    if (vertexSize > frame.vertexSize) {
        // Would need to reallocate buffer
    }
    
    if (indexSize > frame.indexSize) {
        // Would need to reallocate buffer
    }
    
    // Map and copy vertex data
    void* data;
    vkMapMemory(m_LogicalDevice, frame.vertexMemory, 0, vertexSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(vertexSize));
    vkUnmapMemory(m_LogicalDevice, frame.vertexMemory);
    
    // Map and copy index data
    vkMapMemory(m_LogicalDevice, frame.indexMemory, 0, indexSize, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(indexSize));
    vkUnmapMemory(m_LogicalDevice, frame.indexMemory);
}

} // namespace we::UI
