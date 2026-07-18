#include "ECS/World.h"

#include "ECS/CommandBuffer.h"
#include "ECS/ComponentOps.h"
#include "ECS/Components/CoreComponents.h"
#include "ECS/EntityManager.h"
#include "ECS/JobPool.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>

// World pimpl: all STL storage stays inside WEECS.dll.

namespace we::runtime::ecs {

struct World::Impl {
    EntityManager entities;
    ChunkAllocator chunkAllocator;
    ArchetypeManager archetypes;
    JobPool jobs;
    CommandBufferQueue commands{32};
    std::unordered_map<ComponentTypeId, std::vector<std::uint8_t>> singletons;
    ComponentMask defaultEntityMask{};
    std::uint64_t changeVersion = 1;
};

World::World()
    : m_Impl(std::make_unique<Impl>())
{
    ComponentTypeRegistry::Get().EnsureCoreTypesRegistered();
    m_Impl->archetypes.Init(&m_Impl->chunkAllocator, &ComponentTypeRegistry::Get());

    m_Impl->defaultEntityMask.Set(ComponentTypeRegistry::Get().Id<UuidComponent>());
    m_Impl->defaultEntityMask.Set(ComponentTypeRegistry::Get().Id<NameComponent>());
    m_Impl->defaultEntityMask.Set(ComponentTypeRegistry::Get().Id<TransformComponent>());
    m_Impl->defaultEntityMask.Set(ComponentTypeRegistry::Get().Id<HierarchyComponent>());
    m_Impl->defaultEntityMask.Set(ComponentTypeRegistry::Get().Id<VisibilityComponent>());
}

World::~World() {
    Clear();
    m_Impl.reset();
}

EntityManager& World::Entities() { return m_Impl->entities; }
const EntityManager& World::Entities() const { return m_Impl->entities; }
ArchetypeManager& World::Archetypes() { return m_Impl->archetypes; }
const ArchetypeManager& World::Archetypes() const { return m_Impl->archetypes; }
ChunkAllocator& World::Chunks() { return m_Impl->chunkAllocator; }
JobPool& World::Jobs() { return m_Impl->jobs; }
CommandBufferQueue& World::Commands() { return m_Impl->commands; }

std::uint64_t World::ChangeVersion() const { return m_Impl->changeVersion; }
void World::BumpChangeVersion() { ++m_Impl->changeVersion; }

std::unordered_map<ComponentTypeId, std::vector<std::uint8_t>>& World::Singletons() {
    return m_Impl->singletons;
}
const std::unordered_map<ComponentTypeId, std::vector<std::uint8_t>>& World::Singletons() const {
    return m_Impl->singletons;
}

void World::Clear() {
    if (!m_Impl) {
        return;
    }
    for (ArchetypeLayout* archetype : m_Impl->archetypes.All()) {
        if (!archetype) {
            continue;
        }
        for (Chunk* chunk : archetype->chunks) {
            if (chunk) {
                m_Impl->chunkAllocator.DestroyChunk(chunk);
            }
        }
        archetype->chunks.clear();
        archetype->entityCount = 0;
    }
    m_Impl->entities.Clear();
    m_Impl->singletons.clear();
    m_Impl->commands.ClearAll();
    BumpChangeVersion();
}

Entity World::CreateEntity() {
    Entity entity = m_Impl->entities.Create();
    ArchetypeLayout* archetype = m_Impl->archetypes.FindOrCreate(m_Impl->defaultEntityMask);
    if (!archetype) {
        return {};
    }
    ChunkView view = m_Impl->archetypes.AllocateSlot(*archetype, entity);
    Chunk* chunk = view.RawChunk();
    if (!chunk) {
        return {};
    }

    EntityLocation& loc = m_Impl->entities.Location(entity);
    loc.archetypeIndex = archetype->index;
    loc.chunkIndex = chunk->header.chunkIndex;
    loc.slot = static_cast<std::uint16_t>(chunk->header.count - 1u);

    if (UuidComponent* uuids = view.Column<UuidComponent>()) {
        uuids[loc.slot] = UuidComponent{Uuid::Generate()};
    }

    BumpChangeVersion();
    return entity;
}

Entity World::CreateEntity(const std::string& name) {
    Entity entity = CreateEntity();
    if (NameComponent* n = TryGet<NameComponent>(entity)) {
        const std::size_t maxChars = sizeof(n->value) - 1u;
        std::size_t i = 0;
        for (; i < maxChars && i < name.size(); ++i) {
            n->value[i] = name[i];
        }
        n->value[i] = '\0';
    }
    return entity;
}

void World::DestroyEntity(Entity entity) {
    if (!Valid(entity)) {
        return;
    }
    EntityLocation& loc = m_Impl->entities.Location(entity);
    if (ArchetypeLayout* archetype = m_Impl->archetypes.Get(loc.archetypeIndex)) {
        if (loc.chunkIndex < archetype->chunks.size()) {
            Chunk* chunk = archetype->chunks[loc.chunkIndex];
            const SlotRelocation relocation = m_Impl->archetypes.DeallocateSlot(*archetype, chunk, loc.slot);
            ApplySlotRelocation(relocation);
        }
    }
    m_Impl->entities.Destroy(entity);
    BumpChangeVersion();
}

bool World::Valid(Entity entity) const {
    return m_Impl->entities.Valid(entity);
}

bool World::IsEntityEnabled(Entity entity) const {
    return m_Impl->entities.IsEnabled(entity);
}

void World::SetEntityEnabled(Entity entity, bool enabled) {
    m_Impl->entities.SetEnabled(entity, enabled);
    BumpChangeVersion();
}

std::uint64_t World::StructuralVersion() const {
    return m_Impl->archetypes.StructuralVersion();
}

void World::FlushCommands() {
    m_Impl->commands.FlushAll(*this);
}

ArchetypeLayout* World::GetEntityArchetype(Entity entity) const {
    if (!Valid(entity)) {
        return nullptr;
    }
    const EntityLocation& loc = m_Impl->entities.Location(entity);
    return m_Impl->archetypes.Get(loc.archetypeIndex);
}

ChunkView World::GetChunkView(Entity entity) {
    if (!Valid(entity)) {
        return {};
    }
    const EntityLocation& loc = m_Impl->entities.Location(entity);
    ArchetypeLayout* archetype = m_Impl->archetypes.Get(loc.archetypeIndex);
    if (!archetype || loc.chunkIndex >= archetype->chunks.size()) {
        return {};
    }
    return ChunkView(archetype, archetype->chunks[loc.chunkIndex]);
}

ChunkView World::GetChunkView(Entity entity) const {
    if (!Valid(entity)) {
        return {};
    }
    const EntityLocation& loc = m_Impl->entities.Location(entity);
    ArchetypeLayout* archetype = m_Impl->archetypes.Get(loc.archetypeIndex);
    if (!archetype || loc.chunkIndex >= archetype->chunks.size()) {
        return {};
    }
    return ChunkView(archetype, archetype->chunks[loc.chunkIndex]);
}

void World::ApplySlotRelocation(const SlotRelocation& relocation) {
    if (!relocation.valid || !Valid(relocation.entity)) {
        return;
    }
    EntityLocation& loc = m_Impl->entities.Location(relocation.entity);
    loc.chunkIndex = relocation.chunkIndex;
    loc.slot = relocation.slot;
}

void World::MigrateAddComponent(Entity entity, ComponentTypeId typeId, const void* initialData) {
    if (!Valid(entity)) {
        return;
    }

    ArchetypeLayout* srcArchetype = GetEntityArchetype(entity);
    if (!srcArchetype) {
        return;
    }
    if (srcArchetype->mask.Test(typeId)) {
        if (initialData) {
            void* dst = GetChunkView(entity).ColumnPtr(typeId);
            if (dst) {
                const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(typeId);
                const EntityLocation& loc = m_Impl->entities.Location(entity);
                void* slotPtr = static_cast<std::uint8_t*>(dst) + info->size * loc.slot;
                if (const ComponentOps* ops = FindComponentOps(typeId); ops && ops->copy) {
                    ops->copy(slotPtr, initialData, info->size);
                } else {
                    std::memcpy(slotPtr, initialData, info->size);
                }
            }
        }
        return;
    }

    ComponentMask newMask = srcArchetype->mask;
    newMask.Set(typeId);
    const std::uint32_t srcArchetypeIndex = srcArchetype->index;
    const std::uint32_t sharedHash = srcArchetype->sharedComponentHash;
    ArchetypeLayout* dstArchetype = m_Impl->archetypes.FindOrCreate(newMask, sharedHash);
    srcArchetype = m_Impl->archetypes.Get(srcArchetypeIndex);
    if (!srcArchetype || !dstArchetype) {
        return;
    }

    EntityLocation& loc = m_Impl->entities.Location(entity);
    Chunk* srcChunk = srcArchetype->chunks[loc.chunkIndex];
    const std::uint16_t srcSlot = loc.slot;

    ChunkView dstView = m_Impl->archetypes.AllocateSlot(*dstArchetype, entity);
    Chunk* dstChunk = dstView.RawChunk();
    const std::uint16_t dstSlot = static_cast<std::uint16_t>(dstChunk->header.count - 1u);

    for (ComponentTypeId existingId : srcArchetype->typeIds) {
        if (!dstArchetype->mask.Test(existingId)) {
            continue;
        }
        const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(existingId);
        if (!info) {
            continue;
        }
        void* srcColumn = srcChunk->data + srcArchetype->columnOffsets[
            static_cast<std::size_t>(std::distance(
                srcArchetype->typeIds.begin(),
                std::find(srcArchetype->typeIds.begin(), srcArchetype->typeIds.end(), existingId)))];
        void* dstColumn = dstChunk->data + dstArchetype->columnOffsets[
            static_cast<std::size_t>(std::distance(
                dstArchetype->typeIds.begin(),
                std::find(dstArchetype->typeIds.begin(), dstArchetype->typeIds.end(), existingId)))];
        void* srcPtr = static_cast<std::uint8_t*>(srcColumn) + info->size * srcSlot;
        void* dstPtr = static_cast<std::uint8_t*>(dstColumn) + info->size * dstSlot;
        if (const ComponentOps* ops = FindComponentOps(existingId); ops && ops->copy) {
            ops->copy(dstPtr, srcPtr, info->size);
        } else {
            std::memcpy(dstPtr, srcPtr, info->size);
        }
    }

    if (initialData && dstArchetype->mask.Test(typeId)) {
        void* dstColumn = dstView.ColumnPtr(typeId);
        const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(typeId);
        void* dstPtr = static_cast<std::uint8_t*>(dstColumn) + info->size * dstSlot;
        if (const ComponentOps* ops = FindComponentOps(typeId); ops && ops->copy) {
            ops->copy(dstPtr, initialData, info->size);
        } else {
            std::memcpy(dstPtr, initialData, info->size);
        }
    }

    const SlotRelocation relocation = m_Impl->archetypes.DeallocateSlot(*srcArchetype, srcChunk, srcSlot);
    ApplySlotRelocation(relocation);

    loc.archetypeIndex = dstArchetype->index;
    loc.chunkIndex = dstChunk->header.chunkIndex;
    loc.slot = dstSlot;

    std::uint64_t* entityColumn = reinterpret_cast<std::uint64_t*>(
        dstChunk->data + dstArchetype->entityIdColumnOffset);
    entityColumn[dstSlot] = entity.id;

    m_Impl->archetypes.BumpStructuralVersion();
}

void World::MigrateRemoveComponent(Entity entity, ComponentTypeId typeId) {
    if (!Valid(entity)) {
        return;
    }
    ArchetypeLayout* srcArchetype = GetEntityArchetype(entity);
    if (!srcArchetype || !srcArchetype->mask.Test(typeId)) {
        return;
    }

    ComponentMask newMask = srcArchetype->mask;
    newMask.Set(typeId, false);
    const std::uint32_t srcArchetypeIndex = srcArchetype->index;
    const std::uint32_t sharedHash = srcArchetype->sharedComponentHash;
    ArchetypeLayout* dstArchetype = m_Impl->archetypes.FindOrCreate(newMask, sharedHash);
    srcArchetype = m_Impl->archetypes.Get(srcArchetypeIndex);
    if (!srcArchetype || !dstArchetype) {
        return;
    }

    EntityLocation& loc = m_Impl->entities.Location(entity);
    Chunk* srcChunk = srcArchetype->chunks[loc.chunkIndex];
    const std::uint16_t srcSlot = loc.slot;

    if (newMask.Any()) {
        ChunkView dstView = m_Impl->archetypes.AllocateSlot(*dstArchetype, entity);
        Chunk* dstChunk = dstView.RawChunk();
        const std::uint16_t dstSlot = static_cast<std::uint16_t>(dstChunk->header.count - 1u);

        for (ComponentTypeId existingId : dstArchetype->typeIds) {
            const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(existingId);
            if (!info) {
                continue;
            }
            const auto srcIt = std::find(srcArchetype->typeIds.begin(), srcArchetype->typeIds.end(), existingId);
            if (srcIt == srcArchetype->typeIds.end()) {
                continue;
            }
            const std::size_t srcIndex = static_cast<std::size_t>(srcIt - srcArchetype->typeIds.begin());
            const std::size_t dstIndex = static_cast<std::size_t>(
                std::find(dstArchetype->typeIds.begin(), dstArchetype->typeIds.end(), existingId)
                - dstArchetype->typeIds.begin());

            void* srcColumn = srcChunk->data + srcArchetype->columnOffsets[srcIndex];
            void* dstColumn = dstChunk->data + dstArchetype->columnOffsets[dstIndex];
            void* srcPtr = static_cast<std::uint8_t*>(srcColumn) + info->size * srcSlot;
            void* dstPtr = static_cast<std::uint8_t*>(dstColumn) + info->size * dstSlot;
            if (const ComponentOps* ops = FindComponentOps(existingId); ops && ops->copy) {
                ops->copy(dstPtr, srcPtr, info->size);
            } else {
                std::memcpy(dstPtr, srcPtr, info->size);
            }
        }

        const SlotRelocation relocation = m_Impl->archetypes.DeallocateSlot(*srcArchetype, srcChunk, srcSlot);
        ApplySlotRelocation(relocation);
        loc.archetypeIndex = dstArchetype->index;
        loc.chunkIndex = dstChunk->header.chunkIndex;
        loc.slot = dstSlot;
    } else {
        const SlotRelocation relocation = m_Impl->archetypes.DeallocateSlot(*srcArchetype, srcChunk, srcSlot);
        ApplySlotRelocation(relocation);
        loc = {};
    }

    m_Impl->archetypes.BumpStructuralVersion();
}

} // namespace we::runtime::ecs
