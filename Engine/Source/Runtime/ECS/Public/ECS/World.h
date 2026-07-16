#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/ArchetypeManager.h"
#include "ECS/Chunk.h"
#include "ECS/CommandBuffer.h"
#include "ECS/ComponentMask.h"
#include "ECS/ComponentType.h"
#include "ECS/Entity.h"
#include "ECS/EntityManager.h"
#include "ECS/Export.h"
#include "ECS/JobPool.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::ecs {

template <typename... Ts>
class ArchetypeQuery;

// Top-level runtime container: entity manager + archetype/chunk storage + queries.
class ECS_API World {
public:
    World();
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    Entity CreateEntity();
    Entity CreateEntity(const std::string& name);
    void DestroyEntity(Entity entity);
    [[nodiscard]] bool Valid(Entity entity) const;
    void Clear();

    [[nodiscard]] bool IsEntityEnabled(Entity entity) const;
    void SetEntityEnabled(Entity entity, bool enabled);

    template <typename T, typename... Args>
    T& AddComponent(Entity entity, Args&&... args);

    template <typename T>
    bool RemoveComponent(Entity entity);

    template <typename T, typename... Args>
    T& GetOrAdd(Entity entity, Args&&... args);

    template <typename T>
    T* TryGet(Entity entity);

    template <typename T>
    const T* TryGet(Entity entity) const;

    template <typename T>
    T& Get(Entity entity);

    template <typename T>
    const T& Get(Entity entity) const;

    template <typename T>
    bool Has(Entity entity) const;

    template <typename T>
    void SetComponentEnabled(Entity entity, bool enabled);

    template <typename T>
    bool IsComponentEnabled(Entity entity) const;

    template <typename T, typename... Args>
    T& SetSingleton(Args&&... args);

    template <typename T>
    T* TryGetSingleton();

    template <typename T>
    const T* TryGetSingleton() const;

    template <typename... Ts>
    ArchetypeQuery<Ts...> QueryAll();

    [[nodiscard]] EntityManager& Entities() { return m_Entities; }
    [[nodiscard]] const EntityManager& Entities() const { return m_Entities; }
    [[nodiscard]] ArchetypeManager& Archetypes() { return m_Archetypes; }
    [[nodiscard]] const ArchetypeManager& Archetypes() const { return m_Archetypes; }
    [[nodiscard]] ChunkAllocator& Chunks() { return m_ChunkAllocator; }
    [[nodiscard]] JobPool& Jobs() { return m_JobPool; }
    [[nodiscard]] CommandBufferQueue& Commands() { return m_Commands; }

    [[nodiscard]] std::uint64_t StructuralVersion() const;
    [[nodiscard]] std::uint64_t ChangeVersion() const { return m_ChangeVersion; }
    void BumpChangeVersion() { ++m_ChangeVersion; }

    void FlushCommands();

    // Low-level access for systems and extraction paths.
    [[nodiscard]] ChunkView GetChunkView(Entity entity);
    void MigrateAddComponent(Entity entity, ComponentTypeId typeId, const void* initialData);
    void MigrateRemoveComponent(Entity entity, ComponentTypeId typeId);
    void ApplySlotRelocation(const SlotRelocation& relocation);

private:
    friend class CommandBufferQueue;

    [[nodiscard]] ArchetypeLayout* GetEntityArchetype(Entity entity) const;
    void WriteComponentDefault(ComponentTypeId typeId, void* dst);
    void EnsureDefaultComponents(Entity entity);

    EntityManager m_Entities;
    ArchetypeManager m_Archetypes;
    ChunkAllocator m_ChunkAllocator;
    JobPool m_JobPool;
    CommandBufferQueue m_Commands;
    std::unordered_map<ComponentTypeId, std::vector<std::uint8_t>> m_Singletons;
    ComponentMask m_DefaultEntityMask{};
    std::uint64_t m_ChangeVersion = 1;
};

} // namespace we::runtime::ecs

#include "ECS/World.inl"
#include "ECS/ArchetypeQuery.inl"

#pragma warning(pop)
