#pragma once

#include "ECS/Entity.h"
#include "ECS/ComponentPool.h"

#include <cstddef>
#include <functional>
#include <tuple>
#include <utility>
#include <vector>

namespace we::runtime::ecs {

class Registry;

template <typename... Ts>
class View {
public:
    explicit View(Registry& registry) : m_Registry(&registry) {}

    template <typename Fn>
    void Each(Fn&& fn) {
        EachImpl(std::forward<Fn>(fn), true);
    }

    template <typename Fn>
    void EachIncludingDisabled(Fn&& fn) {
        EachImpl(std::forward<Fn>(fn), false);
    }

    std::size_t Count(bool enabledOnly = true) const {
        std::size_t n = 0;
        const_cast<View*>(this)->EachImpl([&](Entity, Ts&...) { ++n; }, enabledOnly);
        return n;
    }

private:
    template <typename Fn>
    void EachImpl(Fn&& fn, bool enabledOnly);

    template <typename T0, typename... Rest>
    struct LeadType { using type = T0; };

    template <typename T0, typename... Rest>
    bool MatchRest(Entity e, bool enabledOnly) const {
        if constexpr (sizeof...(Rest) == 0) {
            (void)e;
            (void)enabledOnly;
            return true;
        } else {
            return MatchRestImpl<Rest...>(e, enabledOnly);
        }
    }

    template <typename U, typename... Rest>
    bool MatchRestImpl(Entity e, bool enabledOnly) const;

    template <typename Fn, std::size_t... I>
    void Invoke(Fn&& fn, Entity e, std::index_sequence<I...>);

    Registry* m_Registry = nullptr;
};

template <typename... Include>
class Query {
public:
    explicit Query(Registry& registry) : m_Registry(&registry) {}

    template <typename... Exclude>
    Query& ExcludeTypes();

    Query& EnabledOnly(bool enabled) {
        m_EnabledOnly = enabled;
        return *this;
    }

    template <typename Fn>
    void Each(Fn&& fn);

    void Invalidate() { m_CachedVersion = 0; }

private:
    void RefreshIfNeeded();

    template <typename Fn, std::size_t... I>
    void Invoke(Fn&& fn, Entity e, std::index_sequence<I...>);

    Registry* m_Registry = nullptr;
    std::vector<Entity> m_Cache;
    std::uint64_t m_CachedVersion = 0;
    bool m_EnabledOnly = true;
    std::function<bool(Registry&, Entity)> m_ExcludeCheck;
};

} // namespace we::runtime::ecs
