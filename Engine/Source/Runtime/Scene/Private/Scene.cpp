#include "Scene/Scene.h"
#include "Core/Logger.h"
#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

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

} // namespace

Scene::Scene() = default;

Scene::~Scene() {
    DestroyEntity(0xFFFFFFFF);
}

void Scene::CreateEntity(const std::string& name, EntityType type) {
    if (type == EntityType::VolumetricClouds && HasEntityOfType(EntityType::VolumetricClouds)) {
        HE_ERROR("Only one Cloud actor is allowed per level; ignored duplicate spawn.");
        return;
    }

    Entity entity{};
    entity.Id = m_NextEntityId++;
    entity.Name = name;
    entity.Type = type;
    ApplyDefaultEntityProperties(entity, type);
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
    if (index == 0xFFFFFFFF) {
        m_Entities.clear();
        m_SelectedEntityIndex = -1;
        return;
    }

    if (index >= m_Entities.size()) return;

    const std::uint64_t destroyedId = m_Entities[index].Id;
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
}

#if WE_HAS_VULKAN

void Scene::Draw(VkCommandBuffer /*cmd*/, DrawMode /*drawMode*/) const {
}

#endif

} // namespace we::runtime::scene
