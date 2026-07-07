#include "Rendering/UICommandBuffer.h"
#include "Rendering/UIRenderer2.h"
#include "Core/Logger.h"
#include <cstring>

namespace we::UI {

UICommandBuffer::UICommandBuffer()
    : m_Device(VK_NULL_HANDLE)
    , m_MaxFramesInFlight(2)
    , m_CurrentFrameIndex(0)
    , m_DrawCallCount(0)
    , m_Recording(false)
{
}

UICommandBuffer::~UICommandBuffer() {
    Shutdown();
}

void UICommandBuffer::Initialize(VkDevice device, uint32_t maxFramesInFlight) {
    m_Device = device;
    m_MaxFramesInFlight = maxFramesInFlight;
    
    CreateCommandPool();
    CreateGeometryBuffers();
    
    m_FrameData.resize(maxFramesInFlight);
}

void UICommandBuffer::Shutdown() {
    if (m_CommandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }
    
    for (auto& frame : m_FrameData) {
        if (frame.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_Device, frame.vertexBuffer, nullptr);
            frame.vertexBuffer = VK_NULL_HANDLE;
        }
        if (frame.vertexMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device, frame.vertexMemory, nullptr);
            frame.vertexMemory = VK_NULL_HANDLE;
        }
        if (frame.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_Device, frame.indexBuffer, nullptr);
            frame.indexBuffer = VK_NULL_HANDLE;
        }
        if (frame.indexMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device, frame.indexMemory, nullptr);
            frame.indexMemory = VK_NULL_HANDLE;
        }
    }
    
    m_FrameData.clear();
}

void UICommandBuffer::BeginRecording(uint32_t frameIndex) {
    m_CurrentFrameIndex = frameIndex;
    m_DrawCallCount = 0;
    m_Recording = true;
    
    FrameData& frame = m_FrameData[frameIndex];
    frame.vertexStaging.clear();
    frame.indexStaging.clear();
    frame.vertexOffset = 0;
    frame.indexOffset = 0;
}

void UICommandBuffer::RecordDraw(const UIRenderBatch& batch,
                                  const std::vector<UIVertex2>& vertices,
                                  const std::vector<uint32_t>& indices) {
    if (!m_Recording) {
        return;
    }
    
    FrameData& frame = m_FrameData[m_CurrentFrameIndex];
    
    // Store geometry in staging buffers
    size_t vertexOffset = frame.vertexStaging.size();
    size_t indexOffset = frame.indexStaging.size();
    
    frame.vertexStaging.insert(frame.vertexStaging.end(), vertices.begin(), vertices.end());
    frame.indexStaging.insert(frame.indexStaging.end(), indices.begin(), indices.end());
    
    m_DrawCallCount++;
}

void UICommandBuffer::EndRecording() {
    if (!m_Recording) {
        return;
    }
    
    UpdateGeometryBuffers();
    m_Recording = false;
}

void UICommandBuffer::Execute(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout layout) {
    FrameData& frame = m_FrameData[m_CurrentFrameIndex];
    
    if (frame.vertexBuffer == VK_NULL_HANDLE || frame.indexBuffer == VK_NULL_HANDLE) {
        return;
    }
    
    VkBuffer vertexBuffers[] = {frame.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, frame.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

VkBuffer UICommandBuffer::GetVertexBuffer(uint32_t frameIndex) const {
    if (frameIndex >= m_FrameData.size()) {
        return VK_NULL_HANDLE;
    }
    return m_FrameData[frameIndex].vertexBuffer;
}

VkBuffer UICommandBuffer::GetIndexBuffer(uint32_t frameIndex) const {
    if (frameIndex >= m_FrameData.size()) {
        return VK_NULL_HANDLE;
    }
    return m_FrameData[frameIndex].indexBuffer;
}

VkDeviceSize UICommandBuffer::GetVertexOffset(uint32_t frameIndex) const {
    if (frameIndex >= m_FrameData.size()) {
        return 0;
    }
    return m_FrameData[frameIndex].vertexOffset;
}

VkDeviceSize UICommandBuffer::GetIndexOffset(uint32_t frameIndex) const {
    if (frameIndex >= m_FrameData.size()) {
        return 0;
    }
    return m_FrameData[frameIndex].indexOffset;
}

void UICommandBuffer::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = 0; // Would need to pass graphics queue family index
    
    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
        HE_ERROR("UICommandBuffer: Failed to create command pool");
    }
}

void UICommandBuffer::CreateGeometryBuffers() {
    for (uint32_t i = 0; i < m_MaxFramesInFlight; ++i) {
        FrameData& frame = m_FrameData[i];
        
        // Create vertex buffer (initial size, will grow as needed)
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = 1024 * 1024; // 1MB initial
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &frame.vertexBuffer) != VK_SUCCESS) {
            HE_ERROR("UICommandBuffer: Failed to create vertex buffer");
            continue;
        }
        
        // Allocate memory (simplified - would need proper memory type selection)
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Device, frame.vertexBuffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = 0;
        
        if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &frame.vertexMemory) != VK_SUCCESS) {
            vkDestroyBuffer(m_Device, frame.vertexBuffer, nullptr);
            frame.vertexBuffer = VK_NULL_HANDLE;
            continue;
        }
        
        vkBindBufferMemory(m_Device, frame.vertexBuffer, frame.vertexMemory, 0);
        
        // Create index buffer
        bufferInfo.size = 1024 * 1024; // 1MB initial
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        
        if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &frame.indexBuffer) != VK_SUCCESS) {
            HE_ERROR("UICommandBuffer: Failed to create index buffer");
            continue;
        }
        
        vkGetBufferMemoryRequirements(m_Device, frame.indexBuffer, &memRequirements);
        
        allocInfo.allocationSize = memRequirements.size;
        
        if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &frame.indexMemory) != VK_SUCCESS) {
            vkDestroyBuffer(m_Device, frame.indexBuffer, nullptr);
            frame.indexBuffer = VK_NULL_HANDLE;
            continue;
        }
        
        vkBindBufferMemory(m_Device, frame.indexBuffer, frame.indexMemory, 0);
    }
}

void UICommandBuffer::UpdateGeometryBuffers() {
    FrameData& frame = m_FrameData[m_CurrentFrameIndex];
    
    if (frame.vertexStaging.empty() || frame.indexStaging.empty()) {
        return;
    }
    
    // Check if buffers need to be resized
    VkDeviceSize vertexSize = frame.vertexStaging.size() * sizeof(UIVertex2);
    VkDeviceSize indexSize = frame.indexStaging.size() * sizeof(uint32_t);
    
    // Map and copy vertex data
    void* data;
    vkMapMemory(m_Device, frame.vertexMemory, 0, vertexSize, 0, &data);
    memcpy(data, frame.vertexStaging.data(), static_cast<size_t>(vertexSize));
    vkUnmapMemory(m_Device, frame.vertexMemory);
    
    // Map and copy index data
    vkMapMemory(m_Device, frame.indexMemory, 0, indexSize, 0, &data);
    memcpy(data, frame.indexStaging.data(), static_cast<size_t>(indexSize));
    vkUnmapMemory(m_Device, frame.indexMemory);
}

} // namespace we::UI
