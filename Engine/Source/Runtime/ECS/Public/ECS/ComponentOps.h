#pragma once

#include "ECS/Export.h"
#include "ECS/Types.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace we::runtime::ecs {

using ComponentConstructFn = void (*)(void* dst, std::size_t size);
using ComponentDestructFn = void (*)(void* ptr, std::size_t size);
using ComponentMoveFn = void (*)(void* dst, void* src, std::size_t size);
using ComponentCopyFn = void (*)(void* dst, const void* src, std::size_t size);

struct ComponentOps {
    ComponentConstructFn construct = nullptr;
    ComponentDestructFn destruct = nullptr;
    ComponentMoveFn move = nullptr;
    ComponentCopyFn copy = nullptr;
    bool trivial = true;
};

ECS_API ComponentOps MakeTrivialOps();
ECS_API ComponentOps MakePODOps();
ECS_API void RegisterComponentOps(std::uint32_t typeId, ComponentOps ops);
ECS_API const ComponentOps* FindComponentOps(std::uint32_t typeId);

template <typename T>
ComponentOps MakeOpsFor() {
    // Always value-construct (T{}) so in-class defaults apply (e.g. scale=1, visible=true).
    // Zero-fill would break Transform/Visibility and other POD components with non-zero defaults.
    if constexpr (std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>) {
        ComponentOps ops{};
        ops.trivial = true;
        ops.construct = [](void* dst, std::size_t) { new (dst) T(); };
        ops.destruct = [](void*, std::size_t) {};
        ops.move = [](void* dst, void* src, std::size_t size) { std::memmove(dst, src, size); };
        ops.copy = [](void* dst, const void* src, std::size_t size) { std::memcpy(dst, src, size); };
        return ops;
    } else {
        ComponentOps ops{};
        ops.trivial = false;
        ops.construct = [](void* dst, std::size_t) { new (dst) T(); };
        ops.destruct = [](void* ptr, std::size_t) { static_cast<T*>(ptr)->~T(); };
        // Move/copy assign into an already-constructed slot (AllocateSlot default-constructs).
        // Caller destroys `src` after move when the source slot is being retired.
        ops.move = [](void* dst, void* src, std::size_t) {
            *static_cast<T*>(dst) = std::move(*static_cast<T*>(src));
        };
        ops.copy = [](void* dst, const void* src, std::size_t) {
            *static_cast<T*>(dst) = *static_cast<const T*>(src);
        };
        return ops;
    }
}

} // namespace we::runtime::ecs
