#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Chunk.h"
#include "ECS/ComponentType.h"
#include "ECS/Entity.h"
#include "ECS/Export.h"
#include "ECS/JobPool.h"

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace we::runtime::ecs {

class World;

template <typename... Include>
class ArchetypeQuery {
public:
    explicit ArchetypeQuery(World& world) : m_World(&world) {}

    template <typename... ExcludeTypes>
    ArchetypeQuery& Without();

    template <typename T>
    ArchetypeQuery& With();

    ArchetypeQuery& EnabledEntitiesOnly(bool enabled) {
        m_EnabledEntitiesOnly = enabled;
        return *this;
    }

    ArchetypeQuery& EnabledComponentsOnly(bool enabled) {
        m_EnabledComponentsOnly = enabled;
        return *this;
    }

    template <typename Fn>
    void Each(Fn&& fn);

    template <typename Fn>
    void EachChunk(Fn&& fn);

    template <typename Fn>
    void ParallelEachChunk(JobPool& jobs, Fn&& fn);

    [[nodiscard]] std::size_t ArchetypeCount() const;
    [[nodiscard]] std::size_t EntityCount() const;

    void Invalidate() { m_CachedStructuralVersion = 0; }

private:
    void RefreshIfNeeded() const;

    template <typename Fn, std::size_t... I>
    void InvokeEntity(Fn&& fn, Entity e, ChunkView& view, std::uint16_t slot, std::index_sequence<I...>);

    World* m_World = nullptr;
    mutable std::vector<ArchetypeLayout*> m_MatchingArchetypes;
    mutable std::uint64_t m_CachedStructuralVersion = 0;
    ComponentMask m_Required{};
    ComponentMask m_Excluded{};
    bool m_EnabledEntitiesOnly = true;
    bool m_EnabledComponentsOnly = true;
};

} // namespace we::runtime::ecs

#pragma warning(pop)
