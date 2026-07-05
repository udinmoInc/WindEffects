#include "Environment/EnvironmentSystem.h"

#include "Environment/EnvironmentLighting.h"
#include "Environment/EnvironmentManager.h"
#include "Renderer/SceneRenderer.hpp"
#include "Core/Logger.hpp"
#include "Core/LogCategory.hpp"

#include <algorithm>

namespace we::runtime::world::environment {

namespace {

using we::runtime::scene::Entity;
using we::runtime::scene::EntityType;
using we::runtime::scene::Scene;

bool IsEnvironmentEntityType(EntityType type) {
    switch (type) {
    case EntityType::DirectionalLight:
    case EntityType::SkyLight:
    case EntityType::SkyAtmosphere:
    case EntityType::HeightFog:
    case EntityType::VolumetricClouds:
        return true;
    default:
        return false;
    }
}

bool IsExposureControllerEntity(const Entity& entity) {
    return entity.Name == kExposureControllerActorName
        && entity.Type == EntityType::EmptyActor;
}

} // namespace

EnvironmentSystem& EnvironmentSystem::Get() {
    static EnvironmentSystem instance;
    return instance;
}

void EnvironmentSystem::BindScene(const std::shared_ptr<Scene>& scene) {
    m_Scene = scene;
    DiscoverExistingActors();
}

void EnvironmentSystem::BindRenderer(const std::shared_ptr<we::runtime::renderer::SceneRenderer>& renderer) {
#if WE_HAS_VULKAN
    m_Renderer = renderer;
    // Environment uniforms are pushed from SyncFromScene / CreateEnvironment once actors exist.
#else
    (void)renderer;
#endif
}

Scene* EnvironmentSystem::GetScene() const {
    return m_Scene.lock().get();
}

bool EnvironmentSystem::HasEnvironment() const {
    return m_FolderEntityId != 0;
}

bool EnvironmentSystem::HasEnvironmentActors() const {
    return m_Sun.EntityId != 0
        || m_SkyLight.EntityId != 0
        || m_SkyAtmosphere.EntityId != 0
        || m_HeightFog.EntityId != 0
        || m_VolumetricClouds.EntityId != 0
        || m_ExposureController.EntityId != 0;
}

bool EnvironmentSystem::ActorExists(EntityType /*type*/, const char* name) const {
    const Scene* scene = GetScene();
    if (!scene) {
        return false;
    }
    // Match by actor name only. Type-only checks falsely match unrelated actors (e.g. the
    // "Environment" folder EmptyActor blocking "Environment Manager" spawns).
    return scene->HasEntityNamed(name);
}

void EnvironmentSystem::ApplySettingsToComponents(const EnvironmentSettings& settings) {
    m_Sun.ApplyDefaults();
    m_Sun.Intensity = settings.sunIntensity;
    m_Sun.TemperatureKelvin = settings.sunTemperature;
    m_Sun.Rotation = glm::vec3(settings.sunRotationPitch, settings.sunRotationYaw, 0.0f);
    m_Sun.Color = m_Sun.GetColorFromTemperature();
    m_Sun.CastDynamicShadows = true;

    m_SkyLight.ApplyDefaults();
    m_SkyLight.Intensity = settings.skyLightIntensity;
    m_SkyLight.RealTimeCapture = settings.skyLightRealTimeCapture;

    m_SkyAtmosphere.ApplyDefaults();
    m_SkyAtmosphere.RayleighScattering = settings.atmosphereRayleighScattering;
    m_SkyAtmosphere.MieScattering = settings.atmosphereMieScattering;

    m_HeightFog.ApplyDefaults();
    m_HeightFog.Density = settings.fogDensity;
    m_HeightFog.HeightFalloff = settings.fogHeightFalloff;
    m_HeightFog.StartDistance = settings.fogStartDistance;
    m_HeightFog.VolumetricFog = settings.enableVolumetricFog;

    m_VolumetricClouds.ApplyDefaults();
    m_VolumetricClouds.Enabled = settings.createVolumetricClouds;
    m_VolumetricClouds.Coverage = settings.cloudCoverage;
    m_VolumetricClouds.Altitude = settings.cloudAltitude;

    m_ExposureController.ApplyDefaults();
}

void EnvironmentSystem::ApplyComponentsToActors() {
    Scene* scene = GetScene();
    if (!scene) {
        return;
    }

    if (Entity* sun = scene->FindEntityById(m_Sun.EntityId)) {
        m_Sun.ApplyToEntity(sun->Position, sun->Rotation, sun->Color);
    }
    if (Entity* sky = scene->FindEntityById(m_SkyLight.EntityId)) {
        m_SkyLight.ApplyToEntity(sky->Color);
    }
    if (Entity* atmosphere = scene->FindEntityById(m_SkyAtmosphere.EntityId)) {
        atmosphere->Color = glm::vec4(m_SkyAtmosphere.GetRayleighColor(), 1.0f);
    }
    if (Entity* fog = scene->FindEntityById(m_HeightFog.EntityId)) {
        m_HeightFog.ApplyToEntity(fog->Color, fog->Scale);
    }
    if (Entity* clouds = scene->FindEntityById(m_VolumetricClouds.EntityId)) {
        clouds->Color = glm::vec4(m_VolumetricClouds.CloudColor, 1.0f);
        clouds->Scale = glm::vec3(0.5f + m_VolumetricClouds.Coverage);
    }
}

std::uint64_t EnvironmentSystem::EnsureFolder() {
    Scene* scene = GetScene();
    if (!scene) {
        return 0;
    }

    if (m_FolderEntityId != 0 && scene->FindEntityById(m_FolderEntityId) != nullptr) {
        return m_FolderEntityId;
    }

    const int existing = scene->FindEntityIndexByName(kEnvironmentFolderName);
    if (existing >= 0) {
        m_FolderEntityId = scene->GetEntities()[static_cast<size_t>(existing)].Id;
        return m_FolderEntityId;
    }

    scene->CreateEntity(kEnvironmentFolderName, EntityType::EmptyActor);
    Entity& folder = scene->GetEntities().back();
    folder.EditorOnly = true;
    folder.Mode = 1;
    folder.ParentId = 0;
    m_FolderEntityId = folder.Id;
    return m_FolderEntityId;
}

std::uint64_t EnvironmentSystem::SpawnActor(
    const char* name,
    EntityType type,
    std::uint64_t parentId,
    const std::function<void(Entity&)>& configure) {

    Scene* scene = GetScene();
#if WE_HAS_VULKAN
    if (!scene || !scene->IsCameraBufferAssigned()) {
#else
    if (!scene) {
#endif
        return 0;
    }

    if (ActorExists(type, name)) {
        const int index = scene->FindEntityIndexByName(name);
        if (index >= 0) {
            Entity& entity = scene->GetEntities()[static_cast<size_t>(index)];
            entity.ParentId = parentId;
            if (configure) {
                configure(entity);
            }
            return entity.Id;
        }
    }

    scene->CreateEntity(name, type);
    Entity& entity = scene->GetEntities().back();
    entity.ParentId = parentId;
    entity.EditorOnly = true;
    entity.Mode = 1;
    if (configure) {
        configure(entity);
    }
    return entity.Id;
}

void EnvironmentSystem::DestroyActor(std::uint64_t entityId) {
    Scene* scene = GetScene();
    if (!scene || entityId == 0) {
        return;
    }

    const int index = scene->FindEntityIndexById(entityId);
    if (index >= 0) {
        scene->DestroyEntity(static_cast<size_t>(index));
    }
}

void EnvironmentSystem::DestroyActorTree(std::uint64_t rootId) {
    Scene* scene = GetScene();
    if (!scene || rootId == 0) {
        return;
    }

    const std::vector<int> children = scene->FindChildIndices(rootId);
    for (int childIndex : children) {
        DestroyActorTree(scene->GetEntities()[static_cast<size_t>(childIndex)].Id);
    }
    DestroyActor(rootId);
}

void EnvironmentSystem::DiscoverExistingActors() {
    Scene* scene = GetScene();
    if (!scene) {
        return;
    }

    m_FolderEntityId = 0;
    m_EnvironmentManagerEntityId = 0;
    m_Sun.EntityId = 0;
    m_SkyLight.EntityId = 0;
    m_SkyAtmosphere.EntityId = 0;
    m_HeightFog.EntityId = 0;
    m_VolumetricClouds.EntityId = 0;
    m_ExposureController.EntityId = 0;

    for (const Entity& entity : scene->GetEntities()) {
        if (entity.Name == kEnvironmentFolderName && entity.Type == EntityType::EmptyActor) {
            m_FolderEntityId = entity.Id;
            continue;
        }
        if (entity.Name == kEnvironmentManagerActorName && entity.Type == EntityType::EmptyActor) {
            m_EnvironmentManagerEntityId = entity.Id;
            continue;
        }

        if (entity.Type == EntityType::DirectionalLight
            || entity.Name == kSunActorName
            || entity.Name == "Sun Light") {
            m_Sun.EntityId = entity.Id;
            m_Sun.SyncFromEntityTransform(entity.Position, entity.Rotation, entity.Color);
            m_Sun.Intensity = EnvironmentSettingsLoader::Get().GetSettings().sunIntensity;
            continue;
        }
        if (entity.Type == EntityType::SkyLight || entity.Name == kSkyLightActorName || entity.Name == "Sky Light") {
            m_SkyLight.EntityId = entity.Id;
            m_SkyLight.SyncFromEntity(entity.Color);
            continue;
        }
        if (entity.Type == EntityType::SkyAtmosphere || entity.Name == kSkyAtmosphereActorName || entity.Name == "Sky Atmosphere") {
            m_SkyAtmosphere.EntityId = entity.Id;
            continue;
        }
        if (entity.Type == EntityType::HeightFog || entity.Name == kHeightFogActorName || entity.Name == "Exponential Height Fog") {
            m_HeightFog.EntityId = entity.Id;
            m_HeightFog.SyncFromEntity(entity.Color, entity.Scale);
            continue;
        }
        if (entity.Type == EntityType::VolumetricClouds || entity.Name == kVolumetricCloudsActorName) {
            m_VolumetricClouds.EntityId = entity.Id;
            m_VolumetricClouds.Enabled = true;
            continue;
        }
        if (IsExposureControllerEntity(entity)) {
            m_ExposureController.EntityId = entity.Id;
            continue;
        }
    }

    ReparentEnvironmentActors();
}

void EnvironmentSystem::ReparentEnvironmentActors() {
    Scene* scene = GetScene();
    if (!scene) {
        return;
    }

    const std::uint64_t folderId = EnsureFolder();
    if (folderId == 0) {
        return;
    }

    auto reparent = [&](std::uint64_t entityId) {
        if (entityId == 0 || entityId == folderId) {
            return;
        }
        if (Entity* entity = scene->FindEntityById(entityId)) {
            entity->ParentId = folderId;
        }
    };

    reparent(m_EnvironmentManagerEntityId);
    reparent(m_Sun.EntityId);
    reparent(m_SkyLight.EntityId);
    reparent(m_SkyAtmosphere.EntityId);
    reparent(m_HeightFog.EntityId);
    reparent(m_VolumetricClouds.EntityId);
    reparent(m_ExposureController.EntityId);
}

bool EnvironmentSystem::EnsureDefaultEnvironment() {
    const EnvironmentSettings& settings = EnvironmentSettingsLoader::Get().GetSettings();
    if (!settings.autoCreateOnNewLevel) {
        HE_INFO("[Environment] Auto environment creation disabled by config.");
        return false;
    }

    ApplySettingsToComponents(settings);

    DiscoverExistingActors();
    if (!HasEnvironmentActors()) {
        CreateEnvironment();
    } else {
        EnsureFolder();
        ReparentEnvironmentActors();
        UpdateRendering();
        ApplyComponentsToActors();
        NotifyChanged();
    }

    DiscoverExistingActors();
    if (!HasEnvironmentActors()) {
        HE_ERROR("[Environment] Default environment is incomplete after initialization (EnvironmentSystem.cpp:EnsureDefaultEnvironment).");
        return false;
    }
    return true;
}

void EnvironmentSystem::CreateEnvironment() {
    Scene* scene = GetScene();
#if WE_HAS_VULKAN
    if (!scene || !scene->IsCameraBufferAssigned()) {
#else
    if (!scene) {
#endif
        return;
    }

    const EnvironmentSettings& settings = EnvironmentSettingsLoader::Get().GetSettings();
    ApplySettingsToComponents(settings);

    const std::uint64_t folderId = EnsureFolder();

    if (m_EnvironmentManagerEntityId == 0) {
        m_EnvironmentManagerEntityId = SpawnActor(kEnvironmentManagerActorName, EntityType::EmptyActor, folderId, [&](Entity& entity) {
            entity.Position = glm::vec3(0.0f);
            entity.Scale = glm::vec3(0.35f);
        });
    }

    if (settings.createDirectionalLight && m_Sun.EntityId == 0) {
        m_Sun.EntityId = SpawnActor(kSunActorName, EntityType::DirectionalLight, folderId, [&](Entity& entity) {
            m_Sun.ApplyToEntity(entity.Position, entity.Rotation, entity.Color);
            entity.Scale = glm::vec3(0.45f);
        });
    }

    if (settings.createSkyLight && m_SkyLight.EntityId == 0) {
        m_SkyLight.EntityId = SpawnActor(kSkyLightActorName, EntityType::SkyLight, folderId, [&](Entity& entity) {
            entity.Position = glm::vec3(-4.0f, 12.0f, 2.0f);
            entity.Scale = glm::vec3(0.4f);
            m_SkyLight.ApplyToEntity(entity.Color);
        });
    }

    if (settings.createSkyAtmosphere && m_SkyAtmosphere.EntityId == 0) {
        m_SkyAtmosphere.EntityId = SpawnActor(kSkyAtmosphereActorName, EntityType::SkyAtmosphere, folderId, [&](Entity& entity) {
            entity.Scale = glm::vec3(2.0f);
            entity.Color = glm::vec4(m_SkyAtmosphere.GetRayleighColor(), 1.0f);
        });
    }

    if (settings.createHeightFog && m_HeightFog.EntityId == 0) {
        m_HeightFog.EntityId = SpawnActor(kHeightFogActorName, EntityType::HeightFog, folderId, [&](Entity& entity) {
            entity.Position = glm::vec3(0.0f, 0.5f, 0.0f);
            m_HeightFog.ApplyToEntity(entity.Color, entity.Scale);
        });
    }

    if ((settings.createVolumetricClouds || m_VolumetricClouds.Enabled) && m_VolumetricClouds.EntityId == 0) {
        m_VolumetricClouds.Enabled = settings.createVolumetricClouds;
        if (m_VolumetricClouds.Enabled) {
            m_VolumetricClouds.EntityId = SpawnActor(kVolumetricCloudsActorName, EntityType::VolumetricClouds, folderId, [&](Entity& entity) {
                entity.Position = glm::vec3(0.0f, m_VolumetricClouds.Altitude * 0.001f, 0.0f);
                entity.Color = glm::vec4(m_VolumetricClouds.CloudColor, 1.0f);
                entity.Scale = glm::vec3(0.5f + m_VolumetricClouds.Coverage);
            });
        }
    }

    if (m_ExposureController.EntityId == 0) {
        m_ExposureController.EntityId = SpawnActor(kExposureControllerActorName, EntityType::EmptyActor, folderId, [&](Entity& entity) {
            entity.Position = glm::vec3(0.0f);
            entity.Scale = glm::vec3(0.3f);
        });
    }

    UpdateRendering();
    ApplyComponentsToActors();
    NotifyChanged();

    DiscoverExistingActors();
    if (!HasEnvironmentActors()) {
        HE_ERROR("[Environment] CreateEnvironment did not spawn required actors (EnvironmentSystem.cpp:CreateEnvironment).");
    } else {
        HE_INFO("[Environment] Default environment created.");
    }
}

void EnvironmentSystem::ResetEnvironment() {
    const EnvironmentSettings& settings = EnvironmentSettingsLoader::Get().GetSettings();
    ApplySettingsToComponents(settings);
    UpdateRendering();
    ApplyComponentsToActors();
    NotifyChanged();
    HE_INFO("[Environment] Environment reset to defaults.");
}

void EnvironmentSystem::RemoveEnvironment() {
    if (m_FolderEntityId != 0) {
        DestroyActorTree(m_FolderEntityId);
    } else {
        DestroyActor(m_Sun.EntityId);
        DestroyActor(m_SkyLight.EntityId);
        DestroyActor(m_SkyAtmosphere.EntityId);
        DestroyActor(m_HeightFog.EntityId);
        DestroyActor(m_VolumetricClouds.EntityId);
        DestroyActor(m_ExposureController.EntityId);
    }

    m_FolderEntityId = 0;
    m_EnvironmentManagerEntityId = 0;
    m_Sun.EntityId = 0;
    m_SkyLight.EntityId = 0;
    m_SkyAtmosphere.EntityId = 0;
    m_HeightFog.EntityId = 0;
    m_VolumetricClouds.EntityId = 0;
    m_ExposureController.EntityId = 0;

    UpdateRendering();
    NotifyChanged();
    HE_INFO("[Environment] Environment removed.");
}

void EnvironmentSystem::RebuildEnvironment() {
    RemoveEnvironment();
    CreateEnvironment();
}

void EnvironmentSystem::SetVolumetricFogEnabled(bool enabled) {
    m_HeightFog.VolumetricFog = enabled;
    EnvironmentSettings settings = EnvironmentSettingsLoader::Get().GetSettings();
    settings.enableVolumetricFog = enabled;
    EnvironmentSettingsLoader::Get().SaveSettings(settings);
    UpdateRendering();
    NotifyChanged();
}

void EnvironmentSystem::SetVolumetricCloudsEnabled(bool enabled) {
    m_VolumetricClouds.Enabled = enabled;
    if (enabled && m_VolumetricClouds.EntityId == 0) {
        const std::uint64_t folderId = EnsureFolder();
        m_VolumetricClouds.EntityId = SpawnActor(kVolumetricCloudsActorName, EntityType::VolumetricClouds, folderId, [&](Entity& entity) {
            entity.Position = glm::vec3(0.0f, m_VolumetricClouds.Altitude * 0.001f, 0.0f);
            entity.Color = glm::vec4(m_VolumetricClouds.CloudColor, 1.0f);
            entity.Scale = glm::vec3(0.5f + m_VolumetricClouds.Coverage);
        });
    } else if (!enabled && m_VolumetricClouds.EntityId != 0) {
        DestroyActor(m_VolumetricClouds.EntityId);
        m_VolumetricClouds.EntityId = 0;
    }

    EnvironmentSettings settings = EnvironmentSettingsLoader::Get().GetSettings();
    settings.createVolumetricClouds = enabled;
    EnvironmentSettingsLoader::Get().SaveSettings(settings);
    UpdateRendering();
    NotifyChanged();
}

bool EnvironmentSystem::IsVolumetricFogEnabled() const {
    return m_HeightFog.VolumetricFog;
}

bool EnvironmentSystem::IsVolumetricCloudsEnabled() const {
    return m_VolumetricClouds.Enabled && m_VolumetricClouds.EntityId != 0;
}

void EnvironmentSystem::ApplyPreset(EnvironmentPreset preset) {
    switch (preset) {
    case EnvironmentPreset::Sunny:
        m_Sun.Intensity = 10.0f;
        m_Sun.TemperatureKelvin = 6500;
        m_Sun.Rotation = glm::vec3(-45.0f, 35.0f, 0.0f);
        m_SkyLight.Intensity = 1.0f;
        m_HeightFog.Density = 0.01f;
        m_HeightFog.VolumetricFog = true;
        break;
    case EnvironmentPreset::Sunset:
        m_Sun.Intensity = 6.0f;
        m_Sun.TemperatureKelvin = 3200;
        m_Sun.Rotation = glm::vec3(-8.0f, 280.0f, 0.0f);
        m_SkyLight.Intensity = 0.7f;
        m_HeightFog.Density = 0.015f;
        break;
    case EnvironmentPreset::Night:
        m_Sun.Intensity = 0.15f;
        m_Sun.TemperatureKelvin = 9000;
        m_Sun.Rotation = glm::vec3(-75.0f, 120.0f, 0.0f);
        m_SkyLight.Intensity = 0.2f;
        m_HeightFog.Density = 0.005f;
        m_HeightFog.VolumetricFog = false;
        break;
    case EnvironmentPreset::Overcast:
        m_Sun.Intensity = 3.0f;
        m_Sun.TemperatureKelvin = 7000;
        m_Sun.Rotation = glm::vec3(-55.0f, 60.0f, 0.0f);
        m_SkyLight.Intensity = 1.4f;
        m_HeightFog.Density = 0.02f;
        break;
    case EnvironmentPreset::Foggy:
        m_Sun.Intensity = 4.0f;
        m_Sun.TemperatureKelvin = 6000;
        m_Sun.Rotation = glm::vec3(-25.0f, 45.0f, 0.0f);
        m_SkyLight.Intensity = 0.9f;
        m_HeightFog.Density = 0.06f;
        m_HeightFog.VolumetricFog = true;
        break;
    case EnvironmentPreset::Studio:
        m_Sun.Intensity = 8.0f;
        m_Sun.TemperatureKelvin = 5500;
        m_Sun.Rotation = glm::vec3(-35.0f, 25.0f, 0.0f);
        m_SkyLight.Intensity = 0.6f;
        m_HeightFog.Density = 0.0f;
        m_HeightFog.VolumetricFog = false;
        break;
    }

    UpdateRendering();
    ApplyComponentsToActors();
    NotifyChanged();
}

void EnvironmentSystem::SyncFromScene(const glm::vec3& cameraPosition) {
    Scene* scene = GetScene();
    if (!scene) {
        return;
    }

    if (m_Sun.EntityId == 0 || scene->FindEntityById(m_Sun.EntityId) == nullptr) {
        DiscoverExistingActors();
    }

    if (Entity* sun = scene->FindEntityById(m_Sun.EntityId)) {
        m_Sun.SyncFromEntityTransform(sun->Position, sun->Rotation, sun->Color);
    }
    if (Entity* sky = scene->FindEntityById(m_SkyLight.EntityId)) {
        m_SkyLight.SyncFromEntity(sky->Color);
    }
    if (Entity* fog = scene->FindEntityById(m_HeightFog.EntityId)) {
        m_HeightFog.SyncFromEntity(fog->Color, fog->Scale);
    }

    UpdateRendering(cameraPosition);
    ApplyComponentsToActors();
}

void EnvironmentSystem::SyncToScene() {
    ApplyComponentsToActors();
    NotifyChanged();
}

void EnvironmentSystem::UpdateRendering(const glm::vec3& cameraPosition) {
#if WE_HAS_VULKAN
    m_Manager.UpdateDerivedState(m_Sun, m_SkyLight, m_HeightFog, m_SkyAtmosphere, cameraPosition);

    if (auto renderer = m_Renderer.lock()) {
        const glm::vec3 worldOrigin = m_Manager.GetWorldOrigin(cameraPosition);
        const auto uniform = BuildSceneEnvironmentUniform(
            m_Sun, m_SkyLight, m_SkyAtmosphere, m_HeightFog, m_VolumetricClouds, m_ExposureController, worldOrigin);
        renderer->SetSceneEnvironment(uniform);

        static bool loggedExposure = false;
        if (!loggedExposure) {
            loggedExposure = true;
            WE_LOG_INFO(
                we::runtime::core::LogCategory::Environment.data(),
                "Environment exposure: EV=" + std::to_string(uniform.exposureEV)
                    + " auto=" + std::to_string(uniform.enableAutoExposure)
                    + " hdrSkyLum=" + std::to_string(uniform.hdrSkyLuminance)
                    + " sunIntensity=" + std::to_string(uniform.sunIntensity)
                    + " bloom=" + std::to_string(uniform.bloomIntensity));
        }
    }
#endif
}

EnvironmentActorKind EnvironmentSystem::GetActorKind(std::uint64_t entityId) const {
    if (entityId == m_FolderEntityId) return EnvironmentActorKind::Folder;
    if (entityId == m_EnvironmentManagerEntityId) return EnvironmentActorKind::EnvironmentManager;
    if (entityId == m_Sun.EntityId) return EnvironmentActorKind::DirectionalLight;
    if (entityId == m_SkyLight.EntityId) return EnvironmentActorKind::SkyLight;
    if (entityId == m_SkyAtmosphere.EntityId) return EnvironmentActorKind::SkyAtmosphere;
    if (entityId == m_HeightFog.EntityId) return EnvironmentActorKind::HeightFog;
    if (entityId == m_VolumetricClouds.EntityId) return EnvironmentActorKind::VolumetricClouds;
    if (entityId == m_ExposureController.EntityId) return EnvironmentActorKind::ExposureController;
    return EnvironmentActorKind::Folder;
}

void EnvironmentSystem::AddChangeListener(ChangeListener listener) {
    m_ChangeListeners.push_back(std::move(listener));
}

void EnvironmentSystem::NotifyChanged() {
    for (const ChangeListener& listener : m_ChangeListeners) {
        if (listener) {
            listener();
        }
    }
}

} // namespace we::runtime::world::environment
