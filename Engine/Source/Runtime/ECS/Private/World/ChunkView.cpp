#include "ECS/Chunk.h"

#include <cstring>

namespace we::runtime::ecs {

ChunkView::ChunkView(ArchetypeLayout* archetype, Chunk* chunk)
    : m_Archetype(archetype)
    , m_Chunk(chunk)
{
}

std::uint16_t ChunkView::Count() const {
    return m_Chunk ? m_Chunk->header.count : 0;
}

std::uint16_t ChunkView::Capacity() const {
    return m_Chunk ? m_Chunk->header.capacity : 0;
}

Entity ChunkView::GetEntity(std::uint16_t slot) const {
    if (!m_Chunk || !m_Archetype || slot >= m_Chunk->header.count) {
        return kNullEntity;
    }
    const std::uint64_t* entityColumn = reinterpret_cast<const std::uint64_t*>(
        m_Chunk->data + m_Archetype->entityIdColumnOffset);
    return Entity(entityColumn[slot]);
}

bool ChunkView::IsEntityEnabled(std::uint16_t slot) const {
    if (!m_Chunk || !m_Archetype || slot >= m_Chunk->header.count) {
        return false;
    }
    const std::uint8_t* enabledColumn = m_Chunk->data + m_Archetype->entityEnabledColumnOffset;
    return enabledColumn[slot] != 0;
}

std::int32_t ChunkView::FindColumnIndex(ComponentTypeId typeId) const {
    if (!m_Archetype) {
        return -1;
    }
    for (std::size_t i = 0; i < m_Archetype->typeIds.size(); ++i) {
        if (m_Archetype->typeIds[i] == typeId) {
            return static_cast<std::int32_t>(i);
        }
    }
    return -1;
}

void* ChunkView::ColumnPtr(ComponentTypeId typeId) {
    if (!m_Chunk || !m_Archetype) {
        return nullptr;
    }
    const std::int32_t index = FindColumnIndex(typeId);
    if (index < 0) {
        return nullptr;
    }
    return m_Chunk->data + m_Archetype->columnOffsets[static_cast<std::size_t>(index)];
}

const void* ChunkView::ColumnPtr(ComponentTypeId typeId) const {
    return const_cast<ChunkView*>(this)->ColumnPtr(typeId);
}

bool ChunkView::IsComponentEnabled(ComponentTypeId typeId, std::uint16_t slot) const {
    if (!m_Chunk || !m_Archetype || slot >= m_Chunk->header.count) {
        return false;
    }
    const std::int32_t index = FindColumnIndex(typeId);
    if (index < 0) {
        return false;
    }
    const std::size_t idx = static_cast<std::size_t>(index);
    if (!m_Archetype->columnEnableable[idx]) {
        return true;
    }
    if (idx >= m_Archetype->columnEnableOffsets.size() || m_Archetype->columnEnableOffsets[idx] == 0) {
        return true;
    }
    const std::uint8_t* compEnabled = m_Chunk->data + m_Archetype->columnEnableOffsets[idx];
    return compEnabled[slot] != 0;
}

} // namespace we::runtime::ecs
