#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Entity.h"
#include "ECS/Export.h"
#include "ECS/Types.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace we::runtime::ecs {

struct EntityLocation {
    std::uint32_t archetypeIndex = kInvalidArchetypeIndex;
    std::uint32_t chunkIndex = kInvalidChunkIndex;
    std::uint16_t slot = kInvalidChunkSlot;
};

struct EntityRecord {
    std::uint32_t generation = 0;
    EntityFlags flags = EntityFlags::Enabled;
    EntityLocation location{};
};

class ECS_API EntityManager {
public:
    EntityManager();

    [[nodiscard]] Entity Create();
    void Destroy(Entity entity);
    [[nodiscard]] bool Valid(Entity entity) const;
    [[nodiscard]] bool IsEnabled(Entity entity) const;
    void SetEnabled(Entity entity, bool enabled);
    void Clear();

    [[nodiscard]] EntityLocation& Location(Entity entity);
    [[nodiscard]] const EntityLocation& Location(Entity entity) const;
    [[nodiscard]] EntityRecord& Record(Entity entity);
    [[nodiscard]] const EntityRecord& Record(Entity entity) const;

    [[nodiscard]] std::size_t LivingCount() const { return m_Living; }
    [[nodiscard]] std::size_t Capacity() const { return m_Records.size(); }

    void ForEachLiving(const std::function<void(Entity)>& fn) const;

private:
    std::vector<EntityRecord> m_Records;
    std::vector<std::uint32_t> m_FreeList;
    std::size_t m_Living = 0;
};

} // namespace we::runtime::ecs

#pragma warning(pop)
