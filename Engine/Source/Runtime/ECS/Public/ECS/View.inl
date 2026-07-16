#pragma once

#include "ECS/Query.h"

namespace we::runtime::ecs {

template <typename... Ts>
template <typename Fn>
void View<Ts...>::EachImpl(Fn&& fn, bool enabledOnly) {
    ArchetypeQuery<Ts...> query = m_Registry->GetWorld().template QueryAll<Ts...>();
    // Entity enable gate only — component enable bits are not reliably preserved
    // across archetype migrations (AllocateSlot defaults them; copies skip enable columns).
    query.EnabledEntitiesOnly(enabledOnly).EnabledComponentsOnly(false).Each(std::forward<Fn>(fn));
}

template <typename... Ts>
template <typename Fn, std::size_t... I>
void View<Ts...>::Invoke(Fn&& fn, Entity e, std::index_sequence<I...>) {
    fn(e, m_Registry->template Get<typename std::tuple_element<I, std::tuple<Ts...>>::type>(e)...);
}

} // namespace we::runtime::ecs
