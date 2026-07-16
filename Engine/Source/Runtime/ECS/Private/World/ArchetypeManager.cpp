#include "ECS/ArchetypeManager.h"
#include "ECS/ComponentOps.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace we::runtime::ecs {
namespace {

constexpr std::uint32_t kChunkHeaderSize = 64u;

std::uint32_t AlignUp(std::uint32_t value, std::uint32_t alignment) {
    if (alignment <= 1) {
        return value;
    }
    const std::uint32_t mask = alignment - 1u;
    return (value + mask) & ~mask;
}

} // namespace

ChunkAllocator::ChunkAllocator(std::size_t chunkSizeBytes)
    : m_ChunkSizeBytes(std::clamp(chunkSizeBytes, kMinChunkSizeBytes, kMaxChunkSizeBytes))
{
}

ChunkAllocator::~ChunkAllocator() {
    for (Chunk* chunk : m_FreeList) {
        if (!chunk) {
            continue;
        }
        delete[] chunk->data;
        delete chunk;
    }
    m_FreeList.clear();
}

Chunk* ChunkAllocator::Allocate(ArchetypeLayout& layout) {
    Chunk* chunk = nullptr;
    if (!m_FreeList.empty()) {
        chunk = m_FreeList.back();
        m_FreeList.pop_back();
        if (chunk->dataSize < layout.chunkDataSize) {
            delete[] chunk->data;
            chunk->data = new std::uint8_t[layout.chunkDataSize];
            chunk->dataSize = layout.chunkDataSize;
        }
    } else {
        chunk = new Chunk{};
        chunk->data = new std::uint8_t[layout.chunkDataSize];
        chunk->dataSize = layout.chunkDataSize;
        ++m_AllocatedChunks;
    }
    std::memset(chunk->data, 0, layout.chunkDataSize);
    chunk->header.archetype = &layout;
    chunk->header.chunkIndex = static_cast<std::uint32_t>(layout.chunks.size());
    chunk->header.count = 0;
    chunk->header.capacity = layout.entitiesPerChunk;
    chunk->header.version = 0;
    layout.chunks.push_back(chunk);
    m_BytesInUse += layout.chunkDataSize;
    return chunk;
}

void ChunkAllocator::Free(Chunk* chunk, ArchetypeLayout& /*layout*/) {
    if (!chunk) {
        return;
    }
    m_FreeList.push_back(chunk);
}

void ChunkAllocator::DestroyChunk(Chunk* chunk) {
    if (!chunk) {
        return;
    }
    delete[] chunk->data;
    delete chunk;
    if (m_AllocatedChunks > 0) {
        --m_AllocatedChunks;
    }
}

void ArchetypeManager::Init(ChunkAllocator* allocator, ComponentTypeRegistry* registry) {
    m_Allocator = allocator;
    m_Registry = registry;
}

std::uint64_t ArchetypeManager::MaskHash(const ComponentMask& mask, std::uint32_t sharedHash) const {
    std::uint64_t hash = sharedHash;
    for (std::uint64_t w : mask.words) {
        hash ^= w + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
    }
    return hash;
}

void ArchetypeManager::BuildLayout(ArchetypeLayout& layout) {
    if (!m_Registry || !m_Allocator) {
        return;
    }

    layout.typeIds.clear();
    layout.columnOffsets.clear();
    layout.columnSizes.clear();
    layout.columnAlignments.clear();
    layout.columnEnableOffsets.clear();
    layout.columnEnableable.clear();

    for (std::uint32_t typeId = 0; typeId < kMaxComponentTypes; ++typeId) {
        if (!layout.mask.Test(typeId)) {
            continue;
        }
        const ComponentTypeInfo* info = m_Registry->Find(typeId);
        if (!info || info->category == ComponentCategory::Singleton || info->category == ComponentCategory::Tag) {
            continue;
        }
        layout.typeIds.push_back(typeId);
    }
    std::sort(layout.typeIds.begin(), layout.typeIds.end());

    std::uint32_t offset = kChunkHeaderSize;
    offset = AlignUp(offset, alignof(std::uint64_t));
    layout.entityIdColumnOffset = offset;
    offset += sizeof(std::uint64_t); // placeholder per entity, scaled later

    offset = AlignUp(offset, alignof(std::uint8_t));
    layout.entityEnabledColumnOffset = offset;
    offset += sizeof(std::uint8_t);

    for (ComponentTypeId typeId : layout.typeIds) {
        const ComponentTypeInfo* info = m_Registry->Find(typeId);
        if (!info) {
            continue;
        }
        offset = AlignUp(offset, static_cast<std::uint32_t>(info->alignment));
        layout.columnOffsets.push_back(offset);
        layout.columnSizes.push_back(static_cast<std::uint32_t>(info->size));
        layout.columnAlignments.push_back(static_cast<std::uint32_t>(info->alignment));
        layout.columnEnableable.push_back(info->enableable);
        offset += static_cast<std::uint32_t>(info->size);
        if (info->enableable) {
            offset = AlignUp(offset, alignof(std::uint8_t));
            offset += sizeof(std::uint8_t);
        }
    }

    const std::uint32_t perEntityMeta = static_cast<std::uint32_t>(sizeof(std::uint64_t) + sizeof(std::uint8_t));
    std::uint32_t perEntityComponents = 0;
    for (std::size_t i = 0; i < layout.columnSizes.size(); ++i) {
        perEntityComponents += layout.columnSizes[i];
        if (layout.columnEnableable[i]) {
            perEntityComponents = AlignUp(perEntityComponents, alignof(std::uint8_t));
            perEntityComponents += sizeof(std::uint8_t);
        }
    }

    const std::uint32_t perEntityBytes = perEntityMeta + perEntityComponents;
    const std::uint32_t available = static_cast<std::uint32_t>(m_Allocator->ChunkSizeBytes());
    std::uint16_t capacity = static_cast<std::uint16_t>(available / std::max(1u, perEntityBytes));
    if (capacity == 0) {
        capacity = 1;
    }
    layout.entitiesPerChunk = capacity;

    // Shrink capacity until packed SoA layout fits the chunk budget.
    // Chunk::data is a separate allocation from ChunkHeader — start at offset 0.
    for (;;) {
        layout.entityIdColumnOffset = 0;
        layout.entityEnabledColumnOffset = layout.entityIdColumnOffset
            + static_cast<std::uint32_t>(sizeof(std::uint64_t)) * layout.entitiesPerChunk;

        offset = layout.entityEnabledColumnOffset
            + static_cast<std::uint32_t>(sizeof(std::uint8_t)) * layout.entitiesPerChunk;
        layout.columnOffsets.clear();
        layout.columnSizes.clear();
        layout.columnAlignments.clear();
        layout.columnEnableOffsets.clear();
        layout.columnEnableable.clear();

        for (ComponentTypeId typeId : layout.typeIds) {
            const ComponentTypeInfo* info = m_Registry->Find(typeId);
            if (!info) {
                continue;
            }
            offset = AlignUp(offset, static_cast<std::uint32_t>(info->alignment));
            layout.columnOffsets.push_back(offset);
            layout.columnSizes.push_back(static_cast<std::uint32_t>(info->size));
            layout.columnAlignments.push_back(static_cast<std::uint32_t>(info->alignment));
            layout.columnEnableable.push_back(info->enableable);
            offset += static_cast<std::uint32_t>(info->size) * layout.entitiesPerChunk;
            if (info->enableable) {
                offset = AlignUp(offset, alignof(std::uint8_t));
                layout.columnEnableOffsets.push_back(offset);
                offset += static_cast<std::uint32_t>(sizeof(std::uint8_t)) * layout.entitiesPerChunk;
            } else {
                layout.columnEnableOffsets.push_back(0);
            }
        }

        layout.chunkDataSize = AlignUp(offset, alignof(std::uint64_t));
        if (layout.chunkDataSize <= m_Allocator->ChunkSizeBytes() || layout.entitiesPerChunk <= 1) {
            break;
        }
        layout.entitiesPerChunk = static_cast<std::uint16_t>(layout.entitiesPerChunk / 2u);
        if (layout.entitiesPerChunk == 0) {
            layout.entitiesPerChunk = 1;
        }
    }

    // Validate every column fits for the last slot.
    for (std::size_t i = 0; i < layout.typeIds.size(); ++i) {
        const ComponentTypeInfo* info = m_Registry->Find(layout.typeIds[i]);
        if (!info) {
            continue;
        }
        const std::uint32_t end =
            layout.columnOffsets[i] + static_cast<std::uint32_t>(info->size) * layout.entitiesPerChunk;
        if (end > layout.chunkDataSize) {
            layout.entitiesPerChunk = 1;
            layout.chunkDataSize = end;
        }
        if (layout.columnEnableable[i] && layout.columnEnableOffsets[i] != 0) {
            const std::uint32_t enableEnd =
                layout.columnEnableOffsets[i] + static_cast<std::uint32_t>(sizeof(std::uint8_t)) * layout.entitiesPerChunk;
            if (enableEnd > layout.chunkDataSize) {
                layout.chunkDataSize = enableEnd;
            }
        }
    }
}

ArchetypeLayout* ArchetypeManager::FindOrCreate(const ComponentMask& mask, std::uint32_t sharedHash) {
    if (ArchetypeLayout* existing = Find(mask, sharedHash)) {
        return existing;
    }

    auto owned = std::make_unique<ArchetypeLayout>();
    owned->index = static_cast<std::uint32_t>(m_Storage.size());
    owned->mask = mask;
    owned->sharedComponentHash = sharedHash;
    BuildLayout(*owned);

    m_Storage.push_back(std::move(owned));
    ArchetypeLayout* created = m_Storage.back().get();
    created->index = static_cast<std::uint32_t>(m_Storage.size() - 1);
    m_Archetypes.push_back(created);
    m_Lookup[MaskHash(mask, sharedHash)] = created->index;
    BumpStructuralVersion();
    return created;
}

ArchetypeLayout* ArchetypeManager::Find(const ComponentMask& mask, std::uint32_t sharedHash) const {
    const auto it = m_Lookup.find(MaskHash(mask, sharedHash));
    if (it == m_Lookup.end()) {
        return nullptr;
    }
    return Get(it->second);
}

ArchetypeLayout* ArchetypeManager::Get(std::uint32_t index) const {
    if (index >= m_Storage.size()) {
        return nullptr;
    }
    return m_Storage[index].get();
}

ChunkView ArchetypeManager::AllocateSlot(ArchetypeLayout& archetype, Entity entity) {
    Chunk* chunk = nullptr;
    for (Chunk* candidate : archetype.chunks) {
        if (candidate && candidate->header.count < candidate->header.capacity) {
            chunk = candidate;
            break;
        }
    }
    if (!chunk) {
        chunk = m_Allocator->Allocate(archetype);
    }

    const std::uint16_t slot = chunk->header.count++;
    std::uint64_t* entityColumn = reinterpret_cast<std::uint64_t*>(
        chunk->data + archetype.entityIdColumnOffset);
    entityColumn[slot] = entity.id;

    std::uint8_t* enabledColumn = chunk->data + archetype.entityEnabledColumnOffset;
    enabledColumn[slot] = 1;

    for (std::size_t i = 0; i < archetype.typeIds.size(); ++i) {
        const ComponentTypeId typeId = archetype.typeIds[i];
        void* column = chunk->data + archetype.columnOffsets[i];
        const ComponentTypeInfo* info = m_Registry->Find(typeId);
        if (!info) {
            continue;
        }
        void* dst = static_cast<std::uint8_t*>(column) + info->size * slot;
        if (const ComponentOps* ops = FindComponentOps(typeId); ops && ops->construct) {
            ops->construct(dst, info->size);
        } else {
            std::memset(dst, 0, info->size);
        }
        if (archetype.columnEnableable[i] && archetype.columnEnableOffsets[i] != 0) {
            std::uint8_t* compEnabled = chunk->data + archetype.columnEnableOffsets[i];
            compEnabled[slot] = 1;
        }
    }

    ++archetype.entityCount;
    BumpStructuralVersion();
    return ChunkView(&archetype, chunk);
}

SlotRelocation ArchetypeManager::DeallocateSlot(ArchetypeLayout& archetype, Chunk* chunk, std::uint16_t slot) {
    SlotRelocation relocation{};
    if (!chunk || slot >= chunk->header.count) {
        return relocation;
    }

    const std::uint16_t last = static_cast<std::uint16_t>(chunk->header.count - 1u);
    if (slot != last) {
        std::uint64_t* entityColumn = reinterpret_cast<std::uint64_t*>(
            chunk->data + archetype.entityIdColumnOffset);

        std::uint8_t* enabledColumn = chunk->data + archetype.entityEnabledColumnOffset;
        enabledColumn[slot] = enabledColumn[last];

        for (std::size_t i = 0; i < archetype.typeIds.size(); ++i) {
            const ComponentTypeId typeId = archetype.typeIds[i];
            const ComponentTypeInfo* info = m_Registry->Find(typeId);
            if (!info) {
                continue;
            }
            void* column = chunk->data + archetype.columnOffsets[i];
            void* dst = static_cast<std::uint8_t*>(column) + info->size * slot;
            void* src = static_cast<std::uint8_t*>(column) + info->size * last;
            if (const ComponentOps* ops = FindComponentOps(typeId)) {
                if (ops->trivial) {
                    if (ops->move) {
                        ops->move(dst, src, info->size);
                    }
                } else {
                    if (ops->move) {
                        ops->move(dst, src, info->size);
                    }
                    if (ops->destruct) {
                        ops->destruct(src, info->size);
                    }
                }
            } else {
                std::memmove(dst, src, info->size);
            }
            if (archetype.columnEnableable[i] && archetype.columnEnableOffsets[i] != 0) {
                std::uint8_t* compEnabled = chunk->data + archetype.columnEnableOffsets[i];
                compEnabled[slot] = compEnabled[last];
            }
        }

        entityColumn[slot] = entityColumn[last];
        relocation.entity = Entity(entityColumn[slot]);
        relocation.chunkIndex = chunk->header.chunkIndex;
        relocation.slot = slot;
        relocation.valid = true;
    } else {
        for (std::size_t i = 0; i < archetype.typeIds.size(); ++i) {
            const ComponentTypeId typeId = archetype.typeIds[i];
            const ComponentTypeInfo* info = m_Registry->Find(typeId);
            if (!info) {
                continue;
            }
            void* column = chunk->data + archetype.columnOffsets[i];
            void* ptr = static_cast<std::uint8_t*>(column) + info->size * slot;
            if (const ComponentOps* ops = FindComponentOps(typeId)) {
                if (ops->destruct) {
                    ops->destruct(ptr, info->size);
                }
            }
        }
    }

    --chunk->header.count;
    if (archetype.entityCount > 0) {
        --archetype.entityCount;
    }
    BumpStructuralVersion();
    return relocation;
}

} // namespace we::runtime::ecs
