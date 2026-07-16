#pragma once

#include "ECS/Query.h"
#include "ECS/World.h"

namespace we::runtime::ecs {

template <typename... Include>
template <typename T>
ArchetypeQuery<Include...>& ArchetypeQuery<Include...>::With() {
    m_Required.Set(ComponentTypeRegistry::Get().Id<T>());
    return *this;
}

template <typename... Include>
template <typename... ExcludeTypes>
ArchetypeQuery<Include...>& ArchetypeQuery<Include...>::Without() {
    (m_Excluded.Set(ComponentTypeRegistry::Get().Id<ExcludeTypes>()), ...);
    return *this;
}

template <typename... Include>
void ArchetypeQuery<Include...>::RefreshIfNeeded() const {
    const std::uint64_t version = m_World->StructuralVersion();
    if (m_CachedStructuralVersion == version && !m_MatchingArchetypes.empty()) {
        return;
    }
    m_MatchingArchetypes.clear();
    for (ArchetypeLayout* archetype : m_World->Archetypes().All()) {
        if (!archetype) {
            continue;
        }
        if (archetype->mask.CompatibleWith(m_Required, m_Excluded)) {
            m_MatchingArchetypes.push_back(archetype);
        }
    }
    m_CachedStructuralVersion = version;
}

template <typename... Include>
template <typename Fn>
void ArchetypeQuery<Include...>::EachChunk(Fn&& fn) {
    RefreshIfNeeded();
    for (ArchetypeLayout* archetype : m_MatchingArchetypes) {
        for (Chunk* chunk : archetype->chunks) {
            if (!chunk || chunk->header.count == 0) {
                continue;
            }
            fn(ChunkView(archetype, chunk));
        }
    }
}

template <typename... Include>
template <typename Fn>
void ArchetypeQuery<Include...>::Each(Fn&& fn) {
    RefreshIfNeeded();
    for (ArchetypeLayout* archetype : m_MatchingArchetypes) {
        for (Chunk* chunk : archetype->chunks) {
            if (!chunk || chunk->header.count == 0) {
                continue;
            }
            ChunkView view(archetype, chunk);
            for (std::uint16_t slot = 0; slot < chunk->header.count; ++slot) {
                if (m_EnabledEntitiesOnly && !view.IsEntityEnabled(slot)) {
                    continue;
                }
                if (m_EnabledComponentsOnly) {
                    const bool allEnabled = (view.IsComponentEnabled(
                        ComponentTypeRegistry::Get().Id<Include>(), slot) && ...);
                    if (!allEnabled) {
                        continue;
                    }
                }
                Entity e = view.GetEntity(slot);
                InvokeEntity(fn, e, view, slot, std::index_sequence_for<Include...>{});
            }
        }
    }
}

template <typename... Include>
template <typename Fn>
void ArchetypeQuery<Include...>::ParallelEachChunk(JobPool& jobs, Fn&& fn) {
    RefreshIfNeeded();
    std::vector<Chunk*> chunks;
    for (ArchetypeLayout* archetype : m_MatchingArchetypes) {
        for (Chunk* chunk : archetype->chunks) {
            if (chunk && chunk->header.count > 0) {
                chunks.push_back(chunk);
            }
        }
    }
    jobs.ParallelForChunks(chunks, [&](Chunk* chunk) {
        fn(ChunkView(chunk->header.archetype, chunk));
    });
}

template <typename... Include>
std::size_t ArchetypeQuery<Include...>::ArchetypeCount() const {
    RefreshIfNeeded();
    return m_MatchingArchetypes.size();
}

template <typename... Include>
std::size_t ArchetypeQuery<Include...>::EntityCount() const {
    RefreshIfNeeded();
    std::size_t total = 0;
    for (ArchetypeLayout* archetype : m_MatchingArchetypes) {
        total += static_cast<std::size_t>(archetype->entityCount);
    }
    return total;
}

template <typename... Include>
template <typename Fn, std::size_t... I>
void ArchetypeQuery<Include...>::InvokeEntity(
    Fn&& fn,
    Entity e,
    ChunkView& view,
    std::uint16_t slot,
    std::index_sequence<I...>)
{
    fn(e, view.Get<typename std::tuple_element<I, std::tuple<Include...>>::type>(slot)...);
}

template <typename... Ts>
ArchetypeQuery<Ts...> World::QueryAll() {
    ArchetypeQuery<Ts...> query(*this);
    (query.template With<Ts>(), ...);
    return query;
}

} // namespace we::runtime::ecs
