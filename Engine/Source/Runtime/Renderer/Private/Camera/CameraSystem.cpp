#include "Camera/CameraSystem.h"
#include "Core/DeviceContext.h"
#include "Core/Validation.h"
#include "Resource/ResourceManager.h"

#include <cstring>

namespace we::runtime::renderer {

CameraSystem::~CameraSystem() {
    Shutdown();
}

void CameraSystem::Init(const CameraSystemConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "CameraSystem", "Already initialized.");
    WE_VALIDATE_INIT(config.deviceContext != nullptr, "CameraSystem", "DeviceContext is null.");
    WE_VALIDATE_INIT(config.resourceManager != nullptr, "CameraSystem", "ResourceManager is null.");

    m_DeviceContext = config.deviceContext;
    m_ResourceManager = config.resourceManager;

    CreateDescriptorLayout();
    CreateBuffers(config.maxFramesInFlight);
    CreateDescriptorSets(config.maxFramesInFlight);

    m_LastUploaded.resize(config.maxFramesInFlight);
    m_Initialized = true;
}

void CameraSystem::Shutdown() {
    if (!m_Initialized) return;

    VkDevice device = m_DeviceContext->GetDevice();

    if (m_DescriptorPool) {
        vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }

    if (m_DescriptorSetLayout) {
        vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
        m_DescriptorSetLayout = VK_NULL_HANDLE;
    }

    for (size_t i = 0; i < m_Buffers.size(); ++i) {
        if (m_Buffers[i]) {
            vkDestroyBuffer(device, m_Buffers[i], nullptr);
        }
        if (m_BufferMemories[i]) {
            vkFreeMemory(device, m_BufferMemories[i], nullptr);
        }
    }

    m_Buffers.clear();
    m_BufferMemories.clear();
    m_DescriptorSets.clear();
    m_LastUploaded.clear();
    m_Initialized = false;
}

void CameraSystem::CreateDescriptorLayout() {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    VkResult result = vkCreateDescriptorSetLayout(m_DeviceContext->GetDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout);
    WE_VALIDATE_INIT(result == VK_SUCCESS, "CameraSystem", "Failed to create descriptor set layout.");
}

void CameraSystem::CreateBuffers(uint32_t frameCount) {
    m_Buffers.resize(frameCount);
    m_BufferMemories.resize(frameCount);

    for (uint32_t i = 0; i < frameCount; ++i) {
        m_ResourceManager->CreateBuffer(
            sizeof(CameraUniform),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_Buffers[i],
            m_BufferMemories[i]);
    }
}

void CameraSystem::CreateDescriptorSets(uint32_t frameCount) {
    VkDevice device = m_DeviceContext->GetDevice();

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = frameCount;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = frameCount;

    VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool);
    WE_VALIDATE_INIT(result == VK_SUCCESS, "CameraSystem", "Failed to create descriptor pool.");

    std::vector<VkDescriptorSetLayout> layouts(frameCount, m_DescriptorSetLayout);
    m_DescriptorSets.resize(frameCount);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = frameCount;
    allocInfo.pSetLayouts = layouts.data();

    result = vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets.data());
    WE_VALIDATE_INIT(result == VK_SUCCESS, "CameraSystem", "Failed to allocate descriptor sets.");

    for (uint32_t i = 0; i < frameCount; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_Buffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraUniform);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

void CameraSystem::Upload(uint32_t frameIndex, const CameraUniform& uniform) {
    WE_VALIDATE_RENDER(m_Initialized, "CameraSystem::Upload", "CameraSystem not initialized.");
    WE_VALIDATE_RENDER(frameIndex < m_Buffers.size(), "CameraSystem::Upload", "Invalid frame index.");

    void* mapped = nullptr;
    VkResult result = vkMapMemory(m_DeviceContext->GetDevice(), m_BufferMemories[frameIndex], 0, sizeof(CameraUniform), 0, &mapped);
    WE_VALIDATE_RENDER(result == VK_SUCCESS, "CameraSystem::Upload", "Failed to map camera buffer memory.");

    std::memcpy(mapped, &uniform, sizeof(CameraUniform));
    vkUnmapMemory(m_DeviceContext->GetDevice(), m_BufferMemories[frameIndex]);

    m_LastUploaded[frameIndex] = uniform;
}

const CameraUniform& CameraSystem::GetLastUploaded(uint32_t frameIndex) const {
    return m_LastUploaded[frameIndex];
}

VkDescriptorSet CameraSystem::GetDescriptorSet(uint32_t frameIndex) const {
    return m_DescriptorSets[frameIndex];
}

VkBuffer CameraSystem::GetBuffer(uint32_t frameIndex) const {
    return m_Buffers[frameIndex];
}

} // namespace we::runtime::renderer
