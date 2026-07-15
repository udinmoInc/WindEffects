#pragma once

#include "ECS/Export.h"

#include <cstdint>
#include <functional>

namespace we::runtime::ecs {

// 64-bit identity: [generation:32 | index:32]. Generation invalidates stale handles.
struct Entity {
    std::uint64_t id = 0;

    constexpr Entity() = default;
    constexpr explicit Entity(std::uint64_t value) : id(value) {}
    constexpr Entity(std::uint32_t index, std::uint32_t generation)
        : id((static_cast<std::uint64_t>(generation) << 32) | index) {}

    constexpr std::uint32_t Index() const { return static_cast<std::uint32_t>(id & 0xFFFFFFFFu); }
    constexpr std::uint32_t Generation() const { return static_cast<std::uint32_t>(id >> 32); }
    constexpr bool IsNull() const { return id == 0; }
    constexpr explicit operator bool() const { return id != 0; }

    constexpr bool operator==(Entity o) const { return id == o.id; }
    constexpr bool operator!=(Entity o) const { return id != o.id; }
    constexpr bool operator<(Entity o) const { return id < o.id; }
};

inline constexpr Entity kNullEntity{};

struct EntityHash {
    std::size_t operator()(Entity e) const noexcept {
        return std::hash<std::uint64_t>{}(e.id);
    }
};

} // namespace we::runtime::ecs
