#include "Scene/Scene.hpp"
#include "Core/Logger.hpp"
#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif
#include <stdexcept>
#include <iostream>
#include <cmath>

namespace we::runtime::scene {

namespace {

void ApplyDefaultEntityProperties(Entity& entity, EntityType type) {
    entity.Mode = 0;

    switch (type) {
    case EntityType::Cube:
        entity.Color = glm::vec4(0.35f, 0.55f, 0.95f, 0.85f);
        entity.Position = glm::vec3(0.0f, 0.5f, 0.0f);
        break;
    case EntityType::Sphere:
        entity.Color = glm::vec4(0.45f, 0.75f, 0.55f, 0.85f);
        entity.Position = glm::vec3(0.0f, 0.5f, 0.0f);
        entity.Scale = glm::vec3(1.0f);
        break;
    case EntityType::Cylinder:
        entity.Color = glm::vec4(0.85f, 0.55f, 0.35f, 0.85f);
        entity.Position = glm::vec3(0.0f, 0.5f, 0.0f);
        break;
    case EntityType::Plane:
        entity.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        entity.Scale = glm::vec3(4.0f, 1.0f, 4.0f);
        break;
    case EntityType::DirectionalLight:
    case EntityType::PointLight:
    case EntityType::SpotLight:
    case EntityType::SkyLight:
    case EntityType::SkyAtmosphere:
    case EntityType::HeightFog:
    case EntityType::VolumetricClouds:
        entity.EditorOnly = true;
        entity.Mode = 1;
        break;
    case EntityType::CameraIcon:
    case EntityType::EmptyActor:
    case EntityType::Character:
    case EntityType::Blueprint:
    case EntityType::AudioSource:
    case EntityType::Volume:
        entity.EditorOnly = true;
        break;
    default:
        break;
    }
}

#if WE_HAS_VULKAN

const char* MeshNameForEntityType(EntityType type) {
    switch (type) {
    case EntityType::Plane: return "Plane";
    case EntityType::GroundPlane: return "Plane";
    default: return "Cube";
    }
}

#endif // WE_HAS_VULKAN

} // namespace

#if WE_HAS_VULKAN

Scene::Scene(const std::shared_ptr<we::runtime::renderer::VulkanContext>& context, const std::shared_ptr<we::runtime::renderer::SceneRenderer>& renderer)
    : m_Context(context), m_Renderer(renderer) {
    volkInitialize();
    volkLoadInstance(m_Context->GetInstance());
    volkLoadDevice(m_Context->GetDevice());
}

#else // !WE_HAS_VULKAN

Scene::Scene() {
}

#endif // WE_HAS_VULKAN

Scene::~Scene() {
    DestroyEntity(0xFFFFFFFF);
}

#if WE_HAS_VULKAN

void Scene::SetCameraBuffer(VkBuffer cameraBuffer) {
    m_CameraBuffer = cameraBuffer;
}

bool Scene::IsCameraBufferAssigned() const {
    return m_CameraBuffer != VK_NULL_HANDLE;
}

#endif // WE_HAS_VULKAN

void Scene::CreateEntity(const std::string& name, EntityType type) {
#if WE_HAS_VULKAN
    if (!IsCameraBufferAssigned()) {
        throw std::runtime_error("Scene camera buffer is not assigned.");
    }
#endif

    Entity entity{};
    entity.Id = m_NextEntityId++;
    entity.Name = name;
    entity.Type = type;
    ApplyDefaultEntityProperties(entity, type);

#if WE_HAS_VULKAN
    VkDeviceSize bufferSize = sizeof(we::runtime::renderer::SceneObjectUniform);
    m_Context->CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        entity.UniformBuffer,
        entity.UniformMemory
    );

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

    m_Renderer->UpdateObjectDescriptorSet(entity.DescriptorSet, m_CameraBuffer, entity.UniformBuffer);
#endif

    m_Entities.push_back(entity);
    HE_INFO("Created scene entity: " + name);
}

bool Scene::HasEntityOfType(EntityType type) const {
    for (const Entity& entity : m_Entities) {
        if (entity.Type == type) {
            return true;
        }
    }
    return false;
}

bool Scene::HasEntityNamed(const std::string& name) const {
    for (const Entity& entity : m_Entities) {
        if (entity.Name == name) {
            return true;
        }
    }
    return false;
}

int Scene::FindEntityIndexById(std::uint64_t id) const {
    for (size_t i = 0; i < m_Entities.size(); ++i) {
        if (m_Entities[i].Id == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int Scene::FindEntityIndexByName(const std::string& name) const {
    for (size_t i = 0; i < m_Entities.size(); ++i) {
        if (m_Entities[i].Name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Entity* Scene::FindEntityById(std::uint64_t id) {
    const int index = FindEntityIndexById(id);
    return index >= 0 ? &m_Entities[static_cast<size_t>(index)] : nullptr;
}

const Entity* Scene::FindEntityById(std::uint64_t id) const {
    const int index = FindEntityIndexById(id);
    return index >= 0 ? &m_Entities[static_cast<size_t>(index)] : nullptr;
}

std::vector<int> Scene::FindChildIndices(std::uint64_t parentId) const {
    std::vector<int> children;
    for (size_t i = 0; i < m_Entities.size(); ++i) {
        if (m_Entities[i].ParentId == parentId) {
            children.push_back(static_cast<int>(i));
        }
    }
    return children;
}

bool Scene::IsEmpty() const {
    return m_Entities.empty();
}

void Scene::Clear() {
    DestroyEntity(0xFFFFFFFF);
}

void Scene::DestroyEntity(size_t index) {
#if WE_HAS_VULKAN
    VkDevice device = m_Context->GetDevice();
    vkDeviceWaitIdle(device);
#endif

    if (index == 0xFFFFFFFF) {
#if WE_HAS_VULKAN
        for (auto& entity : m_Entities) {
            if (entity.UniformBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, entity.UniformBuffer, nullptr);
            if (entity.UniformMemory != VK_NULL_HANDLE) vkFreeMemory(device, entity.UniformMemory, nullptr);
        }
#endif
        m_Entities.clear();
        m_SelectedEntityIndex = -1;
        return;
    }

    if (index >= m_Entities.size()) return;

    Entity& entity = m_Entities[index];
    const std::uint64_t destroyedId = entity.Id;
#if WE_HAS_VULKAN
    if (entity.UniformBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, entity.UniformBuffer, nullptr);
    if (entity.UniformMemory != VK_NULL_HANDLE) vkFreeMemory(device, entity.UniformMemory, nullptr);
#endif

    m_Entities.erase(m_Entities.begin() + index);

    for (auto& childEntity : m_Entities) {
        if (childEntity.ParentId == destroyedId) {
            childEntity.ParentId = 0;
        }
    }

    if (m_SelectedEntityIndex == static_cast<int>(index)) {
        m_SelectedEntityIndex = -1;
    } else if (m_SelectedEntityIndex > static_cast<int>(index)) {
        m_SelectedEntityIndex--;
    }
}

void Scene::Update() {
#if WE_HAS_VULKAN
    VkDevice device = m_Context->GetDevice();

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
#endif
}

#if WE_HAS_VULKAN

void Scene::Draw(VkCommandBuffer cmd, DrawMode drawMode) const {
    for (const auto& entity : m_Entities) {
        if (drawMode == DrawMode::Game && entity.EditorOnly) {
            continue;
        }
        m_Renderer->DrawMesh(cmd, MeshNameForEntityType(entity.Type), entity.DescriptorSet, entity.Mode);
    }
}

#endif // WE_HAS_VULKAN

} // namespace we::runtime::scene
