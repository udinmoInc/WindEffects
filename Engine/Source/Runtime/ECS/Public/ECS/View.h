#pragma once

#include "ECS/Entity.h"

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

    template <typename Fn, std::size_t... I>
    void Invoke(Fn&& fn, Entity e, std::index_sequence<I...>);

    Registry* m_Registry = nullptr;
};

} // namespace we::runtime::ecs
