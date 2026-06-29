#include "Scene/Scene.hpp"
#include "Core/Logger.hpp"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace we::runtime::scene {

Scene::Scene(const std::shared_ptr<we::runtime::renderer::VulkanContext>& context, const std::shared_ptr<we::runtime::renderer::SceneRenderer>& renderer)
    : m_Context(context), m_Renderer(renderer) {
    volkInitialize();
    volkLoadInstance(m_Context->GetInstance());
    volkLoadDevice(m_Context->GetDevice());
}

Scene::~Scene() {
    DestroyEntity(0xFFFFFFFF); // Clean up all
}

void Scene::InitializeDefaultScene(VkBuffer cameraBuffer) {
    // 1. Ground Plane
    CreateEntity("Ground Plane", EntityType::Plane, cameraBuffer);
    Entity& plane = m_Entities.back();
    plane.Position = glm::vec3(0.0f, 0.0f, 0.0f);
    plane.Scale = glm::vec3(1.0f);
    plane.Color = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f); // Dark gray ground
    plane.Mode = 0; // Lit

    // 2. Editor Cube
    CreateEntity("Editor Cube", EntityType::Cube, cameraBuffer);
    Entity& cube = m_Entities.back();
    cube.Position = glm::vec3(0.0f, 0.5f, 0.0f); // Sit on ground
    cube.Scale = glm::vec3(1.0f);
    cube.Color = glm::vec4(0.8f, 0.3f, 0.3f, 1.0f); // Matte red cube
    cube.Mode = 0; // Lit

    // 3. Directional Light Icon
    CreateEntity("Directional Light", EntityType::DirectionalLight, cameraBuffer);
    Entity& light = m_Entities.back();
    light.Position = glm::vec3(5.0f, 8.0f, -2.0f);
    light.Scale = glm::vec3(0.4f);
    light.Color = glm::vec4(0.95f, 0.95f, 0.6f, 1.0f); // Yellow light icon
    light.Mode = 1; // Unlit

    // 4. Camera Icon
    CreateEntity("Main Camera Icon", EntityType::CameraIcon, cameraBuffer);
    Entity& cam = m_Entities.back();
    cam.Position = glm::vec3(0.0f, 6.0f, 12.0f);
    cam.Scale = glm::vec3(0.4f);
    cam.Color = glm::vec4(0.3f, 0.6f, 0.8f, 1.0f); // Cyan camera icon
    cam.Mode = 1; // Unlit
}

void Scene::CreateEntity(const std::string& name, EntityType type, VkBuffer cameraBuffer) {
    Entity entity{};
    entity.Name = name;
    entity.Type = type;
    
    // Default mode is Lit (0)
    entity.Mode = 0;

    // Allocate Uniform Buffer for object matrices/properties
    VkDeviceSize bufferSize = sizeof(we::runtime::renderer::SceneObjectUniform);
    m_Context->CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        entity.UniformBuffer,
        entity.UniformMemory
    );

    // Allocate Descriptor Set from global descriptor pool
    VkDescriptorSetLayout layout = m_Renderer->GetObjectDescLayout();
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Context->GetDescriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    if (vkAllocateDescriptorSets(m_Context->GetDevice(), &allocInfo, &entity.DescriptorSet) != VK_SUCCESS) {
        vkDestroyBuffer(m_Context->GetDevice(), entity.UniformBuffer, nullptr);
        vkFreeMemory(m_Context->GetDevice(), entity.UniformMemory, nullptr);
        throw std::runtime_error("Failed to allocate descriptor set for entity: " + name);
    }

    // Bind descriptor set to UBOs
    m_Renderer->UpdateObjectDescriptorSet(entity.DescriptorSet, cameraBuffer, entity.UniformBuffer);

    m_Entities.push_back(entity);
    HE_INFO("Created scene entity: " + name);
}

void Scene::DestroyEntity(size_t index) {
    VkDevice device = m_Context->GetDevice();

    // Wait for GPU to finish execution before destroying any entity resources
    vkDeviceWaitIdle(device);

    if (index == 0xFFFFFFFF) {
        // Destroy all
        for (auto& entity : m_Entities) {
            if (entity.UniformBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, entity.UniformBuffer, nullptr);
            if (entity.UniformMemory != VK_NULL_HANDLE) vkFreeMemory(device, entity.UniformMemory, nullptr);
        }
        m_Entities.clear();
        m_SelectedEntityIndex = -1;
        return;
    }

    if (index >= m_Entities.size()) return;

    Entity& entity = m_Entities[index];
    if (entity.UniformBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, entity.UniformBuffer, nullptr);
    if (entity.UniformMemory != VK_NULL_HANDLE) vkFreeMemory(device, entity.UniformMemory, nullptr);

    m_Entities.erase(m_Entities.begin() + index);

    if (m_SelectedEntityIndex == static_cast<int>(index)) {
        m_SelectedEntityIndex = -1;
    } else if (m_SelectedEntityIndex > static_cast<int>(index)) {
        m_SelectedEntityIndex--;
    }
}

void Scene::Update() {
    VkDevice device = m_Context->GetDevice();

    // Map each entity's uniform buffer and update model matrix, color, and mode
    for (auto& entity : m_Entities) {
        we::runtime::renderer::SceneObjectUniform ubo{};
        ubo.model = entity.GetModelMatrix();
        ubo.color = entity.Color;
        ubo.mode = entity.Mode;

        void* data;
        vkMapMemory(device, entity.UniformMemory, 0, sizeof(we::runtime::renderer::SceneObjectUniform), 0, &data);
        memcpy(data, &ubo, sizeof(we::runtime::renderer::SceneObjectUniform));
        vkUnmapMemory(device, entity.UniformMemory);
    }
}

bool Scene::HasSkyEnvironment() const {
    for (const auto& entity : m_Entities) {
        std::string nameLower = entity.Name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        // Supports common scene naming conventions until dedicated sky components are added.
        if (nameLower.find("sky") != std::string::npos ||
            nameLower.find("atmosphere") != std::string::npos ||
            nameLower.find("skydome") != std::string::npos ||
            nameLower.find("skybox") != std::string::npos ||
            nameLower.find("hdri") != std::string::npos ||
            nameLower.find("environment") != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace we::runtime::scene
