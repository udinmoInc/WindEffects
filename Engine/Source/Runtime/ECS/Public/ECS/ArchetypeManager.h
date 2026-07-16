#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Chunk.h"
#include "ECS/ComponentMask.h"
#include "ECS/Export.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace we::runtime::ecs {

class ECS_API ChunkAllocator {
public:
    explicit ChunkAllocator(std::size_t chunkSizeBytes = kDefaultChunkSizeBytes);
    ~ChunkAllocator();

    ChunkAllocator(const ChunkAllocator&) = delete;
    ChunkAllocator& operator=(const ChunkAllocator&) = delete;

    Chunk* Allocate(ArchetypeLayout& layout);
    void Free(Chunk* chunk, ArchetypeLayout& layout);
    void DestroyChunk(Chunk* chunk);

    [[nodiscard]] std::size_t ChunkSizeBytes() const { return m_ChunkSizeBytes; }
    [[nodiscard]] std::size_t AllocatedChunkCount() const { return m_AllocatedChunks; }
    [[nodiscard]] std::size_t BytesInUse() const { return m_BytesInUse; }

private:
    std::size_t m_ChunkSizeBytes = kDefaultChunkSizeBytes;
    std::size_t m_AllocatedChunks = 0;
    std::size_t m_BytesInUse = 0;
    std::vector<Chunk*> m_FreeList;
};

struct SlotRelocation {
    Entity entity{};
    std::uint32_t chunkIndex = kInvalidChunkIndex;
    std::uint16_t slot = kInvalidChunkSlot;
    bool valid = false;
};

class ECS_API ArchetypeManager {
public:
    ArchetypeManager() = default;

    void Init(ChunkAllocator* allocator, ComponentTypeRegistry* registry);
    [[nodiscard]] ArchetypeLayout* FindOrCreate(const ComponentMask& mask, std::uint32_t sharedHash = 0);
    [[nodiscard]] ArchetypeLayout* Find(const ComponentMask& mask, std::uint32_t sharedHash = 0) const;
    [[nodiscard]] ArchetypeLayout* Get(std::uint32_t index) const;
    [[nodiscard]] const std::vector<ArchetypeLayout*>& All() const { return m_Archetypes; }
    [[nodiscard]] std::uint64_t StructuralVersion() const { return m_StructuralVersion; }
    void BumpStructuralVersion() { ++m_StructuralVersion; }

    // Allocate a slot in the archetype; may allocate a new chunk.
    [[nodiscard]] ChunkView AllocateSlot(ArchetypeLayout& archetype, Entity entity);
    [[nodiscard]] SlotRelocation DeallocateSlot(ArchetypeLayout& archetype, Chunk* chunk, std::uint16_t slot);

    void BuildLayout(ArchetypeLayout& layout);

private:
    [[nodiscard]] std::uint64_t MaskHash(const ComponentMask& mask, std::uint32_t sharedHash) const;

    ChunkAllocator* m_Allocator = nullptr;
    ComponentTypeRegistry* m_Registry = nullptr;
    std::vector<ArchetypeLayout> m_Storage;
    std::vector<ArchetypeLayout*> m_Archetypes;
    std::unordered_map<std::uint64_t, std::uint32_t> m_Lookup;
    std::uint64_t m_StructuralVersion = 1;
};

} // namespace we::runtime::ecs

#pragma warning(pop)
