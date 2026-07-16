#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/ArchetypeManager.h"
#include "ECS/Chunk.h"
#include "ECS/ComponentMask.h"
#include "ECS/ComponentType.h"
#include "ECS/Entity.h"
#include "ECS/EntityManager.h"
#include "ECS/Export.h"
#include "ECS/JobPool.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::ecs {

class CommandBufferQueue;

template <typename... Ts>
class ArchetypeQuery;

// Storage lives in a DLL-local Impl so STL teardown stays inside WEECS.dll.
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

    [[nodiscard]] EntityManager& Entities();
    [[nodiscard]] const EntityManager& Entities() const;
    [[nodiscard]] ArchetypeManager& Archetypes();
    [[nodiscard]] const ArchetypeManager& Archetypes() const;
    [[nodiscard]] ChunkAllocator& Chunks();
    [[nodiscard]] JobPool& Jobs();
    [[nodiscard]] CommandBufferQueue& Commands();

    [[nodiscard]] std::uint64_t StructuralVersion() const;
    [[nodiscard]] std::uint64_t ChangeVersion() const;
    void BumpChangeVersion();

    void FlushCommands();

    [[nodiscard]] ChunkView GetChunkView(Entity entity);
    [[nodiscard]] ChunkView GetChunkView(Entity entity) const;
    void MigrateAddComponent(Entity entity, ComponentTypeId typeId, const void* initialData);
    void MigrateRemoveComponent(Entity entity, ComponentTypeId typeId);
    void ApplySlotRelocation(const SlotRelocation& relocation);

private:
    friend class CommandBufferQueue;

    struct Impl;

    [[nodiscard]] ArchetypeLayout* GetEntityArchetype(Entity entity) const;
    [[nodiscard]] std::unordered_map<ComponentTypeId, std::vector<std::uint8_t>>& Singletons();
    [[nodiscard]] const std::unordered_map<ComponentTypeId, std::vector<std::uint8_t>>& Singletons() const;

    Impl* m_Impl = nullptr;
};

} // namespace we::runtime::ecs

#include "ECS/World.inl"
#include "ECS/ArchetypeQuery.inl"

#pragma warning(pop)
