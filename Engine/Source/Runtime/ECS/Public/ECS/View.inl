#pragma once

namespace we::runtime::ecs {

template <typename... Ts>
template <typename Fn>
void View<Ts...>::EachImpl(Fn&& fn, bool enabledOnly) {
    auto& leadPool = m_Registry->template Pool<typename LeadType<Ts...>::type>();
    auto& lead = leadPool.Set();
    const Entity* entities = lead.DenseEntities();
    const std::size_t count = lead.Size();
    const std::uint8_t* enabled = lead.EnabledMask();

    for (std::size_t i = 0; i < count; ++i) {
        const Entity e = entities[i];
        if (enabledOnly && enabled[i] == 0) {
            continue;
        }
        if (!MatchRest<Ts...>(e, enabledOnly)) {
            continue;
        }
        Invoke(std::forward<Fn>(fn), e, std::index_sequence_for<Ts...>{});
    }
}

template <typename... Ts>
template <typename U, typename... Rest>
bool View<Ts...>::MatchRestImpl(Entity e, bool enabledOnly) const {
    auto& pool = m_Registry->template Pool<U>().Set();
    if (!pool.Contains(e)) {
        return false;
    }
    if (enabledOnly && !pool.IsEnabled(e)) {
        return false;
    }
    if constexpr (sizeof...(Rest) == 0) {
        return true;
    } else {
        return MatchRestImpl<Rest...>(e, enabledOnly);
    }
}

template <typename... Ts>
template <typename Fn, std::size_t... I>
void View<Ts...>::Invoke(Fn&& fn, Entity e, std::index_sequence<I...>) {
    fn(e, m_Registry->template Get<typename std::tuple_element<I, std::tuple<Ts...>>::type>(e)...);
}

template <typename... Include>
template <typename... Exclude>
Query<Include...>& Query<Include...>::ExcludeTypes() {
    m_ExcludeCheck = [](Registry& reg, Entity e) {
        return !(reg.template Has<Exclude>(e) || ...);
    };
    return *this;
}

template <typename... Include>
template <typename Fn>
void Query<Include...>::Each(Fn&& fn) {
    RefreshIfNeeded();
    for (Entity e : m_Cache) {
        if (m_ExcludeCheck && !m_ExcludeCheck(*m_Registry, e)) {
            continue;
        }
        Invoke(std::forward<Fn>(fn), e, std::index_sequence_for<Include...>{});
    }
}

template <typename... Include>
void Query<Include...>::RefreshIfNeeded() {
    if (m_CachedVersion == m_Registry->Version()) {
        return;
    }
    m_Cache.clear();
    m_Registry->ViewAll<Include...>().EachIncludingDisabled([&](Entity e, Include&...) {
        if (m_EnabledOnly) {
            const bool allEnabled = (m_Registry->template IsEnabled<Include>(e) && ...);
            if (!allEnabled) {
                return;
            }
        }
        m_Cache.push_back(e);
    });
    m_CachedVersion = m_Registry->Version();
}

template <typename... Include>
template <typename Fn, std::size_t... I>
void Query<Include...>::Invoke(Fn&& fn, Entity e, std::index_sequence<I...>) {
    fn(e, m_Registry->template Get<typename std::tuple_element<I, std::tuple<Include...>>::type>(e)...);
}

} // namespace we::runtime::ecs
