// ==============================================================================
// WindEffects — Scene — Scene
// Public API surface for the Scene module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Scene/Export.h"
#include "Entity.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace we::runtime::ecs {
class Registry;
class SystemScheduler;
class World;
struct ExtractedFrameData;
class RenderExtractionSystem;
}

namespace we::runtime::scene {

// High-level container for an ECS World, systems, editor view projection,
// serialization hooks, and render extraction forwarding.
// ECS World is the authority for entity/component data.
class Scene {
public:
    SCENE_API Scene();
    SCENE_API ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    SCENE_API void CreateEntity(const std::string& name, EntityType type);
    SCENE_API bool HasEntityOfType(EntityType type) const;
    SCENE_API bool HasEntityNamed(const std::string& name) const;
    SCENE_API int FindEntityIndexById(std::uint64_t id) const;
    SCENE_API int FindEntityIndexByName(const std::string& name) const;
    SCENE_API Entity* FindEntityById(std::uint64_t id);
    SCENE_API const Entity* FindEntityById(std::uint64_t id) const;
    SCENE_API std::vector<int> FindChildIndices(std::uint64_t parentId) const;
    SCENE_API bool IsEmpty() const;
    SCENE_API void Clear();
    SCENE_API void DestroyEntity(size_t index);

    // Editor/outliner view projection rebuilt from ECS after each Update.
    // Mutations made here are pushed into ECS at the start of Update().
    std::vector<Entity>& GetEntities() { return m_ViewCache; }
    const std::vector<Entity>& GetEntities() const { return m_ViewCache; }

    int GetSelectedEntityIndex() const { return m_SelectedEntityIndex; }
    SCENE_API void SetSelectedEntityIndex(int index);

    std::uint64_t GetSelectedEntityId() const { return m_SelectedEntityId; }
    SCENE_API void SetSelectedEntityId(std::uint64_t id);

    SCENE_API we::runtime::ecs::Registry& Registry();
    SCENE_API const we::runtime::ecs::Registry& Registry() const;
    SCENE_API we::runtime::ecs::World& World();
    SCENE_API const we::runtime::ecs::World& World() const;
    SCENE_API we::runtime::ecs::SystemScheduler& Systems();

    // Frame-local render packet produced by RenderExtractionSystem.
    SCENE_API const we::runtime::ecs::ExtractedFrameData* GetExtractedFrame() const;

    SCENE_API void Update();
    SCENE_API void Update(float deltaSeconds);

    /// Mark the editor view / ECS bridge dirty so the next Update() runs a full sync+extract.
    SCENE_API void MarkDirty() { m_Dirty = true; }
    [[nodiscard]] SCENE_API bool IsDirty() const { return m_Dirty; }

    // Bridge: editor view → ECS (before systems) / ECS → view (after systems).
    SCENE_API void SyncViewToEcs();
    SCENE_API void RebuildViewFromEcs();

    // Deprecated aliases kept for Editor compatibility.
    SCENE_API void SyncLegacyToEcs();
    SCENE_API void SyncEcsToLegacy();

private:
    void AttachEcsComponents(const Entity& entity, std::uint64_t ecsEntityId);
    [[nodiscard]] bool PushViewEntityToEcs(const Entity& entity);
    [[nodiscard]] we::runtime::ecs::RenderExtractionSystem* FindExtractionSystem() const;

    std::unique_ptr<we::runtime::ecs::Registry> m_Registry;
    std::unique_ptr<we::runtime::ecs::SystemScheduler> m_Systems;
    std::vector<Entity> m_ViewCache;
    int m_SelectedEntityIndex = -1;
    std::uint64_t m_SelectedEntityId = 0;
    bool m_Dirty = true;
};

} // namespace we::runtime::scene
