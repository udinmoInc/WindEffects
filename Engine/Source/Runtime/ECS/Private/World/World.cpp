#include "ECS/World.h"

#include "ECS/ComponentOps.h"
#include "ECS/Components/CoreComponents.h"

#include <algorithm>
#include <cstring>

namespace we::runtime::ecs {

World::World()
    : m_Commands(32)
{
    m_Archetypes.Init(&m_ChunkAllocator, &ComponentTypeRegistry::Get());

    m_DefaultEntityMask.Set(ComponentTypeRegistry::Get().Id<UuidComponent>());
    m_DefaultEntityMask.Set(ComponentTypeRegistry::Get().Id<TransformComponent>());
    m_DefaultEntityMask.Set(ComponentTypeRegistry::Get().Id<HierarchyComponent>());
    m_DefaultEntityMask.Set(ComponentTypeRegistry::Get().Id<VisibilityComponent>());
}

World::~World() {
    Clear();
}

Entity World::CreateEntity() {
    Entity entity = m_Entities.Create();
    ArchetypeLayout* archetype = m_Archetypes.FindOrCreate(m_DefaultEntityMask);
    ChunkView view = m_Archetypes.AllocateSlot(*archetype, entity);
    Chunk* chunk = view.RawChunk();

    EntityLocation& loc = m_Entities.Location(entity);
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
    AddComponent<NameComponent>(entity, NameComponent{name});
    return entity;
}

void World::DestroyEntity(Entity entity) {
    if (!Valid(entity)) {
        return;
    }
    EntityLocation& loc = m_Entities.Location(entity);
    if (ArchetypeLayout* archetype = m_Archetypes.Get(loc.archetypeIndex)) {
        if (loc.chunkIndex < archetype->chunks.size()) {
            Chunk* chunk = archetype->chunks[loc.chunkIndex];
            const SlotRelocation relocation = m_Archetypes.DeallocateSlot(*archetype, chunk, loc.slot);
            ApplySlotRelocation(relocation);
        }
    }
    m_Entities.Destroy(entity);
    BumpChangeVersion();
}

bool World::Valid(Entity entity) const {
    return m_Entities.Valid(entity);
}

void World::Clear() {
    for (ArchetypeLayout* archetype : m_Archetypes.All()) {
        if (!archetype) {
            continue;
        }
        for (Chunk* chunk : archetype->chunks) {
            m_ChunkAllocator.DestroyChunk(chunk);
        }
        archetype->chunks.clear();
        archetype->entityCount = 0;
    }
    m_Entities.Clear();
    m_Singletons.clear();
    m_Commands.ClearAll();
    BumpChangeVersion();
}

bool World::IsEntityEnabled(Entity entity) const {
    return m_Entities.IsEnabled(entity);
}

void World::SetEntityEnabled(Entity entity, bool enabled) {
    m_Entities.SetEnabled(entity, enabled);
    BumpChangeVersion();
}

std::uint64_t World::StructuralVersion() const {
    return m_Archetypes.StructuralVersion();
}

void World::FlushCommands() {
    m_Commands.FlushAll(*this);
}

ArchetypeLayout* World::GetEntityArchetype(Entity entity) const {
    if (!Valid(entity)) {
        return nullptr;
    }
    const EntityLocation& loc = m_Entities.Location(entity);
    return m_Archetypes.Get(loc.archetypeIndex);
}

ChunkView World::GetChunkView(Entity entity) {
    if (!Valid(entity)) {
        return {};
    }
    const EntityLocation& loc = m_Entities.Location(entity);
    ArchetypeLayout* archetype = m_Archetypes.Get(loc.archetypeIndex);
    if (!archetype || loc.chunkIndex >= archetype->chunks.size()) {
        return {};
    }
    return ChunkView(archetype, archetype->chunks[loc.chunkIndex]);
}

ChunkView World::GetChunkView(Entity entity) const {
    if (!Valid(entity)) {
        return {};
    }
    const EntityLocation& loc = m_Entities.Location(entity);
    ArchetypeLayout* archetype = m_Archetypes.Get(loc.archetypeIndex);
    if (!archetype || loc.chunkIndex >= archetype->chunks.size()) {
        return {};
    }
    return ChunkView(archetype, archetype->chunks[loc.chunkIndex]);
}

void World::ApplySlotRelocation(const SlotRelocation& relocation) {
    if (!relocation.valid || !Valid(relocation.entity)) {
        return;
    }
    EntityLocation& loc = m_Entities.Location(relocation.entity);
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
                const EntityLocation& loc = m_Entities.Location(entity);
                void* slotPtr = static_cast<std::uint8_t*>(dst) + info->size * loc.slot;
                if (const ComponentOps* ops = FindComponentOps(typeId)) {
                    if (ops->copy) {
                        ops->copy(slotPtr, initialData, info->size);
                    }
                } else {
                    std::memcpy(slotPtr, initialData, info->size);
                }
            }
        }
        return;
    }

    ComponentMask newMask = srcArchetype->mask;
    newMask.Set(typeId);
    ArchetypeLayout* dstArchetype = m_Archetypes.FindOrCreate(newMask, srcArchetype->sharedComponentHash);

    EntityLocation& loc = m_Entities.Location(entity);
    Chunk* srcChunk = srcArchetype->chunks[loc.chunkIndex];
    const std::uint16_t srcSlot = loc.slot;

    ChunkView dstView = m_Archetypes.AllocateSlot(*dstArchetype, entity);
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
        if (const ComponentOps* ops = FindComponentOps(existingId)) {
            if (ops->copy) {
                ops->copy(dstPtr, srcPtr, info->size);
            }
        } else {
            std::memcpy(dstPtr, srcPtr, info->size);
        }
    }

    if (initialData && dstArchetype->mask.Test(typeId)) {
        void* dstColumn = dstView.ColumnPtr(typeId);
        const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(typeId);
        void* dstPtr = static_cast<std::uint8_t*>(dstColumn) + info->size * dstSlot;
        if (const ComponentOps* ops = FindComponentOps(typeId)) {
            if (ops->copy) {
                ops->copy(dstPtr, initialData, info->size);
            }
        } else {
            std::memcpy(dstPtr, initialData, info->size);
        }
    }

    const SlotRelocation relocation = m_Archetypes.DeallocateSlot(*srcArchetype, srcChunk, srcSlot);
    ApplySlotRelocation(relocation);

    loc.archetypeIndex = dstArchetype->index;
    loc.chunkIndex = dstChunk->header.chunkIndex;
    loc.slot = dstSlot;

    std::uint64_t* entityColumn = reinterpret_cast<std::uint64_t*>(
        dstChunk->data + dstArchetype->entityIdColumnOffset);
    entityColumn[dstSlot] = entity.id;

    m_Archetypes.BumpStructuralVersion();
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
    ArchetypeLayout* dstArchetype = m_Archetypes.FindOrCreate(newMask, srcArchetype->sharedComponentHash);

    EntityLocation& loc = m_Entities.Location(entity);
    Chunk* srcChunk = srcArchetype->chunks[loc.chunkIndex];
    const std::uint16_t srcSlot = loc.slot;

    if (newMask.Any()) {
        ChunkView dstView = m_Archetypes.AllocateSlot(*dstArchetype, entity);
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
            if (const ComponentOps* ops = FindComponentOps(existingId)) {
                if (ops->copy) {
                    ops->copy(dstPtr, srcPtr, info->size);
                }
            } else {
                std::memcpy(dstPtr, srcPtr, info->size);
            }
        }

        const SlotRelocation relocation = m_Archetypes.DeallocateSlot(*srcArchetype, srcChunk, srcSlot);
        ApplySlotRelocation(relocation);
        loc.archetypeIndex = dstArchetype->index;
        loc.chunkIndex = dstChunk->header.chunkIndex;
        loc.slot = dstSlot;
    } else {
        const SlotRelocation relocation = m_Archetypes.DeallocateSlot(*srcArchetype, srcChunk, srcSlot);
        ApplySlotRelocation(relocation);
        loc = {};
    }

    m_Archetypes.BumpStructuralVersion();
}

} // namespace we::runtime::ecs
