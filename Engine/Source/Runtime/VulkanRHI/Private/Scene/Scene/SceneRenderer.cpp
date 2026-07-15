#include "Scene/SceneRenderer.h"
#include "Renderer/Renderer.h"
#include "Core/DeviceContext.h"
#include "Core/Validation.h"
#include "Core/AgentDebugLog.h"
#include "Resource/ResourceManager.h"

#include <cstring>

namespace we::runtime::renderer {

SceneRenderer::~SceneRenderer() {
    Shutdown();
}

void SceneRenderer::Init(const SceneRendererConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "SceneRenderer", "Already initialized.");
    (void)config.renderer;
    WE_VALIDATE_INIT(config.deviceContext != nullptr, "SceneRenderer", "DeviceContext is null.");
    WE_VALIDATE_INIT(config.resourceManager != nullptr, "SceneRenderer", "ResourceManager is null.");
    WE_VALIDATE_INIT(config.colorFormat != VK_FORMAT_UNDEFINED, "SceneRenderer", "Color format is undefined.");
    WE_VALIDATE_INIT(config.depthFormat != VK_FORMAT_UNDEFINED, "SceneRenderer", "Depth format is undefined.");

    m_Renderer = config.renderer;
    m_DeviceContext = config.deviceContext;
    m_ResourceManager = config.resourceManager;

    MeshPrimitivesConfig meshConfig{};
    meshConfig.deviceContext = config.deviceContext;
    meshConfig.resourceManager = config.resourceManager;
    m_MeshPrimitives.Init(meshConfig);

    DirectionalLightConfig lightConfig{};
    lightConfig.deviceContext = config.deviceContext;
    lightConfig.resourceManager = config.resourceManager;
    lightConfig.maxFramesInFlight = config.maxFramesInFlight;
    m_DirectionalLight.Init(lightConfig);

    EnvironmentUniformConfig envConfig{};
    envConfig.deviceContext = config.deviceContext;
    envConfig.resourceManager = config.resourceManager;
    envConfig.maxFramesInFlight = config.maxFramesInFlight;
    m_EnvironmentUniform.Init(envConfig);

    CreateObjectResources(config.maxFramesInFlight);
    UploadObjectUniforms();

    m_Initialized = true;
}

void SceneRenderer::Shutdown() {
    if (!m_Initialized) return;

    m_MeshPrimitives.Shutdown();
    m_DirectionalLight.Shutdown();
    m_EnvironmentUniform.Shutdown();

    VkDevice device = m_DeviceContext->GetDevice();

    if (m_ObjectDescriptorPool) {
        vkDestroyDescriptorPool(device, m_ObjectDescriptorPool, nullptr);
        m_ObjectDescriptorPool = VK_NULL_HANDLE;
    }

    if (m_ObjectDescriptorSetLayout) {
        vkDestroyDescriptorSetLayout(device, m_ObjectDescriptorSetLayout, nullptr);
        m_ObjectDescriptorSetLayout = VK_NULL_HANDLE;
    }

    for (size_t i = 0; i < m_ObjectBuffers.size(); ++i) {
        if (m_ObjectBuffers[i]) vkDestroyBuffer(device, m_ObjectBuffers[i], nullptr);
        if (m_ObjectMemories[i]) vkFreeMemory(device, m_ObjectMemories[i], nullptr);
    }

    m_ObjectBuffers.clear();
    m_ObjectMemories.clear();
    m_ObjectDescriptorSets.clear();
    m_Initialized = false;
}

void SceneRenderer::CreateObjectResources(uint32_t frameCount) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    VkResult result = vkCreateDescriptorSetLayout(m_DeviceContext->GetDevice(), &layoutInfo, nullptr, &m_ObjectDescriptorSetLayout);
    WE_VALIDATE_INIT(result == VK_SUCCESS, "SceneRenderer", "Failed to create object descriptor set layout.");

    m_ObjectBuffers.resize(frameCount);
    m_ObjectMemories.resize(frameCount);

    for (uint32_t i = 0; i < frameCount; ++i) {
        m_ResourceManager->CreateBuffer(
            sizeof(ObjectUniform),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_ObjectBuffers[i],
            m_ObjectMemories[i]);
    }

    VkDevice device = m_DeviceContext->GetDevice();

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = frameCount;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = frameCount;

    result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_ObjectDescriptorPool);
    WE_VALIDATE_INIT(result == VK_SUCCESS, "SceneRenderer", "Failed to create object descriptor pool.");

    std::vector<VkDescriptorSetLayout> layouts(frameCount, m_ObjectDescriptorSetLayout);
    m_ObjectDescriptorSets.resize(frameCount);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_ObjectDescriptorPool;
    allocInfo.descriptorSetCount = frameCount;
    allocInfo.pSetLayouts = layouts.data();

    result = vkAllocateDescriptorSets(device, &allocInfo, m_ObjectDescriptorSets.data());
    WE_VALIDATE_INIT(result == VK_SUCCESS, "SceneRenderer", "Failed to allocate object descriptor sets.");

    for (uint32_t i = 0; i < frameCount; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_ObjectBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(ObjectUniform);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_ObjectDescriptorSets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

void SceneRenderer::UploadObjectUniforms() {
#if WE_HAS_GLM
    m_ObjectUniform.model = glm::mat4(1.0f);
    m_ObjectUniform.color = glm::vec4(0.72f, 0.74f, 0.78f, 1.0f);
#endif

    for (size_t i = 0; i < m_ObjectBuffers.size(); ++i) {
        void* mapped = nullptr;
        vkMapMemory(m_DeviceContext->GetDevice(), m_ObjectMemories[i], 0, sizeof(ObjectUniform), 0, &mapped);
        std::memcpy(mapped, &m_ObjectUniform, sizeof(ObjectUniform));
        vkUnmapMemory(m_DeviceContext->GetDevice(), m_ObjectMemories[i]);
    }
}

void SceneRenderer::DrawMeshes(VkCommandBuffer cmd) const {
    WE_VALIDATE_RENDER(m_Initialized, "SceneRenderer::DrawMeshes", "SceneRenderer not initialized.");

    const MeshPrimitive& cube = m_MeshPrimitives.GetUnitCube();
    const VkBuffer buffers[] = { cube.vertexBuffer };
    const VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
    vkCmdBindIndexBuffer(cmd, cube.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    char dataJson[256];
    std::snprintf(
        dataJson,
        sizeof(dataJson),
        "{"
        "\"passName\":\"PBRPass\","
        "\"vertexBuffer\":%llu,"
        "\"indexBuffer\":%llu,"
        "\"indexCount\":%u,"
        "\"instanceCount\":1"
        "}",
        static_cast<unsigned long long>(reinterpret_cast<uint64_t>(cube.vertexBuffer)),
        static_cast<unsigned long long>(reinterpret_cast<uint64_t>(cube.indexBuffer)),
        cube.indexCount);

    // #region agent log
    AgentDebugLog(
        "H5",
        "SceneRenderer.cpp:DrawMeshes",
        "MESH_DRAW_INFO",
        dataJson);
    // #endregion

    vkCmdDrawIndexed(cmd, cube.indexCount, 1, 0, 0, 0);
}

VkDescriptorSet SceneRenderer::GetObjectDescriptorSet(uint32_t frameIndex) const {
    return m_ObjectDescriptorSets[frameIndex];
}

} // namespace we::runtime::renderer
