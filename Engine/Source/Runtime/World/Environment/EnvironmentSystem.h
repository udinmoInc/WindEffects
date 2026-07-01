#pragma once

#include "Environment/EnvironmentDirectionalLight.h"
#include "Environment/EnvironmentHeightFog.h"
#include "Environment/EnvironmentSettings.h"
#include "Environment/EnvironmentSkyAtmosphere.h"
#include "Environment/EnvironmentSkyLight.h"
#include "Environment/EnvironmentTypes.h"
#include "Environment/EnvironmentVolumetricClouds.h"
#include "Scene/Scene.hpp"
#include "Renderer/SceneRenderer.hpp"
#include <functional>
#include <memory>

namespace we::runtime::world::environment {

class EnvironmentSystem {
public:
    static EnvironmentSystem& Get();

    void BindScene(const std::shared_ptr<we::runtime::scene::Scene>& scene);
    void BindRenderer(const std::shared_ptr<we::runtime::renderer::SceneRenderer>& renderer);

    bool HasEnvironment() const;
    bool HasEnvironmentActors() const;
    bool EnsureDefaultEnvironment();
    void CreateEnvironment();
    void ResetEnvironment();
    void RemoveEnvironment();
    void RebuildEnvironment();

    void SetVolumetricFogEnabled(bool enabled);
    void SetVolumetricCloudsEnabled(bool enabled);
    bool IsVolumetricFogEnabled() const;
    bool IsVolumetricCloudsEnabled() const;

    void ApplyPreset(EnvironmentPreset preset);

    void SyncFromScene();
    void SyncToScene();
    void UpdateRendering();

    EnvironmentDirectionalLight& GetSun() { return m_Sun; }
    EnvironmentSkyLight& GetSkyLight() { return m_SkyLight; }
    EnvironmentSkyAtmosphere& GetSkyAtmosphere() { return m_SkyAtmosphere; }
    EnvironmentHeightFog& GetHeightFog() { return m_HeightFog; }
    EnvironmentVolumetricClouds& GetVolumetricClouds() { return m_VolumetricClouds; }

    const EnvironmentDirectionalLight& GetSun() const { return m_Sun; }
    const EnvironmentSkyLight& GetSkyLight() const { return m_SkyLight; }
    const EnvironmentSkyAtmosphere& GetSkyAtmosphere() const { return m_SkyAtmosphere; }
    const EnvironmentHeightFog& GetHeightFog() const { return m_HeightFog; }
    const EnvironmentVolumetricClouds& GetVolumetricClouds() const { return m_VolumetricClouds; }

    std::uint64_t GetFolderEntityId() const { return m_FolderEntityId; }
    EnvironmentActorKind GetActorKind(std::uint64_t entityId) const;

    using ChangeListener = std::function<void()>;
    void AddChangeListener(ChangeListener listener);

    void NotifyChanged();

private:
    we::runtime::scene::Scene* GetScene() const;

    std::uint64_t EnsureFolder();
    std::uint64_t SpawnActor(
        const char* name,
        we::runtime::scene::EntityType type,
        std::uint64_t parentId,
        const std::function<void(we::runtime::scene::Entity&)>& configure);
    void DestroyActor(std::uint64_t entityId);
    void DestroyActorTree(std::uint64_t rootId);
    void ApplySettingsToComponents(const EnvironmentSettings& settings);
    void ApplyComponentsToActors();
    void DiscoverExistingActors();
    void ReparentEnvironmentActors();
    bool ActorExists(we::runtime::scene::EntityType type, const char* name) const;

    std::weak_ptr<we::runtime::scene::Scene> m_Scene;
    std::weak_ptr<we::runtime::renderer::SceneRenderer> m_Renderer;

    std::uint64_t m_FolderEntityId = 0;
    EnvironmentDirectionalLight m_Sun{};
    EnvironmentSkyLight m_SkyLight{};
    EnvironmentSkyAtmosphere m_SkyAtmosphere{};
    EnvironmentHeightFog m_HeightFog{};
    EnvironmentVolumetricClouds m_VolumetricClouds{};

    std::vector<ChangeListener> m_ChangeListeners;
};

} // namespace we::runtime::world::environment
