#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/ComponentMask.h"
#include "ECS/ComponentType.h"
#include "ECS/Entity.h"
#include "ECS/Export.h"
#include "ECS/Types.h"

#include <cstdint>
#include <vector>

namespace we::runtime::ecs {

struct ArchetypeLayout;
class ArchetypeManager;

struct ChunkHeader {
    ArchetypeLayout* archetype = nullptr;
    std::uint32_t chunkIndex = kInvalidChunkIndex;
    std::uint16_t count = 0;
    std::uint16_t capacity = 0;
    std::uint32_t version = 0;
    std::uint32_t flags = 0;
};

// Fixed-size SoA memory block owned by the chunk allocator.
struct Chunk {
    ChunkHeader header{};
    std::uint8_t* data = nullptr;
    std::size_t dataSize = 0;
    Chunk* next = nullptr;
    Chunk* prev = nullptr;
};

struct ArchetypeLayout {
    std::uint32_t index = kInvalidArchetypeIndex;
    ComponentMask mask{};
    std::vector<ComponentTypeId> typeIds;
    std::vector<std::uint32_t> columnOffsets;
    std::vector<std::uint32_t> columnSizes;
    std::vector<std::uint32_t> columnAlignments;
    std::vector<std::uint32_t> columnEnableOffsets; // 0 if not enableable
    std::vector<bool> columnEnableable;
    std::uint16_t entitiesPerChunk = 0;
    std::uint32_t chunkDataSize = 0;
    std::uint32_t entityIdColumnOffset = 0;
    std::uint32_t entityEnabledColumnOffset = 0;
    std::uint32_t sharedComponentHash = 0;
    std::vector<Chunk*> chunks;
    std::uint64_t entityCount = 0;
};

class ECS_API ChunkView {
public:
    ChunkView() = default;
    ChunkView(ArchetypeLayout* archetype, Chunk* chunk);

    [[nodiscard]] bool Valid() const { return m_Archetype && m_Chunk; }
    [[nodiscard]] ArchetypeLayout* Archetype() const { return m_Archetype; }
    [[nodiscard]] Chunk* RawChunk() const { return m_Chunk; }
    [[nodiscard]] std::uint16_t Count() const;
    [[nodiscard]] std::uint16_t Capacity() const;
    [[nodiscard]] Entity GetEntity(std::uint16_t slot) const;
    [[nodiscard]] bool IsEntityEnabled(std::uint16_t slot) const;
    [[nodiscard]] void* ColumnPtr(ComponentTypeId typeId);
    [[nodiscard]] const void* ColumnPtr(ComponentTypeId typeId) const;
    [[nodiscard]] bool IsComponentEnabled(ComponentTypeId typeId, std::uint16_t slot) const;

    template <typename T>
    T* Column() {
        return static_cast<T*>(ColumnPtr(ComponentTypeRegistry::Get().Id<T>()));
    }

    template <typename T>
    const T* Column() const {
        return static_cast<const T*>(ColumnPtr(ComponentTypeRegistry::Get().Id<T>()));
    }

    template <typename T>
    T& Get(std::uint16_t slot) {
        return Column<T>()[slot];
    }

    template <typename T>
    const T& Get(std::uint16_t slot) const {
        return Column<T>()[slot];
    }

private:
    [[nodiscard]] std::int32_t FindColumnIndex(ComponentTypeId typeId) const;

    ArchetypeLayout* m_Archetype = nullptr;
    Chunk* m_Chunk = nullptr;
};

} // namespace we::runtime::ecs

#pragma warning(pop)
